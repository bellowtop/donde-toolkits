cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(DondeToolkits VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message("CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")



# file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/donde/*.cc)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/donde/*.h)
add_library(${PROJECT_NAME} ${headers})
include_directories(src)

if (${PROJECT_NAME}_feature_extract)
    add_subdirectory(src/feature_extract)
    target_link_libraries(${PROJECT_NAME} PRIVATE feature_extract)
endif()

if (${PROJECT_NAME}_feature_identity)
    add_subdirectory(src/feature_identity)
    target_link_libraries(${PROJECT_NAME} PRIVATE feature_identity)
endif()

if (${PROJECT_NAME}_feature_search)
    add_subdirectory(src/feature_search)
    target_link_libraries(${PROJECT_NAME} PRIVATE feature_search)
endif()

if (${PROJECT_NAME}_video_process)
    add_subdirectory(src/video_process)
    target_link_libraries(${PROJECT_NAME} PRIVATE video_process)
endif()


target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)


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

