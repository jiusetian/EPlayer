
#include "header/MediaMetadataRetriever.h"
#include <unistd.h>
#include "AndroidLog.h"

MediaMetadataRetriever::MediaMetadataRetriever() {
    av_register_all();
    avformat_network_init();
    state = NULL;
    mMetadata = new Metadata();
}

MediaMetadataRetriever::~MediaMetadataRetriever() {
    release();
    avformat_network_deinit();
    delete mMetadata;
    mMetadata = NULL;
}

void MediaMetadataRetriever::release() {
    Mutex::Autolock lock(mLock);
    release(&state);
}

int MediaMetadataRetriever::setDataSource(const char *url) {
    Mutex::Autolock lock(mLock);
    return setDataSource(&state, url, NULL);
}

status_t
MediaMetadataRetriever::setDataSource(const char *url, int64_t offset, const char *headers) {
    Mutex::Autolock lock(mLock);
    return setDataSource(&state, url, headers);
}

const char *MediaMetadataRetriever::getMetadata(const char *key) {
    Mutex::Autolock lock(mLock);
    return extractMetadata(&state, key);
}

const char *MediaMetadataRetriever::getMetadata(const char *key, int chapter) {
    Mutex::Autolock lock(mLock);
    return extractMetadata(&state, key, chapter);
}

int MediaMetadataRetriever::getMetadata(AVDictionary **metadata) {
    Mutex::Autolock lock(mLock);
    return getMetadata(&state, metadata);
}

int MediaMetadataRetriever::getEmbeddedPicture(AVPacket *pkt) {
    Mutex::Autolock lock(mLock);
    return getCoverPicture(&state, pkt);
}

int MediaMetadataRetriever::getFrame(int64_t timeUs, AVPacket *pkt) {
    Mutex::Autolock lock(mLock);
    return getFrame(&state, timeUs, pkt);
}

int MediaMetadataRetriever::getFrame(int64_t timeus, AVPacket *pkt, int width, int height) {
    Mutex::Autolock lock(mLock);
    return getFrame(&state, timeus, pkt, width, height);
}


/**
 * 释放资源
 * @param ps
 */
void MediaMetadataRetriever::release(MetadataState **ps) {
    MetadataState *state = *ps;
    if (!state) {
        return;
    }
    if (state->audioStream && state->audioStream->codec) {
        avcodec_close(state->audioStream->codec);
    }
    if (state->videoStream && state->videoStream->codec) {
        avcodec_close(state->videoStream->codec);
    }
    if (state->formatCtx) {
        avformat_close_input(&state->formatCtx);
    }
    if (state->fd != -1) {
        close(state->fd);
    }
    if (state->swsContext) {
        sws_freeContext(state->swsContext);
        state->swsContext = NULL;
    }
    if (state->videoCodecContext) {
        avcodec_close(state->videoCodecContext);
        av_free(state->videoCodecContext);
    }
    if (state->codecContext) {
        avcodec_close(state->codecContext);
        av_free(state->codecContext);
    }
    if (state->swsContext) {
        sws_freeContext(state->swsContext);
    }
    if (state->scaleCodecContext) {
        avcodec_close(state->scaleCodecContext);
        av_free(state->scaleCodecContext);
    }
    if (state->scaleSwsContext) {
        sws_freeContext(state->scaleSwsContext);
    }
    av_freep(&state);
    ps = NULL;
}

/**
 * 设置数据源
 * @param ps
 * @param path
 * @param headers
 * @return
 */
int MediaMetadataRetriever::setDataSource(MetadataState **ps, const char *path, const char *headers) {
    //**p是指针的指针，即二级指针，*p指的是一级指针，*p是一个指向MetadataState结构体的指针，将它赋值给state，此时state也是一个指向MetadataState的指针了，当然这个指针
    //不一定已经分配了内存空间
    MetadataState *state = *ps;

    init(&state);

    state->headers = headers;

    *ps = state;

    return setDataSource(ps, path);
}

/**
 * 设置数据源
 * @param ps
 * @param path
 * @return
 */
