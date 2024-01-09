//
// Created by lsw on 2024/1/9.
//
#include <android/log.h>
#include <stdio.h>
#define __Stdc_CONSTANT_MACROS
#define LOG_TAG "FFNative"
#define ALOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#define ALOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define ALOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define ALOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define ALOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <jni.h>
}

//注意：该宏的定义要放到libavformat/avformat.h文件下边。
static char av_error[10240] = { 0 };
#define av_err2str(errnum) av_make_error_string(av_error, AV_ERROR_MAX_STRING_SIZE, errnum)

/// 确保time_base的分母不为0
static double _r2d(AVRational r)
{
    return r.den == 0 ? 0 : (double)r.num / (double)r.den;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_cc_dewdrop_ffplayer_utils_FFUtils_analyStreams(JNIEnv *env, jclass clazz, jstring video_path) {
    char info[40000] = {0};
    const char *videoPath = env->GetStringUTFChars(video_path, 0);
    AVStream *as = NULL;
    ALOGI("PlayVideo: %s", videoPath);

    if (videoPath == NULL) {
        ALOGE("videoPath is null");

    }
    av_register_all();
    avformat_network_init();
    AVFormatContext *formatContext = avformat_alloc_context();
    // 1.打开媒体文件
    ALOGI("Open video file");
    if (avformat_open_input(&formatContext, videoPath, NULL, NULL) != 0) {
        ALOGE("Cannot open video file: %s\n", videoPath);
    }
    ALOGI("avformat_open_input()called success.\n");
    //获取媒体总时长(单位为毫秒)以及流的数量
    ALOGI("duration is: %lld,nb_stream is: %d\n",formatContext->duration,formatContext->nb_streams);

    //2. 探测获取流信息
    if(avformat_find_stream_info(formatContext,NULL)>=0){
        ALOGI("duration is: %lld,nb_stream is: %d\n",formatContext->duration,formatContext->nb_streams);

    }

    /**
     * 帧率
     */
     int fps = 0;
     int videoStream = 0;
     int audioStream = 1;
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        as = formatContext->streams[i];

    //3.1 查找视频流
    if (as->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        ///// videoStream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        printf("video stream.................\n");
        videoStream = i;
        char videoInfo[200];
        fps = (int)_r2d(as->avg_frame_rate);
        sprintf(videoInfo,"fps : %d, 视频宽度: %d, 视频高度: %d, 视频的编解码ID: %d, 视频像素格式: %d\n",
                fps,
                as->codecpar->width,  //视频宽度
                as->codecpar->height, //视频高度
                as->codecpar->codec_id, //视频的编解码ID
                as->codecpar->format);  //视频像素格式：AVPixelFormat
        strcat(info, videoInfo);
    }
    //3.2 查找音频流
    else if (as->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        printf("audio stream.................\n");
        audioStream = i;
        char audioInfo[200];
        sprintf(audioInfo,"音频采样率: %d, 音频声道数: %d, 音频采样格式: %d\n",
                as->codecpar->sample_rate, //音频采样率
                as->codecpar->channels,    //音频声道数
                as->codecpar->format);   //音频采样格式：AVSampleFormat
        strcat(info, audioInfo);
    }

    }
    // 5、关闭并释放资源
    avformat_close_input(&formatContext);
    return env->NewStringUTF(info);
}