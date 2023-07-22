cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(DondeToolkits LANGUAGES CXX)

if (NOT DEFINED CONAN_ALREADY_SETUP)
  include(build/conanbuildinfo.cmake)
  conan_basic_setup(TARGETS)
  set(CONAN_ALREADY_SETUP true)
endif()


# ----- openvino library ----
# install openvino distribution
if(APPLE)
  # please refer to README_mac_m1.md for openvino install instructions. it has extra steps for openvino running in mac m1.
  list(APPEND CMAKE_PREFIX_PATH "/usr/local/runtime/cmake")
  list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/opencv/lib/cmake")
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
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

file(GLOB_RECURSE includes CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*)

add_library(${PROJECT_NAME} ${sources} ${headers} ${includes})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

target_link_libraries(${PROJECT_NAME}
    CONAN_PKG::fmt
    CONAN_PKG::spdlog
    CONAN_PKG::poco
    CONAN_PKG::msgpack
    CONAN_PKG::sqlitecpp
    CONAN_PKG::libuuid
    CONAN_PKG::nlohmann_json
    CONAN_PKG::cassandra-cpp-driver
    openvino::runtime
)
target_include_directories(${PROJECT_NAME}
    BEFORE
    PRIVATE
    ${CONAN_INCLUDE_DIRS}
)


## ffmpeg
##
# CMAKE_MODULE_PATH:FILEPATH=./cmake
##
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
##
# Find FFmpeg include directories and link libraries.
# Find OpenCV
##
find_package(FFMPEG REQUIRED)
find_package(OpenCV REQUIRED CONFIG)
target_link_libraries(${PROJECT_NAME}
    ${FFMPEG_LIBRARIES}
    ${OpenCV_LIBS}
)
target_include_directories(${PROJECT_NAME}
    # these are system headers, managed by homebrew
    SYSTEM
    PRIVATE
    ${FFMPEG_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

##
# 3rdparty
##
add_subdirectory(3rdparty/cppchannel)
target_link_libraries(${PROJECT_NAME} cppchannel)
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cppchannel)

#
# Set the build/user include directories
#

# Allow usage of header files in the `src` directory, but only for utilities
if (${PROJECT_NAME}_BUILD_HEADERS_ONLY)
    target_include_directories(${PROJECT_NAME}
	INTERFACE
	$<INSTALL_INTERFACE:include>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )
else ()
    target_include_directories(${PROJECT_NAME}
	PUBLIC
	$<INSTALL_INTERFACE:include>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    )

  if (${PROJECT_NAME}_BUILD_EXECUTABLE AND ${PROJECT_NAME}_ENABLE_UNIT_TESTING)
      target_include_directories(${PROJECT_NAME}_LIB
	  PUBLIC
	  $<INSTALL_INTERFACE:include>
	  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
      )
  endif ()
endif ()





# string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)



if (${PROJECT_NAME}_ENABLE_UNIT_TESTING)
    enable_testing()
    message(STATUS "Build unit tests for the project. Tests should always be found in the test folder\n")
    add_subdirectory(tests)
endif ()

if (${PROJECT_NAME}_ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif ()
