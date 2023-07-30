#pragma once

#include <condition_variable>
#include <memory>
#include <string>
#include <thread>

namespace donde_toolkits ::feature_extract {

class VideoFrame {
    virtual void* getFrame() = 0;
    // maybe more functions? such as getData, getPlane...
    // so that it can be a complete replacement for ffmpeg AVFrame
};

// borrow the frame pointer. donot own it. donot free it.
// using FrameProcessor = bool (*)(const VideoFrame *f);
using VideoFrameProcessor = std::function<bool(const VideoFrame* f)>;

class VideoProcessor {
  public:
    virtual bool Process() = 0;

    virtual bool Pause() = 0;
    virtual bool Resume() = 0;

    virtual bool Stop() = 0;
};

} // namespace donde_toolkits::feature_extract
