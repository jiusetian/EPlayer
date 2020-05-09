//
// Created by lxr on 2019/9/19.
//
#include <cstring>
#include "header/VideoEncoder.h"
#include "header/AndroidLog.h"
//供测试文件使用,测试的时候打开
//#define ENCODE_OUT_FILE_1
//供测试文件使用
//#define ENCODE_OUT_FILE_2

VideoEncoder::VideoEncoder() : in_width(0), in_height(0), out_width(0), out_height(0), fps(0), encoder(NULL),
                               nal_nums(0) {
#ifdef ENCODE_OUT_FILE_1
    const char *outfile1 = "/sdcard/111.h264";
    out1 = fopen(outfile1, "wb");
#endif
#ifdef ENCODE_OUT_FILE_2
    const char *outfile2 = "/sdcard/333.h264";
    out2 = fopen(outfile2, "wb");
#endif
}

VideoEncoder::~VideoEncoder() {
    LOGD("调用了VideoEncoder的析构函数");
    close();
}

bool VideoEncoder::open() {
    //是否已经设置了in宽高和out宽高
    if (!validateSettings()) {
        LOGE("请先设置好宽高");
        return false;
    }

    if (encoder) {
        LOGI("编码器已经打开，清先close");
        return false;
    }
    //设置参数
    setParams();

    //按照色度空间分配内存，即为图像结构体x264_picture_t分配内存，并返回内存的首地址作为指针
    //i_csp(图像颜色空间参数，目前只支持I420/YUV420)为X264_CSP_I420
    //为图像结构体x264_picture_t分配内存,用于存储未压缩的数据
    x264_picture_alloc(&pic_in, params.i_csp, params.i_width, params.i_height);

    //打开编码器
    encoder = x264_encoder_open(&params);

    if (!encoder) {
        LOGE("不能打开编码器");
        close(); //关闭
        return false;
    }

    int nheader = 0;
    // write headers，x264_encoder_headers()是libx264的一个API函数，用于输出SPS/PPS/SEI这些H.264码流的头信息
    if (x264_encoder_headers(encoder, &nals, &nheader) < 0) {
        LOGI("x264_encoder_headers() failed");
        return false;
    }

    return true;
}

/**
 * 编码一帧图像数据
 * @param inBytes 输入的要编码的数据
 * @param frameSize 编码数据的大小
 * @param pts 第几帧
 * @param outBytes 保存编码后的输出数据
 * @param outFrameSize 编码后输出数据的大小
 * @return 返回编码后的nal单元的数量
 */
