cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options("-fstandalone-debug")
endif()

project(${CMAKE_PROJECT_NAME}Tests LANGUAGES CXX)

if(${CMAKE_PROJECT_NAME}_BUILD_EXECUTABLE)
  set(${CMAKE_PROJECT_NAME}_TEST_LIB ${CMAKE_PROJECT_NAME}_LIB)
else()
  set(${CMAKE_PROJECT_NAME}_TEST_LIB ${CMAKE_PROJECT_NAME})
endif()

message("PROJECT_NAME: ${PROJECT_NAME}")

# # ----- openvino library ----
# # install openvino distribution for macos
#
# set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH}, "/usr/local/runtime/cmake")
# find_package(OpenVINO REQUIRED COMPONENTS Runtime)


# ---- Create executable ----
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_executable(${PROJECT_NAME} ${headers} ${sources})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ${${CMAKE_PROJECT_NAME}_TEST_LIB}
  CONAN_PKG::gtest
)
message("linked ${CMAKE_PROJECT_NAME}_TEST_LIB: ${${CMAKE_PROJECT_NAME}_TEST_LIB}")
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# enable compiler warnings
if(NOT TEST_INSTALLED_VERSION)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wpedantic -Wextra )
  elseif(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /W4 /WX)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
  endif()
endif()
