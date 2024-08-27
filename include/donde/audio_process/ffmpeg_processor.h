#pragma once

extern "C" {
#include <libavutil/frame.h>
}

#include "processor.h"

#include <functional>
#include <memory>
#include <string>

namespace donde_toolkits ::audio_process {

class FFmpegAudioProcessorImpl;

class FFmpegAudioProcessor : public AudioProcessor {
  public:
    FFmpegAudioProcessor();

    AudioStreamInfo OpenAudioContext(const std::string& filepath);
    void Transcode(const ProcessOptions& opts);

    ~FFmpegAudioProcessor();

    std::unique_ptr<FFmpegAudioProcessorImpl> impl;
};

} // namespace donde_toolkits::audio_process