int VideoEncoder::encodeFrame(char *inBytes, int frameSize, int pts, char *outBytes, int *outFrameSize) {
    //YUV420数据
    int i420_y_size = in_width * in_height;
    int i420_u_size = (in_width >> 1) * (in_height >> 1);
    int i420_v_size = i420_u_size;

    uint8_t *i420_y_data = (uint8_t *) inBytes;
    uint8_t *i420_u_data = (uint8_t *) inBytes + i420_y_size;
    uint8_t *i420_v_data = (uint8_t *) inBytes + i420_y_size + i420_u_size;

    //将Y,U,V数据保存到pic_in.img的对应的分量中，还有一种方法是用AV_fillPicture和sws_scale来进行变换
    //pic_in是x264_picture_t结构体（描述视频的结构特征），pic_in.img是x264_image_t结构体（用来存放一帧图像的实际像素数据），其中x264_image_t结构体
    //的plane用来存放每个图像平面存放数据的起始地址, plane[0]是Y平面，plane[1]和plane[2]分别代表U和V平面

   // pic_in.img.i_csp = X264_CSP_I420;
   // pic_in.img.i_plane = 3; //plane的数量
    memcpy(pic_in.img.plane[0], i420_y_data, i420_y_size);
   // pic_in.img.i_stride[0] = i420_y_size;
    memcpy(pic_in.img.plane[1], i420_u_data, i420_u_size);
   // pic_in.img.i_stride[1] =i420_u_size;
    memcpy(pic_in.img.plane[2], i420_v_data, i420_v_size);
   // pic_in.img.i_stride[2] = i420_v_size;
   // pic_in.i_type = X264_TYPE_AUTO;

    // and encode and store into pic_out
    pic_in.i_pts = pts; //代表第几帧

    /**
     * 编码最主要的函数，代表编码一帧图像，返回编码后的数据大小
     * encoder：编码器
     * nals：保存编码后的nal单元数据，是一个nal单元数组，所以是nal的指针，传参是一个二级指针，可能是要改变一级指针的值，使得它指向一个对应的数组
     * nal_nums：产生的nal单元数量
     * pic_in：图像的输入
     * pic_out：图像的输出
     */
    int size = x264_encoder_encode(encoder, &nals, &nal_nums, &pic_in, &pic_out);

    if (size) {
        /*Here first four bytes proceeding the nal unit indicates frame length*/
        int have_copy = 0;
        //编码后，h264数据保存为nal了，我们可以获取到nals[i].type的类型判断是sps还是pps
        //或者是否是关键帧，nals[i].i_payload表示数据长度，nals[i].p_payload表示存储的数据
        //编码后，我们按照nals[i].i_payload的长度来保存copy h264数据的，然后抛给java端用作
        //rtmp发送数据，outFrameSize是变长的，当有sps pps的时候大于1，其它时候值为1
        for (int i = 0; i < nal_nums; i++) { //当有sps和pps的时候，nal单元的数量大于1，其他时候为1
            outFrameSize[i] = nals[i].i_payload;
            //将 nals单元的数据复制到输出数据outBytes中
            memcpy(outBytes + have_copy, nals[i].p_payload, nals[i].i_payload);
            have_copy += nals[i].i_payload;
        }

        //#ifdef后面跟一个宏定义，意思是如果有定义这个宏，就执行下一步
#ifdef ENCODE_OUT_FILE_1
//    ptr -- 这是指向要被写入的元素数组的指针。
//    size -- 这是要被写入的每个元素的大小，以字节为单位。
//    nmemb -- 这是元素的个数，每个元素的大小为 size 字节。
//    stream -- 这是指向 FILE 对象的指针，该 FILE 对象指定了一个输出流。
        fwrite(outBytes, 1, size, out1);
#endif

#ifdef ENCODE_OUT_FILE_2
        for (int i = 0; i < size; i++) {
            outBytes[i] = (char) nals[0].p_payload[i];
        }
        fwrite(outBytes, 1, size, out2);
        *outFrameSize = size;
#endif
        return nal_nums;
    }
    return -1;
}

/**
 * 设置编码参数，没有问题的配置
 */
void VideoEncoder::setParams() {

    //Preset可选项: ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow, placebo
    //Tune可选项: film（胶片电影）, animation（动画片）, grain（颗粒感很重的）, stillimage（静态图像）, psnr（信噪比）
    //ssim（结构相似性）, fastdecode（快速解码。主要用于一些低性能的播放设备）, zerolatency（低时延。主要用于直播等）
    x264_param_default_preset(&params, "veryfast", "zerolatency");

    params.i_csp = X264_CSP_I420; // 设置帧数据格式为420
    //设置帧宽度和高度
    params.i_width = getOutWidth();
    params.i_height = getOutHeight();
    //LOGD("设置的宽高=%d /// %d",getOutWidth(),getOutHeight());
    //并行编码多帧
    params.i_threads = X264_SYNC_LOOKAHEAD_AUTO;
    params.i_fps_num = 25; //getFps(); //设置帧率
    params.i_fps_den = 1; //帧率时间，1秒
    params.b_sliced_threads =0;//true; //多slice并行编码模式

    //0为根据fps而不是timebase,timestamps来计算帧间距离，可以避免在码率控制为ABR的情况下失效的问题
    params.b_vfr_input = 0;

    // Intra refres:关键帧即I帧的最大间隔和最小间隔，即GOP
    params.i_keyint_max = 25;

    // For streaming:
    //* 码率(比特率,单位Kbps)x264使用的bitrate需要/1000
    //设置码率,在ABR(平均码率)模式下才生效，且必须在设置ABR前先设置bitrate
    params.rc.i_bitrate =1200;// getBitrate() / 1000;
    // params.rc.b_mb_tree = 0;//这个不为0,将导致编码延时帧...在实时编码时,必须为0
    //参数i_rc_method表示码率控制，CQP(恒定质量)，CRF(恒定码率)，ABR(平均码率)
    //恒定码率，会尽量控制在固定码率
    params.rc.i_rc_method = X264_RC_ABR;//X264_RC_CRF;

    //是否把SPS和PPS放入每一个关键帧
    //SPS Sequence Parameter Set 序列参数集，PPS Picture Parameter Set 图像参数集
    //为了提高图像的纠错能力,该参数设置是让每个I帧都附带sps/pps。
    params.b_repeat_headers = 1; //1;
    //设置Level级别,编码复杂度,用于控制视频画质，取值为[0-51]，数值越低画质越好，默认值23，通常取值范围：[18-28]
    //params.i_level_idc = 51;

    //profile
    //默认：无
    //说明：限制输出文件的profile。这个参数将覆盖其它所有值，此选项能保证输出profile兼容的视频流。如果使用了这个选项，将不能进行无损压缩（qp 0 or crf 0）
    //可选：baseline，main，high
    //建议：不设置。除非解码环境只支持main或者baseline profile的解码。
    //如果你的播放器仅能支持特定等级的话，就需要指定等级选项。大多数播放器支持高等级，就不需要指定等级选项了
    x264_param_apply_profile(&params, "high");

}

