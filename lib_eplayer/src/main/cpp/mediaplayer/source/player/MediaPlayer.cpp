
#include "MediaPlayer.h"

/**
 * FFmpeg操作锁管理回调
 * @param mtx 这是一个二级指针，即指针的指针，传递二级指针的目的是为了改变一级指针的值
 * @param op
 * @return
 */
static int lockmgrCallback(void **mtx, enum AVLockOp op) {
    switch (op) {
        case AV_LOCK_CREATE: {
            *mtx = new Mutex();
            if (!*mtx) {
                av_log(NULL, AV_LOG_FATAL, "failed to create mutex.\n");
                return 1;
            }
            return 0;
        }

        case AV_LOCK_OBTAIN: {
            if (!*mtx) {
                return 1;
            }
            return ((Mutex *) (*mtx))->lock() != 0;
        }

        case AV_LOCK_RELEASE: {
            if (!*mtx) {
                return 1;
            }
            return ((Mutex *) (*mtx))->unlock() != 0;
        }

        case AV_LOCK_DESTROY: {
            if (!*mtx) {
                delete (*mtx);
                *mtx = NULL;
            }
            return 0;
        }
    }
    return 1;
}

MediaPlayer::MediaPlayer() {
    av_register_all();
    avformat_network_init();
    //主要用来保存播放器的信息
    playerState = new PlayerState();
    mDuration = -1;
    audioDecoder = NULL;
    videoDecoder = NULL;
    pFormatCtx = NULL;
    lastPaused = -1;
    attachmentRequest = 0;

#if defined(__ANDROID__)
    audioDevice = new SLESDevice();


#else
    audioDevice = new AudioDevice();
#endif

    mediaSync = new MediaSync(playerState);
    audioResampler = NULL;
    readThread = NULL;
    mExit = true;

    // 注册一个多线程锁管理回调，主要是解决多个视频源时保持avcodec_open/close的原子操作
    if (av_lockmgr_register(lockmgrCallback)) {
        av_log(NULL, AV_LOG_FATAL, "Could not initialize lock manager!\n");
    }

}

MediaPlayer::~MediaPlayer() {
    avformat_network_deinit();
    av_lockmgr_register(NULL);
}

status_t MediaPlayer::reset() {
    stop();
    if (mediaSync) {
        mediaSync->reset();
        delete mediaSync;
        mediaSync = NULL;
    }
    if (audioDecoder != NULL) {
        audioDecoder->stop();
        delete audioDecoder;
        audioDecoder = NULL;
    }
    if (videoDecoder != NULL) {
        videoDecoder->stop();
        delete videoDecoder;
        videoDecoder = NULL;
    }
    if (audioDevice != NULL) {
        audioDevice->stop();
        delete audioDevice;
        audioDevice = NULL;
    }
    if (audioResampler) {
        delete audioResampler;
        audioResampler = NULL;
    }
    if (pFormatCtx != NULL) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = NULL;
    }
    if (playerState) {
        delete playerState;
        playerState = NULL;
    }
    return NO_ERROR;
}

void MediaPlayer::setDataSource(const char *url, int64_t offset, const char *headers) {
    //因为Autolock属于一个局部变量，在这里执行了lock上锁操作，当方法执行完，这个局部变量要销毁，会执行析构函数，而在析构函数中会执行解锁操作unlock
    Mutex::Autolock lock(mMutex);
    playerState->url = av_strdup(url); //拷贝字符串
    playerState->offset = offset; //文件偏移量
    if (headers) { //一般没有headers
        playerState->headers = av_strdup(headers);
    }
}

void MediaPlayer::setVideoDevice(VideoDevice *videoDevice) {
    Mutex::Autolock lock(mMutex);
    mediaSync->setVideoDevice(videoDevice);
}

status_t MediaPlayer::prepare() {
    Mutex::Autolock lock(mMutex);
    if (!playerState->url) {
        return BAD_VALUE;
    }
    playerState->abortRequest = 0;
    if (!readThread) {
        readThread = new Thread(this); //因为当前类继承了runnable
        readThread->start();
    }
    return NO_ERROR;
}

status_t MediaPlayer::prepareAsync() {
    Mutex::Autolock lock(mMutex);
    if (!playerState->url) {
        return BAD_VALUE;
    }
    // 发送消息
    if (playerState->messageQueue) {
        //异步的意思就是在工作线程中去准备，这里首先发送一个MSG_REQUEST_PREPARE出去，在CainMediaPlayer类处理消息的线程中收到消息以后，
        //再调用prepare方法，这样就实现了在接收消息的工作线程中去执行prepare方法
        playerState->messageQueue->postMessage(MSG_REQUEST_PREPARE);
    }
    return NO_ERROR;
}

void MediaPlayer::start() {
    Mutex::Autolock lock(mMutex);
    playerState->abortRequest = 0;
    playerState->pauseRequest = 0;
    mExit = false;
    mCondition.signal();
}

void MediaPlayer::pause() {
    Mutex::Autolock lock(mMutex);
    playerState->pauseRequest = 1;
    mCondition.signal();
}

