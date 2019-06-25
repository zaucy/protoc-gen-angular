#pragma once

#include <string>

enum GrpcWebImplementation {
  GWI_NONE = 0,
  GWI_GOOGLE = 1,
  GWI_IMPROBABLE_ENG = 2
};

struct GrpcWebImplementationInfo {
  std::string commonjsImport;
  std::string commonjsImportAs;
  std::string metadataTypeName;
  std::string serviceFileSuffix;
  std::string statusCodeNamespace;

  bool isValid() const;
};

class AngularGrpcCodeGeneratorOptions {
private:
  std::string error_;

public:

  GrpcWebImplementation grpcWebImpl;
  GrpcWebImplementationInfo grpcWebImplInfo;
  std::string webImportPrefix;
  std::string grpcWebImportPrefix;
  std::string moduleName;

  AngularGrpcCodeGeneratorOptions
    ( const std::string& parameter
    );

  bool hasError
    ( std::string* error
    ) const;

};
