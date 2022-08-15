cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)


# use llvm clang, not the apple clang
# set(CMAKE_C_COMPILER    /usr/local/opt/llvm/bin/clang)
# set(CMAKE_CXX_COMPILER  /usr/local/opt/llvm/bin/clang++)

project(FaceRecognitionTests LANGUAGES CXX)


include(../cmake/CPM_0.35.1.cmake)

# ------ FaceRecognition library ----------
if(TEST_INSTALLED_VERSION)
  find_package(FaceRecognition REQUIRED)
else()
  CPMAddPackage(NAME FaceRecognition SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
endif()

include(../build/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
include_directories(${CONAN_INCLUDE_DIRS})


# ----- openvino library ----
# install openvino distribution for macos
set(OpenVINO_DIR "/opt/intel/openvino_2022/runtime/cmake")
find_package(OpenVINO REQUIRED COMPONENTS Runtime)




# ---- Create executable ----
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(${PROJECT_NAME} ${headers} ${sources})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

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
	CONAN_PKG::doctest
	CONAN_PKG::trompeloeil
	openvino::runtime
	FaceRecognition
	)

# enable compiler warnings
if(NOT TEST_INSTALLED_VERSION)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wpedantic -Wextra -Werror)
  elseif(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /W4 /WX)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
  endif()
endif()

enable_testing()

# Note: doctest and similar testing frameworks can automatically configure CMake tests. For other
# testing frameworks add the tests target instead:
#     add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

## no need to include doctest.cmake, conan did that.
# include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(${PROJECT_NAME})

# ---- code coverage ----

if(ENABLE_TEST_COVERAGE)
  target_compile_options(${PROJECT_NAME} PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
  target_link_options(${PROJECT_NAME} PUBLIC -fprofile-arcs -ftest-coverage)
endif()
