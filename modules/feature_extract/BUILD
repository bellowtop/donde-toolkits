load("@rules_cc//cc:defs.bzl", "cc_library")

##
## prepare header files
##
filegroup(
    name = "headers",
    srcs = glob(["include/**/*.h"]),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "feature_extract",
    srcs = glob(["src/**/*.cc", "src/**/*.h"]),
    hdrs = glob(["include/**/*.h"]),
    includes = ["include"],
    copts = ["-Imodules/feature_extract/include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/common:common",
        "@fmt//:fmt",
        "@spdlog//:spdlog",
        "@poco//:poco",
        "@nlohmann_json//:nlohmann_json",
        "@opencv//:opencv",
        "@openvino//:openvino",
    ],
)
