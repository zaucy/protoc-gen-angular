set -ex

echo testing

bazel build @com_google_protobuf//:protoc ///protoc-gen-angular

../bazel-bin/external/com_google_protobuf/protoc.exe --plugin=protoc-gen-angular=../bazel-bin/protoc-gen-angular/protoc-gen-angular.exe --angular_out=grpc-web=improbable-eng,web_import_prefix=web/import/prefix,grpc_web_import_prefix=/grpc-web/import/prefix,module_name=TestModule:dist echo.proto
