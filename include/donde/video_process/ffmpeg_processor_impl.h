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
    FFmpegVideoProcessorImpl();

    VideoStreamInfo OpenVideoContext(const std::string& filepath);

    void Process(const ProcessOptions& opts);

    bool Register(const FFmpegVideoFrameProcessor& p);

    void ScaleFrame(const AVFrame* originalFrame, AVFrame * destFrame) const;

    bool Pause();
    bool IsPaused();
    bool Resume();

    bool ScaleFrame();

    bool Stop();

    ~FFmpegVideoProcessorImpl();

  private:
    bool open_context();

    void demux_video_packet_();
    void decode_video_frame_();
    void process_video_frame_();

    void monitor();

  private:
    std::string video_filepath_;

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
    std::thread monitor_thread_;

    bool started_ = false;
    bool start_over_ = false;

    bool is_demuxing_ = false;
    bool is_decoding_ = false;
    bool is_processing_ = false;
    ProcessOptions processor_opts_;

    size_t frame_count = 0;
    FFmpegVideoFrameProcessor frame_processor_;

    int decode_fps_ = 25;
    int warm_up_frames_ = 0;
    int skip_frames_ = 1;

    // used to send demuxed packet
    Channel<AVPacket*> packet_ch_;
    // used to send decoded frame
    Channel<AVFrame*> frame_ch_;
};

} // namespace donde_toolkits::video_process
