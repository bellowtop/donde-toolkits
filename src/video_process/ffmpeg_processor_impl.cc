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


#ifdef av_err2str
#undef av_err2str
#include <string>
av_always_inline std::string av_err2string(int errnum) {
    char str[AV_ERROR_MAX_STRING_SIZE];
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#define av_err2str(err) av_err2string(err).c_str()
#endif  // av_err2str


#include "donde/video_process/ffmpeg_processor_impl.h"

namespace donde_toolkits ::video_process {

FFmpegVideoProcessorImpl::FFmpegVideoProcessorImpl(const std::string& filepath)
    : filepath{filepath}, packet_ch_{10}, frame_ch_{10} {
    open_context();
}

FFmpegVideoProcessorImpl::~FFmpegVideoProcessorImpl() {}

bool FFmpegVideoProcessorImpl::open_context() {
    int ret = avformat_open_input(&format_context_, filepath.c_str(), nullptr, nullptr);
    if (ret < 0) {
        std::cout << "cannot open input video file: " << filepath << std::endl;
        return false;
    }

    ret = avformat_find_stream_info(format_context_, nullptr);
    if (ret < 0) {
        std::cout << "cannot find stream info: " << filepath << std::endl;
        return false;
    }

    // for debug only.
    av_dump_format(format_context_, 0, filepath.c_str(), 0);

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
                         codec_context_->width, codec_context_->height, AV_PIX_FMT_YUV420P,
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
    demux_thread_.join();
    // decode_thread_.join();
    // process_thread_.join();

    return true;
}

bool FFmpegVideoProcessorImpl::Register(const FFmpegVideoFrameProcessor& p) {
    frame_processor = p;
    return true;
}

bool FFmpegVideoProcessorImpl::Process() {
    demux_thread_ = std::thread([&] { demux_video_packet_(); });
    decode_thread_ = std::thread([&] { decode_video_frame_(); });
    process_thread_ = std::thread([&] { process_video_frame_(); });
    return true;
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

            // std::this_thread::sleep_for(std::chrono::milliseconds(40));

            // DEBUG: pause every 100 frames.
            if (frame_count % 100 == 0) {
                pause_ = true;
                std::cout << "pause at " << frame_count << " frames. " << std::endl;
            }
        } else {
            // unref explicitly, not really needed, because av_read_frame will unref it anyway.
            av_packet_unref(packet);
        }
    }

    av_packet_free(&packet);
}

void FFmpegVideoProcessorImpl::decode_video_frame_() {
    // has no ref-count at first.
    AVFrame* frame = av_frame_alloc();
    DEFER(av_frame_free(&frame))
    // std::shared_ptr<bool> defer(nullptr, [&](bool *) {  });

    while (true) {
        // copy out AVPacket from queue
        AVPacket* packet;
        bool succ = packet_ch_ >> packet;
        if (!succ) {
            std::cerr << "cannot output from packet channel" << std::endl;
            break;
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
}

void FFmpegVideoProcessorImpl::process_video_frame_() {
    long frame_id = 0;
    while (true) {
        if (quit_) {
            break;
        }

        AVFrame* f;
        bool succ = frame_ch_ >> f;
        if (!succ) {
            std::cerr << "cannot output from frame channel" << std::endl;
        }
        // std::cout << "frame channel size: " << frame_ch_.size() << std::endl;

        if (frame_processor != nullptr) {
            frame_processor(std::make_unique<FFmpegVideoFrame>(++frame_id, f).get());
        }

        av_frame_free(&f);
    }
}

} // namespace donde_toolkits::video_process
