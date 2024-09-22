#include "donde/defer.h"
#include "msd/channel.hpp"

#include <Poco/Notification.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <ratio>
#include <sys/types.h>
#include <thread>
#include <tuple>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

#include "donde/video_process/ffmpeg_processor_impl.h"
#include "donde/video_process/utils.h"

namespace donde_toolkits ::video_process {

FFmpegVideoProcessorImpl::FFmpegVideoProcessorImpl() {
    // monitor_thread_ = std::thread([&] { monitor(); });
}

FFmpegVideoProcessorImpl::~FFmpegVideoProcessorImpl() {}

bool FFmpegVideoProcessorImpl::open_context() {
    int ret = avformat_open_input(&format_context_, video_filepath_.c_str(), nullptr, nullptr);
    if (ret < 0) {
        std::cout << "cannot open input video file: " << video_filepath_ << std::endl;
        return false;
    }

    // find video/audio stream
    {
        ret = avformat_find_stream_info(format_context_, nullptr);
        if (ret < 0) {
            std::cout << "cannot find stream info: " << video_filepath_ << std::endl;
            return false;
        }

        // for debug only.
        av_dump_format(format_context_, 0, video_filepath_.c_str(), 0);

        for (int i = 0; i < format_context_->nb_streams; i++) {
            auto& stream = format_context_->streams[i];
            if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                    cover_stream_index_ = i;
                    cover_image_file_ext_ = get_cover_image_file_extension(stream->codecpar->codec_id);
                    AVPacket pkt = stream->attached_pic;
                    cover_image_file_data_.resize(pkt.size);
                    // std::memcpy(cover_image_file_data_.data(), pkt.data, pkt.size);
                    std::copy(pkt.data, pkt.data + pkt.size, cover_image_file_data_.begin());
                    std::cout << "get attached pic! cover_image data size: " << cover_image_file_data_.size()
                              << ", file ext: " << cover_image_file_ext_ << std::endl;
                } else {
                    video_stream_index_ = i;
                    video_stream_seconds_per_unit_ = av_q2d(stream->time_base);
                    video_stream_units_per_second_ = static_cast<int32_t>(1 / video_stream_seconds_per_unit_);
                }
            }
        }

        if (video_stream_index_ == -1) {
            std::cerr << "cannot find video stream" << std::endl;
            return false;
        }
    }

    // open video codec
    {
        auto codec_params = format_context_->streams[video_stream_index_]->codecpar;
        const AVCodec* avcodec = avcodec_find_decoder(codec_params->codec_id);
        if (avcodec == nullptr) {
            std::cout << "unsupoorted codec: " << codec_params->codec_id << std::endl;
            return false;
        }
        video_codec_context_ = avcodec_alloc_context3(avcodec);
        if (video_codec_context_ == nullptr) {
            std::cout << "cannot alloc avcodec context" << std::endl;
            return false;
        }
        ret = avcodec_parameters_to_context(video_codec_context_, codec_params);
        if (ret < 0) {
            std::cout << "cannot copy avcodec params to context, ret " << av_err2str(ret) << std::endl;
            return false;
        }

        ret = avcodec_open2(video_codec_context_, avcodec, nullptr);
        if (ret < 0) {
            std::cout << "cannot avcodec_open2, ret: " << av_err2str(ret) << std::endl;
            return false;
        }
    }

    // for convert video frame
    {
        sws_context_ = sws_getContext(video_codec_context_->width,
                                      video_codec_context_->height,
                                      video_codec_context_->pix_fmt,
                                      video_codec_context_->width,
                                      video_codec_context_->height,
                                      AV_PIX_FMT_BGR24,
                                      SWS_BILINEAR,
                                      nullptr,
                                      nullptr,
                                      nullptr);
        if (sws_context_ == nullptr) {
            std::cout << "cannot get sws context" << std::endl;
            return false;
        }
    }

    return true;
}

bool FFmpegVideoProcessorImpl::Seek(int seconds) {
    std::unique_lock<std::mutex> lk(demux_mu_);
    need_seeking_ = true;
    last_seek_seconds_ = seconds;
    return true;
}

bool FFmpegVideoProcessorImpl::Pause() {
    std::lock_guard<std::mutex> lk(demux_mu_);
    pause_ = true;
    return true;
}