void MediaPlayer::resume() {
    Mutex::Autolock lock(mMutex);
    playerState->pauseRequest = 0;
    mCondition.signal();
}

void MediaPlayer::stop() {
    mMutex.lock();
    playerState->abortRequest = 1;
    mCondition.signal();
    mMutex.unlock();
    mMutex.lock();
    while (!mExit) {
        mCondition.wait(mMutex);
    }
    mMutex.unlock();
    if (readThread != NULL) {
        readThread->join();
        delete readThread;
        readThread = NULL;
    }
}

void MediaPlayer::seekTo(float timeMs) {
    // when is a live media stream, duration is -1
    if (!playerState->realTime && mDuration < 0) {
        return;
    }

    // 等待上一次操作完成
    mMutex.lock();
    while (playerState->seekRequest) {
        mCondition.wait(mMutex);
    }
    mMutex.unlock();

    if (!playerState->seekRequest) {
        int64_t start_time = 0;
        int64_t seek_pos = av_rescale(timeMs, AV_TIME_BASE, 1000);
        start_time = pFormatCtx ? pFormatCtx->start_time : 0;
        if (start_time > 0 && start_time != AV_NOPTS_VALUE) {
            seek_pos += start_time;
        }
        //设置定位位置
        playerState->seekPos = seek_pos;
        playerState->seekRel = 0;
        playerState->seekFlags &= ~AVSEEK_FLAG_BYTE;
        playerState->seekRequest = 1; //有定位请求
        mCondition.signal();
    }

}

void MediaPlayer::setLooping(int looping) {
    mMutex.lock();
    playerState->loop = looping;
    mCondition.signal();
    mMutex.unlock();
}

void MediaPlayer::setVolume(float leftVolume, float rightVolume) {
    if (audioDevice) {
        audioDevice->setStereoVolume(leftVolume, rightVolume);
    }
}

void MediaPlayer::setMute(int mute) {
    mMutex.lock();
    playerState->mute = mute;
    mCondition.signal();
    mMutex.unlock();
}

void MediaPlayer::setRate(float rate) {
    mMutex.lock();
    playerState->playbackRate = rate;
    mCondition.signal();
    mMutex.unlock();
}

void MediaPlayer::setPitch(float pitch) {
    mMutex.lock();
    playerState->playbackPitch = pitch;
    mCondition.signal();
    mMutex.unlock();
}

int MediaPlayer::getRotate() {
    Mutex::Autolock lock(mMutex);
    if (videoDecoder) {
        return videoDecoder->getRotate();
    }
    return 0;
}

int MediaPlayer::getVideoWidth() {
    Mutex::Autolock lock(mMutex);
    if (videoDecoder) {
        return videoDecoder->getCodecContext()->width;
    }
    return 0;
}

int MediaPlayer::getVideoHeight() {
    Mutex::Autolock lock(mMutex);
    if (videoDecoder) {
        return videoDecoder->getCodecContext()->height;
    }
    return 0;
}

long MediaPlayer::getCurrentPosition() {
    Mutex::Autolock lock(mMutex);
    int64_t currentPosition = 0;
    // 处于定位
    if (playerState->seekRequest) {
        currentPosition = playerState->seekPos;
    } else {

        // 起始延时
        int64_t start_time = pFormatCtx->start_time;

        int64_t start_diff = 0;
        if (start_time > 0 && start_time != AV_NOPTS_VALUE) {
            start_diff = av_rescale(start_time, 1000, AV_TIME_BASE);
        }

        // 计算主时钟的时间
        int64_t pos = 0;
        double clock = mediaSync->getMasterClock();
        if (isnan(clock)) {
            pos = playerState->seekPos;
        } else {
            pos = (int64_t) (clock * 1000);
        }
        if (pos < 0 || pos < start_diff) {
            return 0;
        }
        return (long) (pos - start_diff);
    }
    return (long) currentPosition;
}

long MediaPlayer::getDuration() {
    Mutex::Autolock lock(mMutex);
    return (long) mDuration;
}

int MediaPlayer::isPlaying() {
    Mutex::Autolock lock(mMutex);
    return !playerState->abortRequest && !playerState->pauseRequest;
}

int MediaPlayer::isLooping() {
    return playerState->loop;
}

int MediaPlayer::getMetadata(AVDictionary **metadata) {
    if (!pFormatCtx) {
        return -1;
    }
    // TODO getMetadata
    return NO_ERROR;
}

static int avformat_interrupt_cb(void *ctx) {
    PlayerState *playerState = (PlayerState *) ctx;
    if (playerState->abortRequest) {
        return AVERROR_EOF;
    }
    return 0;
}

AVMessageQueue *MediaPlayer::getMessageQueue() {
    Mutex::Autolock lock(mMutex);
    return playerState->messageQueue;
}

PlayerState *MediaPlayer::getPlayerState() {
    Mutex::Autolock lock(mMutex);
    return playerState;
}

void MediaPlayer::run() {
    startPlayer();
}

/**
 * 关键函数，开始播放
 * @return
 */
