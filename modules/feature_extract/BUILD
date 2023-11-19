load("@rules_cc//cc:defs.bzl", "cc_library")


cc_library(
    name = "feature_extract",
    srcs = glob(["src/**/*.cc", "src/**/*.h"]),
    hdrs = glob(["include/**/*.h"]),
    includes = ["include"],
    copts = ["-Imodules/feature_extract/include"],
    visibility = ["//:__pkg__"],
    linkstatic = True,
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
