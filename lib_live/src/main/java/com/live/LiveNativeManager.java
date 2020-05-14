package com.live;

/**
 * Author：Alex
 * Date：2019/9/18
 * Note：对接jni层函数的java接口
 */
public final class LiveNativeManager {

    static {
        System.loadLibrary("live-lib");
    }

    public static native int release();

    public static native int yuvI420ToNV21(byte[] i420Src, byte[] nv21Src, int width, int height);

    /**
     * NV21转化为YUV420P数据
     * @param src         原始数据
     * @param width       原始数据宽度
     * @param height      原始数据高度
     * @param dst         生成数据
     * @param dst_width   生成数据宽度
     * @param dst_height  生成数据高度
     * @param mode        压缩模式，这里为0，1，2，3 速度由快到慢，质量由低到高，一般用0就好了，因为0的速度最快
     * @param degree      旋转的角度，90，180和270三种。切记，如果角度是90或270，则最终i420Dst数据的宽高会调换
     * @param isMirror    是否镜像，一般只有270的时候才需要镜像
     * @return
     */
    public static native int compressYUV(byte[] src, int width, int height, byte[] dst, int dst_width, int dst_height, int mode, int degree, boolean isMirror);

    /**
     * YUV420P数据的裁剪
     * @param src         原始数据
     * @param width       原始数据宽度
     * @param height      原始数据高度
     * @param dst         生成数据
     * @param dst_width   生成数据宽度
     * @param dst_height  生成数据高度
     * @param left        裁剪的起始x点，必须为偶数，否则显示会有问题
     * @param top         裁剪的起始y点，必须为偶数，否则显示会有问题
     * @return
     */
    public static native int cropYUV(byte[] src, int width, int height, byte[] dst, int dst_width, int dst_height, int left, int top);

    /**
     * 编码视频数据准备工作
     * @param src_width
     * @param src_height
     * @param in_width
     * @param in_height
     * @return
     */
    public static native int encoderVideoinit( int src_width, int src_height,int in_width, int in_height);

    /**
     * 编码视频数据接口
     * @param srcFrame      原始数据(YUV420P数据)
     * @param frameSize     帧大小
     * @param fps           fps
     * @param dstFrame      编码后的数据存储
     * @param outFramewSize 编码后的数据大小
     * @return
     */
    public static native int encoderVideoEncode(byte[] srcFrame, int frameSize, int fps, byte[] dstFrame, int[] outFramewSize);

    /**
     *
     * @param sampleRate 音频采样频率
     * @param channels   音频通道
     * @param bitRate    音频bitRate
     * @return 每次编码的数据适合大小
     */
    public static native int encoderAudioInit(int sampleRate, int channels, int bitRate);

    public static native int encoderAudioEncode(byte[] srcFrame, int frameSize, byte[] dstFrame, int dstSize);

    /**
     * 初始化RMTP，建立RTMP与RTMP服务器连接
     * @param url
     * @return
     */
    public static native int initRtmpData(String url);

    /**
     * 发送SPS,PPS数据
     * @param sps       sps数据
     * @param spsLen    sps长度
     * @param pps       pps数据
     * @param ppsLen    pps长度
     * @param timeStamp 时间戳
     * @return
     */
    public static native int sendRtmpVideoSpsPPS(byte[] sps, int spsLen, byte[] pps, int ppsLen, long timeStamp);

    /**
     * 发送视频数据，再发送sps，pps之后
     * @param data
     * @param dataLen
     * @param timeStamp
     * @return
     */
    public static native int sendRtmpVideoData(byte[] data, int dataLen, long timeStamp);

    /**
     * 发送AAC Sequence HEAD 头数据
     * @param timeStamp
     * @return
     */
    public static native int sendRtmpAudioSpec(long timeStamp);

    /**
     * 发送AAC音频数据
     * @param data
     * @param dataLen
     * @param timeStamp
     * @return
     */
    public static native int sendRtmpAudioData(byte[] data, int dataLen, long timeStamp);

    /**
     * 释放RTMP连接
     * @return
     */
    public static native int releaseRtmp();

    /**
     * 释放视频相关资源
     * @return
     */
    public static native int releaseVideo();

    /**
     * 释放音频相关资源
     * @return
     */
    public static native int releaseAudio();
}
