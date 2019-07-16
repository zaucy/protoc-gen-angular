#include <cctype>
#include <set>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include "angular-generator.hh"
#include "angular-generator-options.hh"

using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::FileDescriptor;
using google::protobuf::MethodDescriptor;
using google::protobuf::ServiceDescriptor;
using google::protobuf::compiler::CodeGenerator;
using google::protobuf::compiler::GeneratorContext;
using google::protobuf::compiler::ParseGeneratorParameter;
using google::protobuf::io::Printer;
using google::protobuf::io::ZeroCopyOutputStream;
using std::string;
using std::map;
using std::pair;
using std::vector;

namespace {

  std::string getImportPrefix(const std::string& filename) {
    std::string importPrefix;

    for(const char& c : filename) {
      if(c == '/') {
        importPrefix += "../";
      }
    }

    return importPrefix;
  }

  std::string removePathExtname(const std::string& path) {
    auto dotIndex = path.find_last_of('.');

    if(dotIndex != std::string::npos) {
      return path.substr(0, dotIndex);
    }

    return path;
  }

  std::string parentPath(const std::string& path) {
    auto slashIndex = path.find_last_of('/');

    if(slashIndex != std::string::npos) {
      return path.substr(0, slashIndex);
    }

    return "";
  }

  std::string firstCharToLower(std::string str) {
    if(str.length() > 0) {
      str[0] = std::tolower(str[0]);
    }

    return str;
  }

  std::string firstCharToUpper(std::string str) {
    if(str.length() > 0) {
      str[0] = std::toupper(str[0]);
    }

    return str;
  }

  std::string trimTrailing(std::string str, char c) {
    char lastChar;

    while(str.size() > 0) {
      lastChar = str[str.size() - 1];

      if(lastChar == c) {
        str = str.substr(0, str.size() - 1);
      } else {
        break;
      }
    }

    return str;
  }

  std::string trimLeading(std::string str, char c) {
    char firstChar;

    while(str.size() > 0) {
      firstChar = str[0];

      if(firstChar == c) {
        str = str.substr(1);
      } else {
        break;
      }
    }

    return str;
  }

  std::string trim(std::string str, char c) {
    return trimLeading(trimTrailing(str, c), c);
  }

  template<typename T>
  void printCommentDoc(Printer& printer, T descriptor) {
    google::protobuf::SourceLocation srcLoc;

    if(descriptor.GetSourceLocation(&srcLoc)) {
      auto leadingComments = trim(srcLoc.leading_comments, ' ');

      if(!leadingComments.empty()) {
        map<string, string> vars = {
          {"comment", leadingComments}
        };

        printer.Print(vars, "/**\n");
        printer.Indent();

        printer.Print(vars, "$comment$");
        
        printer.Outdent();
        printer.Print("  */\n");
      }
    }
  }
}

namespace {

  map<string, const Descriptor*> GetAllServiceMessages
    ( const ServiceDescriptor& service
    )
  {
    map<string, const Descriptor*> messageTypes;
    auto methodCount = service.method_count();

    for(int methodIndex=0; methodIndex < methodCount; ++methodIndex) {
      const MethodDescriptor* method = service.method(methodIndex);
      const Descriptor* inputType = method->input_type();
      const Descriptor* outputType = method->output_type();

      messageTypes[inputType->name()] = inputType;
      messageTypes[outputType->name()] = outputType;
    }

    return messageTypes;
  }

  void PrintAngularServiceGoogleUnaryCall
    ( map<string, string>  vars
    , Printer&             printer
    )
  {
    printer.Print(vars, "let req = this._client.$method_name$(request, metadata, callback);\n");
  }

  void PrintAngularServiceImprobableEngUnaryCall
    ( map<string, string>  vars
    , Printer&             printer
    )
  {

    printer.Print(vars, "let responseMetadata: grpc.Metadata|null = null;\n\n");

    printer.Print(vars, "grpc.invoke(__$service_name$.$Method_name$, {\n");
    printer.Indent();

    printer.Print(vars,
      "request: request,\n"
      "host: (<any>window).DEFAULT_ANGULAR_GRPC_HOST || 'https://' + location.hostname,\n"
      "metadata: metadata,\n"
      "onHeaders: headers => responseMetadata = headers,\n"
      "onMessage: response => this._ngZone.run(() => {\n"
      "  callback(null, response, responseMetadata || new grpc.Metadata());\n"
      "}),\n"
      "onEnd: (code, msg, metadata) => this._ngZone.run(() => {\n"
      "  if(code != $statusCodeNamespace$.OK) {\n"
      "    let err = new Error(msg);\n"
      "    (<any>err).code = code;\n"
      "    callback(err);\n"
      "  }\n"
      "})\n"
    );

    printer.Outdent();
    printer.Print("});\n\n");
  }

