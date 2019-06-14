#pragma once

#include <google/protobuf/compiler/code_generator.h>

class AngularGrpcCodeGenerator
  : public google::protobuf::compiler::CodeGenerator
{
public:

  AngularGrpcCodeGenerator();
  ~AngularGrpcCodeGenerator() override;

  bool GenerateAll
    ( const std::vector<const google::protobuf::FileDescriptor*>&  files
    , const std::string&                                           parameter
    , google::protobuf::compiler::GeneratorContext*                context
    , std::string*                                                 error
    ) const override;

  bool Generate
    ( const google::protobuf::FileDescriptor*        file
    , const std::string&                             parameter
    , google::protobuf::compiler::GeneratorContext*  context
    , std::string*                                   error
    ) const override;
};
