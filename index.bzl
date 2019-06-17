load("@npm_angular_bazel//:index.bzl", "ng_module")
load("@com_google_protobuf//:protobuf.bzl", "proto_gen")
load("@npm_bazel_typescript//:defs.bzl", "ts_library")

def _TsNgProtoOuts(srcs):
  ret = [s.path[:-len(".proto")] + "_ng_grpc_pb.ts" for s in srcs]
  return ret

# stolen from:
# https://github.com/protocolbuffers/protobuf/blob/master/protobuf.bzl
def _GenDir(ctx):
  return ctx.genfiles_dir.path + (
    "/" + ctx.attr.includes[0] if ctx.attr.includes and ctx.attr.includes[0] else ""
  )

def _ng_proto_module_srcs_impl(ctx):
  sources = depset()
  protoPaths = depset()
  protoSrcsRoots = []

  pkgLabel = ctx.label.package

  for dep in ctx.attr.deps:
    if hasattr(dep, "proto"):
      protoPaths = depset(transitive = [
        protoPaths,
        dep.proto.transitive_proto_path,
      ])
      protoSrcsRoots.append(dep.proto.proto_source_root)
      sources = depset(transitive = [sources, dep.proto.transitive_sources])

  outFiles = []

  for src in sources:
    srcBasename = src.path.rsplit("." + src.extension, 1)[0]

    for protoPath in protoPaths:
      if srcBasename.startswith(protoPath):
        srcBasename = srcBasename[len(protoPath)+1:]

    if srcBasename.startswith(ctx.bin_dir.path):
      srcBasename = srcBasename[len(ctx.bin_dir.path)+1:]
    if src.owner != None and src.owner.workspace_root:
      srcBasename = srcBasename[len(src.owner.workspace_root)+1:]
    
    filename = srcBasename + "_ng_grpc_pb.ts"
    file = ctx.actions.declare_file(filename)
    outFiles.append(file)

  if ctx.attr.ng_module_name:
    outFiles.append(ctx.actions.declare_file(ctx.attr.ng_module_name + "_ng_grpc.module.ts"))

  outDir = ctx.bin_dir.path
  if pkgLabel:
    outDir += '/' + pkgLabel

  protocArgs = []
  protocArgs.append("--plugin=protoc-gen-angular={}".format(
    ctx.executable.plugin.path
  ))
  protocArgs.append("--angular_out=grpc-web={},web_import_prefix={},grpc_web_import_prefix={},module_name={}:{}".format(
    ctx.attr.grpc_web_implementation,
    ctx.attr.web_import_prefix,
    ctx.attr.grpc_web_import_prefix,
    ctx.attr.ng_module_name,
    outDir,
  ))

  for protoSrcsRoot in protoSrcsRoots:
      protocArgs.append("-I" + protoSrcsRoot)

  for protoPath in protoPaths:
    if protoPath.startswith('external/'):
      protocArgs.append("-I" + ctx.bin_dir.path + "/" + protoPath)
    else:
      protocArgs.append("-I" + protoPath)

  for src in sources:
    protocArgs.append(src.path)

  ctx.actions.run(
    inputs = sources.to_list() + [
      ctx.executable.plugin,
    ],
    outputs = outFiles,
    executable = ctx.executable.protoc,
    arguments = protocArgs,
    mnemonic = "ProtoCompile",
    progress_message = "protoc (angular)",
  )

  return DefaultInfo(
    files = depset(outFiles),
  )

ng_proto_module_srcs = rule(
  implementation = _ng_proto_module_srcs_impl,
  attrs = {
    "web_import_prefix": attr.string(
      doc = "Prefix on import for *_pb.js files",
      mandatory = True,
    ),
    "grpc_web_import_prefix": attr.string(
      doc = "Prefix on import for *_grpc_pb.js files",
      mandatory = True,
    ),
    "grpc_web_implementation": attr.string(
      doc = "gRPC-web runtime implementation",
      values = ["google", "improbable-eng"],
      mandatory = True,
    ),
    "ng_module_name": attr.string(
      doc = "Module name used to generate <ng_module_name>_ng_grpc.module.ts file that provides all services",
      mandatory = False,
    ),
    "deps": attr.label_list(doc = "proto_library targets"),
    "protoc": attr.label(
      default = Label("@com_google_protobuf//:protoc"),
      executable = True,
      cfg = "host",
    ),
    "plugin": attr.label(
      default = Label("@com_github_zaucy_protoc_gen_angular//protoc-gen-angular"),
      executable = True,
      cfg = "host",
    ),
  },
)

def ng_proto_module(
  name,
  srcs = [],
  deps = [],
  ng_module_name = "",
  grpc_web_implementation = None,
  web_import_prefix = None,
  grpc_web_import_prefix = None,
  ts_deps = [],
  plugin = "@com_github_zaucy_protoc_gen_angular//protoc-gen-angular",
  protoc = "@com_google_protobuf//:protoc",
  **kwargs):

  tsSrcsTargetName = "{}__ng_proto_module_srcs".format(name)

  ng_proto_module_srcs(
    name = tsSrcsTargetName,
    deps = deps,
    protoc = protoc,
    plugin = plugin,
    ng_module_name = ng_module_name,
    grpc_web_implementation = grpc_web_implementation,
    web_import_prefix = web_import_prefix,
    grpc_web_import_prefix = grpc_web_import_prefix,
    visibility = ["//visibility:public"],
  )

  ng_deps = [
    "@npm//@angular/core",
    "@npm//rxjs",
  ]

  if grpc_web_implementation == "improbable-eng":
    ng_deps.append("@npm//@improbable-eng/grpc-web")
  elif grpc_web_implementation == "google":
    ng_deps.append("@npm//grpc-web")

  ng_deps.extend(ts_deps)

  ng_module(
    name = name,
    srcs = [":" + tsSrcsTargetName],
    deps = ng_deps,
    **kwargs
  )
