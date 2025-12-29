#ifndef VIDEO_ENGINE_H
#define VIDEO_ENGINE_H

#include "raylib.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>



typedef struct VideoEngine {
    bool isLoaded;
    int width;
    int height;
    double fps;
    double durationSec;
    long long frameCount;
    double baseTimeOffset;
    char codecName[32];
    char pixelFormat[32];
    Texture2D texture;
    bool isPlaying;
    double currentTime;
    double accumulator;
    uint8_t *buffer;
    AVFrame *frame;
    AVFrame *frameRGB;
    AVPacket *packet;
    AVFormatContext *formatCtx;
    AVCodecContext *codecCtx;
    struct SwsContext *swsCtx;
    int videoStreamIndex;

} VideoEngine;

void Video_TogglePlay(VideoEngine *v);
void Video_Seek(VideoEngine *v, double timestamp);
void Video_NextFrame(VideoEngine *v);
void Video_PrevFrame(VideoEngine *v);
bool Video_Load(VideoEngine *v, const char *filename);
void Video_Update(VideoEngine *v);
void Video_Unload(VideoEngine *v);


#endif