  void PrintAngularServiceGoogleServerStreamingCall
    ( map<string, string>  vars
    , Printer&             printer
    )
  {
    printer.Print(vars, "let req = this._client.$method_name$(request, metadata);\n");
  }

  void PrintAngularServiceImprobableEngServerStreamingCall
    ( map<string, string>  vars
    , Printer&             printer
    )
  {
    printer.Print(vars, "let req = grpc.invoke(__$service_name$.$Method_name$, {\n");
    printer.Indent();

    printer.Print(vars,
      "request: request,\n"
      "host: (<any>window).DEFAULT_ANGULAR_GRPC_HOST || 'https://' + location.hostname,\n"
      "metadata: metadata,\n"
      "onMessage: response => this._ngZone.run(() => {\n"
      "  onMessage(response);\n"
      "}),\n"
      "onEnd: (code, msg, metadata) => this._ngZone.run(() => {\n"
      "  if(code == $statusCodeNamespace$.OK) {\n"
      "    onEnd(code, msg, metadata);\n"
      "  } else {\n"
      "    let err = new Error(code + ' ' + (msg||'');\n"
      "    (<any>err).code = code;\n"
      "    onError(err);\n"
      "  }\n"
      "})\n"
    );

    printer.Outdent();
    printer.Print("});\n\n");
  }

  void PrintAngularServiceUnaryMethodBody
    ( map<string, string>                     vars
    , Printer&                                printer
    , const MethodDescriptor&                 method
    , const AngularGrpcCodeGeneratorOptions&  options
    )
  {

    auto inputType = method.input_type();
    auto outputType = method.output_type();

    vars["statusCodeNamespace"] = options.grpcWebImplInfo.statusCodeNamespace;
    vars["service_name"] = method.service()->name();
    vars["method_name"] = firstCharToLower(method.name());
    vars["Method_name"] = method.name();
    vars["input_type"] = inputType->name();
    vars["output_type"] = outputType->name();

    printer.Print("let ret: any;\n");
    printer.Print("let callback: any;\n");
    printer.Print("let metadata: any;\n\n");

    printer.Print(
      "if(typeof arg1 === 'function') {\n"
      "  callback = arg1;\n"
      "} else {\n"
      "  metadata = arg1;\n"
      "  callback = arg2;\n"
      "}\n\n"
    );

    printer.Print("if(!callback) {\n");
    printer.Indent();

    printer.Print(vars, "ret = new Promise<$output_type$>"
      "((resolve, reject) => {\n");
    printer.Indent();

    printer.Print("callback = (err: any, response: any) => {\n");
    printer.Indent();

    printer.Print("if(err) reject(err);\n");
    printer.Print("else resolve(response);\n");

    printer.Outdent();
    printer.Print("};\n");

    printer.Outdent();
    printer.Print("});\n");

    printer.Outdent();
    printer.Print("}\n\n");

    switch(options.grpcWebImpl) {
      case GrpcWebImplementation::GWI_IMPROBABLE_ENG:
        PrintAngularServiceImprobableEngUnaryCall(vars, printer);
        break;
      case GrpcWebImplementation::GWI_GOOGLE:
        PrintAngularServiceGoogleUnaryCall(vars, printer);
        break;
    }

    printer.Print("return ret;\n");
  }

