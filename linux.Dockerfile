FROM zaucy/bazel:latest-gcc

WORKDIR /protoc-gen-angular

COPY WORKSPACE BUILD ./

COPY ./ ./

ENTRYPOINT bazel build -c opt :protoc-gen-angular && cp ./bazel-out/k8-opt/bin/protoc-gen-angular ./release/protoc-gen-angular-linux ; bazel shutdown
