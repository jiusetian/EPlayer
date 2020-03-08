//
// Created by lxr on 2019/9/19.
//
#include <malloc.h>
#include <libx264/x264.h>
#include "header/RtmpLivePush.h"
#include "header/AndroidLog.h"

#define RTMP_HEAD_SIZE (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)

RtmpLivePush::RtmpLivePush() {

}

RtmpLivePush::~RtmpLivePush() {
    release();
}

/**
 * 初始化rtmp数据，与rtmp连接
 * @param url
 */
void RtmpLivePush::init(unsigned char *url) {
    this->rtmp_url = url;
    rtmp = RTMP_Alloc();
    RTMP_Init(rtmp);

    rtmp->Link.timeout = 5;
    //rtmp->Link.lFlags |= RTMP_LF_LIVE;
    RTMP_SetupURL(rtmp, (char *) url);
    RTMP_EnableWrite(rtmp);

    //建立rtmp socket连接
    if (!RTMP_Connect(rtmp, NULL)) {
        LOGE("rtmp连接失败");
    } else {
        LOGE("rtmp连接成功");
    }

    if (!RTMP_ConnectStream(rtmp, 0)) {
        LOGE("rtmp连接流错误");
    } else {
        LOGE("rtmp连接流成功");
    }

    start_time = RTMP_GetTime();
    LOGI("开始时间=%d", start_time);
}

int RtmpLivePush::send_video_sps_pps(unsigned char *sps, int spsLen, unsigned char *pps, int ppsLen){

    RTMPPacket * packet;

    unsigned char * body;

    int i;

    packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+1024);

    memset(packet,0,RTMP_HEAD_SIZE);

    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;

    body = (unsigned char *)packet->m_body;


    i = 0;

    body[i++] = 0x17;

    body[i++] = 0x00;

    body[i++] = 0x00;

    body[i++] = 0x00;

    body[i++] = 0x00;

/*AVCDecoderConfigurationRecord*/

    body[i++] = 0x01;

    body[i++] = sps[1];

    body[i++] = sps[2];

    body[i++] = sps[3];

    body[i++] = 0xff;

/*sps*/

    body[i++]  = 0xe1;

    body[i++] = (spsLen >> 8) & 0xff;

    body[i++] = spsLen & 0xff;

    memcpy(&body[i],sps,spsLen);

    i +=  spsLen;

/*pps*/

    body[i++]  = 0x01;

    body[i++] = (ppsLen >> 8) & 0xff;

    body[i++] = (ppsLen) & 0xff;

    memcpy(&body[i],pps,ppsLen);

    i +=  ppsLen;

    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;

    packet->m_nBodySize = i;

    packet->m_nChannel = 0x04;

    packet->m_nTimeStamp = 0;

    packet->m_hasAbsTimestamp = 0;

    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;

    packet->m_nInfoField2 = rtmp->m_stream_id;

/*调用发送接口*/

    if (packet){
        RTMP_SendPacket(rtmp,packet,TRUE);
        //RTMPPacket_Free(packet);
        free(packet);
    }

    return 0;

}


void RtmpLivePush::pushSPSPPS(unsigned char *sps, int spsLen, unsigned char *pps, int ppsLen) {

    int bodySize = spsLen + ppsLen + 16;
    RTMPPacket *rtmpPacket = static_cast<RTMPPacket *>(malloc(sizeof(RTMPPacket)));
    RTMPPacket_Alloc(rtmpPacket, bodySize);
    RTMPPacket_Reset(rtmpPacket);

    char *body = rtmpPacket->m_body;

    int i = 0;
    //frame type(4bit)和CodecId(4bit)合成一个字节(byte)
    //frame type 关键帧1  非关键帧2
    //CodecId  7表示avc
    body[i++] = 0x17;

    //fixed 4byte
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;

    //configurationVersion： 版本 1byte
    body[i++] = 0x01;

    //AVCProfileIndication：Profile 1byte  sps[1]
    body[i++] = sps[1];

    //compatibility：  兼容性 1byte  sps[2]
    body[i++] = sps[2];

    //AVCLevelIndication： ProfileLevel 1byte  sps[3]
    body[i++] = sps[3];

    //lengthSizeMinusOne： 包长数据所使用的字节数  1byte
    body[i++] = 0xff;

    //sps个数 1byte
    body[i++] = 0xe1;
    //sps长度 2byte
    body[i++] = (spsLen >> 8) & 0xff;
    body[i++] = spsLen & 0xff;

    //sps data 内容
    memcpy(&body[i], sps, spsLen);
    i += spsLen;
    //pps个数 1byte
    body[i++] = 0x01;
    //pps长度 2byte
    body[i++] = (ppsLen >> 8) & 0xff;
    body[i++] = ppsLen & 0xff;
    //pps data 内容
    memcpy(&body[i], pps, ppsLen);


    rtmpPacket->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    rtmpPacket->m_nBodySize = bodySize;
    rtmpPacket->m_nTimeStamp = 0;
    rtmpPacket->m_hasAbsTimestamp = 0;
    rtmpPacket->m_nChannel = 0x04;//音频或者视频
    rtmpPacket->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    rtmpPacket->m_nInfoField2 = this->rtmp->m_stream_id;

    //send rtmp
    if (RTMP_IsConnected(rtmp)&&rtmpPacket) {
        RTMP_SendPacket(rtmp, rtmpPacket, TRUE);
        //RTMPPacket_Free(rtmpPacket);
        free(rtmpPacket);
    }

}

