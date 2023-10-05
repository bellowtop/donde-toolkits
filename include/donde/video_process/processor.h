#pragma once

namespace donde_toolkits ::video_process {

struct ProcessOptions {
    int warm_up_frames;
    int skip_frames;
    int decode_fps;
};

class VideoProcessor {
  public:
    virtual bool Process(const ProcessOptions& opts) = 0;

    virtual bool Pause() = 0;

    virtual bool Resume() = 0;

    virtual bool Stop() = 0;
};


} // namespace donde_toolkits::video_process