  void PrintAngularServiceServerStreamingMethodBody
    ( map<string, string>                     vars
    , Printer&                                printer
    , const MethodDescriptor&                 method
    , const AngularGrpcCodeGeneratorOptions&  options
    )
  {
    auto inputType = method.input_type();
    auto outputType = method.output_type();

    vars["statusCodeNamespace"] = options.grpcWebImplInfo.statusCodeNamespace;
    vars["service_name"] = method.service()->name();
    vars["method_name"] = firstCharToLower(method.name());
    vars["Method_name"] = method.name();
    vars["input_type"] = inputType->name();
    vars["output_type"] = outputType->name();

    printer.Print("let ret: any;\n");
    printer.Print("let metadata: any;\n\n");
    printer.Print("let onMessage: any;\n");
    printer.Print("let onError: any;\n");
    printer.Print("let onEnd: any;\n");

    printer.Print(
      "if(typeof arg1 === 'function') {\n"
      "  onMessage = arg1;\n"
      "  onError = arg2;\n"
      "  onEnd = arg3;\n"
      "} else if(typeof arg2 === 'function') {\n"
      "  metadata = arg1;\n"
      "  onMessage = arg2;\n"
      "  onError = arg3;\n"
      "  onEnd = arg4;\n"
      "} else {\n"
      "  metadata = arg1;\n"
      "}\n\n"
    );

    printer.Print(vars,
      "if(!onMessage) {\n"
      "  let subject = new Subject<$output_type$>();\n"
      "  ret = subject.asObservable();\n\n"
      "  onMessage = (response: any) => {\n"
      "    subject.next(response);\n"
      "  };\n\n"
      "  onError = (err: any) => {\n"
      "    subject.error(err);\n"
      "  };\n\n"
      "  onEnd = (code: any, msg: any, metadata: any) => {\n"
      "    subject.complete();\n"
      "  };\n\n"
      "} else {\n"
      "  if(!onError) {\n"
      "    onError = (err: any) => console.error(err);\n"
      "  }\n\n"
      "  if(!onEnd) {\n"
      "    onEnd = (code: any, msg: any, metadata: any) => {};\n"
      "  }\n"
      "}\n\n"
    );

    switch(options.grpcWebImpl) {
      case GrpcWebImplementation::GWI_GOOGLE:
        vars["cancel_method"] = "cancel";
        PrintAngularServiceGoogleServerStreamingCall(vars, printer);
        break;
      case GrpcWebImplementation::GWI_IMPROBABLE_ENG:
        vars["cancel_method"] = "close";
        PrintAngularServiceImprobableEngServerStreamingCall(vars, printer);
        break;
    }

    printer.Print(vars, "(<any>ret).close = () => {\n");
    printer.Indent();
    printer.Print(vars, "console.warn('[Angular Grpc] .close() is deprecated use .cancel() instead');\n");
    printer.Print(vars, "return req.$cancel_method$();\n");
    printer.Outdent();
    printer.Print("};\n");
    printer.Print(vars, "(<any>ret).cancel = () => req.$cancel_method$();\n");
    printer.Print(vars, "return ret;\n");
  }

  void PrintAngularServiceUnaryMethod
    ( Printer&                                printer
    , const MethodDescriptor&                 method
    , const AngularGrpcCodeGeneratorOptions&  options
    )
  {
    map<string, string> vars;

    auto inputType = method.input_type();
    auto outputType = method.output_type();

    vars["method_name"] = firstCharToLower(method.name());
    vars["Method_name"] = method.name();
    vars["input_type"] = inputType->name();
    vars["output_type"] = outputType->name();
    vars["cb_signature"] =
      "(err: any|null, response: " + outputType->name() + ", metadata: grpc.Metadata) => void";

    printCommentDoc<const MethodDescriptor&>(printer, method);

    // A few signatures
    printer.Print(vars,
      "$method_name$("
        "request: $input_type$"
      "): Promise<$output_type$>;\n"
    );

    printCommentDoc<const MethodDescriptor&>(printer, method);

    printer.Print(vars,
      "$method_name$("
        "request: $input_type$, "
        "metadata: grpc.Metadata"
      "): Promise<$output_type$>;\n"
    );

    printCommentDoc<const MethodDescriptor&>(printer, method);
    
    printer.Print(vars,
      "$method_name$("
        "request: $input_type$, "
        "callback: $cb_signature$"
      "): void;\n"
    );

    printCommentDoc<const MethodDescriptor&>(printer, method);

    printer.Print(vars,
      "$method_name$("
        "request: $input_type$, "
        "metadata: grpc.Metadata, "
        "callback: $cb_signature$"
      "): void;\n\n"
    );

    printer.Print(vars,
      "$method_name$("
        "request: $input_type$, "
        "arg1?: grpc.Metadata|($cb_signature$), "
        "arg2?: $cb_signature$"
      "): Promise<$output_type$>|void {\n"
    );

    printer.Indent();
    PrintAngularServiceUnaryMethodBody(vars, printer, method, options);

    printer.Outdent();

    printer.Print("}\n\n");
  }