int MediaPlayer::startPlayer() {
    int ret = 0;
    //解封装和准备解码器
    ret = demuxAndPrepareDecoder();
    // 出错返回
    if (ret < 0) {
        mExit = true;
        mCondition.signal();
        //通知准备播放器失败
        if (playerState->messageQueue) {
            const char errorMsg[] = "prepare decoder failed!";
            playerState->messageQueue->postMessage(MSG_ERROR, 0, 0, (void *) errorMsg,
                                                   sizeof(errorMsg) / errorMsg[0]);
        }
        return -1; //解封装出错
    }

    //开始解码和同步播放
    startDecodeAndSync();

    //读取packet数据
    ret = readAvPackets();
    //LOGE("返回值%d", ret);
    if (ret < 0 && ret != AVERROR_EOF) { // 播放出错
        //LOGE("出错");
        if (playerState->messageQueue) {
            const char errorMsg[] = "error when reading packets!";
            playerState->messageQueue->postMessage(MSG_ERROR, 0, 0, (void *) errorMsg,
                                                   sizeof(errorMsg) / errorMsg[0]);
        }
    }

    // 停止消息队列
    if (playerState->messageQueue && ret != AVERROR_EOF) {
        playerState->messageQueue->stop();
    }

    return ret;
}

/**
 * 解封装和准备音视频解码器
 * @return
 */
