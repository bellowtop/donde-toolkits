
#include "donde/audio_process/ffmpeg_processor.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

using donde_toolkits::audio_process::FFmpegAudioProcessor;

int main(int argc, char** argv) {
    FFmpegAudioProcessor v{};

    std::string video_filepath = argv[1];
    std::string audio_filepath = std::filesystem::path(video_filepath).replace_extension(".wav").string();

    v.OpenContext(video_filepath);
    v.Transcode(audio_filepath);

    return 0;
}
