package com.eplayer;

import android.content.Context;
import android.media.AudioManager;
import android.net.Uri;
import android.support.annotation.NonNull;
import android.view.Surface;
import android.view.SurfaceHolder;

import java.io.FileDescriptor;
import java.io.IOException;
import java.util.Map;

public class AndroidMediaPlayer implements IMediaPlayer {

    private EasyMediaPlayer mMediaPlayer;

    public AndroidMediaPlayer() {
        initMediaPlayer();
    }

    private synchronized void initMediaPlayer() {
        if (mMediaPlayer == null) {
            mMediaPlayer = new EasyMediaPlayer();
        }
        mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
        mMediaPlayer.setOnPreparedListener(new EasyMediaPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(IMediaPlayer mp) {
                if (mOnPreparedListener != null) {
                    mOnPreparedListener.onPrepared(AndroidMediaPlayer.this);
                }
            }
        });
        mMediaPlayer.setOnCompletionListener(new EasyMediaPlayer.OnCompletionListener() {
            @Override
            public void onCompletion(IMediaPlayer mp) {
                if (mOnCompletionListener != null) {
                    mOnCompletionListener.onCompletion(AndroidMediaPlayer.this);
                }
            }
        });
        mMediaPlayer.setOnBufferingUpdateListener(new EasyMediaPlayer.OnBufferingUpdateListener() {
            @Override
            public void onBufferingUpdate(IMediaPlayer mp, int percent) {
                if (mOnBufferingUpdateListener != null) {
                    mOnBufferingUpdateListener.onBufferingUpdate(AndroidMediaPlayer.this, percent);
                }
            }
        });
        mMediaPlayer.setOnSeekCompleteListener(new EasyMediaPlayer.OnSeekCompleteListener() {
            @Override
            public void onSeekComplete(IMediaPlayer mp) {
                if (mOnSeekCompleteListener != null) {
                    mOnSeekCompleteListener.onSeekComplete(AndroidMediaPlayer.this);
                }
            }
        });
        mMediaPlayer.setOnVideoSizeChangedListener(new EasyMediaPlayer.OnVideoSizeChangedListener() {
            @Override
            public void onVideoSizeChanged(IMediaPlayer mp, int width, int height) {
                if (mOnVideoSizeChangedListener != null) {
                    mOnVideoSizeChangedListener.onVideoSizeChanged(AndroidMediaPlayer.this, width, height);
                }
            }
        });
        mMediaPlayer.setOnTimedTextListener(new EasyMediaPlayer.OnTimedTextListener() {
            @Override
            public void onTimedText(IMediaPlayer mp, ETimedText text) {
                // TODO finish this function using TimeText
            }
        });
        mMediaPlayer.setOnInfoListener(new EasyMediaPlayer.OnInfoListener() {
            @Override
            public boolean onInfo(IMediaPlayer mp, int what, int extra) {
                if (mOnInfoListener != null) {
                    return mOnInfoListener.onInfo(AndroidMediaPlayer.this, what, extra);
                }
                return false;
            }
        });
    }

    @Override
    public void setDisplay(SurfaceHolder sh) {
        if (mMediaPlayer != null) {
            mMediaPlayer.setDisplay(sh);
        }
    }

    @Override
    public void setSurface(Surface surface) {
        if (mMediaPlayer != null) {
            mMediaPlayer.setSurface(surface);
        }
    }

    @Override
    public void setDataSource(@NonNull Context context, @NonNull Uri uri) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        if (mMediaPlayer != null) {
            mMediaPlayer.setDataSource(context, uri);
        }
    }

    @Override
    public void setDataSource(@NonNull Context context, @NonNull Uri uri, Map<String, String> headers) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        if (mMediaPlayer != null) {
            mMediaPlayer.setDataSource(context, uri, headers);
        }
    }

    @Override
    public void setDataSource(@NonNull String path) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        if (mMediaPlayer != null) {
            mMediaPlayer.setDataSource(path);
        }
    }

    @Override
    public void setDataSource(@NonNull String path, Map<String, String> headers) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        if (mMediaPlayer != null) {
            mMediaPlayer.setDataSource(path);
        }
    }

    @Override
    public void setDataSource(FileDescriptor fd) throws IOException, IllegalArgumentException, IllegalStateException {
        if (mMediaPlayer != null) {
            mMediaPlayer.setDataSource(fd);
        }
    }

    @Override
    public void setDataSource(FileDescriptor fd, long offset, long length) throws IOException, IllegalArgumentException, IllegalStateException {
        if (mMediaPlayer != null) {
            mMediaPlayer.setDataSource(fd, offset, length);
        }
    }

    @Override
    public void prepare() throws IOException, IllegalStateException {
        if (mMediaPlayer != null) {
            mMediaPlayer.prepare();
        }
    }

    @Override
    public void prepareAsync() throws IllegalStateException {
        if (mMediaPlayer != null) {
            mMediaPlayer.prepareAsync();
        }
    }

    @Override
    public void start() throws IllegalStateException {
        if (mMediaPlayer != null) {
            mMediaPlayer.start();
        }
    }

    @Override
    public void stop() throws IllegalStateException {
        if (mMediaPlayer != null) {
            mMediaPlayer.stop();
        }
    }

    @Override
    public void pause() throws IllegalStateException {
        if (mMediaPlayer != null) {
            mMediaPlayer.pause();
        }
    }

    @Override
    public void resume() throws IllegalStateException {
        if (mMediaPlayer != null) {
            mMediaPlayer.start();
        }
    }

    @Override
    public void setWakeMode(Context context, int mode) {
        if (mMediaPlayer != null) {
            mMediaPlayer.setWakeMode(context, mode);
        }
    }

    @Override
    public void setScreenOnWhilePlaying(boolean screenOn) {
        if (mMediaPlayer != null) {
            mMediaPlayer.setScreenOnWhilePlaying(screenOn);
        }
    }

    @Override
    public int getRotate() {
        return 0;
    }

    @Override
    public int getVideoWidth() {
        if (mMediaPlayer != null) {
            return mMediaPlayer.getVideoWidth();
        }
        return 0;
    }

    @Override
    public int getVideoHeight() {
        if (mMediaPlayer != null) {
            return mMediaPlayer.getVideoHeight();
        }
        return 0;
    }

    @Override
    public boolean isPlaying() {
        if (mMediaPlayer != null) {
            return mMediaPlayer.isPlaying();
        }
        return false;
    }

    @Override
    public void seekTo(float msec) throws IllegalStateException {
        if (mMediaPlayer != null) {
            mMediaPlayer.seekTo((int) msec);
        }
    }

    @Override
    public long getCurrentPosition() {
        if (mMediaPlayer != null) {
            return mMediaPlayer.getCurrentPosition();
        }
        return 0;
    }

    @Override
    public long getDuration() {
        if (mMediaPlayer != null) {
            return mMediaPlayer.getDuration();
        }
        return 0;
    }

    @Override
    public void release() {
        if (mMediaPlayer != null) {
            mMediaPlayer.release();
        }
    }

    @Override
    public void reset() {
        if (mMediaPlayer != null) {
            mMediaPlayer.reset();
        }
    }

    @Override
    public void setAudioStreamType(int streamtype) {
        if (mMediaPlayer != null) {
            mMediaPlayer.setAudioStreamType(streamtype);
        }
    }

    @Override
    public void setLooping(boolean looping) {
        if (mMediaPlayer != null) {
            mMediaPlayer.setLooping(looping);
        }
    }

    @Override
    public void changeFilter(String name) {
        if (mMediaPlayer != null) {
            mMediaPlayer.changeFilter(name);
        }
    }

    @Override
    public void changeFilter(int id) {
        if (mMediaPlayer != null) {
            mMediaPlayer.changeFilter(id);
        }
    }

    @Override
    public void changeEffect(String name) {
        if (mMediaPlayer!=null)
            mMediaPlayer.changeEffect(name);
    }

    @Override
    public void changeEffect(int id) {
        if (mMediaPlayer!=null)
            mMediaPlayer.changeEffect(id);
    }

    @Override
    public void setWatermark(byte[] data, int dataLen, int width, int height) {
        if (mMediaPlayer!=null){
            mMediaPlayer.setWatermark(data,dataLen,width,height);
        }
    }

    @Override
    public void changeBackground(String name) {
        if (mMediaPlayer!=null)
            mMediaPlayer.changeBackground(name);
    }

    @Override
    public boolean isLooping() {
        if (mMediaPlayer != null) {
            return mMediaPlayer.isLooping();
        }
        return false;
    }

    @Override
    public void setVolume(float leftVolume, float rightVolume) {
        if (mMediaPlayer != null) {
            mMediaPlayer.setVolume(leftVolume, rightVolume);
        }
    }

    @Override
    public void setAudioSessionId(int sessionId) throws IllegalArgumentException, IllegalStateException {
        if (mMediaPlayer != null) {
            mMediaPlayer.setAudioSessionId(sessionId);
        }
    }

    @Override
    public int getAudioSessionId() {
        if (mMediaPlayer != null) {
            return mMediaPlayer.getAudioSessionId();
        }
        return 0;
    }

    @Override
    public void setMute(boolean mute) {
        // do nothing
    }

    @Override
    public void surfaceChange(int width, int height) {
        if (mMediaPlayer != null) {
            mMediaPlayer.surfaceChange(width, height);
        }
    }
    @Override
    public void setRate(float rate) {
        // do nothing
    }

    @Override
    public void setPitch(float pitch) {
        // do nothing
    }

    @Override
    public void setOnPreparedListener(OnPreparedListener listener) {
        mOnPreparedListener = listener;
    }

    private OnPreparedListener mOnPreparedListener;

    @Override
    public void setOnCompletionListener(OnCompletionListener listener) {
        mOnCompletionListener = listener;
    }

    private OnCompletionListener mOnCompletionListener;

    @Override
    public void setOnBufferingUpdateListener(OnBufferingUpdateListener listener) {
        mOnBufferingUpdateListener = listener;
    }

    private OnBufferingUpdateListener mOnBufferingUpdateListener;

    @Override
    public void setOnSeekCompleteListener(OnSeekCompleteListener listener) {
        mOnSeekCompleteListener = listener;
    }

    private OnSeekCompleteListener mOnSeekCompleteListener;

    @Override
    public void setOnVideoSizeChangedListener(OnVideoSizeChangedListener listener) {
        mOnVideoSizeChangedListener = listener;
    }

    private OnVideoSizeChangedListener mOnVideoSizeChangedListener;

    @Override
    public void setOnTimedTextListener(OnTimedTextListener listener) {
        mOnTimedTextListener = listener;
    }

    private OnTimedTextListener mOnTimedTextListener;

    @Override
    public void setOnErrorListener(OnErrorListener listener) {
        mOnErrorListener = listener;
    }

    private OnErrorListener mOnErrorListener;

    @Override
    public void setOnInfoListener(OnInfoListener listener) {
        mOnInfoListener = listener;
    }

    private OnInfoListener mOnInfoListener;
}