int MediaPlayer::demuxAndPrepareDecoder() {
    int ret = 0;
    AVDictionaryEntry *t;
    AVDictionary **opts;
    int scan_all_pmts_set = 0;

    // 准备解码器
    mMutex.lock();
    //
    do {
        // 解封装功能结构体
        pFormatCtx = avformat_alloc_context();
        if (!pFormatCtx) {
            av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
            ret = AVERROR(ENOMEM); //内存不足错误
            break;
        }

        // 设置解封装中断回调
        pFormatCtx->interrupt_callback.callback = avformat_interrupt_cb;
        pFormatCtx->interrupt_callback.opaque = playerState; //是回调函数的参数

        //设置解封装option参数，AV_DICT_MATCH_CASE是精准匹配key值，第二个参数是key值
        //设置"scan_all_pmts"，应该只是针对avformat_open_input()函数用的。之后就又设为NULL了
        if (!av_dict_get(playerState->format_opts, "scan_all_pmts", NULL,
                         AV_DICT_MATCH_CASE)) { //是否有设置了参数
            //scan_all_pmts的特殊处理为基于ffplay开发的软件提供了一个“很好的错误示范”，导致经常看到针对特定编码或封装的特殊选项、特殊处理充满了read_thread
            // 影响代码可读性！
            av_dict_set(&playerState->format_opts, "scan_all_pmts", "1",
                        AV_DICT_DONT_OVERWRITE); //设置参数
            scan_all_pmts_set = 1;
        }

        // 处理文件头
        if (playerState->headers) { //一般没有头文件
            av_dict_set(&playerState->format_opts, "headers", playerState->headers, 0);
        }
        // 处理文件偏移量
        if (playerState->offset > 0) {
            pFormatCtx->skip_initial_bytes = playerState->offset;
        }

        // 设置rtmp/rtsp的超时值
        if (av_stristart(playerState->url, "rtmp", NULL) ||
            av_stristart(playerState->url, "rtsp", NULL)) {
            // There is total different meaning for 'timeout' option in rtmp
            av_log(NULL, AV_LOG_WARNING, "remove 'timeout' option for rtmp.\n");
            av_dict_set(&playerState->format_opts, "timeout", NULL, 0); //设置为null
        }

        // 打开文件
        //参数说明：
        //AVFormatContext **ps, 格式化的上下文。要注意，如果传入的是一个AVFormatContext*的指针，则该空间须自己手动清理，若传入的指针为空，则FFmpeg会内部自己创建。
        //const char *url, 传入的地址。支持http,RTSP,以及普通的本地文件。地址最终会存入到AVFormatContext结构体当中。
        //AVInputFormat *fmt, 指定输入的封装格式。一般传NULL，由FFmpeg自行探测。
        //AVDictionary **options, 其它参数设置。它是一个字典，用于参数传递，不传则写NULL。参见：libavformat/options_table.h,其中包含了它支持的参数设置。
        ret = avformat_open_input(&pFormatCtx, playerState->url, playerState->iformat,
                                  &playerState->format_opts);
        //LOGE("字典的大小%d",playerState->format_opts->count)
        if (ret < 0) {
            LOGE("打开文件失败");
            printError(playerState->url, ret);
            ret = -1;
            break;
        }
        LOGE("打开文件成功");
        // 通知文件已打开
        if (playerState->messageQueue) {
            playerState->messageQueue->postMessage(MSG_OPEN_INPUT);
        }
        //又设置为null
        if (scan_all_pmts_set) {
            av_dict_set(&playerState->format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);
        }
        //上面设置以后，字典大小已经为null了
        //LOGE("字典的大小%d",playerState->format_opts->count)
        //迭代字典，""是所有字符串的前缀，如果有条目，则获取到字典中的第一个条目
        if ((t = av_dict_get(playerState->format_opts, "", NULL,
                             AV_DICT_IGNORE_SUFFIX))) { //这里应该返回null，即字典里没有条目了
            av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
            ret = AVERROR_OPTION_NOT_FOUND;
            break;
        }

        if (playerState->genpts) {
            pFormatCtx->flags |= AVFMT_FLAG_GENPTS;
        }
        //插入全局的附加信息
        av_format_inject_global_side_data(pFormatCtx);
        //设置媒体流信息参数
        opts = setupStreamInfoOptions(pFormatCtx, playerState->codec_opts);

        // 查找媒体流信息
        ret = avformat_find_stream_info(pFormatCtx, opts);
        //释放字典内存
        if (opts != NULL) {
            for (int i = 0; i < pFormatCtx->nb_streams; i++) {
                if (opts[i] != NULL) {
                    av_dict_free(&opts[i]);
                }
            }
            av_freep(&opts);
        }

        if (ret < 0) {
            av_log(NULL, AV_LOG_WARNING, "%s: could not find codec parameters\n", playerState->url);
            ret = -1;
            break;
        }

        // 查找媒体流信息回调
        if (playerState->messageQueue) {
            playerState->messageQueue->postMessage(MSG_FIND_STREAM_INFO);
        }

        // 判断是否实时流，判断是否需要设置无限缓冲区
        playerState->realTime = isRealTime(pFormatCtx);
        //如果是实时流和偏移量<0
        if (playerState->infiniteBuffer < 0 && playerState->realTime) {
            playerState->infiniteBuffer = 1;
        }

        // Gets the duration of the file, -1 if no duration available.
        if (playerState->realTime) { //如果实时流，视频就没有时长，比如直播就属于实时流
            mDuration = -1;
        } else {
            mDuration = -1;
            if (pFormatCtx->duration != AV_NOPTS_VALUE) {
                // a*b/c，函数表示在b下的占a个格子，在c下是多少
                mDuration = av_rescale(pFormatCtx->duration, 1000, AV_TIME_BASE); //视频时长，单位毫秒
            }
        }

        //设置av时长
        playerState->videoDuration = mDuration;
        //AVIOContext *pb //IO上下文，自定义格式读/从内存当中读
        if (pFormatCtx->pb) {
            pFormatCtx->pb->eof_reached = 0; //是否读到文件尾
        }

        // 判断是否以字节方式定位，!!取两次反，是为了把非0值转换成1,而0值还是0。
        playerState->seekByBytes =
                !!(pFormatCtx->iformat->flags & AVFMT_TS_DISCONT) &&
                strcmp("ogg", pFormatCtx->iformat->name);

        // 设置最大帧间隔
        mediaSync->setMaxDuration((pFormatCtx->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0);

        // 如果不是从头开始播放，则跳转到播放位置
        if (playerState->startTime !=
            AV_NOPTS_VALUE) { //AV_NOPTS_VALUE表示没有时间戳的意思，一般刚开始第一帧是AV_NOPTS_VALUE的
            int64_t timestamp;

            timestamp = playerState->startTime;
            if (pFormatCtx->start_time != AV_NOPTS_VALUE) {
                timestamp += pFormatCtx->start_time;
            }
            //加锁
            playerState->mMutex.lock();
            //视频时间定位
            ret = avformat_seek_file(pFormatCtx, -1, INT64_MIN, timestamp, INT64_MAX, 0);
            playerState->mMutex.unlock();
            if (ret < 0) {
                av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
                       playerState->url, (double) timestamp / AV_TIME_BASE);
            }
        }

        // 查找媒体流索引
        int audioIndex = -1;
        int videoIndex = -1;
        for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
            if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                if (audioIndex == -1) {
                    audioIndex = i;
                }
            } else if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                if (videoIndex == -1) {
                    videoIndex = i;
                }
            }
        }

        // 如果不禁止视频流，则查找最合适的视频流索引
        if (!playerState->videoDisable) {
            videoIndex = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, videoIndex, -1, NULL,
                                             0);
        } else {
            videoIndex = -1;
        }
        // 如果不禁止音频流，则查找最合适的音频流索引(与视频流关联的音频流)
        if (!playerState->audioDisable) {
            audioIndex = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_AUDIO, audioIndex, videoIndex,
                                             NULL, 0);
        } else {
            audioIndex = -1;
        }

        // 如果音频流和视频流都没有找到，则直接退出
        if (audioIndex == -1 && videoIndex == -1) {
            av_log(NULL, AV_LOG_WARNING, "%s: could not find audio and video stream\n",
                   playerState->url);
            ret = -1;
            break;
        }

        // 准备视频和音频的解码器
        if (audioIndex >= 0) {
            prepareDecoder(audioIndex);
        }
        if (videoIndex >= 0) {
            prepareDecoder(videoIndex);
        }

        //创建音视频解码器失败
        if (!audioDecoder && !videoDecoder) {
            av_log(NULL, AV_LOG_WARNING, "failed to create audio and video decoder\n");
            ret = -1;
            break;
        }
        ret = 0;

        // 准备解码器消息通知
        if (playerState->messageQueue) {
            playerState->messageQueue->postMessage(MSG_PREPARE_DECODER);
        }

    } while (false);
    mMutex.unlock();
    return ret;
}