  void PrintAngularServiceBidiStreamingMethod
    ( Printer&                                printer
    , const MethodDescriptor&                 method
    , const AngularGrpcCodeGeneratorOptions&  options
    )
  {

  }

  void PrintAngularServiceClientStreamingMethod
    ( Printer&                                printer
    , const MethodDescriptor&                 method
    , const AngularGrpcCodeGeneratorOptions&  options
    )
  {

  }
  
  void PrintAngularServiceServerStreamingMethod
    ( Printer&                                printer
    , const MethodDescriptor&                 method
    , const AngularGrpcCodeGeneratorOptions&  options
    )
  {
    map<string, string> vars;

    auto inputType = method.input_type();
    auto outputType = method.output_type();

    vars["method_name"] = firstCharToLower(method.name());
    vars["Method_name"] = method.name();
    vars["input_type"] = inputType->name();
    vars["output_type"] = outputType->name();
    vars["msg_cb"] =
      "(message?: " + outputType->name() + ") => void";
    vars["error_cb"] = "(err: any) => void";
    vars["end_cb"] = "("
      "code: number, "
      "msg: string|undefined, "
      "metadata: grpc.Metadata"
    ") => void";

    printCommentDoc<const MethodDescriptor&>(printer, method);

    // A few signatures
    printer.Print(vars,
      "$method_name$("
        "request: $input_type$"
      "): {cancel():void;close():void}&Observable<$output_type$>;\n"
    );

    printCommentDoc<const MethodDescriptor&>(printer, method);

    printer.Print(vars,
      "$method_name$("
        "request: $input_type$, "
        "metadata: grpc.Metadata"
      "): {cancel():void;close():void}&Observable<$output_type$>;\n"
    );

    printCommentDoc<const MethodDescriptor&>(printer, method);
    
    printer.Print(vars,
      "$method_name$("
        "request: $input_type$, "
        "onMessage: $msg_cb$,"
        "onError?: $error_cb$,"
        "onEnd?: $end_cb$"
      "): void;\n"
    );

    printCommentDoc<const MethodDescriptor&>(printer, method);

    printer.Print(vars,
      "$method_name$("
        "request: $input_type$, "
        "metadata: grpc.Metadata, "
        "onMessage: $msg_cb$,"
        "onError?: $error_cb$,"
        "onEnd?: $end_cb$"
      "): void;\n\n"
    );

    printer.Print(vars,
      "$method_name$("
        "request: $input_type$, "
        "arg1?: grpc.Metadata|($msg_cb$), "
        "arg2?: ($msg_cb$)|($error_cb$), "
        "arg3?: ($error_cb$)|($end_cb$), "
        "arg4?: $end_cb$"
      "): {cancel():void;close():void}&Observable<$output_type$>|void {\n"
    );

    printer.Indent();

    PrintAngularServiceServerStreamingMethodBody(
      vars, printer, method, options
    );

    printer.Outdent();

    printer.Print("}\n\n");
  }

  void PrintFileServiceMessageImports
    ( Printer&                                printer
    , const FileDescriptor&                   file
    , const AngularGrpcCodeGeneratorOptions&  options
    )
  {
    auto serviceCount = file.service_count();
    map<string, vector<const Descriptor*>> fileToDescriptorMap;

    for(auto i=0; serviceCount > i; ++i) {
      auto service = file.service(i);
      auto serviceMessages = GetAllServiceMessages(*service);

      for(auto message : serviceMessages) {
        auto name = message.first;
        auto descriptor = message.second;
        auto filename = descriptor->file()->name();

        auto it = fileToDescriptorMap.find(filename);
        if(it == fileToDescriptorMap.end()) {
          fileToDescriptorMap.insert({filename, {}});
          it = fileToDescriptorMap.find(filename);
        }

        bool foundExisting = false;
        for(auto existingDescriptor : it->second) {
          if(existingDescriptor->name() == descriptor->name()) {
            foundExisting = true;
            break;
          }
        }

        // Skip existing descriptors
        if(foundExisting) {
          continue;
        }

        it->second.push_back(descriptor);
      }
    }

    for(auto& pair : fileToDescriptorMap) {
      auto filename = pair.first;
      auto descriptors = pair.second;
      auto importPath = removePathExtname(filename) + "_pb";

      map<string, string> vars = {
        {"import_prefix", options.webImportPrefix},
        {"import_path", importPath}
      };

      if(descriptors.size() > 0) {
        printer.Print(vars, "import {\n");
        printer.Indent();
        for(auto descriptor : descriptors) {
          vars["full_name"] = descriptor->full_name();
          vars["name"] = descriptor->name();
          printer.Print(vars, "$name$,\n");
        }
        printer.Outdent();
        printer.Print(vars, "} from '$import_prefix$/$import_path$';\n");
      }
    }

    printer.PrintRaw("\n");
  }

