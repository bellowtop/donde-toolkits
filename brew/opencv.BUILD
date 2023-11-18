load("@rules_cc//cc:defs.bzl", "cc_library")


cc_library(
    name = "opencv",
    srcs = glob(["lib/*.dylib"]),
    hdrs = glob(["include/**/*.hpp", "include/**/*.h"]),
    strip_include_prefix = "include/opencv4",
    visibility = ["//visibility:public"],
)
