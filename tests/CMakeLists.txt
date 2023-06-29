cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)


# use llvm clang, not the apple clang
# set(CMAKE_C_COMPILER    /opt/homebrew/opt/llvm@14/bin/clang)
# set(CMAKE_CXX_COMPILER  /opt/homebrew/opt/llvm@14/bin/clang++ )


if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options("-fstandalone-debug")
  add_compile_options("-stdlib=libc++")

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  add_compile_options("-std=c++20")
  set(CMAKE_CXX_STANDARD 20)

endif()

project(DondeToolkitsTests LANGUAGES CXX)


include(../cmake/CPM_0.35.1.cmake)
CPMAddPackage(NAME DondeToolkits SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)


if (NOT DEFINED CONAN_ALREADY_SETUP)
  include(${CMAKE_CURRENT_SOURCE_DIR}/../build/conanbuildinfo.cmake)
  conan_basic_setup(TARGETS)
  include_directories(${CONAN_INCLUDE_DIRS})
  set(CONAN_ALREADY_SETUP true)
endif()


# # ----- openvino library ----
# # install openvino distribution for macos
#
# set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH}, "/usr/local/runtime/cmake")
# find_package(OpenVINO REQUIRED COMPONENTS Runtime)


# ---- Create executable ----
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
add_executable(${PROJECT_NAME} ${headers} ${sources})

target_include_directories(${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../include>
  			 $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../src>
  			 $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/..>
         # $<INSTALL_INTERFACE:../include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

target_link_libraries(${PROJECT_NAME}
	CONAN_PKG::grpc
	CONAN_PKG::protobuf
	CONAN_PKG::fmt
	CONAN_PKG::opencv
	CONAN_PKG::poco
	CONAN_PKG::spdlog
	CONAN_PKG::msgpack
	CONAN_PKG::sqlitecpp
	CONAN_PKG::nlohmann_json
	CONAN_PKG::cassandra-cpp-driver
	CONAN_PKG::toml11
	CONAN_PKG::gtest
	CONAN_PKG::trompeloeil
	
	DondeToolkits
	)

# enable compiler warnings
if(NOT TEST_INSTALLED_VERSION)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wpedantic -Wextra )
  elseif(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /W4 /WX)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
  endif()
endif()

enable_testing()

# gtest_discover_tests(${PROJECT_NAME})
