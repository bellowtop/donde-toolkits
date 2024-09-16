#include "donde/video_process/ffmpeg_processor.h"

#include "donde/video_process/ffmpeg_processor_impl.h"

namespace donde_toolkits ::video_process {

FFmpegVideoProcessor::FFmpegVideoProcessor() : impl(new FFmpegVideoProcessorImpl()) {}

FFmpegVideoProcessor::~FFmpegVideoProcessor(){};

VideoStreamInfo FFmpegVideoProcessor::OpenVideoContext(const std::string& filepath) {
    return impl->OpenVideoContext(filepath);
};

void FFmpegVideoProcessor::Process(const ProcessOptions& opts) { impl->Process(opts); };

bool FFmpegVideoProcessor::AddObserver(const VideoFrameObserver& p) { return impl->AddObserver(p); }

void FFmpegVideoProcessor::ScaleFrame(const AVFrame* originalFrame, AVFrame* destFrame) const {
    impl->ScaleFrame(originalFrame, destFrame);
}

bool FFmpegVideoProcessor::Seek(int seconds) { return impl->Seek(seconds); };

bool FFmpegVideoProcessor::Pause() { return impl->Pause(); };

bool FFmpegVideoProcessor::IsPaused() { return impl->IsPaused(); };

bool FFmpegVideoProcessor::Resume() { return impl->Resume(); };

bool FFmpegVideoProcessor::Stop() { return impl->Stop(); };

} // namespace donde_toolkits::video_process
