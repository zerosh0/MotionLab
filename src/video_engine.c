#include "video_engine.h"
#include <stdlib.h>
#include <math.h>


double GetRawFrameTime(VideoEngine *v) {
    int64_t ts = v->frame->best_effort_timestamp;
    if (ts == AV_NOPTS_VALUE) ts = v->frame->pts;
    if (ts == AV_NOPTS_VALUE) ts = v->frame->pkt_dts;

    if (ts != AV_NOPTS_VALUE) {
        return ts * av_q2d(v->formatCtx->streams[v->videoStreamIndex]->time_base);
    }
    
    return -1.0; 
}


double GetRelativeFrameTime(VideoEngine *v) {
    double raw = GetRawFrameTime(v);
    if (raw < 0) {
        return v->currentTime + (1.0 / v->fps);
    }

    double rel = raw - v->baseTimeOffset;
    if (rel < 0) return 0.0;
    return rel;
}


void Video_ProcessFrame(VideoEngine *v) {
    if (!v->frame->data[0] || v->frame->width <= 0 || v->frame->height <= 0) return;

    v->swsCtx = sws_getCachedContext(v->swsCtx,
        v->frame->width, v->frame->height, v->frame->format,
        v->width, v->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, NULL, NULL, NULL);

    if (v->swsCtx) {
        sws_scale(v->swsCtx, (const uint8_t *const *)v->frame->data, v->frame->linesize, 
                  0, v->frame->height, v->frameRGB->data, v->frameRGB->linesize);
        UpdateTexture(v->texture, v->buffer);
    }
    
    v->currentTime = GetRelativeFrameTime(v);
}


bool Video_DecodeAndDisplayOne(VideoEngine *v) {
    if (!v->isLoaded) return false;
    
    while (av_read_frame(v->formatCtx, v->packet) >= 0) {
        if (v->packet->stream_index == v->videoStreamIndex) {
            if (avcodec_send_packet(v->codecCtx, v->packet) == 0) {
                if (avcodec_receive_frame(v->codecCtx, v->frame) == 0) {
                    

                    if (v->swsCtx == NULL) {
                         v->swsCtx = sws_getCachedContext(NULL,
                            v->frame->width, v->frame->height, v->frame->format,
                            v->width, v->height, AV_PIX_FMT_RGB24,
                            SWS_BILINEAR, NULL, NULL, NULL);
                    }
                    if(v->swsCtx) {
                        sws_scale(v->swsCtx, (const uint8_t *const *)v->frame->data, v->frame->linesize, 
                                0, v->frame->height, v->frameRGB->data, v->frameRGB->linesize);
                        UpdateTexture(v->texture, v->buffer);
                    }
                    
                    av_packet_unref(v->packet);
                    return true; 
                }
            }
        }
        av_packet_unref(v->packet);
    }
    return false;
}


bool Video_Load(VideoEngine *v, const char *filename) {
    v->formatCtx = NULL;
    if (avformat_open_input(&v->formatCtx, filename, NULL, NULL) != 0) return false;
    if (avformat_find_stream_info(v->formatCtx, NULL) < 0) return false;

    v->videoStreamIndex = av_find_best_stream(v->formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (v->videoStreamIndex < 0) return false;

    AVCodecParameters *codecParams = v->formatCtx->streams[v->videoStreamIndex]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);
    v->codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(v->codecCtx, codecParams);
    
    if (avcodec_open2(v->codecCtx, codec, NULL) < 0) return false;

    v->width = v->codecCtx->width;
    v->height = v->codecCtx->height;
    
    AVStream *stream = v->formatCtx->streams[v->videoStreamIndex];
    if (stream->avg_frame_rate.den > 0) v->fps = av_q2d(stream->avg_frame_rate);
    else v->fps = av_q2d(stream->r_frame_rate);


    if (stream->duration != AV_NOPTS_VALUE) v->durationSec = stream->duration * av_q2d(stream->time_base);
    else v->durationSec = (double)v->formatCtx->duration / AV_TIME_BASE;

    if (stream->nb_frames > 0) v->frameCount = stream->nb_frames;
    else v->frameCount = (long long)(v->durationSec * v->fps);


    memset(v->codecName, 0, sizeof(v->codecName));
    memset(v->pixelFormat, 0, sizeof(v->pixelFormat));
    const char *cName = avcodec_get_name(codecParams->codec_id);
    if (cName) strncpy(v->codecName, cName, 31);
    const char *pName = av_get_pix_fmt_name(v->codecCtx->pix_fmt);
    if (pName) strncpy(v->pixelFormat, pName, 31);

    v->frame = av_frame_alloc();
    v->frameRGB = av_frame_alloc();
    v->packet = av_packet_alloc();

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, v->width, v->height, 32);

    v->buffer = (uint8_t *)av_malloc(numBytes); 
    if (!v->buffer) return false;
    av_image_fill_arrays(v->frameRGB->data, v->frameRGB->linesize, 
                        v->buffer, AV_PIX_FMT_RGB24, 
                        v->width, v->height, 1); 

    v->swsCtx = NULL;
    Image img = { v->buffer, v->width, v->height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8 };
    v->texture = LoadTextureFromImage(img);
    
    v->isLoaded = true;
    v->isPlaying = false;
    v->baseTimeOffset = 0.0; 

    if (Video_DecodeAndDisplayOne(v)) {
        double firstFrameRaw = GetRawFrameTime(v);
        

        if (firstFrameRaw >= 0) {
            v->baseTimeOffset = firstFrameRaw;
        } 
        
        v->currentTime = 0.0;

        if (v->durationSec > v->baseTimeOffset) {
            v->durationSec -= v->baseTimeOffset;
        }
    }

    return true;
}