int MediaMetadataRetriever::setDataSource(MetadataState **ps, const char *path) {
    int audioIndex = -1;
    int videoIndex = -1;

    MetadataState *state = *ps;

    AVDictionary *options = NULL;
    av_dict_set(&options, "icy", "1", 0);
    av_dict_set(&options, "user_agent", "FFmpegMediaMetadataRetriever", 0);

    if (state->headers) { //媒体头文件
        //AVDictionary是一个健值对存储工具，类似于c++中的map，ffmpeg中有很多 API 通过它来传递参数，av_dict_set就是通过AVDictionary传递参数
        av_dict_set(&options, "headers", state->headers, 0);
    }

    if (state->offset > 0) { //偏移量
        state->formatCtx = avformat_alloc_context();
        state->formatCtx->skip_initial_bytes = state->offset;
    }
    //该函数用于打开多媒体数据并且获得一些相关的信息
    if (avformat_open_input(&state->formatCtx, path, NULL, &options) != 0) {
        LOGE("Metadata could not be retrieved\n");
        *ps = NULL;
        return -1;
    }

    //查找媒体流信息
    if (avformat_find_stream_info(state->formatCtx, NULL) < 0) {
        LOGE("Metadata could not be retrieved\n");
        avformat_close_input(&state->formatCtx);
        *ps = NULL;
        return -1;
    }


    // 查找媒体流
    for (int i = 0; i < state->formatCtx->nb_streams; i++) {
        if (state->formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoIndex < 0) {
            videoIndex = i;
        }
        if (state->formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioIndex < 0) {
            audioIndex = i;
        }
        // 设置编解码器信息
        mMetadata->setCodec(state->formatCtx, i);
    }
    // 打开音频流
    if (audioIndex >= 0) {
        openStream(state, audioIndex);
    }

    // 打开视频流
    if (videoIndex >= 0) {
        openStream(state, videoIndex);
    }

    // 设置metadata数据
    mMetadata->setDuration(state->formatCtx);
    mMetadata->setShoutcastMetadata(state->formatCtx);
    mMetadata->setRotation(state->formatCtx, state->audioStream, state->videoStream);
    mMetadata->setFrameRate(state->formatCtx, state->audioStream, state->videoStream);
    mMetadata->setFileSize(state->formatCtx);
    mMetadata->setChapterCount(state->formatCtx);
    mMetadata->setVideoSize(state->formatCtx, state->videoStream);

    *ps = state;
    return 0;
}

/**
 * 获取metadata
 * @param ps
 * @param metadata
 * @return
 */
int MediaMetadataRetriever::getMetadata(MetadataState **ps, AVDictionary **metadata) {
    MetadataState *state = *ps;

    if (!state || !state->formatCtx) {
        return -1;
    }

    mMetadata->getMetadata(state->formatCtx, metadata);

    return 0;
}


/**
 * 解析metadata
 * @param ps
 * @param key
 * @return
 */
const char *MediaMetadataRetriever::extractMetadata(MetadataState **ps, const char *key) {
    char *value = NULL;

    MetadataState *state = *ps;

    if (!state || !state->formatCtx) {
        return value;
    }

    return mMetadata->extractMetadata(state->formatCtx, state->audioStream, state->videoStream, key);
}

/**
 * 解析metadata
 * @param ps
 * @param key
 * @param chapter
 * @return
 */
const char *MediaMetadataRetriever::extractMetadata(MetadataState **ps, const char *key,
                                                    int chapter) {
    char *value = NULL;

    MetadataState *state = *ps;

    if (!state || !state->formatCtx || state->formatCtx->nb_chapters <= 0) {
        return value;
    }

    if (chapter < 0 || chapter >= state->formatCtx->nb_chapters) {
        return value;
    }

    return mMetadata->extractMetadata(state->formatCtx, state->audioStream, state->videoStream,
                                      key, chapter);
}

/**
 * 获取专辑/封面图片
 * @param ps
 * @param pkt
 * @return
 */
