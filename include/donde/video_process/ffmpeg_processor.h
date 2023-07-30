#pragma once

extern "C" {
#include <libavutil/frame.h>
}

#include "processor.h"

#include <condition_variable>
#include <memory>
#include <string>
#include <thread>

namespace donde_toolkits ::video_process {

// maybe we can have more frame methods here,
// so that to eliminate the avframe.h include.
// remove dependency for AVFrame
class FFmpegVideoFrame {
  public:
    FFmpegVideoFrame(AVFrame* f) : av_frame(f){};
    void* getFrame() { return av_frame; }
    ~FFmpegVideoFrame(){};

  private:
    AVFrame* av_frame;
};

using FFmpegVideoFrameProcessor = std::function<bool(const FFmpegVideoFrame* f)>;

// borrow the frame pointer. donot own it. donot free it.
// using VideoFrameProcessor = bool (*)(const AVFrame *f);

class FFmpegVideoProcessorImpl;

class FFmpegVideoProcessor : public VideoProcessor {
  public:
    FFmpegVideoProcessor(const std::string& filepath);

    bool Process();
    bool Register(const FFmpegVideoFrameProcessor& p);

    bool Pause();
    bool IsPaused();
    bool Resume();

    bool Stop();

    ~FFmpegVideoProcessor();

    std::unique_ptr<FFmpegVideoProcessorImpl> impl;
};

} // namespace donde_toolkits::video_process
