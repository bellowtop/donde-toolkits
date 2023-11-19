load("//generated:dependencies.bzl", "load_conan_dependencies")
load_conan_dependencies()

new_local_repository(
    name = "opencv",
    path = "/opt/homebrew/Cellar/opencv/4.8.1_1",
    build_file = "brew/opencv.BUILD",
)

new_local_repository(
    name = "ffmpeg",
    path = "/opt/homebrew/Cellar/ffmpeg/6.0_1",
    build_file = "brew/ffmpeg.BUILD",
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
    urls = [
        "https://github.com/google/bazel_rules_install/archive/5ae7c2a8d22de2558098e3872fc7f3f7edc61fb4.zip",
    ],
    # The installer uses an option -T that is not available on MacOS, but
    # it is benign to leave out.
    # Upstream bug https://github.com/google/bazel_rules_install/issues/31
    patch_args = ["-p1"],
    patches = ["//bazel:installer.patch"],
    sha256 = "880217b21dbd40928bbe3bca3d97bd4de7d70d5383665ec007d7e1aac41d9739",
    strip_prefix = "bazel_rules_install-5ae7c2a8d22de2558098e3872fc7f3f7edc61fb4",
)

load("@com_github_google_rules_install//:deps.bzl", "install_rules_dependencies")

install_rules_dependencies()

load("@com_github_google_rules_install//:setup.bzl", "install_rules_setup")

install_rules_setup()



#
# pkg_tar
#

http_archive(
    name = "rules_pkg",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_pkg/releases/download/0.9.1/rules_pkg-0.9.1.tar.gz",
        "https://github.com/bazelbuild/rules_pkg/releases/download/0.9.1/rules_pkg-0.9.1.tar.gz",
    ],
    sha256 = "8f9ee2dc10c1ae514ee599a8b42ed99fa262b757058f65ad3c384289ff70c4b8",
)
load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")
rules_pkg_dependencies()



#
# cmake
#

http_archive(
    name = "rules_foreign_cc",
    # TODO: Get the latest sha256 value from a bazel debug message or the latest
    #       release on the releases page: https://github.com/bazelbuild/rules_foreign_cc/releases
    #
    # sha256 = "...",
    strip_prefix = "rules_foreign_cc-0258d350a9ff4c5145f131b7247d161a432dec8d",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0258d350a9ff4c5145f131b7247d161a432dec8d.tar.gz",
)

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies()