/**
 * 发送sps pps，即发送H264SequenceHead头
 * @param sps
 * @param sps_len
 * @param pps
 * @param pps_len
 */
void RtmpLivePush::addSequenceH264Header(unsigned char *sps, int sps_len, unsigned char *pps,int pps_len) {
    //LOGI("%s","发送header");
    //LOGI("#######addSequenceH264Header#########pps_lem=%d, sps_len=%d", pps_len, sps_len);
    RTMPPacket *packet = (RTMPPacket *) malloc(RTMP_HEAD_SIZE + 1024);
    //将packet内存空间设置为0值
    memset(packet, 0, RTMP_HEAD_SIZE + 1024);
    packet->m_body = (char *) packet + RTMP_HEAD_SIZE;

    unsigned char *body = (unsigned char *) packet->m_body;

    int i = 0;
    /*1:keyframe 7:AVC*/
    body[i++] = 0x17; // FrameType == 1，CodecID == 7

    //以下：vedio data

    /* AVC head */
    //AVCPacketType == 0x00
    body[i++] = 0x00;
    //CompositionTime == 0x000000,3个字节
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;

    /*AVCDecoderConfigurationRecord*/
    //configurationVersion版本号，1
    body[i++] = 0x01;
    //AVCProfileIndication sps[1]
    body[i++] = sps[1];
    //profile_compatibility sps[2]
    body[i++] = sps[2];
    //AVCLevelIndication sps[3]
    body[i++] = sps[3];

    //6bit的reserved为二进制位111111和2bitlengthSizeMinusOne一般为3，
    //二进制位11，合并起来为11111111，即为0xff
    body[i++] = 0xff;

    /*sps*/
    //3bit的reserved，二进制位111，5bit的numOfSequenceParameterSets，
    //sps个数，一般为1，及合起来二进制位11100001，即为0xe1
    body[i++] = 0xe1;
    //SequenceParametersSetNALUnits（sps_size + sps）的数组
    //sps data length
    body[i++] = (sps_len >> 8) & 0xff;
    body[i++] = sps_len & 0xff;
    memcpy(&body[i], sps, sps_len); //赋值sps数据
    i += sps_len;

    /*pps*/
    //numOfPictureParameterSets一般为1，即为0x01
    body[i++] = 0x01;
    //SequenceParametersSetNALUnits（pps_size + pps）的数组
    body[i++] = (pps_len >> 8) & 0xff;
    body[i++] = (pps_len) & 0xff;
    memcpy(&body[i], pps, pps_len);
    i += pps_len;

    //Message Type，RTMP_PACKET_TYPE_VIDEO：0x09
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    //Payload Length
    packet->m_nBodySize = i;
    //Time Stamp：4字节
    //记录了每一个tag相对于第一个tag（File Header）的相对时间。
    //以毫秒为单位。而File Header的time stamp永远为0。
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    //Channel ID，Audio和Vidio通道
    packet->m_nChannel = 0x04;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet->m_nInfoField2 = rtmp->m_stream_id;

    //send rtmp
    if (RTMP_IsConnected(rtmp)&&packet) {
        RTMP_SendPacket(rtmp, packet, TRUE);
       // RTMPPacket_Free(packet);
        free(packet);
    }

}

/**
 * 发送H264数据
 * @param buf
 * @param len
 * @param timeStamp
 */
