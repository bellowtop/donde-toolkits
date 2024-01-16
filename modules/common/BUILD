load("@rules_cc//cc:defs.bzl", "cc_library")


##
## prepare header files
##
filegroup(
    name = "install_header_files",
    srcs = glob(["include/**/*.h"]),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "common",
    srcs = glob([]),
    hdrs = glob(["include/**/*.h"]),
    includes = ["include"],
    copts = ["-Imodules/common/include"],
    visibility = ["//visibility:public"],
    deps = [
        "@opencv//:opencv",
    ]
)
