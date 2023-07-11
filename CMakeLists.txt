cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(DondeToolkits LANGUAGES CXX)

if (NOT DEFINED CONAN_ALREADY_SETUP)
  include(build/conanbuildinfo.cmake)
  conan_basic_setup(TARGETS)
  include_directories(${CONAN_INCLUDE_DIRS})
  set(CONAN_ALREADY_SETUP true)
endif()


# ----- openvino library ----
# install openvino distribution
if(APPLE)
  # please refer to README_mac_m1.md for openvino install instructions. it has extra steps for openvino running in mac m1.
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
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

file(GLOB_RECURSE includes CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*)

add_library(${PROJECT_NAME} ${sources} ${headers} ${includes})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)



#
# Set the build/user include directories
#

# Allow usage of header files in the `src` directory, but only for utilities
if (${PROJECT_NAME}_BUILD_HEADERS_ONLY)
  message(">>>> build header only")
    target_include_directories(
            ${PROJECT_NAME}
            INTERFACE
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )
else ()
  message(">>>> build something else: ${PROJECT_NAME}, ${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_include_directories(
            ${PROJECT_NAME}
            PUBLIC
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    )
    if (${PROJECT_NAME}_BUILD_EXECUTABLE AND ${PROJECT_NAME}_ENABLE_UNIT_TESTING)
      message(">>>> build exec and ut")
        target_include_directories(
                ${PROJECT_NAME}_LIB
                PUBLIC
                $<INSTALL_INTERFACE:include>
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        )
    endif ()
endif ()





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


if (${PROJECT_NAME}_ENABLE_UNIT_TESTING)
    enable_testing()
    message(STATUS "Build unit tests for the project. Tests should always be found in the test folder\n")
    add_subdirectory(tests)
endif ()
