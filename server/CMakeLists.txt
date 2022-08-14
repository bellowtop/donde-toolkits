cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# use llvm clang, not the apple clang
set(CMAKE_C_COMPILER    /usr/local/opt/llvm/bin/clang)
set(CMAKE_CXX_COMPILER  /usr/local/opt/llvm/bin/clang++)

project(FaceRecognitionServer LANGUAGES CXX)


include(../cmake/CPM_0.35.1.cmake)

# ------ FaceRecognition library ----------
CPMAddPackage(NAME FaceRecognition SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)


include(../build/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
include_directories(${CONAN_INCLUDE_DIRS})

# ----- openvino library ----
# install openvino distribution for macos
set(OpenVINO_DIR "/opt/intel/openvino_2022/runtime/cmake")
find_package(OpenVINO REQUIRED COMPONENTS Runtime)


# ----- GRPC pb ----
file(GLOB_RECURSE pb_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/protos/gen/pb-cpp/*.h")
file(GLOB_RECURSE pb_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/protos/gen/pb-cpp/*.cc")
add_library(PB ${pb_headers} ${pb_sources})
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/protos
)



# ---- Create executable ----
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(${PROJECT_NAME} ${headers} ${sources})
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "FaceRecognitionServer")

target_include_directories(${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../include>
         # $<INSTALL_INTERFACE:../include/${PROJECT_NAME}-${PROJECT_VERSION}>
)


target_link_libraries(${PROJECT_NAME}
	CONAN_PKG::grpc
	CONAN_PKG::protobuf
	CONAN_PKG::fmt
	CONAN_PKG::opencv
	CONAN_PKG::poco
	CONAN_PKG::spdlog
	CONAN_PKG::nlohmann_json
	CONAN_PKG::toml11
	openvino::runtime
	PB
	FaceRecognition
	)