void VideoEncoder::setParams2() {

    /*有问题的配置====》是params.b_intra_refresh = 1;设置有点问题，注释掉即可*/

    //解决编码延时的设置
    //preset
    //默认：medium
    //一些在压缩效率和运算时间中平衡的预设值。如果指定了一个预设值，它会在其它选项生效前生效。
    //可选：ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow and placebo.
    //建议：可接受的最慢的值

    //tune
    //默认：无
    //说明：在上一个选项基础上进一步优化输入。如果定义了一个tune值，它将在preset之后，其它选项之前生效。
    //可选：film, animation, grain, stillimage, psnr, ssim, fastdecode, zerolatency and touhou.
    //建议：根据输入选择。如果没有合适的就不要指定。
    //后来发现设置x264_param_default_preset(&param, "fast" , "zerolatency" );后就能即时编码了
    x264_param_default_preset(&params, "veryfast", "zerolatency");

    params.i_csp = X264_CSP_I420; // 设置帧数据格式为420
   //设置帧宽度和高度
    params.i_width = getOutWidth();
    params.i_height = getOutHeight();
    //LOGD("设置的宽高=%d /// %d",getOutWidth(),getOutHeight()); //并行编码多帧
    params.i_threads = X264_SYNC_LOOKAHEAD_AUTO;
    params.i_fps_num = 25; //getFps(); //设置帧率
    params.i_fps_den = 1; //帧率时间，1秒

    //I帧和P帧之间的B帧数量，若设置为0则表示不使用B帧，B帧会同时参考其前面与后面的帧，因此增加B帧数量可以提高压缩比，但也因此会降低压缩的速度。
    params.i_bframe = 5;

    /**
     * x264有两种并行模式，slice并行和frame并行。slice并行把一帧划分为多个矩形slice，在这多个slice之间并行处理，是一种非延时性的并行模式，
     * 多slice会稍微降低编码性能。frame并行是同时开启多帧编码，x264在N个frame并行的时候需要集齐N帧再开始一起编码，
     * 因此x264 frame并行是一种延时性的并行模式
     */
    params.b_sliced_threads =true; //多slice并行编码模式

    //0为根据fps而不是timebase,timestamps来计算帧间距离，VFR输入。1 ：时间基和时间戳用于码率控制，0 ：仅帧率用于码率控制
    params.b_vfr_input = 0;
    params.i_timebase_num = params.i_fps_den;
    params.i_timebase_den = params.i_fps_num;

    // Intra refres:关键帧即I帧的最大间隔和最小间隔
    params.i_keyint_max = 25;
    params.i_keyint_min = 1;
    //params.b_intra_refresh = 1; //是否使用周期帧内刷新替代IDR帧,这个要注释，不然在中途接流的时候会出现只有声音没有画面的bug

    params.rc.b_mb_tree = 0;//这个不为0,将导致编码延时帧...在实时编码时,必须为0
    //参数i_rc_method表示码率控制，CQP(恒定质量)，CRF(恒定码率)，ABR(平均码率)
    //恒定码率，会尽量控制在固定码率
    params.rc.i_rc_method =X264_RC_CRF;//X264_RC_ABR;// ;
    //ABR模式下调整i_bitrate，CQP下调整i_qp_constant调整QP值，范围0~51，值越大图像越模糊，默认23
    //CRF下调整f_rf_constant和f_rf_constant_max影响编码速度和图像质量
    //图像质量控制,rc.f_rf_constant是实际质量，越大图像越花，越小越清晰
    //param.rc.f_rf_constant_max ，图像质量的最大值
    // 和QP的范围一样RF的范围也是[0, 51]。其中0为无损模式，23为缺省，51质量最差。和QP一样的趋势。RF值加6，输出码率大概减少一半；减6，输出码率翻倍
    params.rc.f_rf_constant = 25; //实际质量，值越大图像越花,越小越清晰
    params.rc.f_rf_constant_max = 35; //最大码率因子，该选项仅在使用CRF并开启VBV时有效，

    // For streaming:
    //* 码率(比特率,单位Kbps)x264使用的bitrate需要/1000
    params.rc.i_bitrate = getBitrate() / 1000;
    //瞬时最大码率,平均码率模式下，最大瞬时码率，默认0(与-B设置相同)
    params.rc.i_vbv_max_bitrate = getBitrate() / 1000 * 1.2;
    //params.b_repeat_headers = 1;
    // 值为true，则NALU之前是4字节前缀码0x00000001，值为false，则NALU之前的4个字节为NALU长度
    params.b_annexb = 1;

    //是否把SPS和PPS放入每一个关键帧
    //SPS Sequence Parameter Set 序列参数集，PPS Picture Parameter Set 图像参数集
    //为了提高图像的纠错能力,该参数设置是让每个I帧都附带sps/pps。
    params.b_repeat_headers = 1;
    //设置Level级别,编码复杂度
    params.i_level_idc = 51;

    //profile
    //默认：无
    //说明：限制输出文件的profile。这个参数将覆盖其它所有值，此选项能保证输出profile兼容的视频流。如果使用了这个选项，将不能进行无损压缩（qp 0 or crf 0）
    //可选：baseline，main，high
    //建议：不设置。除非解码环境只支持main或者baseline profile的解码
    x264_param_apply_profile(&params, "main");
}

