cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(DondeToolkits VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT DEFINED CONAN_ALREADY_SETUP)
  include(build/conanbuildinfo.cmake)
  conan_basic_setup(TARGETS)
  set(CONAN_ALREADY_SETUP true)
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
file(GLOB_RECURSE sources_h CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*)

add_library(${PROJECT_NAME} ${sources} ${sources_h} ${headers})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    CONAN_PKG::fmt
    CONAN_PKG::spdlog
    CONAN_PKG::poco
    CONAN_PKG::msgpack
    CONAN_PKG::sqlitecpp
    CONAN_PKG::libuuid
    CONAN_PKG::nlohmann_json
    CONAN_PKG::cassandra-cpp-driver
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
# Find OpenVino
##
find_package(FFMPEG REQUIRED)
find_package(OpenCV REQUIRED CONFIG)
find_package(OpenVINO REQUIRED COMPONENTS Runtime)
message("OpenVINO_FOUND: ${OpenVINO_FOUND}")
message("OpenVINO_Runtime_FOUND: ${OpenVINO_Runtime_FOUND}")
# we use opencv openvino in our implementation, so link them PRIVATE'ly
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${OpenCV_LIBS}
    openvino::runtime
)
# we use ffmpeg in our public header. so link it PUBLIC'ly
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    ${FFMPEG_LIBRARIES}
)

target_include_directories(${PROJECT_NAME}
    # these are system headers, managed by homebrew
    SYSTEM
    PUBLIC
    ${FFMPEG_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

##
# 3rdparty
##
add_subdirectory(3rdparty/cppchannel)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    cppchannel
)
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cppchannel
)

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
    )

  if (${PROJECT_NAME}_BUILD_EXECUTABLE AND ${PROJECT_NAME}_ENABLE_UNIT_TESTING)
      target_include_directories(${PROJECT_NAME}_LIB
	  PUBLIC
	  $<INSTALL_INTERFACE:include>
	  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
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



##########################################
### cmake install
##########################################

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "where the library will be installed to " FORCE)
install(TARGETS ${PROJECT_NAME}
    EXPORT "${PROJECT_NAME}Targets"
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include(GNUInstallDirs)
foreach(header ${headers})
    file(RELATIVE_PATH header_file_path "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}" "${header}")
    get_filename_component(header_directory_path "${header_file_path}" DIRECTORY)
    install(FILES ${header} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${header_directory_path}")
endforeach()



##########################################
### generate and install export file
##########################################
set(namespace "donde")

install(EXPORT "${PROJECT_NAME}Targets"
    FILE "${PROJECT_NAME}Targets.cmake"
    NAMESPACE ${namespace}::
    DESTINATION cmake
)
include(CMakePackageConfigHelpers)

# generate version file from config file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION "${version}"
    COMPATIBILITY  AnyNewerVersion
)
# create config file, depends on ./Config.cmake.in
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION cmake
)
# install config file
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    DESTINATION cmake
)
# generate the export targets for the build tree
export(EXPORT "${PROJECT_NAME}Targets"
    FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Targets.cmake"
    NAMESPACE ${namespace}::
)
