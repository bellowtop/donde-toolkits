#include "donde/defer.h"
#include "msd/channel.hpp"

#include <Poco/Notification.h>
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
#include <libavformat/avio.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

#include "donde/audio_process/ffmpeg_processor_impl.h"
#include "donde/audio_process/utils.h"

namespace donde_toolkits ::audio_process {

FFmpegAudioProcessorImpl::FFmpegAudioProcessorImpl() {
    // monitor_thread_ = std::thread([&] { monitor(); });
}

FFmpegAudioProcessorImpl::~FFmpegAudioProcessorImpl() {}

AudioStreamInfo FFmpegAudioProcessorImpl::OpenContext(const std::string& filepath) {
    video_filepath_ = filepath;

    bool succ = open_context();
    if (!succ) {
        return AudioStreamInfo{.open_success = false};
    }

    AudioStreamInfo info{};

    return info;
}

bool FFmpegAudioProcessorImpl::Transcode(const std::string& output_filepath) {
    // start
    start_audio_extract_context(output_filepath);
    DEFER(clear_audio_extract_context());

    // extract
    // blocking call.
    start_audio_extract_process(output_filepath);

    return true;
}

bool FFmpegAudioProcessorImpl::open_context() {
    int ret = avformat_open_input(&format_context_, video_filepath_.c_str(), nullptr, nullptr);
    if (ret < 0) {
        std::cout << "cannot open input video file: " << video_filepath_ << std::endl;
        return false;
    }

    // find audio stream
    {
        ret = avformat_find_stream_info(format_context_, nullptr);
        if (ret < 0) {
            std::cout << "cannot find stream info: " << video_filepath_ << std::endl;
            return false;
        }

        // for debug only.
        av_dump_format(format_context_, 0, video_filepath_.c_str(), 0);

        for (int i = 0; i < format_context_->nb_streams; i++) {
            if (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index_ < 0) {
                audio_stream_index_ = i;
            }
        }

        if (audio_stream_index_ == -1) {
            std::cerr << "cannot find audio stream" << std::endl;
            return false;
        }
    }

    // open audio codec
    {
        auto codec_params = format_context_->streams[audio_stream_index_]->codecpar;
        const AVCodec* avcodec = avcodec_find_decoder(codec_params->codec_id);
        if (avcodec == nullptr) {
            std::cout << "unsupoorted codec: " << codec_params->codec_id << std::endl;
            return false;
        }
        audio_codec_context_ = avcodec_alloc_context3(avcodec);
        if (audio_codec_context_ == nullptr) {
            std::cout << "cannot alloc avcodec context" << std::endl;
            return false;
        }
        ret = avcodec_parameters_to_context(audio_codec_context_, codec_params);
        if (ret < 0) {
            std::cout << "cannot copy avcodec params to context, ret " << av_err2str(ret) << std::endl;
            return false;
        }

        ret = avcodec_open2(audio_codec_context_, avcodec, nullptr);
        if (ret < 0) {
            std::cout << "cannot avcodec_open2, ret: " << av_err2str(ret) << std::endl;
            return false;
        }

        audio_codec_context_->time_base = format_context_->streams[audio_stream_index_]->time_base;
    }

    return true;
}

bool FFmpegAudioProcessorImpl::start_audio_extract_context(const std::string& filepath) {
    int ret;

    // open output file
    {
        ret = avio_open(&output_io_context_, filepath.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            std::cerr << "cannot avio_open, ret: " << av_err2str(ret) << std::endl;
            return false;
        }
    }

    // initialize output_format_context_
    {
        /* Create a new format context for the output container format. */
        output_format_context_ = avformat_alloc_context();
        if (output_format_context_ == nullptr) {
            std::cerr << "cannot avformat_alloc_context for audio output." << std::endl;
            return false;
        };
        output_format_context_->pb = output_io_context_;

        /* Guess the desired container format based on the file extension. */
        output_format_context_->oformat = av_guess_format(nullptr, filepath.c_str(), nullptr);
        if (output_format_context_->oformat == nullptr) {
            std::cerr << "cannot guess format from filepath:" << filepath << std::endl;
            return false;
        }

        output_format_context_->url = av_strdup(filepath.c_str());
        if (output_format_context_->url == nullptr) {
            std::cerr << "cannot alloc string url." << std::endl;
            return false;
        }
    }

    // initialize output_codec_ctx
    {
        /* Find the encoder to be used by its name. */
        output_codec_ = avcodec_find_encoder(output_codec_id_);

        /* Set the basic encoder parameters.
         * The input file's sample rate is used to avoid a sample rate conversion. */
        output_codec_context_ = avcodec_alloc_context3(output_codec_);
        av_channel_layout_default(&output_codec_context_->ch_layout, output_nb_channels_);
        output_codec_context_->sample_rate = output_sample_rate_;
        output_codec_context_->sample_fmt = output_codec_->sample_fmts[0];

        // explict set output frame_size. this value is not restricted, just means samples per frame.
        // it doesn't affect audio quality.
        // also for PCM, no need to set it.
        // output_codec_context_->frame_size = 1024;

        // PCM don't need bit_rate, because it's un-compressed fmt.
        // it's bit_rate is calculated by sample_rate and channel numbers
        // output_codec_context_->bit_rate = output_bit_rate;
        output_codec_context_->bit_rate = 0;

        /* Some container formats (like MP4) require global headers to be present.
         * Mark the encoder so that it behaves accordingly. */
        if (output_format_context_->oformat->flags & AVFMT_GLOBALHEADER) {
            output_codec_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
    }

    // initialize output_stream
    {
        /* Create a new audio stream in the output file container. */
        output_stream_ = avformat_new_stream(output_format_context_, nullptr);
        /* Set the sample rate for the container. */
        output_stream_->time_base.den = audio_codec_context_->sample_rate;
        output_stream_->time_base.num = 1;
    }

    // start open encoder
    {
        /* Open the encoder for the audio stream to use it later. */
        ret = avcodec_open2(output_codec_context_, output_codec_, nullptr);
        if (ret < 0) {
            std::cerr << "cannot avcodec_open2, ret: " << av_err2str(ret) << std::endl;
            return false;
        }
        ret = avcodec_parameters_from_context(output_stream_->codecpar, output_codec_context_);
        if (ret < 0) {
            std::cerr << "cannot avcodec_parameters_from_context, ret: " << av_err2str(ret) << std::endl;
            return false;
        }
    }

    // write output header
    {
        ret = avformat_write_header(output_format_context_, nullptr);
        if (ret < 0) {
            std::cerr << "failed avformat_write_header, ret: " << av_err2str(ret) << std::endl;
            return false;
        }
    }

    // init resampler context
    {
        ret = swr_alloc_set_opts2(&output_swr_context_,
                                  &output_codec_context_->ch_layout,
                                  output_codec_context_->sample_fmt,
                                  output_codec_context_->sample_rate,
                                  &audio_codec_context_->ch_layout,
                                  audio_codec_context_->sample_fmt,
                                  audio_codec_context_->sample_rate,
                                  0,
                                  nullptr);
        if (ret < 0) {
            std::cerr << "cannot alloc swr context, ret: " << av_err2str(ret) << std::endl;
            return false;
        }

        /*
         * Perform a sanity check so that the number of converted samples is
         * not greater than the number of samples to be converted.
         * If the sample rates differ, this case has to be handled differently
         */
        av_assert0(output_codec_context_->sample_rate <= audio_codec_context_->sample_rate);

        ret = swr_init(output_swr_context_);
        if (ret < 0) {
            std::cerr << "cannot init swr context, ret: " << av_err2str(ret) << std::endl;
            return false;
        }
    }

    // init fifo
    {
        output_fifo_
            = av_audio_fifo_alloc(output_codec_context_->sample_fmt, output_codec_context_->ch_layout.nb_channels, 1);
        if (output_fifo_ == nullptr) {
            std::cerr << "cannot av_audio_fifo_alloc " << std::endl;
            return false;
        }
    }

    return true;
}

bool FFmpegAudioProcessorImpl::clear_audio_extract_context() {
    if (output_codec_context_) {
        avcodec_free_context(&output_codec_context_);
        output_codec_context_ = nullptr;
    }
    if (output_format_context_) {
        // write trailer
        {
            int ret = av_write_trailer(output_format_context_);
            if (ret < 0) {
                std::cerr << "failed av_write_trailer: err: " << av_err2str(ret) << std::endl;
            }
        }
        // close file handle
        if (output_format_context_->pb) {
            avio_closep(&output_format_context_->pb);
            output_format_context_->pb = nullptr;
        }
        avformat_free_context(output_format_context_);
        output_format_context_ = nullptr;
    }
    if (output_swr_context_) {
        swr_free(&output_swr_context_);
    }
    return true;
}

bool FFmpegAudioProcessorImpl::start_audio_extract_process(const std::string& filepath) {
    demux_thread_ = std::thread([&] { demux_audio_packet_(); });
    decode_thread_ = std::thread([&] { decode_audio_frame_(); });
    trancode_thread_ = std::thread([&] { transcode_audio_frame_(); });
    save_thread_ = std::thread([&] { save_output_audio_packet_(); });

    // wait for them to stop...
    if (demux_thread_.joinable()) {
        std::cout << "joining demux_thread_" << std::endl;
        demux_thread_.join();
    }
    if (decode_thread_.joinable()) {
        std::cout << "joining decode_thread_" << std::endl;
        decode_thread_.join();
    }
    if (trancode_thread_.joinable()) {
        std::cout << "joining trancode_thread_" << std::endl;
        trancode_thread_.join();
    }
    if (save_thread_.joinable()) {
        std::cout << "joining save_thread_" << std::endl;
        save_thread_.join();
    }

    std::cout << "process finished." << std::endl;
    return true;
}

bool FFmpegAudioProcessorImpl::demux_audio_packet_() {
    AVPacket* packet = av_packet_alloc();
    if (packet == nullptr) {
        std::cerr << "failed to alloc packet" << std::endl;
        return false;
    }
    DEFER(av_packet_free(&packet));

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
        int ret = av_read_frame(format_context_, packet);
        if (ret < 0) {
            std::cerr << "can't read packet from format context, " << av_err2string(ret) << std::endl;
            break;
        }
        DEFER(av_packet_unref(packet));
        if (packet->stream_index == audio_stream_index_) {
            //
            AVPacket* cloned = av_packet_clone(packet);
            // sync block channel, if channel full, then block this statement
            audio_packet_ch_ << cloned;
            // std::cout << "send audio packet to channel" << std::endl;
        }
    }

    quit_ = true;
    audio_fifo_ready_cv_.notify_all();

    audio_packet_ch_.close();

    std::cout << "return from thread: demux_audio_packet_" << std::endl;
    return true;
}

bool FFmpegAudioProcessorImpl::decode_audio_frame_() {
    AVFrame* frame = av_frame_alloc();
    if (frame == nullptr) {
        std::cerr << "failed to alloc frame" << std::endl;
        return false;
    }
    // don't free frame here. every frame is pushed to ch_, let the ch_ consumer free them.
    // DEFER(av_frame_free(&frame));

    // close frame channel at last
    // use DEFER, or you should close at every return branch.
    DEFER(audio_frame_ch_.close());

    while (true) {
        AVPacket* packet;
        audio_packet_ch_ >> packet;
        if (packet == nullptr) {
            // channel is closed;
            break;
        }
        DEFER(av_packet_unref(packet));

        int ret = avcodec_send_packet(audio_codec_context_, packet);
        if (ret < 0) {
            std::cerr << "failed to send packet to audio codec context" << av_err2string(ret) << std::endl;
            return false;
        }
        while (true) {
            ret = avcodec_receive_frame(audio_codec_context_, frame);
            // EAGAIN: need more packet data to decode a frame
            if (ret == AVERROR(EAGAIN)) {
                break;
            }
            // AVERROF_EOF: end of file. finished.
            if (ret == AVERROR_EOF) {
                std::cout << "reach end-of-file when decode audio frame. quit." << std::endl;
                return true;
            }
            if (ret < 0) {
                std::cerr << "failed to receive frame from audio codec context" << av_err2string(ret) << std::endl;
                break;
            }
            audio_frame_ch_ << av_frame_clone(frame);
        }
    }

    ;

    std::cout << "return from thread: decode_audio_frame_" << std::endl;
    return true;
}

bool FFmpegAudioProcessorImpl::transcode_audio_frame_() {

    auto fn_write_frame_samples_to_fifo = [&](AVFrame* frame) -> bool {
        uint8_t** converted_frame_data = nullptr;

        // output frame_size
        int dst_nb_samples = av_rescale_rnd(swr_get_delay(output_swr_context_, frame->sample_rate) + frame->nb_samples,
                                            output_codec_context_->sample_rate,
                                            frame->sample_rate,
                                            AV_ROUND_UP);
        std::cout << "dst_nb_samples: " << dst_nb_samples << std::endl;
        // number of samples in one audio frame;
        int frame_size = frame->nb_samples;
        // prepare converted_frame_data memory
        int ret = av_samples_alloc_array_and_samples(&converted_frame_data,
                                                     nullptr,
                                                     output_codec_context_->ch_layout.nb_channels,
                                                     dst_nb_samples,
                                                     output_codec_context_->sample_fmt,
                                                     0);
        if (ret < 0) {
            std::cerr << "failed to av_samples_alloc_array_and_samples: " << av_err2string(ret) << std::endl;

            std::cerr << "\toutput_codec_context_->ch_layout.nb_channels: "
                      << output_codec_context_->ch_layout.nb_channels << std::endl;

            std::cerr << "\toutput_codec_context_->sample_fmt: "
                      << av_get_sample_fmt_name(output_codec_context_->sample_fmt) << std::endl;

            std::cerr << "\tframe_size: " << frame_size << std::endl;

            return false;
        }

        bool write_ok = false;

        // now converted_frame_data is pointing at the newly alloc memory.
        // make sure it's freed if some err happened,
        // note that is no err happened, then no need to free it.
        // because fifo manage this memory now (when read out from fifo, AVFrame will manage it.).
        DEFER({
            if (!write_ok) {
                if (converted_frame_data) {
                    av_freep(&converted_frame_data[0]);
                }
                // av_freep accept nullptr's address, just like free()
                av_freep(&converted_frame_data);
            }
        })

        ret = swr_convert(output_swr_context_, converted_frame_data, dst_nb_samples, frame->extended_data, frame_size);
        if (ret < 0) {
            std::cerr << "failed to swr_convert: " << av_err2string(ret) << std::endl;
            write_ok = false;
            return write_ok;
        }
        if (ret != dst_nb_samples) {
            std::cout << "swr_convert ret != dst_nb_samples, ret: " << ret << ", " << dst_nb_samples << std::endl;
        }
        dst_nb_samples = ret;

        // fifo is not thread safe.
        {
            std::unique_lock lk(audio_fifo_ready_mu_);
            ret = av_audio_fifo_realloc(output_fifo_, av_audio_fifo_size(output_fifo_) + dst_nb_samples);
            if (ret < 0) {
                std::cerr << "failed to av_audio_fifo_realloc" << av_err2string(ret) << std::endl;
                write_ok = false;
                return write_ok;
            }
            ret = av_audio_fifo_write(output_fifo_, (void**)converted_frame_data, dst_nb_samples);
            if (ret < 0) {
                std::cerr << "failed to av_audio_fifo_write" << av_err2string(ret) << std::endl;
                write_ok = false;
                return write_ok;
            }
        }

        write_ok = true;
        return write_ok;
    };

    while (true) {
        AVFrame* frame;
        audio_frame_ch_ >> frame;
        if (frame == nullptr) {
            // channel is closed.
            break;
        }
        DEFER(av_frame_free(&frame));

        // std::cout << "transcode_audio_frame_ get frame: ";

        // write to fifo
        bool write_ok = fn_write_frame_samples_to_fifo(frame);
        if (!write_ok) {
            break;
        }

        if (av_audio_fifo_size(output_fifo_) >= output_frame_size_) {
            audio_fifo_ready_cv_.notify_all();
        }
    }

    std::cout << "return from thread: transcode_audio_frame_" << std::endl;
    return true;
}

bool FFmpegAudioProcessorImpl::save_output_audio_packet_() {
    std::cout << "save_output_audio_packet_: output_frame_size: " << output_frame_size_ << std::endl;

    auto more_than = [&](int want_size) -> bool {
        std::cout << "av_audio_fifo_size: " << av_audio_fifo_size(output_fifo_) << ", want_size: " << want_size
                  << std::endl;
        return av_audio_fifo_size(output_fifo_) >= want_size;
    };

    int pts = 0;

    auto fn_read_samples_and_save_packet = [&](AVFrame* output_frame) -> bool {
        // fifo is not thread safe.
        {
            std::unique_lock lk(audio_fifo_ready_mu_);
            // int want_read = FFMIN(av_audio_fifo_size(output_fifo_), output_frame_size_);
            int want_read = output_frame_size_;
            int real_read = av_audio_fifo_read(output_fifo_, (void**)output_frame->data, want_read);
            std::cout << "in save packet, read from fifo, want_read: " << want_read << ", real_read: " << real_read
                      << std::endl;

            if (real_read < want_read) {
                std::cerr << "fifo corrupt read. real_read: " << real_read << ", want_read: " << want_read << std::endl;
                return false;
            }
        }

        output_frame->pts = pts;
        pts += output_frame->nb_samples;

        // encode frame to packet.
        AVPacket* output_packet = av_packet_alloc();
        if (output_packet == nullptr) {
            std::cerr << "failed to alloc packet when save output packet." << std::endl;
            return false;
        }
        DEFER(av_packet_free(&output_packet));

        int ret = avcodec_send_frame(output_codec_context_, output_frame);
        if (ret < 0) {
            std::cerr << "failed to avcodec_send_frame" << av_err2string(ret) << std::endl;
            return false;
        }

        ret = avcodec_receive_packet(output_codec_context_, output_packet);
        if (ret == AVERROR(EAGAIN)) {
            std::cout << "need more frame data to receive a full packet" << std::endl;
            return true;
        }
        if (ret == AVERROR_EOF) {
            std::cout << "reach end-of-file, abort receive packet" << std::endl;
            return false;
        }
        if (ret < 0) {
            std::cerr << "failed to avcodec_receive_packet: " << av_err2string(ret) << std::endl;
            return false;
        }
        // save to file.
        ret = av_write_frame(output_format_context_, output_packet);
        if (ret < 0) {
            std::cerr << "failed to av_write_frame: " << av_err2string(ret) << std::endl;
            return false;
        }

        return true;
    };

    // alloc the frame
    AVFrame* frame = av_frame_alloc();
    if (frame == nullptr) {
        std::cerr << "failed to alloc frame" << std::endl;
        return false;
    }
    DEFER(av_frame_free(&frame));

    // initialize the frame inner data buffer
    {
        frame->nb_samples = output_frame_size_;
        av_channel_layout_copy(&frame->ch_layout, &output_codec_context_->ch_layout);
        frame->format = output_codec_context_->sample_fmt;
        frame->sample_rate = output_codec_context_->sample_rate;
        // This function will fill AVFrame.data and AVFrame.buf arrays
        int ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            std::cerr << "failed to av_frame_get_buffer" << av_err2string(ret) << std::endl;
            return false;
        }
    }

    while (true) {
        if (quit_) {
            // drain last fifo frames.
            while (more_than(output_frame_size_)) {
                // read fifo
                bool has_more = fn_read_samples_and_save_packet(frame);
                if (!has_more) {
                    break;
                }
            }
            break;
        }

        {
            std::unique_lock lk(audio_fifo_ready_mu_);
            // std::cout << "check more_than: " << output_frame_size_ << std::endl;
            if (!more_than(output_frame_size_)) {
                std::cout << "wait for audio_fifo_ready_cv_: " << std::endl;
                audio_fifo_ready_cv_.wait(lk, [&]() { return more_than(output_frame_size_) || quit_ == true; });
            }
        }
        // std::cout << "wake from audio_fifo_ready_cv_" << std::endl;

        // read fifo
        bool has_more = fn_read_samples_and_save_packet(frame);
        if (has_more) {
            continue;
        }
    }

    std::cout << "return from thread: save_output_audio_packet_" << std::endl;

    return true;
}

bool FFmpegAudioProcessorImpl::ExportSubtitle(const std::string& output_filepath) { return true; }

} // namespace donde_toolkits::audio_process
