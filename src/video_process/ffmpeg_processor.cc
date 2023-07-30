#include "donde/video_process/ffmpeg_processor.h"

#include "donde/video_process/ffmpeg_processor_impl.h"

namespace donde_toolkits ::feature_extract {

FFmpegVideoProcessor::FFmpegVideoProcessor(const std::string& filepath)
    : impl(new FFmpegVideoProcessorImpl(filepath)) {}

FFmpegVideoProcessor::~FFmpegVideoProcessor(){};

bool FFmpegVideoProcessor::Process() { return impl->Process(); };

bool FFmpegVideoProcessor::Register(const FFmpegVideoFrameProcessor& p) {
    return impl->Register(p);
}

bool FFmpegVideoProcessor::Pause() { return impl->Pause(); };

bool FFmpegVideoProcessor::IsPaused() { return impl->IsPaused(); };

bool FFmpegVideoProcessor::Resume() { return impl->Resume(); };

bool FFmpegVideoProcessor::Stop() { return impl->Stop(); };

} // namespace donde_toolkits::feature_extract