bool FFmpegVideoProcessorImpl::IsPaused() {
    std::lock_guard<std::mutex> lk(demux_mu_);
    return pause_;
}

bool FFmpegVideoProcessorImpl::Resume() {
    std::lock_guard<std::mutex> lk(demux_mu_);
    pause_ = false;
    demux_cv_.notify_all();
    return true;
}

bool FFmpegVideoProcessorImpl::Stop() {
    quit_ = true;
    demux_cv_.notify_all();

    demux_thread_.join();

    return true;
}

bool FFmpegVideoProcessorImpl::AddObserver(const VideoFrameObserver& p) {
    frame_observers_.push_back(p);
    return true;
}

VideoStreamInfo FFmpegVideoProcessorImpl::OpenVideoContext(const std::string& filepath) {
    video_filepath_ = filepath;

    bool succ = open_context();
    if (!succ) {
        return VideoStreamInfo{.open_success = false};
    }

    // video stream information, from open_context we get correct video_stream.
    auto video_stream = format_context_->streams[video_stream_index_];
    double avg_frame_rate = av_q2d(video_stream->avg_frame_rate);
    int64_t nb_frames = video_stream->nb_frames;
    int64_t duration_seconds = video_stream->duration * av_q2d(video_stream->time_base);

    VideoStreamInfo info{
        .open_success = true,
        .nb_frames = nb_frames,
        .duration_seconds = duration_seconds,
        .avg_frame_rate = avg_frame_rate,
        .time_units_per_second = video_stream_units_per_second_,
    };

    if (!cover_image_file_data_.empty()) {
        // copy vector data
        info.cover_file_data = cover_image_file_data_;
        info.cover_file_ext = cover_image_file_ext_;
    }

    // well, RVO
    return info;
}

void FFmpegVideoProcessorImpl::Process(const ProcessOptions& opts) {
    processor_opts_ = opts;
    decode_fps_ = opts.decode_fps;
    warm_up_frames_ = opts.warm_up_frames;
    skip_frames_ = opts.skip_frames;
    start_over_ = opts.loop_forever;

    demux_thread_ = std::thread([&] { demux_video_packet_(); });
    decode_thread_ = std::thread([&] { decode_video_frame_(); });
    process_thread_ = std::thread([&] { process_video_frame_(); });

    started_ = true;
}

void FFmpegVideoProcessorImpl::ScaleFrame(const AVFrame* originalFrame, AVFrame* destFrame) const {
    destFrame->pts = originalFrame->pts;
    destFrame->key_frame = originalFrame->key_frame;
    destFrame->width = originalFrame->width;
    destFrame->height = originalFrame->height;
    sws_scale(sws_context_,
              originalFrame->data,
              originalFrame->linesize,
              0,
              originalFrame->height,
              destFrame->data,
              destFrame->linesize);
}

//
// inner threads
//
void FFmpegVideoProcessorImpl::demux_video_packet_() {

    AVPacket* packet = av_packet_alloc();
    if (packet == nullptr) {
        std::cerr << "cannot allocate packet" << std::endl;
        return;
    }

    // default fps is 25, but may speed up by client.
    if (decode_fps_ == 0) {
        decode_fps_ = 25;
    }
    int sleep_ms = 1000 / decode_fps_;

    is_demuxing_ = true;

    while (true) {
        {
            std::unique_lock<std::mutex> lk(demux_mu_);
            if (quit_) {
                break;
            }
            if (pause_) {
                demux_cv_.wait(lk, [&] { return pause_ == false || quit_ == true; });
            }
        }

        if (quit_) {
            break;
        }

        // seeking.
        {
            std::unique_lock<std::mutex> lk(demux_mu_);
            if (need_seeking_) {
                if (last_seek_seconds_ <= 0) {
                    std::cerr << "need_seeking, but last_seek_seconds is wrong: " << last_seek_seconds_ << std::endl;
                } else {
                    // seeking.
                    auto timestamp = last_seek_seconds_ * video_stream_units_per_second_;
                    int ret = av_seek_frame(format_context_, video_stream_index_, timestamp, AVSEEK_FLAG_BACKWARD);
                    if (ret < 0) {
                        std::cerr << "av_seek_frame failed, ret: " << av_err2str(ret) << std::endl;
                    }
                    avcodec_flush_buffers(video_codec_context_);
                    std::cout << "av_seek_frame success, new location: " << last_seek_seconds_ << std::endl;
                }
                // reset seeking operation.
                need_seeking_ = false;
            }
        }

        int ret = av_read_frame(format_context_, packet);
        if (ret < 0) {
            std::cerr << "cannot read frame from context, ret: " << av_err2str(ret) << std::endl;
            break;
        }
        DEFER(av_packet_unref(packet));

        if (packet->stream_index == video_stream_index_) {
            frame_count++;
            // create a new clone packet and make ref to src packet.
            AVPacket* cloned = av_packet_clone(packet);
            packet_ch_ << cloned;

            // std::cout << "packet channel size: " << packet_ch_.size() << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));

            // // DEBUG: pause every 100 frames.
            // if (frame_count % 100 == 0) {
            //     pause_ = true;
            //     std::cout << "pause at " << frame_count << " frames. " << std::endl;
            // }
        } else {
            // unref explicitly, not really needed, because av_read_frame will unref it anyway.
            // av_packet_unref(packet);
        }
    }

    packet_ch_.close();
    is_demuxing_ = false;

    av_packet_free(&packet);
}