int MediaMetadataRetriever::getCoverPicture(MetadataState **ps, AVPacket *pkt) {
    int i = 0;
    int got_packet = 0;
    AVFrame *frame = NULL;

    MetadataState *state = *ps;

    if (!state || !state->formatCtx) {
        return -1;
    }
    //pFormatCtx描述了一个媒体文件或媒体流的构成和基本信息
    for (i = 0; i < state->formatCtx->nb_streams; i++) {
        //附加picture
        if (state->formatCtx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            if (pkt) { //pkt没有指向
                av_packet_unref(pkt);
                av_init_packet(pkt); //初始化packet
            }
            //将视频流中的附加picture赋值到pkt中
            av_copy_packet(pkt, &state->formatCtx->streams[i]->attached_pic);

            got_packet = 1;

            //如果是视频流的附加picture,需要做一些其他操作，主要是格式的转换，
            if (pkt->stream_index == state->videoStreamIndex) {
                AVCodecContext *avCodecContext = avcodec_alloc_context3(NULL);
                avcodec_parameters_to_context(avCodecContext, state->videoStream->codecpar);
                int codec_id = state->videoStream->codecpar->codec_id;
                //int pix_fmt = state->videoStream->codec->pix_fmt;
                int pix_fmt = avCodecContext->pix_fmt;

                if (!formatSupport(codec_id, pix_fmt)) { //如果不支持指定的格式，则下面进行一下格式的转换
                    int got_frame = 0;

                    frame = av_frame_alloc();

                    if (!frame) {
                        break;
                    }

                    // 解码视频帧，将pkt的压缩数据解码成原始视频帧数据frame，此方法已经过时
                    //错误返回负值，负值返回解码的byte数量，如果没有帧可以解码，则got_frame为0，否则非零
//                    if (avcodec_decode_video2(avCodecContext, frame, &got_frame, pkt) <= 0) {
//                        break; //如果发生错误或者没有数据可以解码，则跳出当前循环
//                    }
                    //新api 的使用，成功返回0
                    if (avcodec_send_packet(avCodecContext, pkt) < 0) {
                        break;
                    }
                    got_frame = avcodec_receive_frame(avCodecContext, frame);
                    if (got_frame < 0) {
                        break;
                    }
                    //解码成功
                    if (got_frame == 0) {
                        AVPacket avPacket;
                        av_init_packet(&avPacket);
                        avPacket.size = 0;
                        avPacket.data = NULL;
                        //主要进行格式的相关转换
                        encodeImage(state, avCodecContext, frame, &avPacket,
                                    &got_packet, -1, -1);

                        av_packet_unref(pkt);
                        av_init_packet(pkt);
                        //复制视频帧数据
                        av_copy_packet(pkt, &avPacket);

                        av_packet_unref(&avPacket);
                        break;
                    }
                } else {
                    av_packet_unref(pkt);
                    av_init_packet(pkt);
                    av_copy_packet(pkt, &state->formatCtx->streams[i]->attached_pic);

                    got_packet = 1;
                    break;
                }
                //释放
                avcodec_free_context(&avCodecContext);
            }
        }
    }

    av_frame_free(&frame);

    return got_packet ? 0 : -1;
}

/**
 * 提取视频帧
 * @param ps
 * @param timeUs
 * @param pkt
 * @return
 */
int MediaMetadataRetriever::getFrame(MetadataState **ps, int64_t timeUs, AVPacket *pkt) {
    return getFrame(ps, timeUs, pkt, -1, -1);
}

/**
 * 提取视频帧
 * @param ps
 * @param timeUs
 * @param pkt 保存提取到的压缩视频数据
 * @param width
 * @param height
 * @return
 */
int MediaMetadataRetriever::getFrame(MetadataState **ps, int64_t timeUs, AVPacket *pkt, int width,
                                     int height) {
    int got_packet = 0;
    int64_t desired_frame_number = -1;

    MetadataState *state = *ps;

    if (!state || !state->formatCtx || state->videoStreamIndex < 0) {
        return -1;
    }

    if (timeUs > -1) {

        // 计算定位的时间，需要转换为time_base对应的时钟格式
        int stream_index = state->videoStreamIndex;
        //转换time_base对应的时间格式
        int64_t seek_time = av_rescale_q(timeUs, AV_TIME_BASE_Q,
                                         state->formatCtx->streams[stream_index]->time_base);
        int64_t seek_stream_duration = state->formatCtx->streams[stream_index]->duration; //时长
        if (seek_stream_duration > 0 && seek_time > seek_stream_duration) {
            seek_time = seek_stream_duration;
        }

        if (seek_time < 0) {
            return -1;
        }

        // 定位到指定的时间点，这样获取到的就是seek_time的视频数据了
        int ret = av_seek_frame(state->formatCtx, stream_index, seek_time, AVSEEK_FLAG_BACKWARD);

        // 刷新缓冲
        if (ret < 0) {
            return -1;
        } else {
            //需要重置内部的解码状态和buffers，在seek定位的时候用到
            if (state->audioStreamIndex >= 0) {
                avcodec_flush_buffers(state->audioStream->codec);
            }
            if (state->videoStreamIndex >= 0) {
                avcodec_flush_buffers(state->videoCodecContext);
            }
        }
    }

    // 解码
    decodeFrame(state, pkt, &got_packet, desired_frame_number, width, height);

    return got_packet ? 0 : -1;
}

