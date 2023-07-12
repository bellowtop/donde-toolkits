#pragma once

#include <condition_variable>
#include <memory>
#include <string>
#include <thread>

namespace donde_toolkits {

namespace feature_extract {

class VideoFrame {};

// borrow the frame pointer. donot own it. donot free it.
// using FrameProcessor = bool (*)(const VideoFrame *f);
using VideoFrameProcessor = std::function<bool(const VideoFrame* f)>;

class Processor {
  public:
    virtual bool Process() = 0;
    virtual bool Register(const VideoFrameProcessor& p) = 0;

    virtual bool Pause() = 0;
    virtual bool Resume() = 0;

    virtual bool Stop() = 0;
};

} // namespace feature_extract

} // namespace donde_toolkits
