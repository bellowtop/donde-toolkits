#include "donde/video_process/ffmpeg_processor.h"

#include "donde/video_process/ffmpeg_processor_impl.h"

namespace donde_toolkits ::video_process {

FFmpegVideoProcessor::FFmpegVideoProcessor(const std::string& filepath)
    : impl(new FFmpegVideoProcessorImpl(filepath)) {}

FFmpegVideoProcessor::~FFmpegVideoProcessor(){};

bool FFmpegVideoProcessor::Process(const ProcessOptions& opts) { return impl->Process(opts); };

bool FFmpegVideoProcessor::Register(const FFmpegVideoFrameProcessor& p) {
    return impl->Register(p);
}

void FFmpegVideoProcessor::ScaleFrame(const AVFrame* originalFrame, AVFrame* destFrame) const {
    impl->ScaleFrame(originalFrame, destFrame);
}

bool FFmpegVideoProcessor::Pause() { return impl->Pause(); };

bool FFmpegVideoProcessor::IsPaused() { return impl->IsPaused(); };

bool FFmpegVideoProcessor::Resume() { return impl->Resume(); };

bool FFmpegVideoProcessor::Stop() { return impl->Stop(); };

} // namespace donde_toolkits::video_process
