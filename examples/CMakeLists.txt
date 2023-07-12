CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(examples)

set(DONDE_TOOLKIT_LIB ${CMAKE_PROJECT_NAME})

add_executable(video_decode video_decode.cc)
target_link_libraries(video_decode
    PUBLIC
    ${DONDE_TOOLKIT_LIB})
