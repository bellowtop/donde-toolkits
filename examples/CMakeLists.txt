CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(examples)

set(DONDE_TOOLKIT_LIB ${CMAKE_PROJECT_NAME})
set(CMAKE_BUILD_TYPE Debug)

add_executable(video_cover_image video_cover_image.cc)
target_link_libraries(video_cover_image
    PUBLIC
    ${DONDE_TOOLKIT_LIB})

add_executable(video_decode video_decode.cc)
target_link_libraries(video_decode
    PUBLIC
    ${DONDE_TOOLKIT_LIB})

add_executable(audio_extract audio_extract.cc)
target_link_libraries(audio_extract
    PUBLIC
    ${DONDE_TOOLKIT_LIB})

add_executable(text_recog text_recog.cc)
target_link_libraries(text_recog
    PUBLIC
    ${DONDE_TOOLKIT_LIB}

    Poco::Foundation
    spdlog::spdlog
    # SQLiteCpp
    nlohmann_json::nlohmann_json
    msd_channel
)
target_compile_features(text_recog PRIVATE cxx_std_20)
