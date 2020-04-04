
#include "sync/header/MediaSync.h"

MediaSync::MediaSync(PlayerState *playerState) {
    this->playerState = playerState;
    audioDecoder = NULL;
    videoDecoder = NULL;
    audioClock = new MediaClock();
    videoClock = new MediaClock();
    extClock = new MediaClock();

    mExit = true;
    abortRequest = true;
    syncThread = NULL;

    forceRefresh = 0;
    maxFrameDuration = 10.0;
    frameTimerRefresh = 1;
    frameTimer = 0;


    videoDevice = NULL;
    swsContext = NULL;
    mBuffer = NULL;
    pFrameARGB = NULL;
}

MediaSync::~MediaSync() {
}

void MediaSync::reset() {
    stop();
    playerState = NULL;
    videoDecoder = NULL;
    audioDecoder = NULL;
    videoDevice = NULL;

    if (pFrameARGB) {
        av_frame_free(&pFrameARGB);
        av_free(pFrameARGB);
        pFrameARGB = NULL;
    }
    if (mBuffer) {
        av_freep(&mBuffer);
        mBuffer = NULL;
    }
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = NULL;
    }
}

void MediaSync::start(VideoDecoder *videoDecoder, AudioDecoder *audioDecoder) {
    mMutex.lock();
    this->videoDecoder = videoDecoder;
    this->audioDecoder = audioDecoder;
    abortRequest = false;
    mExit = false;
    mCondition.signal();
    mMutex.unlock();
    if (videoDecoder && !syncThread) {
        syncThread = new Thread(this);
        syncThread->start();
    }
}

void MediaSync::stop() {
    mMutex.lock();
    abortRequest = true;
    mCondition.signal();
    mMutex.unlock();

    mMutex.lock();
    while (!mExit) {
        mCondition.wait(mMutex);
    }
    mMutex.unlock();
    if (syncThread) {
        syncThread->join();
        delete syncThread;
        syncThread = NULL;
    }
}

void MediaSync::setVideoDevice(VideoDevice *device) {
    Mutex::Autolock lock(mMutex);
    this->videoDevice = device;
}

void MediaSync::setMaxDuration(double maxDuration) {
    this->maxFrameDuration = maxDuration;
}

/**
 * 定位请求的时候，会强制刷新视频帧时间
 */
void MediaSync::refreshVideoTimer() {
    mMutex.lock();
    this->frameTimerRefresh = 1;
    mCondition.signal();
    mMutex.unlock();
}

/**
 * 更新音频时钟
 * @param pts 当前播放的时间点，单位是秒
 * @param time 音频回调时间，单位也是秒
 */
void MediaSync::updateAudioClock(double pts, double time) {
    audioClock->setClock(pts, time);
    extClock->syncToSlave(audioClock);
}

double MediaSync::getAudioDiffClock() {
    return audioClock->getClock() - getMasterClock();
}

void MediaSync::updateExternalClock(double pts) {
    extClock->setClock(pts);
}

/**
 * 默认以音频时钟为主时钟
 * @return
 */
double MediaSync::getMasterClock() {
    double val = 0;
    switch (playerState->syncType) {
        case AV_SYNC_VIDEO: {
            val = videoClock->getClock();
            break;
        }
        case AV_SYNC_AUDIO: {
            val = audioClock->getClock();
            break;
        }
        case AV_SYNC_EXTERNAL: {
            val = extClock->getClock();
            break;
        }
    }
    return val;
}

MediaClock *MediaSync::getAudioClock() {
    return audioClock;
}

MediaClock *MediaSync::getVideoClock() {
    return videoClock;
}

MediaClock *MediaSync::getExternalClock() {
    return extClock;
}

void MediaSync::run() {
    double remaining_time = 0.0;

    while (true) {

        if (abortRequest || playerState->abortRequest) { //停止
            if (videoDevice != NULL) {
                videoDevice->terminate();
            }
            break;
        }

        if (remaining_time > 0.0) {
            av_usleep((int64_t) (remaining_time * 1000000.0));
        }
        remaining_time = REFRESH_RATE; //刷新率
        //暂停的时候不会执行
        if (!playerState->pauseRequest || forceRefresh) {
            refreshVideo(&remaining_time);
        }
    }

    mExit = true;
    mCondition.signal();
}