/**
 * 开始解码和同步播放
 * @return
 */
int MediaPlayer::startDecodeAndSync() {
    int ret = 0;
    //视频解码器
    if (videoDecoder) {
        AVCodecParameters *codecpar = videoDecoder->getStream()->codecpar;
        //通知播放视频的宽高
        if (playerState->messageQueue) {
            playerState->messageQueue->postMessage(MSG_VIDEO_SIZE_CHANGED, codecpar->width,
                                                   codecpar->height);
            playerState->messageQueue->postMessage(MSG_SAR_CHANGED,
                                                   codecpar->sample_aspect_ratio.num,
                                                   codecpar->sample_aspect_ratio.den);
        }
    }

    // 准备完成回调
    if (playerState->messageQueue) {
        playerState->messageQueue->postMessage(MSG_PREPARED);
    }

    if (videoDecoder != NULL) {
        // 视频解码器开始解码
        videoDecoder->start();
        if (playerState->messageQueue) {
            playerState->messageQueue->postMessage(MSG_VIDEO_START);
        }
    } else {
        if (playerState->syncType == AV_SYNC_VIDEO) {
            playerState->syncType = AV_SYNC_AUDIO;
        }
    }

    if (audioDecoder != NULL) {
        // 音频解码器开始解码
        audioDecoder->start();
        if (playerState->messageQueue) {
            playerState->messageQueue->postMessage(MSG_AUDIO_START);
        }
    } else {
        if (playerState->syncType == AV_SYNC_AUDIO) {
            playerState->syncType = AV_SYNC_EXTERNAL;
        }
    }

    // 打开音频输出设备
    if (audioDecoder != NULL) {
        AVCodecContext *avctx = audioDecoder->getCodecContext(); //解码上下文
        ret = openAudioDevice(avctx->channel_layout, avctx->channels, avctx->sample_rate);
        if (ret < 0) {
            av_log(NULL, AV_LOG_WARNING, "could not open audio device\n");
            // 如果音频设备打开失败，则调整时钟的同步类型
            if (playerState->syncType == AV_SYNC_AUDIO) {
                if (videoDecoder != NULL) {
                    playerState->syncType = AV_SYNC_VIDEO;
                } else {
                    playerState->syncType = AV_SYNC_EXTERNAL;
                }
            }
        } else {
            // 启动音频输出设备
            audioDevice->start();
        }
    }

    if (videoDecoder) {
        if (playerState->syncType == AV_SYNC_AUDIO) { //默认的
            videoDecoder->setMasterClock(mediaSync->getAudioClock());
        } else if (playerState->syncType == AV_SYNC_VIDEO) {
            videoDecoder->setMasterClock(mediaSync->getVideoClock());
        } else {
            videoDecoder->setMasterClock(mediaSync->getExternalClock());
        }
    }

    /*开始同步*/
    mediaSync->start(videoDecoder, audioDecoder);

    // 等待开始，当mediaplayer调用start或resume方法的时候，pauseRequest就为true
    if (playerState->pauseRequest) {
        // 请求开始
        if (playerState->messageQueue) {
            playerState->messageQueue->postMessage(MSG_REQUEST_START);
        }
        while ((!playerState->abortRequest) && playerState->pauseRequest) {
            av_usleep(10 * 1000);
        }
    }

    if (playerState->messageQueue) {
        playerState->messageQueue->postMessage(MSG_STARTED);
    }

    return ret;
}

/**
 * 读取av数据
 * @return
 */
