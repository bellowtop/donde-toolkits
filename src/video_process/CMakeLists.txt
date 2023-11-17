cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(video_process VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message("CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")



## set(CMAKE_PREFIX_PATH "build" ${CMAKE_PREFIX_PATH})
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(Poco REQUIRED)
find_package(SQLiteCpp REQUIRED)
find_package(nlohmann_json REQUIRED)




# ---- Create library ----
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)
file(GLOB_RECURSE sources_h CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*)

add_library(video_process OBJECT ${sources} ${sources_h} ${headers})

target_link_libraries(video_process
    PRIVATE
    fmt::fmt
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

add_subdirectory(3rdparty/cpp-channel)
target_include_directories(video_process
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cpp-channel/include
)



## ffmpeg
##
# CMAKE_MODULE_PATH:FILEPATH=./cmake
##
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
message("CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")

##
# Find FFmpeg include directories and link libraries.
# Find OpenCV
##

find_package(FFMPEG REQUIRED)
message("FFMPEG_LIBRARIES: ${FFMPEG_LIBRARIES}")
message("FFMPEG_INCLUDE_DIRS: ${FFMPEG_INCLUDE_DIRS}")

find_package(OpenCV REQUIRED CONFIG)
message("OpenCV_FOUND: ${OpenCV_FOUND}")
message("OpenCV_LIBS: ${OpenCV_LIBS}")

# we use ffmpeg in our public header. so link it PUBLIC'ly
target_link_libraries(video_process
    PUBLIC
    ${FFMPEG_LIBRARIES}
)

target_include_directories(video_process
    # these are system headers, managed by homebrew
    SYSTEM
    PUBLIC
    ${FFMPEG_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)


target_include_directories(video_process
    PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

#####
#####
#####
##### # string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)
#####
#####
#####
##### if (${PROJECT_NAME}_ENABLE_UNIT_TESTING)
#####     enable_testing()
#####     message(STATUS "Build unit tests for the project. Tests should always be found in the test folder\n")
#####     add_subdirectory(tests)
##### endif ()
#####
##### if (${PROJECT_NAME}_ENABLE_EXAMPLES)
#####     add_subdirectory(examples)
##### endif ()
#####
#####

####
#### ##########################################
#### ### cmake install
#### ##########################################
####
#### set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "where the library will be installed to " FORCE)
#### install(TARGETS ${PROJECT_NAME}
####     EXPORT "${PROJECT_NAME}Targets"
####     PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
####     INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#### )
####
#### include(GNUInstallDirs)
#### foreach(header ${headers})
####     file(RELATIVE_PATH header_file_path "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}" "${header}")
####     get_filename_component(header_directory_path "${header_file_path}" DIRECTORY)
####     install(FILES ${header} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${header_directory_path}")
#### endforeach()
####
####
####
#### ##########################################
#### ### generate and install export file
#### ##########################################
#### set(namespace "donde")
####
#### install(EXPORT "${PROJECT_NAME}Targets"
####     FILE "${PROJECT_NAME}Targets.cmake"
####     NAMESPACE ${namespace}::
####     DESTINATION cmake
#### )
#### include(CMakePackageConfigHelpers)
####
#### # generate version file from config file
#### write_basic_package_version_file(
####     "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
####     VERSION "${version}"
####     COMPATIBILITY  AnyNewerVersion
#### )
#### # create config file, depends on ./Config.cmake.in
#### configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
####     "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
####     INSTALL_DESTINATION cmake
#### )
#### # install config file
#### install(FILES
####     "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
####     "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
####     DESTINATION cmake
#### )
#### # generate the export targets for the build tree
#### export(EXPORT "${PROJECT_NAME}Targets"
####     FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Targets.cmake"
####     NAMESPACE ${namespace}::
#### )
#### #####
####
