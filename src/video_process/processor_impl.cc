#include "processor_impl.h"

#include "donde/video_process/processor.h"
#include "video_context.h"

namespace donde_toolkits {

namespace feature_extract {

ProcessorImpl::ProcessorImpl(const std::string& filepath){};
ProcessorImpl::~ProcessorImpl(){};

bool ProcessorImpl::Process() { return true; }
bool ProcessorImpl::Register(const VideoFrameProcessor& p) { return true; }

bool ProcessorImpl::Pause() { return true; }
bool ProcessorImpl::Resume() { return true; }

bool ProcessorImpl::Stop() { return true; }

} // namespace feature_extract

} // namespace donde_toolkits
