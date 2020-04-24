
#include "EMediaPlayer.h"
#include <AVMessageQueue.h>

EMediaPlayer::EMediaPlayer() {
    msgThread = nullptr;
    abortRequest = true;
    videoDevice = nullptr;
    mediaPlayer = nullptr;
    mListener = nullptr;
    mPrepareSync = false;
    mPrepareStatus = NO_ERROR;
    mAudioSessionId = 0;
    mSeeking = false;
    mSeekingPosition = 0;

}

EMediaPlayer::~EMediaPlayer() {

}

// 对应于java层的mediaplayer创建对象的时候调用，即在构造函数中被调用
void EMediaPlayer::init() {
    mMutex.lock();
    abortRequest = false;
    mCondition.signal();
    mMutex.unlock();

    mMutex.lock();
    // 视频播放设备
    if (videoDevice == nullptr) {
        videoDevice = new GLESDevice();
        videoDevice->setFilterState(&filterState);
    }

    // 消息分发的线程
    if (msgThread == nullptr) {
        // this就是runnable
        msgThread = new Thread(this);
        msgThread->start();
    }
    mMutex.unlock();
}

// java层对应mediaplayer的release方法的时候被调用
void EMediaPlayer::disconnect() {

    mMutex.lock();
    abortRequest = true;
    mCondition.signal();
    mMutex.unlock();
    // 重置
    reset();
    LOGD("调用disconnect");
    if (msgThread != nullptr) {
        LOGD("开始删除消息通知线程");
        msgThread->join();
        delete msgThread;
        LOGD("删除消息通知线程");
        msgThread = nullptr;
    }

    if (videoDevice != nullptr) {
        delete videoDevice;
        videoDevice = nullptr;
    }
    if (mListener != nullptr) {
        delete mListener;
        mListener = nullptr;
    }

}

status_t EMediaPlayer::setDataSource(const char *url, int64_t offset, const char *headers) {
    if (url == nullptr) {
        return BAD_VALUE;
    }

    if (mediaPlayer == nullptr) {
        mediaPlayer = new MediaPlayer();
    }
    mediaPlayer->setDataSource(url, offset, headers);
    mediaPlayer->setVideoDevice(videoDevice);
    return NO_ERROR;
}

status_t EMediaPlayer::setMetadataFilter(char **allow, char **block) {
    // do nothing
    return NO_ERROR;
}

status_t EMediaPlayer::getMetadata(bool update_only, bool apply_filter, AVDictionary **metadata) {
    if (mediaPlayer != nullptr) {
        return mediaPlayer->getMetadata(metadata);
    }
    return NO_ERROR;
}

void EMediaPlayer::surfaceChanged(int width, int height) {
    if (videoDevice != nullptr) {
        videoDevice->surfaceChanged(width, height);
    }
}

void EMediaPlayer::setFilterType(GLint filterType) {
    filterState.setFilterType(filterType);
}

void EMediaPlayer::setFilterColor(GLfloat *filterColor) {
    // 设置纹理渲染的滤镜颜色
    filterState.setFilterColor(filterColor);
}

void EMediaPlayer::setTwoScreen(bool isTwoScreen) {
    if (videoDevice!= nullptr)
        videoDevice->setTwoScreen(isTwoScreen);
}

status_t EMediaPlayer::setVideoSurface(ANativeWindow *native_window) {
    if (mediaPlayer == nullptr) {
        return NO_INIT;
    }
    if (native_window != nullptr) {
        videoDevice->surfaceCreated(native_window);
        return NO_ERROR;
    }
    return NO_ERROR;
}


status_t EMediaPlayer::setListener(MediaPlayerListener *listener) {
    if (mListener != nullptr) {
        delete mListener;
        mListener = nullptr;
    }
    mListener = listener;
    return NO_ERROR;
}

status_t EMediaPlayer::prepare() {
    if (mediaPlayer == nullptr) {
        return NO_INIT;
    }
    if (mPrepareSync) {
        return -EALREADY;
    }
    mPrepareSync = true;
    status_t ret = mediaPlayer->prepare();
    if (ret != NO_ERROR) {
        return ret;
    }
    if (mPrepareSync) {
        mPrepareSync = false;
    }
    return mPrepareStatus;
}

status_t EMediaPlayer::prepareAsync() {
    if (mediaPlayer != nullptr) {
        return mediaPlayer->prepareAsync();
    }
    return INVALID_OPERATION;
}

void EMediaPlayer::start() {
    if (mediaPlayer != nullptr) {
        mediaPlayer->start();
    }
}

void EMediaPlayer::stop() {
    if (mediaPlayer) {
        mediaPlayer->stop();
    }
}

void EMediaPlayer::pause() {
    if (mediaPlayer) {
        mediaPlayer->pause();
    }
}

void EMediaPlayer::resume() {
    if (mediaPlayer) {
        mediaPlayer->resume();
    }
}

bool EMediaPlayer::isPlaying() {
    if (mediaPlayer) {
        return (mediaPlayer->isPlaying() != 0);
    }
    return false;
}

