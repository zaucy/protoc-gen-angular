#include <cctype>
#include <set>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include "generator.h"

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
}

namespace {

  enum GrpcWebImplementation {
    NONE = 0,
    GOOGLE = 1,
    IMPROBABLE_ENG = 2
  };

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
    printer.Print(vars,
      "callback(new Error('"
        "protoc-gen-angular unary call not implemented"
      "'));\n\n"
    );
  }

  void PrintAngularServiceImprobableEngUnaryCall
    ( map<string, string>  vars
    , Printer&             printer
    )
  {

    printer.Print(vars, "let responseMetadata: grpc.Metadata = null;\n\n");

    printer.Print(vars, "grpc.invoke(__service.$Method_name$, {\n");
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
      "  if(code != grpc.Code.OK) {\n"
      "    callback(new Error(msg));\n"
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

  }

  void PrintAngularServiceImprobableEngServerStreamingCall
    ( map<string, string>  vars
    , Printer&             printer
    )
  {
    printer.Print(vars, "let req = grpc.invoke(__service.$Method_name$, {\n");
    printer.Indent();

    printer.Print(vars,
      "request: request,\n"
      "host: (<any>window).DEFAULT_ANGULAR_GRPC_HOST || 'https://' + location.hostname,\n"
      "metadata: metadata,\n"
      "onMessage: response => this._ngZone.run(() => {\n"
      "  onMessage(response);\n"
      "}),\n"
      "onEnd: (code, msg, metadata) => this._ngZone.run(() => {\n"
      "  if(code == grpc.Code.OK) {\n"
      "    onEnd(code, msg, metadata);\n"
      "  } else {\n"
      "    onError(new Error(code + ' ' + (msg||'')));\n"
      "  }\n"
      "})\n"
    );

    printer.Outdent();
    printer.Print("});\n\n");
  }

  void PrintAngularServiceUnaryMethodBody
    ( map<string, string>           vars
    , Printer&                      printer
    , const MethodDescriptor&       method
    , const GrpcWebImplementation&  grpcWebImpl
    )
  {

    auto inputType = method.input_type();
    auto outputType = method.output_type();

    vars["method_name"] = firstCharToLower(method.name());
    vars["Method_name"] = method.name();
    vars["input_type"] = inputType->name();
    vars["output_type"] = outputType->name();

    printer.Print("let ret, callback, metadata;\n\n");

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

    printer.Print("callback = (err, response) => {\n");
    printer.Indent();

    printer.Print("if(err) reject(err);\n");
    printer.Print("else resolve(response);\n");

    printer.Outdent();
    printer.Print("};\n");

    printer.Outdent();
    printer.Print("});\n");

    printer.Outdent();
    printer.Print("}\n\n");

    switch(grpcWebImpl) {
      case GrpcWebImplementation::IMPROBABLE_ENG:
        PrintAngularServiceImprobableEngUnaryCall(vars, printer);
        break;
      case GrpcWebImplementation::GOOGLE:
        PrintAngularServiceGoogleUnaryCall(vars, printer);
        break;
      default:
        break;
    }

    printer.Print("return ret;\n");
  }

  void PrintAngularServiceServerStreamingMethodBody
    ( map<string, string>           vars
    , Printer&                      printer
    , const MethodDescriptor&       method
    , const GrpcWebImplementation&  grpcWebImpl
    )
  {
    auto inputType = method.input_type();
    auto outputType = method.output_type();

    vars["method_name"] = firstCharToLower(method.name());
    vars["Method_name"] = method.name();
    vars["input_type"] = inputType->name();
    vars["output_type"] = outputType->name();

    printer.Print("let ret, metadata, onMessage, onError, onEnd;\n\n");

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
      "  onMessage = (response) => {\n"
      "    subject.next(response);\n"
      "  };\n\n"
      "  onError = (err) => {\n"
      "    subject.error(err);\n"
      "  };\n\n"
      "  onEnd = (code, msg, metadata) => {\n"
      "    subject.complete();\n"
      "  };\n\n"
      "} else {\n"
      "  if(!onError) {\n"
      "    onError = (err) => console.error(err);\n"
      "  }\n\n"
      "  if(!onEnd) {\n"
      "    onEnd = (code, msg, metadata) => {};\n"
      "  }\n"
      "}\n\n"
    );

    switch(grpcWebImpl) {
      case GrpcWebImplementation::GOOGLE:
        PrintAngularServiceGoogleServerStreamingCall(vars, printer);
        break;
      case GrpcWebImplementation::IMPROBABLE_ENG:
        PrintAngularServiceImprobableEngServerStreamingCall(vars, printer);
        break;
      default:
        break;
    }

    printer.Print("ret.close = () => req.close();\n");

    printer.Print("return ret;\n");
  }

  void PrintAngularServiceUnaryMethod
    ( map<string, string>           vars
    , Printer&                      printer
    , const MethodDescriptor&       method
    , const GrpcWebImplementation&  grpcWebImpl
    )
  {
    auto inputType = method.input_type();
    auto outputType = method.output_type();

    vars["method_name"] = firstCharToLower(method.name());
    vars["Method_name"] = method.name();
    vars["input_type"] = inputType->name();
    vars["output_type"] = outputType->name();
    vars["cb_signature"] =
      "(err: any|null, response: " + outputType->name() + ", metadata: grpc.Metadata) => void";

    // A few signatures
    printer.Print(vars,
      "$method_name$("
        "request: $input_type$"
      "): Promise<$output_type$>;\n"
    );

    printer.Print(vars,
      "$method_name$("
        "request: $input_type$, "
        "metadata: grpc.Metadata"
      "): Promise<$output_type$>;\n"
    );
    
    printer.Print(vars,
      "$method_name$("
        "request: $input_type$, "
        "callback: $cb_signature$"
      "): void;\n"
    );

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

    PrintAngularServiceUnaryMethodBody(vars, printer, method, grpcWebImpl);

    printer.Outdent();

    printer.Print("}\n\n");
  }

  void PrintAngularServiceBidiStreamingMethod
    ( map<string, string>           vars
    , Printer&                      printer
    , const MethodDescriptor&       method
    , const GrpcWebImplementation&  grpcWebImpl
    )
  {

  }

  void PrintAngularServiceClientStreamingMethod
    ( map<string, string>           vars
    , Printer&                      printer
    , const MethodDescriptor&       method
    , const GrpcWebImplementation&  grpcWebImpl
    )
  {

  }
  
  void PrintAngularServiceServerStreamingMethod
    ( map<string, string>           vars
    , Printer&                      printer
    , const MethodDescriptor&       method
    , const GrpcWebImplementation&  grpcWebImpl
    )
  {
    auto inputType = method.input_type();
    auto outputType = method.output_type();

    vars["method_name"] = firstCharToLower(method.name());
    vars["Method_name"] = method.name();
    vars["input_type"] = inputType->name();
    vars["output_type"] = outputType->name();
    vars["msg_cb"] =
      "(message?: " + outputType->name() + ") => void";
    vars["error_cb"] = "(err) => void";
    vars["end_cb"] = "("
      "code: grpc.Code, "
      "msg: string|undefined, "
      "metadata: grpc.Metadata"
    ") => void";

    // A few signatures
    printer.Print(vars,
      "$method_name$("
        "request: $input_type$"
      "): {close():void}&Observable<$output_type$>;\n"
    );

    printer.Print(vars,
      "$method_name$("
        "request: $input_type$, "
        "metadata: grpc.Metadata"
      "): {close():void}&Observable<$output_type$>;\n"
    );
    
    printer.Print(vars,
      "$method_name$("
        "request: $input_type$, "
        "onMessage: $msg_cb$,"
        "onError?: $error_cb$,"
        "onEnd?: $end_cb$"
      "): void;\n"
    );

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
      "): {close():void}&Observable<$output_type$>|void {\n"
    );

    printer.Indent();

    PrintAngularServiceServerStreamingMethodBody(
      vars, printer, method, grpcWebImpl
    );

    printer.Outdent();

    printer.Print("}\n\n");
  }

  void PrintAngularService
    ( map<string, string>           vars
    , Printer&                      printer
    , const ServiceDescriptor&      service
    , const GrpcWebImplementation&  grpcWebImpl
    )
  {
    string filename = service.file()->name();
    filename = filename.substr(0, filename.size() - 6);

    vars["service_name"] = service.name();
    vars["service_import"] = filename + "_grpcjs_pb_service";
    vars["file_import_prefix"] = getImportPrefix(filename);
    printer.Print("import { Injectable, NgZone } from '@angular/core';\n");
    printer.Print("import { Observable } from 'rxjs';\n");
    printer.Print("import { Subject } from 'rxjs';\n");
    printer.Print("import { grpc } from 'grpc-web-client';\n\n");
    
    auto importTypes = GetAllServiceMessages(service);

    for(auto pair : importTypes) {
      map<string, string> importVars{vars};
      auto importName = pair.first;
      auto importType = pair.second;
      string filename = importType->file()->name();
      filename = filename.substr(0, filename.size() - 6);

      importVars["import_name"] = importName;
      importVars["type_import"] = filename + "_grpcjs_pb";

      printer.Print(importVars,
        "import { $import_name$ } from '$file_import_prefix$$web_import_prefix$/$type_import$';\n");
    }

    printer.Print("\n");

    printer.Print(vars,
      "import { $service_name$ as __service } from '$file_import_prefix$$grpc_web_import_prefix$/$service_import$';\n\n");

    printer.Print("@Injectable()\n");
    printer.Print(("export class " + service.name() + " {\n\n").c_str());

    printer.Indent();

    printer.Print(
      "constructor("
        "private _ngZone: NgZone"
      ") {}\n\n"
    );

    auto methodCount = service.method_count();

    for(auto i=0; methodCount > i; ++i) {
      auto method = service.method(i);

      if(!method->client_streaming() && !method->server_streaming()) {
        PrintAngularServiceUnaryMethod(vars, printer, *method, grpcWebImpl);
      } else
      if(method->client_streaming() && method->server_streaming()) {
        PrintAngularServiceBidiStreamingMethod(vars, printer, *method, grpcWebImpl);
      } else
      if(method->client_streaming()) {
        PrintAngularServiceClientStreamingMethod(vars, printer, *method, grpcWebImpl);
      } else
      if(method->server_streaming()) {
        PrintAngularServiceServerStreamingMethod(vars, printer, *method, grpcWebImpl);
      }
    }

    printer.Outdent();

    printer.Print("}\n");
  }
}

