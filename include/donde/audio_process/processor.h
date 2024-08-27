#pragma once

#include <cstdint>
#include <string>

namespace donde_toolkits ::audio_process {

struct ProcessOptions {};

struct AudioStreamInfo {
    bool open_success;
};

class AudioProcessor {
  public:
    virtual AudioStreamInfo OpenAudioContext(const std::string& filepath) = 0;

    virtual void TransCode(const ProcessOptions& opts) = 0;
};

} // namespace donde_toolkits::audio_process
