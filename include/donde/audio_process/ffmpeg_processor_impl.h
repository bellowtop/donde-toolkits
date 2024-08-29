#pragma once

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
#include <thread>

namespace donde_toolkits ::audio_process {

class FFmpegAudioProcessorImpl {
  public:
    FFmpegAudioProcessorImpl();

    AudioStreamInfo OpenContext(const std::string& filepath);

    bool Transcode(const std::string& output_filepath);

    ~FFmpegAudioProcessorImpl();

  private:
    bool open_context();

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

    AVFormatContext* format_context_ = nullptr;
    AVCodecContext* audio_codec_context_ = nullptr;
    int audio_stream_index_ = -1;

    // for audio transcode output.
    AVIOContext* output_io_context_ = nullptr;
    AVFormatContext* output_format_context_ = nullptr;
    const AVCodec* output_codec_ = nullptr;
    AVStream* output_stream_ = nullptr;
    AVCodecContext* output_codec_context_ = nullptr;
    SwrContext* output_swr_context_ = nullptr;
    AVAudioFifo* output_fifo_ = nullptr;

    std::mutex audio_fifo_ready_mu_;
    std::condition_variable audio_fifo_ready_cv_;

    msd::channel<AVPacket*> audio_packet_ch_{1};
    msd::channel<AVFrame*> audio_frame_ch_{1};
    std::thread demux_thread_;
    std::thread decode_thread_;
    std::thread trancode_thread_;
    std::thread save_thread_;
    std::thread monitor_thread_;
    std::mutex demux_mu_;
    std::condition_variable demux_cv_;
    // end audio transcode output.

    const int output_bit_rate = 96000; // bit/s
    const int output_frame_size_ = 1024;
    const int output_sample_rate_ = 16000;
    const int output_nb_channels_ = 1;
    const AVCodecID output_codec_id_ = AV_CODEC_ID_PCM_S16LE;

    std::atomic_bool quit_ = false;
    std::atomic_bool pause_ = false;
    ProcessOptions processor_opts_;
};

} // namespace donde_toolkits::audio_process