int MediaPlayer::readAvPackets() {

    // 读数据包流程
    eof = 0;
    int ret = 0;
    AVPacket pkt1, *pkt = &pkt1;
    int64_t stream_start_time;
    int playInRange = 0;
    int64_t pkt_ts;
    int waitToSeek = 0;

    //主要是循环读取数据包压入到队列，以供播放音视频
    for (;;) {

        // 退出播放器
        if (playerState->abortRequest) {
            break;
        }

        // 是否暂停网络流
        if (playerState->pauseRequest != lastPaused) {
            lastPaused = playerState->pauseRequest;
            if (playerState->pauseRequest) { //暂停
                av_read_pause(pFormatCtx);
            } else {
                av_read_play(pFormatCtx);
            }
        }

#if CONFIG_RTSP_DEMUXER || CONFIG_MMSH_PROTOCOL
        if (playerState->pauseRequest &&
            (!strcmp(pFormatCtx->iformat->name, "rtsp") ||
             (pFormatCtx->pb && !strncmp(url, "mmsh:", 5)))) {
            av_usleep(10 * 1000);
            continue;
        }
#endif
        // 定位处理
        if (playerState->seekRequest) {
            int64_t seek_target = playerState->seekPos;
            //seekRel默认为0
            int64_t seek_min =
                    playerState->seekRel > 0 ? seek_target - playerState->seekRel + 2 : INT64_MIN;
            int64_t seek_max =
                    playerState->seekRel < 0 ? seek_target - playerState->seekRel - 2 : INT64_MAX;
            // 定位
            playerState->mMutex.lock();
            //avformat_seek_file定位
            ret = avformat_seek_file(pFormatCtx, -1, seek_min, seek_target, seek_max,
                                     playerState->seekFlags);
            playerState->mMutex.unlock();
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "%s: error while seeking\n", playerState->url);
            } else {
                if (audioDecoder) {
                    audioDecoder->flush();
                }
                if (videoDecoder) {
                    videoDecoder->flush();
                }

                // 更新外部时钟值
                if (playerState->seekFlags & AVSEEK_FLAG_BYTE) {
                    mediaSync->updateExternalClock(NAN);
                } else {
                    mediaSync->updateExternalClock(seek_target / (double) AV_TIME_BASE);
                }
                //更新视频帧的计时器
                mediaSync->refreshVideoTimer();
            }

            attachmentRequest = 1;
            playerState->seekRequest = 0;
            mCondition.signal();
            eof = 0;
            // 定位完成回调通知
            if (playerState->messageQueue) {
                playerState->messageQueue->postMessage(MSG_SEEK_COMPLETE,(int) av_rescale(seek_target, 1000,AV_TIME_BASE), ret);
            }
        }

        // 取得封面数据包
        if (attachmentRequest) {
            if (videoDecoder &&
                (videoDecoder->getStream()->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
                AVPacket copy;
                //该函数用于复制AVPacket中的buf 和 data，如果使用了计数buffer AVBufferRef,则将AVBufferRef中的数据空间计数加一，不复制其他成员
                if ((ret = av_packet_ref(&copy, &videoDecoder->getStream()->attached_pic)) < 0) { //附加pic
                    break;
                }
                videoDecoder->pushPacket(&copy);
            }
            attachmentRequest = 0;
        }

        // 如果队列中存在足够的数据包，则等待消耗
        // 备注：这里要等待一定时长的缓冲队列，要不然会导致OpenSLES播放音频出现卡顿等现象
        //暂停的时候，也会一直执行里面的continue，因为队列满了但没消耗
        if (playerState->infiniteBuffer < 1 &&
            ((audioDecoder ? audioDecoder->getMemorySize() : 0) +
             (videoDecoder ? videoDecoder->getMemorySize() : 0) > MAX_QUEUE_SIZE
             || (!audioDecoder || audioDecoder->hasEnoughPackets()) &&
                (!videoDecoder || videoDecoder->hasEnoughPackets()))) {
            //当播放器执行暂停的时候，也会不断执行这里，因为暂停的时候音视频就会停止消耗数据，所以这里就符合条件执行括号里面的代码了
            // 然后也就暂停了从文件中读取数据
            //LOGE("暂停");
            continue;
        }

        // 读出数据包
        if (!waitToSeek) { //没有等待定位
            //读取音视频的数据包
            ret = av_read_frame(pFormatCtx, pkt); //返回0即为OK，小于0就是出错了或者读到了文件的结尾
        } else {
            ret = -1;
        }
        //LOGE("文件返回值%d", ret);
        //获取读文件的返回值ret，成功ret等于0，否则为负数
        if (ret < 0) { //出错了或者文件读完了

            // 如果没能读出裸数据包，判断是否是结尾
            if ((ret == AVERROR_EOF || avio_feof(pFormatCtx->pb)) && !eof) {
                // LOGE("文件尾")通知
                if (playerState->messageQueue) {
                    playerState->messageQueue->postMessage(MSG_COMPLETED);
                }
                eof = 1;
            }
            // 读取出错，则直接退出，退出for循环
            if (pFormatCtx->pb && pFormatCtx->pb->error) {
                ret = -1;
                break;
            }

            // 如果不处于暂停状态，并且队列中所有数据都没有
            if (!playerState->pauseRequest && (!audioDecoder || audioDecoder->getPacketSize() == 0)
                && (!videoDecoder || (videoDecoder->getPacketSize() == 0 && videoDecoder->getFrameSize() == 0))) {

                if (playerState->loop) { //循环播放
                    seekTo(playerState->startTime != AV_NOPTS_VALUE ? playerState->startTime : 0); //定位到开始位置循环播放
                } else if (playerState->autoExit) { //自动退出
                    ret = AVERROR_EOF;
                    break;
                }
//                else if (eof == 1) { //播放完毕退出
//                    break;
//                }
            }
            // 睡眠10毫秒继续
            av_usleep(10 * 1000);
            continue;//继续下一个循环
        } else {
            eof = 0;
        }

        // 计算pkt的pts是否处于播放范围内
        stream_start_time = pFormatCtx->streams[pkt->stream_index]->start_time;
        pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts; //当前packet的时间戳
        // 是否在播放范围
        playInRange = playerState->duration == AV_NOPTS_VALUE ||
                      (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
                      av_q2d(pFormatCtx->streams[pkt->stream_index]->time_base) -
                      (double) (playerState->startTime != AV_NOPTS_VALUE ? playerState->startTime : 0) / 1000000
                      <= ((double) playerState->duration / 1000000);

        //将音频或者视频数据包压入队列
        if (playInRange && audioDecoder && pkt->stream_index == audioDecoder->getStreamIndex()) {
            audioDecoder->pushPacket(pkt);
        } else if (playInRange && videoDecoder && pkt->stream_index == videoDecoder->getStreamIndex()) {
            videoDecoder->pushPacket(pkt);
        } else {
            av_packet_unref(pkt);
        }
        // 等待定位
        if (!playInRange) {
            waitToSeek = 1;
        }
    } //循环结束

    if (audioDecoder) {
        audioDecoder->stop();
    }
    if (videoDecoder) {
        videoDecoder->stop();
    }
    if (audioDevice) {
        audioDevice->stop();
    }
    if (mediaSync) {
        mediaSync->stop();
    }
    //主要作用是当线程执行完毕，才退出，关键是上面几个stop也执行了
    mExit = true;
    mCondition.signal();
    return ret;
}