  void PrintFileServiceImports
    ( Printer&                                printer
    , const FileDescriptor&                   file
    , const AngularGrpcCodeGeneratorOptions&  options
    )
  {

    auto serviceCount = file.service_count();

    if(serviceCount == 0) {
      return;
    }

    auto filename = file.name();
    auto importPath =
      removePathExtname(filename) + options.grpcWebImplInfo.serviceFileSuffix;

    map<string, string> vars = {
      {"import_prefix", options.grpcWebImportPrefix},
      {"import_path", importPath}
    };

    printer.Print(vars, "import {\n");
    printer.Indent();
    for(auto i=0; serviceCount > i; ++i) {
      auto service = file.service(i);

      vars["service_name"] = service->name();
      switch(options.grpcWebImpl) {
        case GWI_GOOGLE:
          printer.Print(vars, "$service_name$Client as __$service_name$Client,\n");
          break;
        case GWI_IMPROBABLE_ENG:
          printer.Print(vars, "$service_name$ as __$service_name$,\n");
          break;
      }
    }
    printer.Outdent();
    printer.Print(vars, "} from '$import_prefix$/$import_path$';\n");

  }

  void PrintAngularService
    ( Printer&                                printer
    , const ServiceDescriptor&                service
    , const AngularGrpcCodeGeneratorOptions&  options
    )
  {

    map<string, string> vars = {
      {"service_name", service.name()}
    };

    printer.PrintRaw("\n");

    printCommentDoc<const ServiceDescriptor&>(printer, service);

    printer.Print("@Injectable()\n");
    printer.Print(vars, "export class $service_name$ {\n\n");

    printer.Indent();

    switch(options.grpcWebImpl) {
      case GWI_GOOGLE:
        printer.Print(vars, "private _client: __$service_name$Client;\n\n");
        break;
    }

    printer.Print("constructor(private _ngZone: NgZone) {\n");
    printer.Indent();

    switch(options.grpcWebImpl) {
      case GWI_GOOGLE:
        printer.Print(vars, "this._client = new __$service_name$Client(\n");
        printer.Indent();

        printer.Print(vars, "(<any>window).DEFAULT_ANGULAR_GRPC_HOST || 'https://' + location.hostname,\n");

        printer.Outdent();
        printer.Print(vars, ");\n\n");
        break;
    }

    printer.Outdent();
    printer.Print("}\n\n");

    auto methodCount = service.method_count();

    for(auto i=0; methodCount > i; ++i) {
      auto method = service.method(i);

      if(!method->client_streaming() && !method->server_streaming()) {
        PrintAngularServiceUnaryMethod(printer, *method, options);
      } else
      if(method->client_streaming() && method->server_streaming()) {
        PrintAngularServiceBidiStreamingMethod(printer, *method, options);
      } else
      if(method->client_streaming()) {
        PrintAngularServiceClientStreamingMethod(printer, *method, options);
      } else
      if(method->server_streaming()) {
        PrintAngularServiceServerStreamingMethod(printer, *method, options);
      }
    }

    printer.Outdent();

    printer.Print("}\n\n");
  }

  void PrintFileAngularServices
    ( Printer&                                printer
    , const FileDescriptor&                   file
    , const AngularGrpcCodeGeneratorOptions&  options
    )
  {
    auto serviceCount = file.service_count();

    for(auto i=0; serviceCount > i; ++i) {
      auto service = file.service(i);

      PrintAngularService(
        printer,
        *service,
        options
      );
    }
  }

