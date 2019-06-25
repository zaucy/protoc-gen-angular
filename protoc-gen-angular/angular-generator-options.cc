#include "angular-generator-options.hh"

#include <vector>
#include <google/protobuf/compiler/code_generator.h>

using google::protobuf::compiler::ParseGeneratorParameter;

namespace {
  std::string removeTrailingSlash(std::string str) {
    char lastChar;

    while(str.size() > 0) {
      lastChar = str[str.size() - 1];

      if(lastChar == '/' || lastChar == '\\') {
        str = str.substr(0, str.size() - 1);
      } else {
        break;
      }
    }

    return str;
  }

  struct ImprobableEngGrpcWebInfo : GrpcWebImplementationInfo {
    inline ImprobableEngGrpcWebInfo() {
      commonjsImport = "@improbable-eng/grpc-web";
      commonjsImportAs = "{ grpc }";
      serviceFileSuffix = "_pb_service";
      metadataTypeName = "grpc.Metadata";
      statusCodeNamespace = "grpc.Code";
    }
  };

  struct GoogleGrpcWebInfo : GrpcWebImplementationInfo {
    inline GoogleGrpcWebInfo() {
      commonjsImport = "grpc-web";
      commonjsImportAs = "* as grpc";
      serviceFileSuffix = "_grpc_web_pb";
      metadataTypeName = "grpc.Metadata";
      statusCodeNamespace = "grpc.StatusCode";
    }
  };
}

bool GrpcWebImplementationInfo::isValid() const {
  return
    !commonjsImport.empty() &&
    !metadataTypeName.empty() &&
    !commonjsImportAs.empty() &&
    !serviceFileSuffix.empty() &&
    !statusCodeNamespace.empty();
}

AngularGrpcCodeGeneratorOptions::AngularGrpcCodeGeneratorOptions
  ( const std::string& parameter
  )
{
  std::vector<std::pair<std::string, std::string> > options;
  ParseGeneratorParameter(parameter, &options);

  for(auto keyValue : options) {
    const auto& key = keyValue.first;
    const auto& value = keyValue.second;

    if(key == "grpc-web") {
      if(value == "improbable-eng") {
        grpcWebImpl = GrpcWebImplementation::GWI_IMPROBABLE_ENG;
        grpcWebImplInfo = ImprobableEngGrpcWebInfo();
      } else
      if(value == "google") {
        grpcWebImpl = GrpcWebImplementation::GWI_GOOGLE;
        grpcWebImplInfo = GoogleGrpcWebInfo();
      }
    } else
    if(key == "grpc-web_out") {
      
    } else
    if(key == "js_out") {
      
    } else
    if(key == "module_name") {
      moduleName = value;
    } else
    if(key == "web_import_prefix") {
      webImportPrefix = removeTrailingSlash(value);
    } else
    if(key == "grpc_web_import_prefix") {
      grpcWebImportPrefix = removeTrailingSlash(value);
    } else {
      error_ = "Unknown option: " + key;
      break;
    }
  }

  switch(grpcWebImpl) {
    case GrpcWebImplementation::GWI_GOOGLE:
    case GrpcWebImplementation::GWI_IMPROBABLE_ENG:
      break;
    default:
      error_ = "options: invalid grpc-web value. "
        "Valid options are 'google' or 'improbable-eng'";
      return;
  }

  if(webImportPrefix.empty()) {
    error_ = "options: web_import_prefix must not be empty";
    return;
  }

  if(grpcWebImportPrefix.empty()) {
    error_ = "options: grpc_web_import_prefix must not be empty";
    return;
  }

  if(!grpcWebImplInfo.isValid()) {
    error_ = "options: Unexpected error with grpc web implementation info";
    return;
  }
}

bool AngularGrpcCodeGeneratorOptions::hasError
  ( std::string* error
  ) const
{
  if(!error_.empty()) {
    (*error) = error_;
    return true;
  }

  return false;
}
