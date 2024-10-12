#pragma once

#include <vector>
#include <string>

namespace donde_toolkits ::audio_process {

struct ProcessOptions {};

struct AudioTrack {
    std::string lang;
};

struct AudioStreamInfo {
    bool open_success;
    std::vector<AudioTrack> audioTracks;
};

class AudioProcessor {
  public:
    virtual AudioStreamInfo OpenContext(const std::string& filepath) = 0;

    virtual void Transcode(const std::string& output_filepath) = 0;
};

} // namespace donde_toolkits::audio_process