/**
 * 判断格式是否支持
 * @param codec_id
 * @param pix_fmt
 * @return
 */
int MediaMetadataRetriever::formatSupport(int codec_id, int pix_fmt) {
    if ((codec_id == AV_CODEC_ID_PNG ||
         codec_id == AV_CODEC_ID_MJPEG ||
         codec_id == AV_CODEC_ID_BMP) &&
        pix_fmt == AV_PIX_FMT_RGBA) {
        return 1;
    }

    return 0;
}

/**
 * 初始化
 * @param ps
 */
void MediaMetadataRetriever::init(MetadataState **ps) {
    MetadataState *state = *ps;

    if (state && state->formatCtx) {
        avformat_close_input(&state->formatCtx);
    }

    if (state && state->fd != -1) {
        close(state->fd);
    }

    if (!state) { //如果指针还没有指向，则分配内存空间
        state = static_cast<MetadataState *>(av_mallocz(sizeof(MetadataState)));
    }

    state->formatCtx = NULL;
    state->audioStreamIndex = -1;
    state->videoStreamIndex = -1;
    state->audioStream = NULL;
    state->videoStream = NULL;
    state->fd = -1;
    state->offset = 0;
    state->headers = NULL;
    //改变一级指针的值
    *ps = state;
}

/**
 * 初始化缩放转码上下文
 * @param s
 * @param pCodecCtx
 * @param width
 * @param height
 * @return
 */
int MediaMetadataRetriever::initScaleContext(MetadataState *s, AVCodecContext *pCodecCtx, int width,
                                             int height) {
    AVCodec *targetCodec = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!targetCodec) {
        LOGE("avcodec_find_decoder() failed to find encoder\n");
        return -1;
    }
    //转换编解码上下文
    s->scaleCodecContext = avcodec_alloc_context3(targetCodec); //分配内存空间
    if (!s->scaleCodecContext) {
        LOGE("avcodec_alloc_context3 failed\n");
        return -1;
    }

    s->scaleCodecContext->bit_rate = s->videoStream->codecpar->bit_rate;
    s->scaleCodecContext->width = width;
    s->scaleCodecContext->height = height;
    s->scaleCodecContext->pix_fmt = AV_PIX_FMT_RGBA;
    s->scaleCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    s->scaleCodecContext->time_base.num = s->videoCodecContext->time_base.num;
    s->scaleCodecContext->time_base.den = s->videoCodecContext->time_base.den;

    if (avcodec_open2(s->scaleCodecContext, targetCodec, NULL) < 0) {
        LOGE("avcodec_open2() failed\n");
        return -1;
    }

    //第四、五个参数代表输出宽高
    s->scaleSwsContext = sws_getContext(s->videoStream->codecpar->width,
                                        s->videoStream->codecpar->height,
                                        s->videoCodecContext->pix_fmt,
                                        width,
                                        height,
                                        AV_PIX_FMT_RGBA,
                                        SWS_BILINEAR,
                                        NULL,
                                        NULL,
                                        NULL);

    return 0;
}

/**
 * 打开媒体流
 * @param s
 * @param streamIndex
 * @return
 */