void PrintAngularModuleIndex
  ( Printer&                                      printer
  , const std::vector<const ServiceDescriptor*>&  services
  )
{

  printer.Print("import { NgModule } from '@angular/core';\n\n");

  for(auto service : services) {
    map<string, string> vars;

    vars["service_name"] = service->name();
    vars["service_import"] = "./" + service->name() + ".service";

    printer.Print(vars,
      "import { $service_name$ } from '$service_import$';\n");
  }

  printer.Print("\n@NgModule({\n");
  printer.Indent();

  printer.Print("providers: [\n");
  printer.Indent();

  for(auto service: services) {
    map<string, string> vars;
    vars["service_name"] = service->name();

    printer.Print(vars, "$service_name$,\n");
  }

  printer.Outdent();
  printer.Print("]\n");

  printer.Outdent();
  printer.Print("})\n");

  printer.Print("export class GeneratedGrpcAngularModule {\n");
  printer.Print("};\n\n");

  printer.Print("export default GeneratedGrpcAngularModule;\n");
}

AngularGrpcCodeGenerator::AngularGrpcCodeGenerator() {}

AngularGrpcCodeGenerator::~AngularGrpcCodeGenerator() {}

bool AngularGrpcCodeGenerator::GenerateFileGroup
  ( const string&                         rootDir
  , const vector<const FileDescriptor*>&  files
  , const string&                         parameter
  , GeneratorContext*                     context
  , string*                               error
  ) const
{
  std::vector<const ServiceDescriptor*> services;
  std::unique_ptr<ZeroCopyOutputStream> moduleFileStream(
    context->Open(rootDir + "/index.ts")
  );

  for(auto file : files) {
    auto serviceCount = file->service_count();

    for(auto i=0; serviceCount > i; ++i) {
      services.push_back(file->service(i));
    }

    if(!Generate(file, parameter, context, error)) {
      return false;
    }
  }

  Printer printer(moduleFileStream.get(), '$');

  PrintAngularModuleIndex(printer, services);

  return true;
}