int MediaPlayer::prepareDecoder(int streamIndex) {
    AVCodecContext *avctx; //解码上下文
    AVCodec *codec = NULL; //解码器
    AVDictionary *opts = NULL; //参数字典
    AVDictionaryEntry *t = NULL; //字典条目
    int ret = 0;
    const char *forcedCodecName = NULL;

    if (streamIndex < 0 || streamIndex >= pFormatCtx->nb_streams) {
        return -1;
    }

    // 创建解码上下文
    avctx = avcodec_alloc_context3(NULL);
    if (!avctx) {
        return AVERROR(ENOMEM);
    }

    do {
        // 复制解码上下文参数
        ret = avcodec_parameters_to_context(avctx, pFormatCtx->streams[streamIndex]->codecpar);
        if (ret < 0) {
            break;
        }

        // 设置时钟基准
        av_codec_set_pkt_timebase(avctx, pFormatCtx->streams[streamIndex]->time_base);

        // 优先使用指定的解码器
        switch (avctx->codec_type) {
            case AVMEDIA_TYPE_AUDIO: {
                LOGE("音频时间基%d,%d", pFormatCtx->streams[streamIndex]->time_base.den,
                     pFormatCtx->streams[streamIndex]->time_base.num);
                forcedCodecName = playerState->audioCodecName;
                break;
            }
            case AVMEDIA_TYPE_VIDEO: {
                LOGE("视频时间基%d,%d", pFormatCtx->streams[streamIndex]->time_base.den,
                     pFormatCtx->streams[streamIndex]->time_base.num);
                forcedCodecName = playerState->videoCodecName;
                break;
            }
        }
        //通过解码器名字去查找
        if (forcedCodecName) {
            LOGD("forceCodecName = %s", forcedCodecName);
            codec = avcodec_find_decoder_by_name(forcedCodecName);
        }

        // 如果没有找到指定的解码器，则查找默认的解码器
        if (!codec) {
            if (forcedCodecName) {
                av_log(NULL, AV_LOG_WARNING, "No codec could be found with name '%s'\n",
                       forcedCodecName);
            }
            //查找解码器
            codec = avcodec_find_decoder(avctx->codec_id);
        }

        // 判断是否成功得到解码器
        if (!codec) {
            av_log(NULL, AV_LOG_WARNING, "No codec could be found with id %d\n", avctx->codec_id);
            ret = AVERROR(EINVAL);
            break;
        }
        avctx->codec_id = codec->id;

        // 设置一些播放参数
        int stream_lowres = playerState->lowres; //解码器支持的最低分辨率
        if (stream_lowres > av_codec_get_max_lowres(codec)) {
            av_log(avctx, AV_LOG_WARNING,
                   "The maximum value for lowres supported by the decoder is %d\n",
                   av_codec_get_max_lowres(codec));
            stream_lowres = av_codec_get_max_lowres(codec);
        }
        av_codec_set_lowres(avctx, stream_lowres);
#if FF_API_EMU_EDGE
        if (stream_lowres) {
            avctx->flags |= CODEC_FLAG_EMU_EDGE;
        }
#endif
        if (playerState->fast) {
            avctx->flags2 |= AV_CODEC_FLAG2_FAST;
        }

#if FF_API_EMU_EDGE
        if (codec->capabilities & AV_CODEC_CAP_DR1) {
            avctx->flags |= CODEC_FLAG_EMU_EDGE;
        }
#endif

        //过滤解码参数
        opts = filterCodecOptions(playerState->codec_opts, avctx->codec_id, pFormatCtx,
                                  pFormatCtx->streams[streamIndex], codec);
        if (!av_dict_get(opts, "threads", NULL, 0)) {
            av_dict_set(&opts, "threads", "auto", 0);
        }

        if (stream_lowres) {
            av_dict_set_int(&opts, "lowres", stream_lowres, 0);
        }

        if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            av_dict_set(&opts, "refcounted_frames", "1", 0);
        }

        // 打开解码器
        if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
            break;
        }
        if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
            av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
            ret = AVERROR_OPTION_NOT_FOUND;
            break;
        }

        // 根据解码器类型创建解码器
        pFormatCtx->streams[streamIndex]->discard = AVDISCARD_DEFAULT; //抛弃无用的数据比如像0大小的packet
        //根据解码器类型，创建对应的解码器
        switch (avctx->codec_type) {
            case AVMEDIA_TYPE_AUDIO: {
                audioDecoder = new AudioDecoder(avctx, pFormatCtx->streams[streamIndex], streamIndex, playerState);
                break;
            }

            case AVMEDIA_TYPE_VIDEO: {
                videoDecoder = new VideoDecoder(pFormatCtx, avctx, pFormatCtx->streams[streamIndex], streamIndex,
                                                playerState);
                attachmentRequest = 1;
                break;
            }

            default: {
                break;
            }
        }
    } while (false);

    // 准备失败，则需要释放创建的解码上下文
    if (ret < 0) {
        if (playerState->messageQueue) {
            const char errorMsg[] = "failed to open stream!";
            playerState->messageQueue->postMessage(MSG_ERROR, 0, 0, (void *) errorMsg, sizeof(errorMsg) / errorMsg[0]);
        }

        avcodec_free_context(&avctx);
    }
    // 释放参数
    av_dict_free(&opts);

    return ret;
}