bool VideoEncoder::close() {
    LOGD("调用了videoencoder的close函数");
    if (encoder) {
        x264_picture_clean(&pic_in);
        memset((char *) &pic_in, 0, sizeof(pic_in));
        memset((char *) &pic_out, 0, sizeof(pic_out));
        x264_encoder_close(encoder);
        encoder = NULL;
    }

#ifdef ENCODE_OUT_FILE_1
    if (out1) {
        fclose(out1);
    }
#endif
#ifdef ENCODE_OUT_FILE_2
    if (out2) {
        fclose(out2);
    }
#endif

    return true;

}

/**
 * 验证设置是否已经可行
 * @return
 */
bool VideoEncoder::validateSettings() {
    if (!in_width) {
        LOGE("in_width不能为0");
        return false;
    }

    if (!in_height) {
        LOGE("in_height不能为0");
        return false;
    }

    if (!out_height) {
        LOGE("out_height不能为0");
        return false;
    }

    if (!out_width) {
        LOGE("out_width不能为0");
        return false;
    }

    return true;
}

int VideoEncoder::getFps() const {
    return fps;
}

void VideoEncoder::setFps(int fps) {
    this->fps = fps;
}

int VideoEncoder::getInHeight() const {
    return in_height;
}

void VideoEncoder::setInHeight(int inHeight) {
    in_height = inHeight;
}

int VideoEncoder::getInWidth() const {
    return in_width;
}

void VideoEncoder::setInWidth(int inWidth) {
    in_width = inWidth;
}

int VideoEncoder::getNumNals() const {
    return nal_nums;
}

void VideoEncoder::setNumNals(int numNals) {
    nal_nums = numNals;
}

int VideoEncoder::getOutHeight() const {
    return out_height;
}

void VideoEncoder::setOutHeight(int outHeight) {
    out_height = outHeight;
}

int VideoEncoder::getOutWidth() const {
    return out_width;
}

void VideoEncoder::setOutWidth(int outWidth) {
    out_width = outWidth;
}

int VideoEncoder::getBitrate() const {
    return bitrate;
}

void VideoEncoder::setBitrate(int bitrate) {
    this->bitrate = bitrate;
}

int VideoEncoder::getSliceMaxSize() const {
    return i_slice_max_size;
}

void VideoEncoder::setSliceMaxSize(int sliceMaxSize) {
    i_slice_max_size = sliceMaxSize;
}

int VideoEncoder::getVbvBufferSize() const {
    return i_vbv_buffer_size;
}

void VideoEncoder::setVbvBufferSize(int vbvBufferSize) {
    i_vbv_buffer_size = vbvBufferSize;
}

int VideoEncoder::getIThreads() const {
    return i_threads;
}

void VideoEncoder::setIThreads(int threads) {
    i_threads = threads;
}

int VideoEncoder::getBFrameFrq() const {
    return b_frame_frq;
}

void VideoEncoder::setBFrameFrq(int frameFrq) {
    b_frame_frq = frameFrq;
}

