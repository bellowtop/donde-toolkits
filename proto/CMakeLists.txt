cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)



# # use llvm clang, not the apple clang
# set(CMAKE_C_COMPILER    /opt/homebrew/opt/llvm/bin/clang )
# set(CMAKE_CXX_COMPILER  /opt/homebrew/opt/llvm/bin/clang++ )

project(FeatureProtos LANGUAGES CXX)

include(../build/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
include_directories(${CONAN_INCLUDE_DIRS})


# ---- Create library ----
file(GLOB_RECURSE pb_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/gen/pb-cpp/*.h")
file(GLOB_RECURSE pb_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/gen/pb-cpp/*.cc")

add_library(${PROJECT_NAME} ${pb_sources} ${pb_headers})
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/
)

target_include_directories(${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/>
         # $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)
