
#include "donde/video_process/ffmpeg_processor.h"

#include <chrono>
#include <iostream>
#include <thread>

using donde_toolkits::video_process::FFmpegVideoFrame;
using donde_toolkits::video_process::FFmpegVideoProcessor;

bool callback(const FFmpegVideoFrame* frame) {
    std::cout << "process frame " << frame->getFrameId() << std::endl;
    return true;
};

int main(int argc, char** argv) {
    FFmpegVideoProcessor v{};

    v.AddObserver([&](const FFmpegVideoFrame* f) -> bool { return true; });
    v.OpenVideoContext("./contrib/Iron_Man-Trailer_HD.mp4");
    v.Process({
        .warm_up_frames = 0,
        .skip_frames = 1,
        .decode_fps = 30,
        .loop_forever = true,
    });

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