int MediaMetadataRetriever::openStream(MetadataState *s, int streamIndex) {
    AVFormatContext *pFormatCtx = s->formatCtx;
    AVCodecContext *codecCtx = avcodec_alloc_context3(NULL);


    if (streamIndex < 0 || streamIndex >= pFormatCtx->nb_streams) {
        return -1;
    }

    // 解码上下文
    //codecCtx = pFormatCtx->streams[streamIndex]->codec;
    avcodec_parameters_to_context(codecCtx, pFormatCtx->streams[streamIndex]->codecpar);

    // 查找解码器
    AVCodec *codec = avcodec_find_decoder(codecCtx->codec_id);
    if (codec == NULL) {
        LOGE("avcodec_find_decoder() failed to find audio decoder\n");
        return -1;
    }

    // 打开解码器
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        LOGE("avcodec_open2() failed\n");
        return -1;
    }

    // 根据解码类型查找媒体，并设置媒体流信息到MetadataState中
    switch (codecCtx->codec_type) {
        case AVMEDIA_TYPE_AUDIO: {
            s->audioStreamIndex = streamIndex;
            s->audioStream = pFormatCtx->streams[streamIndex];
            break;
        }

        case AVMEDIA_TYPE_VIDEO: {
            s->videoStreamIndex = streamIndex;
            s->videoStream = pFormatCtx->streams[streamIndex];
            //给编解码上下文赋值
            s->videoCodecContext = avcodec_alloc_context3(NULL);
            avcodec_parameters_to_context(s->videoCodecContext, s->formatCtx->streams[streamIndex]->codecpar);
            //查找图像png编码器
            AVCodec *pCodec = avcodec_find_encoder(AV_CODEC_ID_PNG);
            if (!pCodec) {
                LOGE("avcodec_find_decoder() failed to find encoder\n");
                return -1;
            }

            s->codecContext = avcodec_alloc_context3(pCodec);
            if (!s->codecContext) {
                LOGE("avcodec_alloc_context3 failed\n");
                return -1;
            }

            //设置视频相关参数信息
            s->codecContext->bit_rate = s->videoStream->codecpar->bit_rate;
            s->codecContext->width = s->videoStream->codecpar->width;
            s->codecContext->height = s->videoStream->codecpar->height;
            s->codecContext->pix_fmt = AV_PIX_FMT_RGBA;
            s->codecContext->codec_type = AVMEDIA_TYPE_VIDEO;
            s->codecContext->time_base.num = codecCtx->time_base.num;
            s->codecContext->time_base.den = codecCtx->time_base.den;

            // 该函数用于初始化一个视音频编解码器的AVCodecContext
            if (avcodec_open2(s->codecContext, pCodec, NULL) < 0) {
                LOGE("avcodec_open2() failed\n");
                return -1;
            }

            //成功后返回SwsContext 类型的结构体,视频格式转换结构体
            //参数1：被转换源的宽
            //参数2：被转换源的高
            //参数3：被转换源的格式，eg：YUV、RGB……（枚举格式，也可以直接用枚举的代号表示eg：AV_PIX_FMT_YUV420P这些枚举的格式在libavutil/pixfmt.h中列出）
            //参数4：转换后指定的宽
            //参数5：转换后指定的高pScaleSwsContext
            //参数6：转换后指定的格式同参数3的格式
            //参数7：转换所使用的算法，
            //参数8：NULL
            //参数9：NULL
            //参数10：NULL
            s->swsContext = sws_getContext(s->videoStream->codecpar->width,
                                           s->videoStream->codecpar->height,
                                           codecCtx->pix_fmt,
                                           s->videoStream->codecpar->width,
                                           s->videoStream->codecpar->height,
                                           AV_PIX_FMT_RGBA, //像素格式
                                           SWS_BILINEAR,
                                           NULL,
                                           NULL,
                                           NULL);
            break;
        }

        default: {
            break;
        }
    }
    avcodec_free_context(&codecCtx);
    return 0;
}

/**
 * 解码视频帧，
 * @param state
 * @param pkt 保存压缩数据
 * @param got_frame
 * @param desired_frame_number
 * @param width 格式转换后的宽
 * @param height 格式转换后的高
 */
void MediaMetadataRetriever::decodeFrame(MetadataState *state, AVPacket *pkt, int *got_frame,
                                         int64_t desired_frame_number, int width, int height) {
    AVFrame *frame = av_frame_alloc();

    *got_frame = 0;

    if (!frame) {
        return;
    }

    // 读入数据
    //    s：输入的AVFormatContext
    //    pkt：输出的AVPacket
    while (av_read_frame(state->formatCtx, pkt) >= 0) {
        // 找到视频流所在的裸数据
        if (pkt->stream_index == state->videoStreamIndex) {

            int codec_id = state->videoStream->codecpar->codec_id;
            int pix_fmt = state->videoCodecContext->pix_fmt;

            if (!formatSupport(codec_id, pix_fmt)) {
                *got_frame = 0;

                // 解码得到视频帧，将pkt数据解码得到视频帧frame
//                if (avcodec_decode_video2(state->videoStream->codec, frame, got_frame, pkt) <= 0) {
//                    *got_frame = 0;
//                    break;
//                }

                //新api 的使用，成功返回0
                if (avcodec_send_packet(state->videoCodecContext, pkt) < 0) {
                    *got_frame = 0;
                    break;
                }
                if (avcodec_receive_frame(state->videoCodecContext, frame) < 0) {
                    *got_frame = 0;
                    break;
                }
                *got_frame = 1;

                // 得到frame数据以后进行相关格式转换
                if (*got_frame) {
                    if (desired_frame_number == -1 ||
                        (desired_frame_number != -1 && frame->pts >= desired_frame_number)) {
                        if (pkt->data) {
                            av_packet_unref(pkt);
                        }
                        av_init_packet(pkt);
                        encodeImage(state, state->videoCodecContext, frame, pkt, got_frame, width,
                                    height);
                        break;
                    }
                }

            } else {
                *got_frame = 1;
                break;
            }

        }

    }

    av_frame_free(&frame);
}


