#include <google/protobuf/compiler/plugin.h>
#include "angular-generator.hh"

using google::protobuf::compiler::PluginMain;

int main(int argc, char* argv[]) {
  AngularGrpcCodeGenerator generator;
  PluginMain(argc, argv, &generator);
  return 0;
}
