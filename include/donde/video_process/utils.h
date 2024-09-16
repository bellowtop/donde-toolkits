#pragma once

#include <string>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
}

// clang-format off

#ifdef av_err2str
#undef av_err2str

#include <string>
av_always_inline std::string av_err2string(int errnum) {
    char str[AV_ERROR_MAX_STRING_SIZE];
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

#define av_err2str(err) av_err2string(err).c_str()
#endif // av_err2str

// clang-format on

inline std::string get_cover_image_file_extension(AVCodecID codec_id) {
    switch (codec_id) {
    case AV_CODEC_ID_JPEG2000:
    case AV_CODEC_ID_MJPEG:
    case AV_CODEC_ID_MJPEGB:
    case AV_CODEC_ID_JPEGLS:
    case AV_CODEC_ID_JPEGXL:
        return ".jpg";
    case AV_CODEC_ID_PNG:
        return ".png";
    default:
        return ".bin";
    }
}