void MediaSync::refreshVideo(double *remaining_time) {
    double time;

    // 检查外部时钟
    if (!playerState->pauseRequest && playerState->realTime &&
        playerState->syncType == AV_SYNC_EXTERNAL) {
        checkExternalClockSpeed();
    }

    for (;;) {

        if (playerState->abortRequest || !videoDecoder) {
            break;
        }

        // 判断是否存在帧队列是否存在数据
        if (videoDecoder->getFrameSize() > 0) {
            double lastDuration, duration, delay;
            Frame *currentFrame, *lastFrame;
            // 上一帧，即已经开始播放的那一帧视频
            lastFrame = videoDecoder->getFrameQueue()->lastFrame();
            // 当前帧，即将要播放的视频帧
            currentFrame = videoDecoder->getFrameQueue()->currentFrame();
            // 判断是否需要强制更新帧的时间
            if (frameTimerRefresh) {
                frameTimer = av_gettime_relative() / 1000000.0;
                frameTimerRefresh = 0;
            }

            // 如果处于暂停状态，则跳出，则会一直播放正在显示的那一帧
            if (playerState->abortRequest || playerState->pauseRequest) {
                break;
            }

            // 计算上一帧显示时长，即当前正在播放的帧的正常播放时长
            lastDuration = calculateDuration(lastFrame, currentFrame);
            // 计算延时，即到播放下一帧需要延时多长时间
            delay = calculateDelay(lastDuration);
            // 处理超过延时阈值的情况
            if (fabs(delay) > AV_SYNC_THRESHOLD_MAX) {
                if (delay > 0) {
                    delay = AV_SYNC_THRESHOLD_MAX;
                } else {
                    delay = 0;
                }
            }
            // 获取当前时间
            time = av_gettime_relative() / 1000000.0;
            if (isnan(frameTimer) || time < frameTimer) {
                frameTimer = time;
            }
            // 如果当前时间小于当前帧的播放时刻，表示还没到播放时间
            if (time < frameTimer + delay) {
                //取小的那个作为延时时长，比如第一个delay应该是0.015秒，而*remaining_time默认每次延时过后都会重新赋值为0.01
                // 所以第一次延时就是0.01秒
                //而第二次，frameTimer + delay - time应该变为0.005秒，所以这次就延时0.005秒，这样就完成了延时任务，然后就可以播放视频帧了
                *remaining_time = FFMIN(frameTimer + delay - time, *remaining_time);
                break;
            }
            //当前帧播放时刻到了，需要更新帧计时器，此时的帧计时器其实代表当前帧的播放时刻
            frameTimer += delay;
            // 帧计时器落后当前时间超过了阈值，则用当前的时间作为帧计时器时间，一般播放视频暂停以后，因为time一直在增加，而frameTimer
            //切一直没有增加，所以time - frameTimer会大于最大阈值，此时应该将frameTimer更新为当前时间
            if (delay > 0 && time - frameTimer > AV_SYNC_THRESHOLD_MAX) {
                //一般暂停后重新播放会执行这里
                frameTimer = time;
            }

            // 更新视频时钟，代表当前视频的播放时刻
            mMutex.lock();
            if (!isnan(currentFrame->pts)) {
                //设置视频时钟
                videoClock->setClock(currentFrame->pts);
                extClock->syncToSlave(videoClock);
            }
            mMutex.unlock();

            // 如果队列中还剩余超过一帧的数据时，需要拿到下一帧，然后计算间隔，并判断是否需要进行舍帧操作
            if (videoDecoder->getFrameSize() > 1) {
                Frame *nextFrame = videoDecoder->getFrameQueue()->nextFrame();
                duration = calculateDuration(currentFrame, nextFrame);
                // 如果不处于同步到视频状态，并且处于跳帧状态，则跳过当前帧
                //当前帧vp未能及时播放，即下一帧播放时刻(is->frame_timer+duration)小于当前系统时刻(time)，那么可能要弃帧了
                if ((time > frameTimer + duration) && (playerState->frameDrop > 0 || (playerState->frameDrop &&
                                                                                      playerState->syncType !=
                                                                                      AV_SYNC_VIDEO))) {
                    //舍弃上一帧，不播放当前帧，继续循环
                    videoDecoder->getFrameQueue()->popFrame();
                    continue;
                }
            }

            // 取出并舍弃一帧，即上一帧lastFrame，此时当前帧就变成了上一帧
            videoDecoder->getFrameQueue()->popFrame();
            //当还需要延时的时候，是执行不到这里的，所以延时阶段forceRefresh为0，所以就不会调用renderVideo方法，但是当延时到期以后，就会执行到这里，
            //代表需要进行视频渲染了，然后就会调用renderVideo方法，调用之后forceRefresh又为0
            forceRefresh = 1;
        }

        break;
    }

    // 回调当前时长
    if (playerState->messageQueue && playerState->syncType == AV_SYNC_VIDEO) {
        // 起始延时
        int64_t start_time = videoDecoder->getFormatContext()->start_time;
        int64_t start_diff = 0;
        if (start_time > 0 && start_time != AV_NOPTS_VALUE) {
            start_diff = av_rescale(start_time, 1000, AV_TIME_BASE);
        }
        // 计算主时钟的时间
        int64_t pos = 0;
        double clock = getMasterClock();
        if (isnan(clock)) {
            pos = playerState->seekPos;
        } else {
            pos = (int64_t) (clock * 1000);
        }
        if (pos < 0 || pos < start_diff) {
            pos = 0;
        }
        pos = (long) (pos - start_diff);
        if (playerState->videoDuration < 0) {
            pos = 0;
        }
        playerState->messageQueue->postMessage(MSG_CURRENT_POSITON, pos, playerState->videoDuration);
    }

    // 显示画面
    if (!playerState->displayDisable && forceRefresh && videoDecoder
        && videoDecoder->getFrameQueue()->getShowIndex()) {
        renderVideo(); //渲染
    }
    forceRefresh = 0;
}