void RtmpLivePush::addH264Body(unsigned char *buf, int len, long timeStamp) {
    //LOGI("%s","发送body");
    //去掉起始码(界定符)
    if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x00 && buf[3] == 0x01) {
        //00 00 00 01
        buf += 4; //移动指针
        len -= 4;
    } else if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x01) {
        // 00 00 01
        buf += 3;
        len -= 3;
    }

    int body_size = len + 9;
    RTMPPacket *packet = (RTMPPacket *) malloc(RTMP_HEAD_SIZE + 9 + len);
    memset(packet, 0, RTMP_HEAD_SIZE);
    packet->m_body = (char *) packet + RTMP_HEAD_SIZE;

    unsigned char *body = (unsigned char *) packet->m_body;
    //当NAL头信息中，type（5位）等于5，说明这是关键帧NAL单元
    //buf[0] NAL Header与运算，获取type，根据type判断关键帧和普通帧
    //00000101 & 00011111(0x1f) = 00000101
    int type = buf[0] & 0x1f; //获取帧类型
    //Pframe  7:AVC
    body[0] = 0x27; //非I帧
    //IDR I帧图像
    //Iframe  7:AVC ，FrameType == （H264 I 帧 ? 1 : 2）
    if (type == NAL_SLICE_IDR) {
        //LOGE("关键帧");
        body[0] = 0x17;
    }
    //AVCPacketType = 1 包类型
    /*nal unit,NALUs（AVCPacketType == 1)*/
    body[1] = 0x01;
    //composition time 0x000000 24bit，3个字节
    body[2] = 0x00;
    body[3] = 0x00;
    body[4] = 0x00;

    //写入NALU信息，右移8位，一个字节的读取
    // nulu的长度为int类型，这里分4个字节读取，从最左边的8位开始读，然后由左往右读，每次读8位
    body[5] = (len >> 24) & 0xff;
    body[6] = (len >> 16) & 0xff;
    body[7] = (len >> 8) & 0xff;
    body[8] = (len) & 0xff;
    /*copy data*/
    memcpy(&body[9], buf, len);

    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = body_size;
    //当前packet的类型：Video
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nChannel = 0x04;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nInfoField2 = rtmp->m_stream_id;
    //记录了每一个tag相对于第一个tag（File Header）的相对时间
    packet->m_nTimeStamp = RTMP_GetTime() - start_time;

    //send rtmp h264 body data
    if (RTMP_IsConnected(rtmp)&&packet) {
        RTMP_SendPacket(rtmp, packet, TRUE);
       // RTMPPacket_Free(packet);
        free(packet);
    }

}


int RtmpLivePush::getSampleRateIndex(int sampleRate) {
    int sampleRateArray[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000,
                             11025, 8000, 7350};
    for (int i = 0; i < 13; ++i) {
        if (sampleRateArray[i] == sampleRate) {
            return i;
        }
    }
    return -1;
}

/**
 * 发送AAC Sequence HEAD 头数据
 * @param sampleRate 采样率
 * @param channel 声道
 * @param timestamp
 */
