#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <string>
#include <unistd.h>

#define LOG_TAG "FFNative"
#define ALOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#define ALOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define ALOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define ALOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define ALOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

extern "C" {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include <libavutil/imgutils.h>


JNIEXPORT jstring JNICALL
Java_cc_dewdrop_ffplayer_utils_FFUtils_urlProtocolInfo(JNIEnv *env, jclass type) {
    char info[40000] = {0};
    av_register_all();

    struct URLProtocol *pup = NULL;

    struct URLProtocol **p_temp = (struct URLProtocol **) &pup;
    avio_enum_protocols((void **) p_temp, 0);

    while ((*p_temp) != NULL) {
        sprintf(info, "%sInput: %s\n", info, avio_enum_protocols((void **) p_temp, 0));
    }
    pup = NULL;
    avio_enum_protocols((void **) p_temp, 1);
    while ((*p_temp) != NULL) {
        sprintf(info, "%sInput: %s\n", info, avio_enum_protocols((void **) p_temp, 1));
    }
    ALOGI("%s", info);
    return env->NewStringUTF(info);
}

JNIEXPORT jstring JNICALL
Java_cc_dewdrop_ffplayer_utils_FFUtils_avFormatInfo(JNIEnv *env, jclass type) {
    char info[40000] = {0};

    av_register_all();

    AVInputFormat *if_temp = av_iformat_next(NULL);
    AVOutputFormat *of_temp = av_oformat_next(NULL);
    while (if_temp != NULL) {
        sprintf(info, "%sInput: %s\n", info, if_temp->name);
        if_temp = if_temp->next;
    }
    while (of_temp != NULL) {
        sprintf(info, "%sOutput: %s\n", info, of_temp->name);
        of_temp = of_temp->next;
    }
    ALOGI("%s", info);
    return env->NewStringUTF(info);
}

JNIEXPORT jstring JNICALL
Java_cc_dewdrop_ffplayer_utils_FFUtils_avCodecInfo(JNIEnv *env, jclass type) {
    char info[40000] = {0};

    av_register_all();

    AVCodec *c_temp = av_codec_next(NULL);

    while (c_temp != NULL) {
        if (c_temp->decode != NULL) {
            sprintf(info, "%sdecode:", info);
        } else {
            sprintf(info, "%sencode:", info);
        }
        switch (c_temp->type) {
            case AVMEDIA_TYPE_VIDEO:
                sprintf(info, "%s(video):", info);
                break;
            case AVMEDIA_TYPE_AUDIO:
                sprintf(info, "%s(audio):", info);
                break;
            case AVMEDIA_TYPE_SUBTITLE:
                sprintf(info, "%s(subtitle):", info);
                break;
            default:
                sprintf(info, "%s(other):", info);
                break;
        }
        sprintf(info, "%s[%10s]\n", info, c_temp->name);
        c_temp = c_temp->next;
    }
    ALOGI("%s", info);
    return env->NewStringUTF(info);
}

JNIEXPORT jstring JNICALL
Java_cc_dewdrop_ffplayer_utils_FFUtils_avFilterInfo(JNIEnv *env, jclass type) {
    char info[40000] = {0};
    avfilter_register_all();

    AVFilter *f_temp = (AVFilter *) avfilter_next(NULL);
    while (f_temp != NULL) {
        sprintf(info, "%s%s\n", info, f_temp->name);
        f_temp = f_temp->next;
    }
    ALOGI("%s", info);
    return env->NewStringUTF(info);
}

JNIEXPORT void JNICALL
Java_cc_dewdrop_ffplayer_utils_FFUtils_playVideo(JNIEnv *env, jclass type, jstring videoPath_,
                                                 jobject surface) {
    const char *videoPath = env->GetStringUTFChars(videoPath_, 0);
    ALOGI("PlayVideo: %s", videoPath);

    if (videoPath == NULL) {
        ALOGE("videoPath is null");
        return;
    }
    avformat_network_init();
    AVFormatContext *formatContext = avformat_alloc_context();
    // open video file
    ALOGI("Open video file");
    if (avformat_open_input(&formatContext, videoPath, NULL, NULL) != 0) {
        ALOGE("Cannot open video file: %s\n", videoPath);
        return;
    }

    // Retrieve stream information
    ALOGI("Retrieve stream information");
    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        ALOGE("Cannot find stream information.");
        return;
    }

    // Find the first video stream
    ALOGI("Find video stream");
    int video_stream_index = -1;
    //定义音频流
    int audio_stream_index = -1;
    for (int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
        } else if(formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_index = i;
        }
    }

    if (video_stream_index == -1) {
        ALOGE("No video stream found.");
        return; // no video stream found.
    }

    // Get a pointer to the codec context for the video stream
    ALOGI("Get a pointer to the codec context for the video stream");
    AVCodecParameters *codecParameters = formatContext->streams[video_stream_index]->codecpar;

    // Find the decoder for the video stream
    ALOGI("Find the decoder for the video stream");
    AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
    if (codec == NULL) {
        ALOGE("Codec not found.");
        return; // Codec not found
    }

    //分配解码器上下文：使用avcodec_alloc_context3函数，为解码器分配上下文AVCodecContext。
    AVCodecContext *codecContext = avcodec_alloc_context3(codec);

    if (codecContext == NULL) {
        ALOGE("CodecContext not found.");
        return; // CodecContext not found
    }

    // 填充解码器上下文：使用avcodec_parameters_to_context函数，将解码器参数填充到解码器上下文中。
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        ALOGD("Fill CodecContext failed.");
        return;
    }

    // init codex context
    ALOGI("open Codec");
    //打开解码器：使用avcodec_open2函数打开解码器。
    if (avcodec_open2(codecContext, codec, NULL)) {
        ALOGE("Init CodecContext failed.");
        return;
    }
    //通过将dstFormat设置为AV_PIX_FMT_RGBA，代码表明后续操作可能涉及到将视频帧转换为RGBA像素格式的处理
    AVPixelFormat dstFormat = AV_PIX_FMT_RGBA;


    /**
     * 分配AVPacket和AVFrame：使用av_packet_alloc和av_frame_alloc函数分别分配AVPacket和AVFrame结构体，用于存储解码前后的数据。
     */
    // Allocate av packet
    AVPacket *packet = av_packet_alloc();
    if (packet == NULL) {
        ALOGD("Could not allocate av packet.");
        return;
    }
    // Allocate video frame
    ALOGI("Allocate video frame");
    AVFrame *frame = av_frame_alloc();
    // Allocate render frame
    ALOGI("Allocate render frame");
    AVFrame *renderFrame = av_frame_alloc();
    if (frame == NULL || renderFrame == NULL) {
        ALOGD("Could not allocate video frame.");
        return;
    }

    // 确定缓冲区大小和分配缓冲区：使用av_image_get_buffer_size函数确定渲染帧的缓冲区大小，然后使用av_malloc函数分配缓冲区空间。
    ALOGI("Determine required buffer size and allocate buffer");
    int size = av_image_get_buffer_size(dstFormat, codecContext->width, codecContext->height, 1);
    uint8_t *buffer = (uint8_t *) av_malloc(size * sizeof(uint8_t));
    //这行代码使用了FFmpeg库中的av_image_fill_arrays函数，用于填充图像数据的AVFrame结构体。下面是该函数的参数和功能解释：
    //
    //renderFrame->data：指向AVFrame中数据指针数组的指针。在这里，它用于接收填充后的图像数据。
    //renderFrame->linesize：指向AVFrame中数据行大小的数组的指针。在这里，它用于接收填充后的图像数据行大小。
    //buffer：指向用于填充数据的缓冲区指针。这个缓冲区应该是通过av_malloc函数分配的足够大的内存空间。
    //dstFormat：目标图像数据的像素格式。它应该是FFmpeg中定义的像素格式之一，例如AV_PIX_FMT_RGBA。
    //codecContext->width：视频帧的宽度。
    //codecContext->height：视频帧的高度。
    //1：对齐参数。在这里，它表示要求数据在内存中按字节对齐。
    av_image_fill_arrays(renderFrame->data, renderFrame->linesize, buffer, dstFormat,
                         codecContext->width, codecContext->height, 1);

    // 初始化SwsContext：使用sws_getContext函数初始化SwsContext结构体，用于进行图像格式转换。
    ALOGI("init SwsContext");
    struct SwsContext *swsContext = sws_getContext(codecContext->width,
                                                   codecContext->height,
                                                   codecContext->pix_fmt,
                                                   codecContext->width,
                                                   codecContext->height,
                                                   dstFormat,
                                                   SWS_BILINEAR,
                                                   NULL,
                                                   NULL,
                                                   NULL);
    if (swsContext == NULL) {
        ALOGE("Init SwsContext failed.");
        return;
    }

    // 获取Native Window：使用ANativeWindow_fromSurface函数将传入的Surface对象转换为Native Window对象。
    ALOGI("native window");
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    ANativeWindow_Buffer windowBuffer;

    // get video width , height
    ALOGI("get video width , height");
    int videoWidth = codecContext->width;
    int videoHeight = codecContext->height;
    ALOGI("VideoSize: [%d,%d]", videoWidth, videoHeight);

    // 设置Native Window的缓冲区大小：使用ANativeWindow_setBuffersGeometry函数设置Native Window的缓冲区大小,可自动拉伸
    ALOGI("set native window");
    ANativeWindow_setBuffersGeometry(nativeWindow, videoWidth, videoHeight,
                                     WINDOW_FORMAT_RGBA_8888);

    //解码和渲染视频帧：使用循环从视频文件中读取AVPacket，判断是否为视频流的数据包，然后将数据包发送给解码器进行解码。
    // 解码成功后，使用SwsContext进行图像格式转换，并通过Native Window将渲染帧显示在屏幕上。
    ALOGI("read frame");
    while (av_read_frame(formatContext, packet) == 0) { //返回值为0表示成功读取数据包。
        // Is this a packet from the video stream?
        if (packet->stream_index == video_stream_index) {
            

            // 将数据包发送给解码器进行解码，使用avcodec_send_packet函数。返回值sendPacketState表示发送数据包的状态。
            int sendPacketState = avcodec_send_packet(codecContext, packet);
            if (sendPacketState == 0) {
                ALOGD("向解码器-发送数据");
                //从解码器接收解码后的帧，使用avcodec_receive_frame函数。返回值receiveFrameState表示接收帧的状态。
                int receiveFrameState = avcodec_receive_frame(codecContext, frame);
                if (receiveFrameState == 0) {
                    ALOGD("从解码器-接收数据");
                    // 锁定Native Window的缓冲区，使用ANativeWindow_lock函数。
                    ANativeWindow_lock(nativeWindow, &windowBuffer, NULL);

                    // 使用sws_scale函数进行图像格式转换，将解码后的帧数据转换为目标格式。
                    sws_scale(swsContext, (uint8_t const *const *) frame->data,
                              frame->linesize, 0, codecContext->height,
                              renderFrame->data, renderFrame->linesize);


                    uint8_t *dst = (uint8_t *) windowBuffer.bits; // 获取Native Window缓冲区的目标地址。
                    uint8_t *src = (renderFrame->data[0]); //获取转换后的帧数据的源地址。
                    int dstStride = windowBuffer.stride * 4;
                    int srcStride = renderFrame->linesize[0];

                    // 由于window的stride和帧的stride不同,因此需要逐行复制转换后的帧数据到Native Window的缓冲区中，以便在屏幕上显示。
                    for (int i = 0; i < videoHeight; i++) {
                        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
                    }
                    //解锁Native Window的缓冲区，并将更新的内容提交到屏幕上，使用ANativeWindow_unlockAndPost函数。
                    ANativeWindow_unlockAndPost(nativeWindow);
                } else if (receiveFrameState == AVERROR(EAGAIN)) {
                    ALOGD("从解码器-接收-数据失败：AVERROR(EAGAIN)");
                } else if (receiveFrameState == AVERROR_EOF) {
                    ALOGD("从解码器-接收-数据失败：AVERROR_EOF");
                } else if (receiveFrameState == AVERROR(EINVAL)) {
                    ALOGD("从解码器-接收-数据失败：AVERROR(EINVAL)");
                } else {
                    ALOGD("从解码器-接收-数据失败：未知");
                }
                } else if (sendPacketState == AVERROR(EAGAIN)) {//发送数据被拒绝，必须尝试先读取数据
                ALOGD("向解码器-发送-数据包失败：AVERROR(EAGAIN)");//解码器已经刷新数据但是没有新的数据包能发送给解码器
                } else if (sendPacketState == AVERROR_EOF) {
                ALOGD("向解码器-发送-数据失败：AVERROR_EOF");
                } else if (sendPacketState == AVERROR(EINVAL)) {//遍解码器没有打开，或者当前是编码器，也或者需要刷新数据
                ALOGD("向解码器-发送-数据失败：AVERROR(EINVAL)");
                } else if (sendPacketState == AVERROR(ENOMEM)) {//数据包无法压如解码器队列，也可能是解码器解码错误
                ALOGD("向解码器-发送-数据失败：AVERROR(ENOMEM)");
                } else {
                ALOGD("向解码器-发送-数据失败：未知");
                }


        }
        av_packet_unref(packet); //释放数据包占用的资源，使用av_packet_unref函数。
    }


    //内存释放
    ALOGI("release memory");
    ANativeWindow_release(nativeWindow);
    av_frame_free(&frame);
    av_frame_free(&renderFrame);
    av_packet_free(&packet);
    avcodec_close(codecContext);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);
    avformat_free_context(formatContext);
    env->ReleaseStringUTFChars(videoPath_, videoPath);
}

}



extern "C"
JNIEXPORT jstring JNICALL
Java_cc_dewdrop_ffplayer_utils_FFUtils_ffmpegInfo(JNIEnv *env, jobject thiz) {
    char info[40000] = {0};
    AVCodec *c_temp = av_codec_next(NULL);
    while (c_temp != NULL) {
        if (c_temp->decode != NULL) {
            sprintf(info, "%sdecode:", info);
        } else {
            sprintf(info, "%sencode:", info);
        }
        switch (c_temp->type) {
            case AVMEDIA_TYPE_VIDEO:
                sprintf(info, "%s(video):", info);
                break;
            case AVMEDIA_TYPE_AUDIO:
                sprintf(info, "%s(audio):", info);
                break;
            default:
                sprintf(info, "%s(other):", info);
                break;
        }


        sprintf(info, "%s[%s]\n", info, c_temp->name);
        c_temp = c_temp->next;
    }
    return env->NewStringUTF(info);
}