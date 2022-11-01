cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)



# # use llvm clang, not the apple clang
# set(CMAKE_C_COMPILER    /opt/homebrew/opt/llvm/bin/clang )
# set(CMAKE_CXX_COMPILER  /opt/homebrew/opt/llvm/bin/clang++ )

project(FeatureLibraries LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../build/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
include_directories(${CONAN_INCLUDE_DIRS})


# ----- openvino library ----
# install openvino distribution for macos
# set(OpenVINO_DIR "/opt/intel/openvino_2022/runtime/cmake")

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH}, "/usr/local/runtime/cmake")
find_package(OpenVINO REQUIRED COMPONENTS Runtime)

# faiss
find_package(faiss CONFIG REQUIRED)
if (faiss_FOUND)
  message(STATUS "faiss found ${faiss_FOUND}, ${faiss_INCLUDE_DIR}, ${faiss_LIBRARIES}")
else()
  message(STATUS "faiss not found, ${faiss_INCLUDE_DIR}, ${faiss_LIBRARIES}")
endif()


# ---- Create library ----
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h*)

add_library(${PROJECT_NAME} ${sources} ${headers})

target_include_directories(${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         # $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)


target_link_libraries(${PROJECT_NAME}
	CONAN_PKG::fmt
	CONAN_PKG::opencv
	CONAN_PKG::spdlog
	CONAN_PKG::poco
	CONAN_PKG::msgpack
	CONAN_PKG::sqlitecpp
	CONAN_PKG::libuuid
	CONAN_PKG::nlohmann_json
	openvino::runtime
	faiss
	)