/**
 * 编码成图片
 * @param state
 * @param pCodecCtx
 * @param pFrame 原始数据
 * @param packet 保存编码后的数据
 * @param got_packet 是否编码成功
 * @param width 编码后的宽
 * @param height
 */
void MediaMetadataRetriever::encodeImage(MetadataState *state, AVCodecContext *pCodecCtx,
                                         AVFrame *pFrame, AVPacket *packet, int *got_packet,
                                         int width, int height) {
    AVCodecContext *codecCtx;
    struct SwsContext *scaleCtx;
    AVFrame *frame = av_frame_alloc();

    *got_packet = 0;

    //有没有指定的宽高
    if (width != -1 && height != -1) {
        if (state->scaleCodecContext == NULL ||
            state->scaleSwsContext == NULL) {
            initScaleContext(state, pCodecCtx, width, height);
        }

        codecCtx = state->scaleCodecContext;
        scaleCtx = state->scaleSwsContext;
    } else {
        codecCtx = state->codecContext;
        scaleCtx = state->swsContext;
    }

    if (width == -1) {
        width = pCodecCtx->width;
    }

    if (height == -1) {
        height = pCodecCtx->height;
    }

    // 设置视频帧参数
    frame->format = AV_PIX_FMT_RGBA;
    frame->width = codecCtx->width;
    frame->height = codecCtx->height;

    // 创建缓冲区
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height,
                                            1); //计算所需内存大小
    uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t)); //申请内存

    //前面是分配了内存，但是frame->data的指针并没有指向内存，这里把av_malloc得到的内存和AVFrame关联起来，同时该函数还好对AVFrame->linesize变量进行赋值
    //看到这里，可能有些读者会疑问：data成员是一个指针数组(即数组里面的每一个元素都是一个指针)，一个buff怎么够用(多对一的关系)。其实，
    // 这就是FFmpeg设计的一个巧妙之处。还记得前一篇博文说到的 图像物理存储有 planar和packed两种模式吗？
    //这个data指针数组就是为了planar设计的。对于planar模式的YUV。data[0]指向Y分量的开始位置、data[1]指向U分量的开始位置、data[2]指向V分量的开始位置。
    //对于packed模式YUV，data[0]指向数据的开始位置，而data[1]和data[2]都为NULL
    //在下面的代码中，运行av_image_fill_arrays后，data[0]将指向buff的开始位置，即data[0]等于buff。data[1]指向buff数组的某一个位置(该位置为U分量的开始处)，
    // data[2]也指向buff数组某一个位置(该位置为V分量的开始处)
    av_image_fill_arrays(frame->data,
                         frame->linesize,
                         buffer,
                         AV_PIX_FMT_RGBA,
                         codecCtx->width,
                         codecCtx->height, 1);

    // 转码，按照SwsContext的指定格式将src数据转换为dst数据
    sws_scale(scaleCtx, //按照创建好的scale上下文
              (const uint8_t *const *) pFrame->data,
              pFrame->linesize,
              0,
              pFrame->height,
              frame->data,
              frame->linesize);


//    int ret = avcodec_encode_video2(codecCtx, packet, frame, got_packet);
//    if (ret < 0) {
//        *got_packet = 0;
//    }
    //frame首先经过格式转换，然后再进行压缩的得到AVpacket
    // 视频帧编码即压缩
    int ret1 = avcodec_send_frame(codecCtx, frame); //返回0代表成功，负数代表失败
    int ret2 = avcodec_receive_packet(codecCtx, packet); //同上
    if (ret1 == 0 && ret2 == 0) { //成功
        *got_packet = 1;
    }

    // 释放内存
    av_frame_free(&frame);
    if (buffer) {
        free(buffer);
    }

    // 出错时释放裸数据包资源
    if (!*got_packet) {
        av_packet_unref(packet);
    }
}