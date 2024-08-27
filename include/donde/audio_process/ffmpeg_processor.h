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

    AudioStreamInfo OpenContext(const std::string& filepath);
    void Transcode(const std::string& output_filepath);

    ~FFmpegAudioProcessor();

    std::unique_ptr<FFmpegAudioProcessorImpl> impl;
};

} // namespace donde_toolkits::audio_process
