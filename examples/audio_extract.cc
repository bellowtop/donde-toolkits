
#include "donde/audio_process/ffmpeg_processor.h"

#include <chrono>
#include <iostream>
#include <thread>

using donde_toolkits::audio_process::FFmpegAudioProcessor;

int main(int argc, char** argv) {
    FFmpegAudioProcessor v{};

    v.OpenContext("./contrib/Iron_Man-Trailer_HD.mp4");
    v.Transcode("./contrib/Iron_Man-Trailer_HD.wav");

    return 0;
}
