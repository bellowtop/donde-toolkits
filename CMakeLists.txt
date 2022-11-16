cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)



# # use llvm clang, not the apple clang
# set(CMAKE_C_COMPILER    /opt/homebrew/opt/llvm/bin/clang )
# set(CMAKE_CXX_COMPILER  /opt/homebrew/opt/llvm/bin/clang++ )

project(Donde LANGUAGES CXX)

if (NOT DEFINED CONAN_ALREADY_SETUP)
  include(build/conanbuildinfo.cmake)
  conan_basic_setup(TARGETS)
  include_directories(${CONAN_INCLUDE_DIRS})
  set(CONAN_ALREADY_SETUP true)
endif()


# ----- openvino library ----
# install openvino distribution
if(APPLE)
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH}, "/usr/local/runtime/cmake")
  find_package(OpenVINO REQUIRED COMPONENTS Runtime)
elseif(UNIX AND NOT APPLE)
  set(OpenVINO_DIR "/opt/l_openvino_toolkit_ubuntu20_2022.2.0.7713.af16ea1d79a_x86_64/runtime/cmake")
endif()


# faiss
# find_package(faiss CONFIG REQUIRED)
# if (faiss_FOUND)
#   message(STATUS "faiss found ${faiss_FOUND}, ${faiss_INCLUDE_DIR}, ${faiss_LIBRARIES}")
# else()
#   message(STATUS "faiss not found, ${faiss_INCLUDE_DIR}, ${faiss_LIBRARIES}")
# endif()



# ---- Create library ----
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cc)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.h)

file(GLOB_RECURSE donde CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/donde/*)
file(GLOB_RECURSE api CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/api/*)

add_library(${PROJECT_NAME} ${sources} ${headers} ${donde} ${api})

target_include_directories(${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
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
	CONAN_PKG::cassandra-cpp-driver
	openvino::runtime
	# faiss
	)