/**
 * 音频取pcm数据的回调方法
 * @param opaque
 * @param stream
 * @param len
 */
void audioPCMQueueCallback(void *opaque, uint8_t *stream, int len) {
    MediaPlayer *mediaPlayer = (MediaPlayer *) opaque;
    mediaPlayer->pcmQueueCallback(stream, len);
}

/**
 * 打开音频设备
 * @param wanted_channel_layout 声道分布
 * @param wanted_nb_channels 声道数量
 * @param wanted_sample_rate 采样率
 * @return
 */
int MediaPlayer::openAudioDevice(int64_t wanted_channel_layout, int wanted_nb_channels,
                                 int wanted_sample_rate) {
    AudioDeviceSpec wanted_spec, spec;
    const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
    const int next_sample_rates[] = {44100, 48000};
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1; //最后一个index开始
    //av_get_channel_layout_nb_channels为根据通道layout返回通道个数
    if (wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)
        || !wanted_channel_layout) {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels); //获取默认的通道layout
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    //期望的声道数
    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    wanted_spec.channels = wanted_nb_channels;
    wanted_spec.freq = wanted_sample_rate;

    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
        return -1;
    }

    //找到在next_sample_rates数组中比wanted_spec.freq小的那个采样率的index
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq) {
        next_sample_rate_idx--;
    }

    wanted_spec.format = AV_SAMPLE_FMT_S16;
    wanted_spec.samples = FFMAX(AUDIO_MIN_BUFFER_SIZE,
                                2 << av_log2(wanted_spec.freq / AUDIO_MAX_CALLBACKS_PER_SEC));

    //设置音频取数据的回调方法
    wanted_spec.callback = audioPCMQueueCallback;
    wanted_spec.userdata = this;

    // 打开音频设备
    while (audioDevice->open(&wanted_spec, &spec) < 0) {
        av_log(NULL, AV_LOG_WARNING, "Failed to open audio device: (%d channels, %d Hz)!\n",
               wanted_spec.channels,
               wanted_spec.freq);
        //打开音频设备失败的话，重新设置wanted_spec相关参数
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels) {
            //重新设置采样率
            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
            wanted_spec.channels = wanted_nb_channels;
            if (!wanted_spec.freq) {
                av_log(NULL, AV_LOG_ERROR, "No more combinations to try, audio open failed\n");
                return -1;
            }
        }
        wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
    }

    //不支持16bit的采样精度
    if (spec.format != AV_SAMPLE_FMT_S16) {
        av_log(NULL, AV_LOG_ERROR, "audio format %d is not supported!\n", spec.format);
        return -1;
    }

    //不支持所期望的采样率
    if (spec.channels != wanted_spec.channels) {
        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
        if (!wanted_channel_layout) {
            av_log(NULL, AV_LOG_ERROR, "channel count %d is not supported!\n", spec.channels);
            return -1;
        }
    }

    // 初始化音频重采样器
    if (!audioResampler) {
        audioResampler = new AudioResampler(playerState, audioDecoder, mediaSync);
    }
    // 设置需要重采样的参数
    audioResampler->setResampleParams(&spec, wanted_channel_layout);

    return spec.size;
}

/**
 * 取PCM数据
 * @param stream
 * @param len
 */
void MediaPlayer::pcmQueueCallback(uint8_t *stream, int len) {
    if (!audioResampler) {
        memset(stream, 0, sizeof(len));
        return;
    }
    audioResampler->pcmQueueCallback(stream, len);
    if (playerState->messageQueue && playerState->syncType != AV_SYNC_VIDEO) {
        playerState->messageQueue->postMessage(MSG_CURRENT_POSITON, getCurrentPosition(), playerState->videoDuration);
    }
}



























