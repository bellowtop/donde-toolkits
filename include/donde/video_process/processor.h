#pragma once

#include <cstdint>
#include <string>
#include <sys/_types/_int64_t.h>
#include <vector>

namespace donde_toolkits ::video_process {

struct ProcessOptions {
    int warm_up_frames;
    int skip_frames;
    int decode_fps;
    bool loop_forever;
};

struct AudioTrack {};

struct VideoStreamInfo {
    bool open_success;
    int64_t nb_frames;
    int64_t duration_seconds;
    double avg_frame_rate;
    int time_units_per_second;
    int64_t width;
    int64_t height;

    std::vector<uint8_t> cover_file_data{};
    std::string cover_file_ext{};
};

class VideoProcessor {
  public:
    virtual VideoStreamInfo OpenVideoContext(const std::string& filepath) = 0;

    virtual void Process(const ProcessOptions& opts) = 0;

    virtual bool Pause() = 0;

    virtual bool Resume() = 0;

    virtual bool Stop() = 0;
};

} // namespace donde_toolkits::video_process