void RtmpLivePush::addSequenceAacHeader(int sampleRate, int channel, int timestamp) {
    //音频同步包为4个字节
    int body_size = 2 + 2;
    //分配packet内存空间
    RTMPPacket *packet = (RTMPPacket *) malloc(RTMP_HEAD_SIZE + 4);
    memset(packet, 0, RTMP_HEAD_SIZE);
    //给body的指针赋值
    packet->m_body = (char *) packet + RTMP_HEAD_SIZE;

    unsigned char *body = (unsigned char *) packet->m_body;
    //头信息配置
    /*AF 00 + AAC RAW data*/
    body[0] = 0xAF;
    //AACPacketType:0表示AAC sequence header
    body[1] = 0x00;

    //配置信息占用2个字节，下面是2字节中对应位置保存的信息
    //5bit audioObjectType 编码结构类型，AAC-LC为2 二进制位00010
    //4bit samplingFrequencyIndex 音频采样索引值，44100对应值是4，二进制位0100
    //4bit channelConfiguration 音频输出声道，对应的值是2，二进制位0010
    //1bit frameLengthFlag 标志位用于表明IMDCT窗口长度 0 二进制位0
    //1bit dependsOnCoreCoder 标志位，表面是否依赖与corecoder 0 二进制位0
    //1bit extensionFlag 选择了AAC-LC,这里必须是0 二进制位0
    //上面都合成二进制00010 0100 0010 000
    uint16_t audioConfig = 0; //2个字节
    //AAC-LC是一种音频的编码规格，（LC）是比较简单，没有增益控制，但提高了编码效率，故使用（LC）是为了降低文件体积，有一定音质损失
    //这里的2表示对应的是AAC-LC 由于是5个bit，左移11位，变为16bit，2个字节
    //按位与11111000 00000000(0xF800)，&0xF800代表只保留前5个bit，即00010000 00000000，|= 是按位或后赋值，audioConfig刚开始的值为 00000000 00000000 2个字节16个bit
    // 按位或后就等于00010000 00000000了
    audioConfig |= ((2 << 11) & 0xF800);

    int sampleRateIndex = getSampleRateIndex(sampleRate); //0100
    if (-1 == sampleRateIndex) {
        free(packet);
        packet = NULL;
        LOGE("addSequenceAacHeader: no support current sampleRate[%d]", sampleRate);
        return;
    }

    //sampleRateIndex为4，左移7位以后的二进制位是01000000000，再运算& 0x0780，相当于0000001000000000 & 0000011110000000(0x0780)（只保留5bit后4位）
    //因为最前面的5bit是AAC-LC，所以这里只保留5bit后4位
    audioConfig |= ((sampleRateIndex << 7) & 0x0780);
    //sampleRateIndex为4，
    //channelConfiguration音频输出频道，对应值为2，4个bit 0010，0010左移3位变成7bit再加上前面的9bit就是16bit，
    // 下面的运算为二进制位00000000 00010000 & 00000000 01111000(0x78)（只保留5+4后4位）
    audioConfig |= ((channel << 3) & 0x78);
    //最后三个bit都为0保留最后三位111(0x07)
    audioConfig |= (0 & 0x07);
    //最后得到合成后的数据00010010 00010000，然后分别取这两个字节
    body[2] = (audioConfig >> 8) & 0xFF; //取高八位
    body[3] = (audioConfig & 0xFF); //取低八位

    //参数设置--->
    //消息类型ID（1-7协议控制；8，9音视频；10以后为AMF编码消息）
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    //消息载荷大小
    packet->m_nBodySize = body_size;
    //块流ID
    packet->m_nChannel = 0x04;
    //时间戳是绝对值还是相对值
    packet->m_hasAbsTimestamp = 0;
    //时间戳
    packet->m_nTimeStamp = 0;
    //块消息头的类型（4种）
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    //消息流ID
    packet->m_nInfoField2 = rtmp->m_stream_id;

    //send rtmp aac head
    if (RTMP_IsConnected(rtmp)&&packet) {
        RTMP_SendPacket(rtmp, packet, TRUE);
        //LOGD("send packet sendAacSpec");
        //RTMPPacket_Free(packet);
        free(packet);
    }

}

/**
 * 发送rtmp AAC data
 * @param buf
 * @param len aac数据的长度
 * @param timeStamp
 */
void RtmpLivePush::addAccBody(unsigned char *buf, int len, long timeStamp) {
    //加2个字节的头信息
    int body_size = 2 + len;
    //packet是一个指向RTMPPacket类型的指针，这里是分配内存空间，参数是字节大小
    RTMPPacket *packet = (RTMPPacket *) malloc(RTMP_HEAD_SIZE + len + 2);
    memset(packet, 0, RTMP_HEAD_SIZE);
    //m_body是一个char类型的指针
    packet->m_body = (char *) packet + RTMP_HEAD_SIZE;
    //在内存中，char与unsigned char没有什么不同，都是一个字节，唯一的区别是，char的最高位为符号位，
    // 因此char能表示-128~127, unsigned char没有符号位，因此能表示0~25
    unsigned char *body = (unsigned char *) packet->m_body;

    //头信息配置
    /*AF 00 + AAC RAW data*/
    body[0] = 0xAF;
    //AACPacketType:1表示AAC raw
    body[1] = 0x01;

    /*spec_buf是AAC raw数据*/
    memcpy(&body[2], buf, len);
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 0x04;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nTimeStamp = RTMP_GetTime() - start_time;
    //LOGI("aac m_nTimeStamp = %d", packet->m_nTimeStamp);
    packet->m_nInfoField2 = rtmp->m_stream_id;

    //send rtmp aac data
    if (RTMP_IsConnected(rtmp)&&packet) {
        RTMP_SendPacket(rtmp, packet, TRUE);
        //RTMPPacket_Free(packet);
        free(packet);
    }

}

void RtmpLivePush::release() {
    RTMP_Close(rtmp);
    RTMP_Free(rtmp);
}

















