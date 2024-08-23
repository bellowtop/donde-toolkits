cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(DondeToolkits VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message("CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)


include("cmake/deps.cmake")




# ---- Create library ----
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)
file(GLOB_RECURSE sources_h CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*)

add_library(${PROJECT_NAME} STATIC ${sources} ${sources_h} ${headers})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)


target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Poco::Foundation
    spdlog::spdlog
    SQLiteCpp
    nlohmann_json::nlohmann_json
    msd_channel
)





##
## Find FFMPEG
##

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
message("CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(FFMPEG_HOME_DIR "C:/Users/nicke/Downloads/libraries/ffmpeg-master-latest-win64-gpl-shared")
endif()
find_package(FFMPEG)
message("FFMPEG_LIBRARIES: ${FFMPEG_LIBRARIES}")
message("FFMPEG_INCLUDE_DIRS: ${FFMPEG_INCLUDE_DIRS}")
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${FFMPEG_LIBRARIES}
)
target_include_directories(${PROJECT_NAME}
    SYSTEM
    PUBLIC
    ${FFMPEG_INCLUDE_DIRS}
)



##
## Find OpenCV
##

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  list(APPEND CMAKE_PREFIX_PATH "C:/Users/nicke/Downloads/libraries/opencv4.8/build")
endif()

find_package(OpenCV REQUIRED CONFIG)
message("OpenCV_FOUND: ${OpenCV_FOUND}")
message("OpenCV_LIBS: ${OpenCV_LIBS}")
# we use opencv openvino in our implementation, so link them PRIVATE'ly
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${OpenCV_LIBS}
)
target_include_directories(${PROJECT_NAME}
    SYSTEM
    PUBLIC
    ${OpenCV_INCLUDE_DIRS}
)




##
## Find OpenVINO
##

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(OPENVINO_HOME_DIR "C:/Users/nicke/Downloads/libraries/w_openvino_toolkit_windows_2023.0.2.11065.e662b1a3301_x86_64")
endif()
list(APPEND CMAKE_PREFIX_PATH "${OPENVINO_HOME_DIR}/runtime/cmake")
find_package(OpenVINO REQUIRED COMPONENTS Runtime)
message("OpenVINO_FOUND: ${OpenVINO_FOUND}")
message("OpenVINO_Runtime_FOUND: ${OpenVINO_Runtime_FOUND}")
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    openvino::runtime
)


##
## Find Torch
##
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/Cellar/pytorch/2.2.0_8/share/cmake/Torch/")
find_package(Torch REQUIRED)
message("TORCH_INCLUDE_DIRS: ${TORCH_INCLUDE_DIRS}")
message("TORCH_LIBRARIES: ${TORCH_LIBRARIES}")
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    "${TORCH_LIBRARIES}"
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



# if (${PROJECT_NAME}_ENABLE_UNIT_TESTING)
#     enable_testing()
#     message(STATUS "Build unit tests for the project. Tests should always be found in the test folder\n")
    add_subdirectory(tests)
# endif ()

# if (${PROJECT_NAME}_ENABLE_EXAMPLES)
#     add_subdirectory(examples)
# endif ()



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



# ##########################################
# ### generate and install export file
# ##########################################
# set(namespace "donde")

# install(EXPORT "${PROJECT_NAME}Targets"
#     FILE "${PROJECT_NAME}Targets.cmake"
#     NAMESPACE ${namespace}::
#     DESTINATION cmake
# )
# include(CMakePackageConfigHelpers)

# # generate version file from config file
# write_basic_package_version_file(
#     "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
#     VERSION "${version}"
#     COMPATIBILITY  AnyNewerVersion
# )
# # create config file, depends on ./Config.cmake.in
# configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
#     "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
#     INSTALL_DESTINATION cmake
# )
# # install config file
# install(FILES
#     "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
#     "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
#     DESTINATION cmake
# )
# # generate the export targets for the build tree
# export(EXPORT "${PROJECT_NAME}Targets"
#     FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Targets.cmake"
#     NAMESPACE ${namespace}::
# )