bool AngularGrpcCodeGenerator::GenerateAll
  ( const std::vector<const FileDescriptor*>&  files
  , const string&                              parameter
  , GeneratorContext*                          context
  , string*                                    error
  ) const
{

  std::map<string, std::vector<const FileDescriptor*>> dirFiles;

  for(auto file : files) {
    string filename = file->name();
    string dir = parentPath(filename);

    auto findIt = dirFiles.find(dir);

    if(findIt == dirFiles.end()) {
      auto pair = dirFiles.insert({dir, vector<const FileDescriptor*>{}});
      findIt = pair.first;
    }

    findIt->second.push_back(file);
  }

  for(const auto& pair : dirFiles) {
    const auto& dir = pair.first;
    const auto& files = pair.second;

    if(!GenerateFileGroup(dir, files, parameter, context, error)) {
      return false;
    }
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

  if(file->service_count() == 0) {
    // No services, nothing to do.
    return true;
  }

  vector<pair<string, string> > options;
  ParseGeneratorParameter(parameter, &options);
  
  string grpcWebOutDir;
  string jsOut;

  GrpcWebImplementation grpcWebImpl = GrpcWebImplementation::NONE;

  for(auto keyValue : options) {
    const auto& key = keyValue.first;
    const auto& value = keyValue.second;

    if(key == "grpc-web") {
      if(value == "improbable-eng") {
        grpcWebImpl = GrpcWebImplementation::IMPROBABLE_ENG;
      } else
      if(value == "google") {
        grpcWebImpl = GrpcWebImplementation::GOOGLE;
      }
    } else
    if(key == "grpc-web_out") {
      grpcWebOutDir = value;
    } else
    if(key == "js_out") {
      jsOut = value;
    } else {
      *error = "Unknown option: " + key;
      return false;
    }
  }

  switch(grpcWebImpl) {
    case GrpcWebImplementation::GOOGLE:
    case GrpcWebImplementation::IMPROBABLE_ENG:
      break;
    default:
      *error = "options: invalid grpc-web value. "
        "Valid options are 'google' or 'improbable-eng'";
      return false;
  }

  if(grpcWebOutDir.empty()) {
    *error = "options: grpc-web_out is required";
    return false;
  }

  if(jsOut.empty()) {
    *error = "options: js_out is required";
    return false;
  }

  string filename = file->name();
  string dir = parentPath(filename);

  if(grpcWebOutDir[grpcWebOutDir.size()-1] == '/') {
    grpcWebOutDir = grpcWebOutDir.substr(1, grpcWebOutDir.size() - 1);
  }

  if(jsOut[jsOut.size()-1] == '/') {
    jsOut = jsOut.substr(1, jsOut.size() - 1);
  }

  map<string, string> vars;
  string package = file->package();
  vars["package"] = package;
  vars["package_dot"] = package.empty() ? "" : package + '.';
  vars["grpc_web_import_prefix"] = grpcWebOutDir;
  vars["web_import_prefix"] = jsOut;

  auto serviceCount = file->service_count();

  for(auto i=0; serviceCount > i; ++i) {
    auto service = file->service(i);
    std::unique_ptr<ZeroCopyOutputStream> fileStream(
      context->Open(dir + "/" + service->name() + ".service.ts")
    );

    Printer printer(fileStream.get(), '$');

    PrintAngularService(vars, printer, *service, grpcWebImpl);
  }

  return true;
}