int EMediaPlayer::getRotate() {
    if (mediaPlayer != nullptr) {
        return mediaPlayer->getRotate();
    }
    return 0;
}

int EMediaPlayer::getVideoWidth() {
    if (mediaPlayer != nullptr) {
        return mediaPlayer->getVideoWidth();
    }
    return 0;
}

int EMediaPlayer::getVideoHeight() {
    if (mediaPlayer != nullptr) {
        return mediaPlayer->getVideoHeight();
    }
    return 0;
}

status_t EMediaPlayer::seekTo(float msec) {
    if (mediaPlayer != nullptr) {
        // if in seeking state, put seek message in queue, to process after preview seeking.
        if (mSeeking) {
            mediaPlayer->getMessageQueue()->postMessage(MSG_REQUEST_SEEK, msec);
        } else {
            mediaPlayer->seekTo(msec);
            mSeekingPosition = (long) msec;
            mSeeking = true;
        }
    }
    return NO_ERROR;
}

long EMediaPlayer::getCurrentPosition() {
    if (mediaPlayer != nullptr) {
        if (mSeeking) {
            return mSeekingPosition;
        }
        return mediaPlayer->getCurrentPosition();
    }
    return 0;
}

long EMediaPlayer::getDuration() {
    if (mediaPlayer != nullptr) {
        return mediaPlayer->getDuration();
    }
    return -1;
}

status_t EMediaPlayer::reset() {
    mPrepareSync = false;
    if (mediaPlayer != nullptr) {
        mediaPlayer->reset();
        delete mediaPlayer;
        mediaPlayer = nullptr;
    }
    return NO_ERROR;
}

status_t EMediaPlayer::setAudioStreamType(int type) {
    // TODO setAudioStreamType
    return NO_ERROR;
}

status_t EMediaPlayer::setLooping(bool looping) {
    if (mediaPlayer != nullptr) {
        mediaPlayer->setLooping(looping);
    }
    return NO_ERROR;
}

bool EMediaPlayer::isLooping() {
    if (mediaPlayer != nullptr) {
        return (mediaPlayer->isLooping() != 0);
    }
    return false;
}

status_t EMediaPlayer::setVolume(float leftVolume, float rightVolume) {
    if (mediaPlayer != nullptr) {
        mediaPlayer->setVolume(leftVolume, rightVolume);
    }
    return NO_ERROR;
}

void EMediaPlayer::setMute(bool mute) {
    if (mediaPlayer != nullptr) {
        mediaPlayer->setMute(mute);
    }
}

void EMediaPlayer::setRate(float speed) {
    if (mediaPlayer != nullptr) {
        mediaPlayer->setRate(speed);
    }
}

void EMediaPlayer::setPitch(float pitch) {
    if (mediaPlayer != nullptr) {
        mediaPlayer->setPitch(pitch);
    }
}

status_t EMediaPlayer::setAudioSessionId(int sessionId) {
    if (sessionId < 0) {
        return BAD_VALUE;
    }
    mAudioSessionId = sessionId;
    return NO_ERROR;
}

int EMediaPlayer::getAudioSessionId() {
    return mAudioSessionId;
}

void EMediaPlayer::setOption(int category, const char *type, const char *option) {
    if (mediaPlayer != nullptr) {
        mediaPlayer->getPlayerState()->setOption(category, type, option);
    }
}

void EMediaPlayer::setOption(int category, const char *type, int64_t option) {
    if (mediaPlayer != nullptr) {
        mediaPlayer->getPlayerState()->setOptionLong(category, type, option);
    }
}

void EMediaPlayer::notify(int msg, int ext1, int ext2, void *obj, int len) {
    if (mediaPlayer != nullptr) {
        mediaPlayer->getMessageQueue()->postMessage(msg, ext1, ext2, obj, len);
    }
}

void EMediaPlayer::postEvent(int what, int arg1, int arg2, void *obj) {
    string str;

    if (mListener != nullptr) {
        mListener->notify(what, arg1, arg2, obj);
    }
}

/**
 * 这个线程用来循环获取消息队列中的消息，然后通知出去，可以在java层接收到消息的通知，主要是播放状态之类的消息
 */
