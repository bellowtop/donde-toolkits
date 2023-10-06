#pragma once

extern "C" {
#include <libavutil/frame.h>
}

#include "processor.h"

#include <functional>
#include <string>
#include <memory>

namespace donde_toolkits ::video_process {

// maybe we can have more frame methods here,
// so that to eliminate the avframe.h include.
// remove dependency for AVFrame
class FFmpegVideoFrame {
  public:
    FFmpegVideoFrame(long frame_id, AVFrame* f) : id(frame_id), av_frame(f){};
    const void* getFrame() const { return av_frame; }
    long getFrameId() const { return id; }

    ~FFmpegVideoFrame(){};

  private:
    long id;
    AVFrame* av_frame;
};

// borrow the frame pointer. donot own it. donot free it.
// using VideoFrameProcessor = bool (*)(const FFmpegVideoFrame *f);
using FFmpegVideoFrameProcessor = std::function<bool(const FFmpegVideoFrame* f)>;

class FFmpegVideoProcessorImpl;

class FFmpegVideoProcessor : public VideoProcessor {
  public:
    FFmpegVideoProcessor();

    VideoStreamInfo OpenVideoContext(const std::string& filepath);
    void Process(const ProcessOptions& opts);

    bool Register(const FFmpegVideoFrameProcessor& p);
    void ScaleFrame(const AVFrame* originalFrame, AVFrame* destFrame) const;
    bool Pause();
    bool IsPaused();
    bool Resume();

    bool Stop();

    ~FFmpegVideoProcessor();

    std::unique_ptr<FFmpegVideoProcessorImpl> impl;
};

} // namespace donde_toolkits::video_process
