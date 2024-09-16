
#include "donde/video_process/ffmpeg_processor.h"

#include <chrono>
#include <fstream>
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
    auto videoInfo = v.OpenVideoContext(argv[1]);
    if (!videoInfo.cover_file_data.empty()) {
        // save to /tmp/
        auto filepath = "/tmp/cover" + videoInfo.cover_file_ext;
        std::cout << "cover filepath: " << filepath << std::endl;

        std::ofstream ofs(filepath, std::ios::binary | std::ios::out);
        if (!ofs) {
            std::cerr << "failed to open " << filepath << std::endl;
            return 1;
        }
        ofs.write(reinterpret_cast<const char*>(videoInfo.cover_file_data.data()), videoInfo.cover_file_data.size());
        ofs.close();
    }

    return 0;
}
