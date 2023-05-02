workspace(name = "com_github_zaucy_protoc_gen_angular")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
  name = "com_google_protobuf_old",
  sha256 = "9510dd2afc29e7245e9e884336f848c8a6600a14ae726adb6befdb4f786f0be2",
  url = "https://github.com/protocolbuffers/protobuf/archive/v3.6.1.3.zip",
  strip_prefix = "protobuf-3.6.1.3",
)

http_archive(
    name = "rules_proto_grpc",
    sha256 = "8383116d4c505e93fd58369841814acc3f25bdb906887a2023980d8f49a0b95b",
    strip_prefix = "rules_proto_grpc-4.1.0",
    urls = ["https://github.com/rules-proto-grpc/rules_proto_grpc/archive/4.1.0.tar.gz"],
)

load("@rules_proto_grpc//:repositories.bzl", "rules_proto_grpc_toolchains", "rules_proto_grpc_repos")
rules_proto_grpc_toolchains()
rules_proto_grpc_repos()

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")
rules_proto_dependencies()
rules_proto_toolchains()

load("@rules_proto_grpc//js:repositories.bzl", rules_proto_grpc_js_repos = "js_repos")

rules_proto_grpc_js_repos()