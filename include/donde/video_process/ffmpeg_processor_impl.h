#pragma once

#include <Poco/Notification.h>
#include <__atomic/atomic.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include "ffmpeg_processor.h"
#include "msd/channel.hpp"

#include <condition_variable>
#include <memory>
#include <string>
#include <vector>
#include <thread>

namespace donde_toolkits ::video_process {

class FFmpegVideoProcessorImpl {
  public:
    FFmpegVideoProcessorImpl();

    VideoStreamInfo OpenVideoContext(const std::string& filepath);

    void Process(const ProcessOptions& opts);

    bool ExtractAudio(const std::string& filepath);

    bool AddObserver(const VideoFrameObserver& p);

    void ScaleFrame(const AVFrame* originalFrame, AVFrame* destFrame) const;

    bool Seek(int seconds);
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

    // for audio trancode output
    bool start_audio_extract_context(const std::string& filepath);
    bool start_audio_extract_process(const std::string& filepath);
    bool clear_audio_extract_context();
    bool demux_audio_packet_();
    bool decode_audio_frame_();
    bool transcode_audio_frame_();
    bool save_output_audio_packet_();
    // end audio trancode output

    void monitor();

  private:
    std::string video_filepath_;

    // for read video and audio
    AVFormatContext* format_context_ = nullptr;
    AVCodecContext* video_codec_context_ = nullptr;
    int video_stream_index_ = -1;
    int32_t video_stream_units_per_second_ = 0;
    float video_stream_seconds_per_unit_ = 0.0f;

    // cover image
    int cover_stream_index_ = -1;
    std::string cover_image_file_ext_ = "";
    std::vector<uint8_t> cover_image_file_data_ = {};

    // for convert video frame fmt
    SwsContext* sws_context_ = nullptr;

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

    std::atomic<int> last_seek_seconds_ = 0;
    std::atomic<bool> need_seeking_ = false;

    bool is_demuxing_ = false;
    bool is_decoding_ = false;
    bool is_processing_ = false;
    ProcessOptions processor_opts_;

    size_t frame_count = 0;
    std::vector<VideoFrameObserver> frame_observers_;

    int decode_fps_ = 25;
    int warm_up_frames_ = 0;
    int skip_frames_ = 1;

    // used to send demuxed packet, un-buffered
    msd::channel<AVPacket*> packet_ch_{1};
    // used to send decoded frame, un-buffered
    msd::channel<AVFrame*> frame_ch_{1};
};

} // namespace donde_toolkits::video_process