  void PrintAngularServiceCommonImports
    ( Printer&                                printer
    , const AngularGrpcCodeGeneratorOptions&  options
    )
  {
    map<string, string> vars = {
      {"grpc_impl_import", options.grpcWebImplInfo.commonjsImport},
      {"grpc_impl_import_as", options.grpcWebImplInfo.commonjsImportAs}
    };

    printer.Print(vars,
      "import { Injectable, NgZone } from '@angular/core';\n"
      "import { Observable, Subject } from 'rxjs';\n"
      "import $grpc_impl_import_as$ from '$grpc_impl_import$';\n"
      "\n"
    );
  }
}

void PrintAngularModuleImports
  ( Printer&                                 printer
  , const vector<const ServiceDescriptor*>&  services
  , AngularGrpcCodeGeneratorOptions&         options
  )
{
  map<string, string> vars = {};

  printer.Print(vars, "import { NgModule } from '@angular/core';\n\n");

  map<string, vector<const ServiceDescriptor*>> fileToServiceDescriptorMap;

  for(auto service : services) {
    auto filename = service->file()->name();

    auto it = fileToServiceDescriptorMap.find(filename);
    if(it == fileToServiceDescriptorMap.end()) {
      fileToServiceDescriptorMap.insert({filename, {}});
      it = fileToServiceDescriptorMap.find(filename);
    }

    it->second.push_back(service);
  }

  for(auto pair : fileToServiceDescriptorMap) {
    auto filename = pair.first;
    auto fileServices = pair.second;

    vars["import_path"] = removePathExtname(filename) + "_ng_grpc_pb";

    printer.Print(vars, "import {\n");
    printer.Indent();

    for(auto service : fileServices) {
      vars["service_name"] = service->name();
      printer.Print(vars, "$service_name$,\n");
    }

    printer.Outdent();
    printer.Print(vars, "} from './$import_path$';\n");
  }

  printer.PrintRaw("\n");
}

void PrintAngularModule
  ( Printer&                                 printer
  , const vector<const ServiceDescriptor*>&  services
  , AngularGrpcCodeGeneratorOptions&         options
  )
{
  assert(!options.moduleName.empty());

  map<string, string> vars = {
    {"module_name", options.moduleName}
  };

  printer.Print(vars, "@NgModule({\n");
  printer.Indent();

  printer.Print(vars, "providers: [\n");
  printer.Indent();

  for(auto service : services) {
    
    vars["service_name"] = service->name();
    printer.Print(vars, "$service_name$,\n");
  }

  printer.Outdent();
  printer.Print(vars, "],\n");

  printer.Outdent();
  printer.Print(vars, "})\n");
  printer.Print(vars, "export class $module_name$ {\n}\n");
}

AngularGrpcCodeGenerator::AngularGrpcCodeGenerator() {}

AngularGrpcCodeGenerator::~AngularGrpcCodeGenerator() {}


bool AngularGrpcCodeGenerator::GenerateAll
  ( const std::vector<const FileDescriptor*>&  files
  , const string&                              parameter
  , GeneratorContext*                          context
  , string*                                    error
  ) const
{

  AngularGrpcCodeGeneratorOptions options(parameter);

  if(options.hasError(error)) {
    return false;
  }

  for(const FileDescriptor* file : files) {
    if(!Generate(file, parameter, context, error)) {
      return false;
    }
  }

  if(!options.moduleName.empty()) {

    string filename =
      removePathExtname(options.moduleName) + "_ng_grpc.module.ts";
    Printer printer(context->Open(filename), '$');

    std::vector<const ServiceDescriptor*> services;

    for(auto file : files) {
      auto serviceCount = file->service_count();

      for(auto i=0; serviceCount > i; ++i) {
        auto service = file->service(i);
        services.push_back(service);
      }
    }

    PrintAngularModuleImports(printer, services, options);
    PrintAngularModule(printer, services, options);
  }

  return true;
}

bool AngularGrpcCodeGenerator::Generate
  ( const FileDescriptor*  file
  , const string&          parameter
  , GeneratorContext*      context
  , string*                error
  ) const
{

  AngularGrpcCodeGeneratorOptions options(parameter);

  if(options.hasError(error)) {
    return false;
  }

  string filename = removePathExtname(file->name()) + "_ng_grpc_pb.ts";
  Printer printer(context->Open(filename), '$');

  PrintAngularServiceCommonImports(printer, options);
  PrintFileServiceMessageImports(printer, *file, options);
  PrintFileServiceImports(printer, *file, options);
  PrintFileAngularServices(printer, *file, options);

  return true;
}
