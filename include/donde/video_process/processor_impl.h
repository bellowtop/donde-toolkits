#include "donde/video_process/processor.h"

namespace donde_toolkits {

namespace feature_extract {

class ProcessorImpl : Processor {

  public:

    ProcessorImpl(const std::string& filepath);
    virtual ~ProcessorImpl();

    bool Process();
    bool Register(const VideoFrameProcessor& p);

    bool Pause();
    bool Resume();

    bool Stop();
};

} // namespace feature_extract

} // namespace donde_toolkits
