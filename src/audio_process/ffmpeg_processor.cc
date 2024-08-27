#include "donde/audio_process/ffmpeg_processor.h"

#include "donde/audio_process/ffmpeg_processor_impl.h"

namespace donde_toolkits ::audio_process {

FFmpegAudioProcessor::FFmpegAudioProcessor() : impl(new FFmpegAudioProcessorImpl()) {}

FFmpegAudioProcessor::~FFmpegAudioProcessor(){};

AudioStreamInfo FFmpegAudioProcessor::OpenContext(const std::string& filepath) { return impl->OpenContext(filepath); };

void FFmpegAudioProcessor::Transcode(const std::string& output_filepath) { impl->Transcode(output_filepath); };

} // namespace donde_toolkits::audio_process