void MediaSync::checkExternalClockSpeed() {
    if (videoDecoder && videoDecoder->getPacketSize() <= EXTERNAL_CLOCK_MIN_FRAMES ||
        audioDecoder && audioDecoder->getPacketSize() <= EXTERNAL_CLOCK_MIN_FRAMES) {
        extClock->setSpeed(FFMAX(EXTERNAL_CLOCK_SPEED_MIN, extClock->getSpeed() - EXTERNAL_CLOCK_SPEED_STEP));
    } else if ((!videoDecoder || videoDecoder->getPacketSize() > EXTERNAL_CLOCK_MAX_FRAMES) &&
               (!audioDecoder || audioDecoder->getPacketSize() > EXTERNAL_CLOCK_MAX_FRAMES)) {
        extClock->setSpeed(FFMIN(EXTERNAL_CLOCK_SPEED_MAX, extClock->getSpeed() + EXTERNAL_CLOCK_SPEED_STEP));
    } else {
        double speed = extClock->getSpeed();
        if (speed != 1.0) {
            extClock->setSpeed(speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
        }
    }
}

/**
 *
 * @param delay 为上一帧视频的播放时长
 * @return
 */
double MediaSync::calculateDelay(double delay) {
    double sync_threshold, diff = 0;
    // 如果不是同步到视频流，则需要计算延时时间
    if (playerState->syncType != AV_SYNC_VIDEO) {
        // 计算差值，videoClock->getClock()是当前播放帧的时间戳，这里计算当前播放视频帧的时间戳和音频时间戳的差值
        //diff代表
        diff = videoClock->getClock() - getMasterClock(); //这里是同步到音频时钟，所以主时钟就是音频时钟
        // 计算阈值
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (!isnan(diff) && fabs(diff) < maxFrameDuration) {
            if (diff <= -sync_threshold) { //视频慢了，需要加快视频播放
                delay = FFMAX(0, delay + diff);
            } else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD) { //视频快了而且上一帧视频的播放时长超过了阈值
                delay = delay + diff;
            } else if (diff >= sync_threshold) { //视频快了
                delay = 2 * delay;
            }
        }
    }

    av_log(NULL, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n", delay, -diff);

    return delay;
}

double MediaSync::calculateDuration(Frame *vp, Frame *nextvp) {
    double duration = nextvp->pts - vp->pts;
    if (isnan(duration) || duration <= 0 || duration > maxFrameDuration) {
        return vp->duration;
    } else {
        return duration;
    }
}

void MediaSync::renderVideo() {
    mMutex.lock();
    if (!videoDecoder || !videoDevice) {
        mMutex.unlock();
        return;
    }
    //取出帧进行播放
    Frame *vp = videoDecoder->getFrameQueue()->lastFrame();
    int ret = 0;
    if (!vp->uploaded) {
        // 根据图像格式更新纹理数据
        switch (vp->frame->format) {
            // YUV420P 和 YUVJ420P 除了色彩空间不一样之外，其他的没什么区别
            // YUV420P表示的范围是 16 ~ 235，而YUVJ420P表示的范围是0 ~ 255
            // 这里做了兼容处理，后续可以优化，shader已经过验证
            case AV_PIX_FMT_YUVJ420P:
            case AV_PIX_FMT_YUV420P: {
                //LOGE("yuv数据");
                // 初始化纹理
                videoDevice->onInitTexture(vp->frame->width, vp->frame->height, FMT_YUV420P, BLEND_NONE);

                if (vp->frame->linesize[0] < 0 || vp->frame->linesize[1] < 0 || vp->frame->linesize[2] < 0) {
                    // LOGE("linesize为负数")
                    av_log(NULL, AV_LOG_ERROR, "Negative linesize is not supported for YUV.\n");
                    return;
                }
                //上载纹理数据，比如YUV420中，data[0]专门存Y，data[1]专门存U，data[2]专门存V
                ret = videoDevice->onUpdateYUV(vp->frame->data[0], vp->frame->linesize[0],
                                               vp->frame->data[1], vp->frame->linesize[1],
                                               vp->frame->data[2], vp->frame->linesize[2]);
                if (ret < 0) {
                    return;
                }
                break;
            }

                // 直接渲染BGRA，对应的是shader->argb格式
            case AV_PIX_FMT_BGRA: {
                //LOGE("rgb数据");
                videoDevice->onInitTexture(vp->frame->width, vp->frame->height,
                                           FMT_ARGB, BLEND_NONE);
                ret = videoDevice->onUpdateARGB(vp->frame->data[0], vp->frame->linesize[0]);
                if (ret < 0) {
                    return;
                }
                break;
            }

                // 其他格式转码成BGRA格式再做渲染
            default: {
                //LOGE("其他格式%d",vp->frame->format);

                //首先通过传入的swsContext上下文根据参数去缓存空间里面找，如果有对应的缓存则返回，没有则开辟一个新的上下文空间
                //参数说明：
                //    第一参数可以传NULL，默认会开辟一块新的空间。
                //    srcW,srcH, srcFormat， 原始数据的宽高和原始像素格式(YUV420)，
                //    dstW,dstH,dstFormat;   目标宽，目标高，目标的像素格式(这里的宽高可能是手机屏幕分辨率，RGBA8888)，这里不仅仅包含了尺寸的转换和像素格式的转换
                //    flag 提供了一系列的算法，快速线性，差值，矩阵，不同的算法性能也不同，快速线性算法性能相对较高。只针对尺寸的变换。对像素格式转换无此问题
                //    #define SWS_FAST_BILINEAR 1
                //    #define SWS_BILINEAR 2
                //    #define SWS_BICUBIC 4
                //    #define SWS_X      8
                //    #define SWS_POINT 0x10
                //    #define SWS_AREA 0x20
                //    #define SWS_BICUBLIN 0x40
                //后面还有两个参数是做过滤器用的，一般用不到，传NULL，最后一个参数是跟flag算法相关，也可以传NULL。
                swsContext = sws_getCachedContext(swsContext,
                                                  vp->frame->width, vp->frame->height,
                                                  (AVPixelFormat) vp->frame->format,
                                                  vp->frame->width, vp->frame->height,
                                                  AV_PIX_FMT_BGRA, SWS_BICUBIC, NULL, NULL, NULL);
                if (!mBuffer) {
                    //函数的作用是通过指定像素格式、图像宽、图像高来计算所需的内存大小
                    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGRA, vp->frame->width, vp->frame->height, 1);
                    mBuffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
                    pFrameARGB = av_frame_alloc();
                    //主要给pFrameARGB的保存数据的地址指针赋值，按照指定的参数指向mBuffer中对应的位置，还有linesize，比如YUV格式的数据，分别
                    //有保存YUV三个不同数据的指针会得到空间的分配
                    av_image_fill_arrays(pFrameARGB->data, pFrameARGB->linesize, mBuffer, AV_PIX_FMT_BGRA,
                                         vp->frame->width, vp->frame->height, 1);
                }
                if (swsContext != NULL) {

                    //1.参数 SwsContext *c， 转换格式的上下文。也就是 sws_getContext 函数返回的结果。
                    //2.参数 const uint8_t *const srcSlice[], 输入图像的每个颜色通道的数据指针。其实就是解码后的AVFrame中的data[]数组。因为不同像素的存储格式不同，所以srcSlice[]维数
                    //也有可能不同。
                    //以YUV420P为例，它是planar格式，它的内存中的排布如下：
                    //YYYYYYYY UUUU VVVV
                    //使用FFmpeg解码后存储在AVFrame的data[]数组中时：
                    //data[0]——-Y分量, Y1, Y2, Y3, Y4, Y5, Y6, Y7, Y8……
                    //data[1]——-U分量, U1, U2, U3, U4……
                    //data[2]——-V分量, V1, V2, V3, V4……
                    //linesize[]数组中保存的是对应通道的数据宽度 ，
                    //linesize[0]——-Y分量的宽度
                    //linesize[1]——-U分量的宽度
                    //linesize[2]——-V分量的宽度
                    //
                    //而RGB24，它是packed格式，它在data[]数组中则只有一维，它在存储方式如下：
                    //data[0]: R1, G1, B1, R2, G2, B2, R3, G3, B3, R4, G4, B4……
                    //这里要特别注意，linesize[0]的值并不一定等于图片的宽度，有时候为了对齐各解码器的CPU，实际尺寸会大于图片的宽度，这点在我们编程时（比如OpengGL硬件转换/渲染）
                    // 要特别注意，否则解码出来的图像会异常。
                    //
                    //3.参数const int srcStride[]，输入图像的每个颜色通道的跨度。.也就是每个通道的行字节数，对应的是解码后的AVFrame中的linesize[]数组。
                    // 根据它可以确立下一行的起始位置，不过stride和width不一定相同，这是因为：
                    //a.由于数据帧存储的对齐，有可能会向每行后面增加一些填充字节这样 stride = width + N；
                    //b.packet色彩空间下，每个像素几个通道数据混合在一起，例如RGB24，每个像素3字节连续存放，因此下一行的位置需要跳过3*width字节。
                    //
                    //4.参数int srcSliceY, int srcSliceH,定义在输入图像上处理区域，srcSliceY是起始位置，srcSliceH是处理多少行。如果srcSliceY=0，srcSliceH=height，
                    // 表示一次性处理完整个图像。这种设置是为了多线程并行，例如可以创建两个线程，第一个线程处理 [0, h/2-1]行，第二个线程处理 [h/2, h-1]行。并行处理加快速度。
                    //5.参数uint8_t *const dst[], const int dstStride[]定义输出图像信息（输出的每个颜色通道数据指针，每个颜色通道行字节数）
                    sws_scale(swsContext, (uint8_t const *const *) vp->frame->data,
                              vp->frame->linesize, 0, vp->frame->height,
                              pFrameARGB->data, pFrameARGB->linesize);
                }

                videoDevice->onInitTexture(vp->frame->width, vp->frame->height, FMT_ARGB, BLEND_NONE,
                                           videoDecoder->getRotate());
                ret = videoDevice->onUpdateARGB(pFrameARGB->data[0], pFrameARGB->linesize[0]);
                if (ret < 0) {
                    return;
                }
                break;
            }
        }
        vp->uploaded = 1;
    }
    // 请求渲染视频
    if (videoDevice != NULL) {
        videoDevice->onRequestRender(vp->frame->linesize[0] < 0);
    }
    mMutex.unlock();
}

