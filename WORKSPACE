load("//generated:dependencies.bzl", "load_conan_dependencies")
load_conan_dependencies()

new_local_repository(
    name = "opencv",
    path = "/opt/homebrew/Cellar/opencv/4.8.1_1",
    build_file = "brew/opencv.BUILD",
)

new_local_repository(
    name = "openvino",
    path = "/opt/homebrew/Cellar/openvino/2023.1.0/",
    build_file = "brew/openvino.BUILD",
)


load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

#
# enable skylib
#


http_archive(
    name = "bazel_skylib",
    sha256 = "cd55a062e763b9349921f0f5db8c3933288dc8ba4f76dd9416aac68acee3cb94",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.5.0/bazel-skylib-1.5.0.tar.gz",
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.5.0/bazel-skylib-1.5.0.tar.gz",
    ],
)

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()


#
# bazle install
#
http_archive(
    name = "com_github_google_rules_install",
    urls = ["https://github.com/google/bazel_rules_install/releases/download/0.3/bazel_rules_install-0.3.tar.gz"],
    sha256 = "ea2a9f94fed090859589ac851af3a1c6034c5f333804f044f8f094257c33bdb3",
    strip_prefix = "bazel_rules_install-0.3",
)

load("@com_github_google_rules_install//:deps.bzl", "install_rules_dependencies")

install_rules_dependencies()

load("@com_github_google_rules_install//:setup.bzl", "install_rules_setup")

install_rules_setup()
