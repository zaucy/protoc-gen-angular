FROM gcc:8

RUN apt-get -y update &&\
  apt-get -y install pkg-config zip g++ zlib1g-dev unzip python

ARG BAZEL_VERSION=0.14.1

RUN wget https://github.com/bazelbuild/bazel/releases/download/${BAZEL_VERSION}/bazel-${BAZEL_VERSION}-installer-linux-x86_64.sh

RUN chmod +x bazel-${BAZEL_VERSION}-installer-linux-x86_64.sh ./bazel-${BAZEL_VERSION}-installer-linux-x86_64.sh

RUN ./bazel-${BAZEL_VERSION}-installer-linux-x86_64.sh

RUN bazel version

WORKDIR /protoc-gen-angular

COPY WORKSPACE BUILD ./

RUN bazel build @com_google_protobuf//:protoc_lib

COPY ./ ./

CMD bazel build :protoc-gen-angular