void Video_Update(VideoEngine *v) {
    if (!v->isLoaded || !v->isPlaying) return;

    v->accumulator += GetFrameTime();
    double frameDelay = 1.0 / v->fps;

    if (v->accumulator >= frameDelay) {
        bool frameDecoded = false;
        while (!frameDecoded) {
            if (av_read_frame(v->formatCtx, v->packet) < 0) {
                v->isPlaying = false;
                v->currentTime = v->durationSec; 
                break;
            }

            if (v->packet->stream_index == v->videoStreamIndex) {
                if (avcodec_send_packet(v->codecCtx, v->packet) == 0) {
                    if (avcodec_receive_frame(v->codecCtx, v->frame) == 0) {
                        Video_ProcessFrame(v);
                        frameDecoded = true;
                    }
                }
            }
            av_packet_unref(v->packet);
        }
        v->accumulator -= frameDelay;
    }
}


void Video_Seek(VideoEngine *v, double targetTime) {
    if (!v->isLoaded) return;

    if (targetTime < 0) targetTime = 0;
    if (targetTime > v->durationSec + 0.5) targetTime = v->durationSec + 0.5;

    double absoluteTarget = targetTime + v->baseTimeOffset;
    int64_t targetTS = (int64_t)(absoluteTarget / av_q2d(v->formatCtx->streams[v->videoStreamIndex]->time_base));
    
    if (av_seek_frame(v->formatCtx, v->videoStreamIndex, targetTS, AVSEEK_FLAG_BACKWARD) < 0) {
        return; 
    }

    avcodec_flush_buffers(v->codecCtx);
    
    bool reachedTarget = false;
    bool anyFrameDecoded = false;
    int safetyCount = 0;

    double halfFrame = (1.0 / v->fps) * 0.5;
    double threshold = targetTime - halfFrame;
    if (threshold < 0) threshold = -0.0001;

    while (!reachedTarget && safetyCount < 1000) { 
        int ret = av_read_frame(v->formatCtx, v->packet);
        if (ret < 0) break; // EOF

        if (v->packet->stream_index == v->videoStreamIndex) {
            if (avcodec_send_packet(v->codecCtx, v->packet) == 0) {
                while (avcodec_receive_frame(v->codecCtx, v->frame) == 0) {
                    anyFrameDecoded = true;
                    
                    double currentRel = GetRelativeFrameTime(v);
                    
                    if (currentRel >= threshold) {
                        Video_ProcessFrame(v);
                        reachedTarget = true;
                    } else {
                        v->currentTime = currentRel;
                    }
                }
            }
        }
        av_packet_unref(v->packet);
        if (!reachedTarget) safetyCount++;
    }

    if (!reachedTarget && anyFrameDecoded) {
        Video_ProcessFrame(v);
    }
    
    v->accumulator = 0;
}

void Video_NextFrame(VideoEngine *v) {
    if (!v->isLoaded) return;
    Video_Seek(v, v->currentTime + (1.0 / v->fps));
}

void Video_PrevFrame(VideoEngine *v) {
    if (!v->isLoaded) return;
    Video_Seek(v, v->currentTime - (1.0 / v->fps));
}

void Video_TogglePlay(VideoEngine *v) {
    if (v->isLoaded) {
        v->isPlaying = !v->isPlaying;
        if (v->isPlaying && v->currentTime >= v->durationSec - 0.05) {
            Video_Seek(v, 0);
        }
    }
}

void Video_Unload(VideoEngine *v) {
    if (!v->isLoaded) return;
    UnloadTexture(v->texture);
    if (v->swsCtx) {
        sws_freeContext(v->swsCtx);
        v->swsCtx = NULL;
    }
    if (v->frameRGB) av_frame_free(&v->frameRGB);
    if (v->frame) av_frame_free(&v->frame);
    if (v->packet) av_packet_free(&v->packet);
    if (v->codecCtx) avcodec_free_context(&v->codecCtx);
    if (v->formatCtx) avformat_close_input(&v->formatCtx);
    if (v->buffer) {
        av_free(v->buffer);
        v->buffer = NULL;
    }

    v->isLoaded = false;
}