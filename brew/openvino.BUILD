load("@rules_cc//cc:defs.bzl", "cc_library")


cc_library(
    name = "openvino",
    srcs = glob(["lib/*.dylib"]),
    hdrs = glob(["include/**/*.hpp", "include/**/*.h"]),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
