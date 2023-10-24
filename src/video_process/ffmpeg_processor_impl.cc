#include "channel.h"
#include "donde/defer.h"

#include <chrono>
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
#include <libavutil/imgutils.h>
}

#include "donde/video_process/ffmpeg_processor_impl.h"
#include "donde/video_process/utils.h"

namespace donde_toolkits ::video_process {

FFmpegVideoProcessorImpl::FFmpegVideoProcessorImpl() : packet_ch_{10}, frame_ch_{10} {
    monitor_thread_ = std::thread([&] { monitor(); });
}

FFmpegVideoProcessorImpl::~FFmpegVideoProcessorImpl() {}

bool FFmpegVideoProcessorImpl::open_context() {
    int ret = avformat_open_input(&format_context_, video_filepath_.c_str(), nullptr, nullptr);
    if (ret < 0) {
        std::cout << "cannot open input video file: " << video_filepath_ << std::endl;
        return false;
    }

    ret = avformat_find_stream_info(format_context_, nullptr);
    if (ret < 0) {
        std::cout << "cannot find stream info: " << video_filepath_ << std::endl;
        return false;
    }

    // for debug only.
    av_dump_format(format_context_, 0, video_filepath_.c_str(), 0);

    for (int i = 0; i < format_context_->nb_streams; i++) {
        if (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
            && video_stream_index_ < 0) {
            video_stream_index_ = i;
            break;
        }
    }
    if (video_stream_index_ == -1) {
        std::cout << "cannot find video stream" << std::endl;
        return false;
    }

    auto codec_params = format_context_->streams[video_stream_index_]->codecpar;
    const AVCodec* avcodec = avcodec_find_decoder(codec_params->codec_id);
    if (avcodec == nullptr) {
        std::cout << "unsupoorted codec: " << codec_params->codec_id << std::endl;
        return false;
    }
    codec_context_ = avcodec_alloc_context3(avcodec);
    if (codec_context_ == nullptr) {
        std::cout << "cannot alloc avcodec context" << std::endl;
        return false;
    }
    ret = avcodec_parameters_to_context(codec_context_, codec_params);
    if (ret < 0) {
        std::cout << "cannot copy avcodec params to context, ret " << av_err2str(ret) << std::endl;
        return false;
    }

    ret = avcodec_open2(codec_context_, avcodec, nullptr);
    if (ret < 0) {
        std::cout << "cannot avcodec_open2, ret: " << av_err2str(ret) << std::endl;
        return false;
    }

    sws_context_
        = sws_getContext(codec_context_->width, codec_context_->height, codec_context_->pix_fmt,
                         codec_context_->width, codec_context_->height, AV_PIX_FMT_BGR24,
                         SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (sws_context_ == nullptr) {
        std::cout << "cannot get sws context" << std::endl;
        return false;
    }

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
    if (pause_) {
        pause_ = false;
        demux_cv_.notify_all();
    }

    demux_thread_.join();
    // decode_thread_.join();
    // process_thread_.join();

    return true;
}

bool FFmpegVideoProcessorImpl::Register(const FFmpegVideoFrameProcessor& p) {
    frame_processor_list_.push_back(p);
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
    int64_t nb_frames = video_stream->nb_frames;
    int64_t duration_s = video_stream->duration / av_q2d(video_stream->time_base);

    VideoStreamInfo stream_infomation{
        .open_success = true,
        .nb_frames = nb_frames,
        .duration_s = duration_s,
    };

    return stream_infomation;
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
    destFrame->coded_picture_number = originalFrame->coded_picture_number;
    destFrame->display_picture_number = originalFrame->display_picture_number;
    destFrame->width = originalFrame->width;
    destFrame->height = originalFrame->height;
    sws_scale(sws_context_, originalFrame->data, originalFrame->linesize, 0, originalFrame->height,
              destFrame->data, destFrame->linesize);
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
        std::unique_lock<std::mutex> lk(demux_mu_);

        if (pause_) {
            demux_cv_.wait(lk, [&] { return pause_ == false; });
        }

        if (quit_) {
            break;
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
            bool succ = packet_ch_ << cloned;
            if (!succ) {
                std::cerr << "cannot input to packet" << std::endl;
                av_packet_unref(cloned);
            }

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
        auto status = packet_ch_.try_output(packet);
        if (status == CHANNEL_CLOSED) {
            std::cerr << "cannot output from packet channel, closed" << std::endl;
            break;
        } else if (status == CHANNEL_NOT_READY) {
            // FIXME sleep ? block ?
            // std::cerr << "cannot output from packet channel, not ready" << std::endl;
            continue;
        }

        DEFER(av_packet_unref(packet));

        int ret = avcodec_send_packet(codec_context_, packet);
        if (ret < 0) {
            std::cerr << "cannot avcodec_send_packet: ret: " << av_err2str(ret) << std::endl;
            break;
        }
        while (true) {
            // will auto unref the frame first.
            // then will auto ref the new decoded frame.
            // so we donot need to unref the frame in the end of while loop
            ret = avcodec_receive_frame(codec_context_, frame);
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
            bool succ = frame_ch_ << av_frame_clone(frame);
            if (!succ) {
                std::cerr << "cannot input to frame channel" << std::endl;
                break;
            }
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

        auto status = frame_ch_.try_output(f);
        if (status == CHANNEL_CLOSED) {
            std::cerr << "cannot output from frame channel, closed" << std::endl;
            break;
        } else if (status == CHANNEL_NOT_READY) {
            // FIXME sleep ? block ?
            // std::cerr << "cannot output from frame channel, not ready" << std::endl;
            continue;
        }

        DEFER(av_frame_free(&f));

        // std::cout << "frame channel size: " << frame_ch_.size() << std::endl;
        //
        if (frame_id <= warm_up_frames_) {
            frame_id++;
            continue;
        }
        if (frame_id % skip_frames_ != 0) {
            frame_id++;
            continue;
        }

        for (const auto& func : frame_processor_list_) {
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
                  << ", is_decoding_: " << is_decoding_ << ", is_processing_: " << is_processing_
                  << std::endl;

        if (started_ && !is_demuxing_ && !is_decoding_ && !is_processing_) {
            if (demux_thread_.joinable())
                demux_thread_.join();
            if (decode_thread_.joinable())
                decode_thread_.join();
            if (process_thread_.joinable())
                process_thread_.join();

            if (start_over_) {
                std::cout << " start over processor " << std::endl;

                packet_ch_ = Channel<AVPacket*>(10);
                frame_ch_ = Channel<AVFrame*>(10);

                if (format_context_) {
                    avformat_close_input(&format_context_);
                }

                if (codec_context_) {
                    avcodec_close(codec_context_);
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
