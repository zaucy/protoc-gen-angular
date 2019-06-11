load("@npm_angular_bazel//:index.bzl", "ng_module")
load("@com_google_protobuf//:protobuf.bzl", "proto_gen")

def ng_proto_module(
  name,
  plugin = "@com_github_zaucy_protoc_gen_angular//:protoc-gen-angular",
  **kwargs):

  proto_gen(
    name = name + "_angular_genproto",
    srcs = srcs,
    deps = [s + "_angular_genproto" for s in deps],
    plugin = plugin,
    plugin_language = "angular",
    plugin_options = [
      "module_name={}".format(name),
    ],
    protoc = protoc,
    outs = ["{}.grpc.module.ts".format(name)],
    visibility = ["//visibility:public"],
  )

  ng_module(
    name = name,
    **kwargs
  )