void EMediaPlayer::run() {

    int retval;
    while (true) {

        if (abortRequest) {
            break;
        }

        // 如果此时播放器还没准备好，则睡眠10毫秒，等待播放器初始化
        if (!mediaPlayer || !mediaPlayer->getMessageQueue()) {
            av_usleep(10 * 1000);
            continue;
        }

        AVMessage msg;
        retval = mediaPlayer->getMessageQueue()->getMessage(&msg);
        if (retval < 0) {
            LOGE("getMessage error");
            break;
        }
        assert(retval > 0);

        switch (msg.what) {
            case MSG_FLUSH: {
                LOGD("EMediaPlayer is flushing.\n");
                postEvent(MEDIA_NOP, 0, 0);
                break;
            }

            case MSG_ERROR: {
                LOGE("EMediaPlayer occurs error: %d,%s\n", msg.arg1,msg.obj);
                if (mPrepareSync) {
                    mPrepareSync = false;
                    mPrepareStatus = msg.arg1;
                }
                postEvent(MEDIA_ERROR, msg.arg1, 0);
                break;
            }

            case MSG_PREPARED: {
                LOGD("EMediaPlayer is prepared.\n");
                if (mPrepareSync) {
                    mPrepareSync = false;
                    mPrepareStatus = NO_ERROR;
                }
                postEvent(MEDIA_PREPARED, 0, 0);
                break;
            }

            case MSG_STARTED: {
                LOGD("EMediaPlayer is started!");
                postEvent(MEDIA_STARTED, 0, 0);
                break;
            }

            case MSG_COMPLETED: {
                LOGD("EMediaPlayer is playback completed.\n");
                postEvent(MEDIA_PLAYBACK_COMPLETE, 0, 0);
                break;
            }

            case MSG_VIDEO_SIZE_CHANGED: {
                LOGD("EMediaPlayer is video size changing: %d, %d\n", msg.arg1, msg.arg2);
                postEvent(MEDIA_SET_VIDEO_SIZE, msg.arg1, msg.arg2);
                break;
            }

            case MSG_SAR_CHANGED: {
                LOGD("EMediaPlayer is sar changing: %d, %d\n", msg.arg1, msg.arg2);
                postEvent(MEDIA_SET_VIDEO_SAR, msg.arg1, msg.arg2);
                break;
            }

            case MSG_VIDEO_RENDERING_START: {
                LOGD("EMediaPlayer is video playing.\n");
                break;
            }

            case MSG_AUDIO_RENDERING_START: {
                LOGD("EMediaPlayer is audio playing.\n");
                break;
            }

            case MSG_VIDEO_ROTATION_CHANGED: {
                LOGD("EMediaPlayer's video rotation is changing: %d\n", msg.arg1);
                break;
            }

            case MSG_AUDIO_START: {
                LOGD("EMediaPlayer starts audio decoder.\n");
                break;
            }

            case MSG_VIDEO_START: {
                LOGD("EMediaPlayer starts video decoder.\n");
                break;
            }

            case MSG_OPEN_INPUT: {
                LOGD("EMediaPlayer is opening input file.\n");
                break;
            }

            case MSG_FIND_STREAM_INFO: {
                LOGD("EMediaPlayer is finding media stream info.\n");
                break;
            }

            case MSG_BUFFERING_START: {
                LOGD("EMediaPlayer is buffering start.\n");
                postEvent(MEDIA_INFO, MEDIA_INFO_BUFFERING_START, msg.arg1);
                break;
            }

            case MSG_BUFFERING_END: {
                LOGD("EMediaPlayer is buffering finish.\n");
                postEvent(MEDIA_INFO, MEDIA_INFO_BUFFERING_END, msg.arg1);
                break;
            }

            case MSG_BUFFERING_UPDATE: {
                LOGD("EMediaPlayer is buffering: %d, %d", msg.arg1, msg.arg2);
                postEvent(MEDIA_BUFFERING_UPDATE, msg.arg1, msg.arg2);
                break;
            }

            case MSG_BUFFERING_TIME_UPDATE: {
                LOGD("EMediaPlayer time text update");
                break;
            }

            case MSG_SEEK_COMPLETE: {
                LOGD("EMediaPlayer seeks completed!\n");
                mSeeking = false;
                postEvent(MEDIA_SEEK_COMPLETE, 0, 0);
                break;
            }

            case MSG_PLAYBACK_STATE_CHANGED: {
                LOGD("EMediaPlayer's playback state is changed.");
                break;
            }

            case MSG_TIMED_TEXT: {
                LOGD("EMediaPlayer is updating time text");
                postEvent(MEDIA_TIMED_TEXT, 0, 0, msg.obj);
                break;
            }

            case MSG_REQUEST_PREPARE: {
                LOGD("EMediaPlayer is preparing...");
                status_t ret = prepare();
                if (ret != NO_ERROR) {
                    LOGE("EMediaPlayer prepare error - '%d'", ret);
                }
                break;
            }

            case MSG_REQUEST_START: {
                LOGD("EMediaPlayer is waiting to start.");
                break;
            }

            case MSG_REQUEST_PAUSE: {
                LOGD("EMediaPlayer is pausing...");
                pause();
                break;
            }

            case MSG_REQUEST_SEEK: {
                LOGD("EMediaPlayer is seeking...");
                mSeeking = true;
                mSeekingPosition = (long) msg.arg1;
                if (mediaPlayer != nullptr) {
                    mediaPlayer->seekTo(mSeekingPosition);
                }
                break;
            }

            case MSG_CURRENT_POSITON: {
                postEvent(MEDIA_CURRENT, msg.arg1, msg.arg2);
                break;
            }

            default: {
                LOGE("EMediaPlayer unknown MSG_xxx(%d)\n", msg.what);
                break;
            }
        }
        message_free_resouce(&msg);
    }
}