load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")


##
## prepare header files
##
filegroup(
    name = "install_header_files",
    srcs = glob(["include/**/*.h", "include/**/*.hpp", "include/**/*.inl"]),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "video_process",
    srcs = glob(["src/**/*.cc", "src/**/*.h"]),
    hdrs = glob(["include/**/*.h", "include/**/*.hpp", "include/**/*.inl"]),
    includes = ["include", "include/msd"],
    copts = ["-Imodules/video_process/include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/common:common",
        "@fmt//:fmt",
        "@spdlog//:spdlog",
        "@poco//:poco",
        "@nlohmann_json//:nlohmann_json",
        "@ffmpeg//:ffmpeg",
    ],
)
