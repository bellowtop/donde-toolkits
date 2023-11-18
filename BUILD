load("@rules_cc//cc:defs.bzl", "cc_library", "cc_binary")
load("@bazel_skylib//rules:common_settings.bzl", "string_flag")



string_flag(
	name = "enable_module",
	build_setting_default = "feature_extract",
)

config_setting(
	name = "feature_extract",
	flag_values = {
	       ":enable_module": "feature_extract",
	},
)

config_setting(
	name = "feature_search",
	flag_values = {
	       ":enable_module": "feature_search",
	},
)

config_setting(
	name = "video_process",
	flag_values = {
	       ":enable_module": "video_process",
	},
)

##
## cc_binary(
##     name = "donde",
##     srcs = ["examples/feature_extract.cc"],
##     deps = select({
##     	":feature_extract": ["//modules/feature_extract:feature_extract"],
##     	":feature_search": ["//modules/feature_search:feature_search"],
##     	":video_process": ["//modules/video_process:video_process"],
##     }),
## )
##

cc_library(
    name = "donde",
    deps = select({
    	 ":feature_extract": ["//modules/feature_extract:feature_extract"],
    	 ":feature_search": ["//modules/feature_search:feature_search"],
    	 ":video_process": ["//modules/video_process:video_process"],
    }),
)

load("@com_github_google_rules_install//installer:def.bzl", "installer")
installer(
    name = "install_donde",
    data = select({
    	":feature_extract": ["//modules/feature_extract:feature_extract"],
    	":feature_search": ["//modules/feature_search:feature_search"],
    	":video_process": ["//modules/video_process:video_process"],
    }),
)
