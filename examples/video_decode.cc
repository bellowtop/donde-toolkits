#include "donde/video_process/ffmpeg_processor.h"

#include <chrono>
#include <iostream>
#include <thread>

using donde_toolkits::feature_extract::FFmpegVideoFrame;
using donde_toolkits::feature_extract::FFmpegVideoFrameProcessor;
using donde_toolkits::feature_extract::FFmpegVideoProcessor;

bool callback(const FFmpegVideoFrame* frame) {
    std::cout << "process frame." << std::endl;
    return true;
};

int main() {
    FFmpegVideoProcessor v("/tmp/Iron_Man-Trailer_HD.mp4");
    FFmpegVideoFrameProcessor p(callback);
    v.Register(p);
    v.Process();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "sleep 1s" << std::endl;
        if (v.IsPaused()) {
            std::cout << "video processor is paused, contiune 100 frames" << std::endl;
            v.Resume();
        }
    }

    return 0;
}