void FFmpegVideoProcessorImpl::decode_video_frame_() {
    // has no ref-count at first.
    AVFrame* frame = av_frame_alloc();
    DEFER(av_frame_free(&frame))
    // std::shared_ptr<bool> defer(nullptr, [&](bool *) {  });

    is_decoding_ = true;

    while (true) {
        // copy out AVPacket from queue
        AVPacket* packet;
        packet_ch_ >> packet;
        DEFER(av_packet_unref(packet));

        int ret = avcodec_send_packet(video_codec_context_, packet);
        if (ret < 0) {
            std::cerr << "cannot avcodec_send_packet: ret: " << av_err2str(ret) << std::endl;
            break;
        }
        while (true) {
            // will auto unref the frame first.
            // then will auto ref the new decoded frame.
            // so we donot need to unref the frame in the end of while loop
            ret = avcodec_receive_frame(video_codec_context_, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                std::cerr << "cannot avcodec_receive_frame: ret: " << av_err2str(ret) << std::endl;
                break;
            } else {
                // std::cout << "got one new frame " << std::endl;
            }

            // will alloc and ref-count a new frame.
            // make the original frame ref-count + 1
            // do we have to unref the original frame? no.
            frame_ch_ << av_frame_clone(frame);
            // std::cout << "frame channel size: " << frame_ch_.size() << std::endl;
        }
    }

    frame_ch_.close();
    is_decoding_ = false;
}

void FFmpegVideoProcessorImpl::process_video_frame_() {
    long frame_id = 0;

    is_processing_ = false;

    while (true) {
        if (quit_) {
            break;
        }

        AVFrame* f;
        frame_ch_ >> f;
        DEFER(av_frame_free(&f));

        if (frame_id <= warm_up_frames_) {
            frame_id++;
            continue;
        }
        if (frame_id % skip_frames_ != 0) {
            frame_id++;
            continue;
        }

        for (const auto& func : frame_observers_) {
            func(std::make_unique<FFmpegVideoFrame>(++frame_id, f).get());
        }
    }

    is_processing_ = false;
}

void FFmpegVideoProcessorImpl::monitor() {
    while (true) {
        if (quit_) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "started_: " << started_ << ", is_demuxing_: " << is_demuxing_
                  << ", is_decoding_: " << is_decoding_ << ", is_processing_: " << is_processing_ << std::endl;

        if (started_ && !is_demuxing_ && !is_decoding_ && !is_processing_) {
            if (demux_thread_.joinable())
                demux_thread_.join();
            if (decode_thread_.joinable())
                decode_thread_.join();
            if (process_thread_.joinable())
                process_thread_.join();

            if (start_over_) {
                std::cout << " start over processor " << std::endl;

                if (format_context_) {
                    avformat_close_input(&format_context_);
                }

                if (video_codec_context_) {
                    avcodec_close(video_codec_context_);
                }

                if (sws_context_) {
                    sws_freeContext(sws_context_);
                }

                OpenVideoContext(video_filepath_);

                Process(processor_opts_);

            } else {
                break;
            }
        }
    }
}

} // namespace donde_toolkits::video_process
