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

