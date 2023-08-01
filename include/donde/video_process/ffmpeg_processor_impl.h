#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "channel.h"
#include "ffmpeg_processor.h"

#include <condition_variable>
#include <memory>
#include <string>
#include <thread>

namespace donde_toolkits ::video_process {

class FFmpegVideoProcessorImpl {
  public:
    FFmpegVideoProcessorImpl(const std::string& filepath);

    bool Process();
    bool Register(const FFmpegVideoFrameProcessor& p);

    bool Pause();
    bool IsPaused();
    bool Resume();

    bool Stop();

    ~FFmpegVideoProcessorImpl();

  private:
    bool open_context();

    void demux_video_packet_();
    void decode_video_frame_();
    void process_video_frame_();

  private:
    std::string filepath;

    AVFormatContext* format_context_ = nullptr;
    AVCodecContext* codec_context_ = nullptr;
    SwsContext* sws_context_ = nullptr;
    int video_stream_index_ = -1;

    size_t video_width_ = 0;
    size_t video_height_ = 0;

    bool quit_ = false;
    bool pause_ = false;

    std::mutex demux_mu_;
    std::condition_variable demux_cv_;

    std::thread demux_thread_;
    std::thread decode_thread_;
    std::thread process_thread_;

    size_t frame_count = 0;
    FFmpegVideoFrameProcessor frame_processor;

    // used to send demuxed packet
    Channel<AVPacket*> packet_ch_;
    // used to send decoded frame
    Channel<AVFrame*> frame_ch_;
};

} // namespace donde_toolkits::video_process
