//------------------------------------------------------------------------
/**
* Author:  coloriy
* Email:   coloriy@qq.com
* Github:  https://github.com/coloriy
* Copyright (c) 2015 by coloriy
*
* Brief:   Using the ffmpeg api to get the thumbnail
*/
//------------------------------------------------------------------------

#include "stdafx.h"
extern "C"
{
#include "libavformat\avformat.h"
#include "libswscale\swscale.h"
}

AVFormatContext* m_pFormatContext = NULL;
AVCodecContext*  m_pCodecContext = NULL;
int              m_nStreamIndex[AVMEDIA_TYPE_NB] = { -1 };
AVCodec*         m_pVideoCodec = NULL;
AVFrame*         m_pAVFrame = NULL;
AVFrame*         m_pThumbFrame = NULL;

const char* strInputFileName[] = {
    "D:\\testcontent\\MP4_H.264_Base_L4.0_1080P_7Mbps_25fps_AAC_LC.mp4",
    "D:\\testcontent\\MP4_H.264_BP_L1.3_480P_1.1Mbps_29.970fps_AMR_NB.mp4",
    "D:\\testcontent\\MP4_H.264_BP_L2.1_480x270_500Kbps_23.959fps_AAC_LC.mp4",
    "D:\\testcontent\\MP4_H.264_BP_L2.1_512x288_1.2Mbps_15fps_AAC_LC.mp4",
    "D:\\testcontent\\MP4_H.264_BP_L4.1_CIF_2Mbps_25fps_AAC_LC.mp4",
    "D:\\testcontent\\MP4_H.264_MP_L4.0_1080P_762Kbps_30fps_AAC_LC.mp4",
    "D:\\testcontent\\MP4_H.264_MP_L3.1_720P_2Mbps_30fps_AAC_LC.mp4",
    "D:\\testcontent\\MP4_H.264_MP_L2.2_640x320_463Kbps_25fps_AAC_LC.mp4",
    "D:\\testcontent\\MP4_H.264_MP_L3.1_720P_1Mbps_29.585fps_AAC_LC.mp4"
};
const char strThumbFileName[] = "D:\\test_thumb.rgb";
#define THUMB_WIDTH   640
#define THUMB_HEIGHT  480
#define BRIGHTNESS_VALUE 0xF0
#define DARKNESS_VALUE   16

int initFFmpegContext()
{
    avcodec_register_all();
    av_register_all();

    return 0;
}


int setDataSource(const char* url)
{
    int ret = -1;
    if (m_pFormatContext)
    {
        avformat_free_context(m_pFormatContext);
        m_pFormatContext = NULL;
    }

    m_pFormatContext = avformat_alloc_context();
    if (!m_pFormatContext)
    {
        return -1;
    }
    ret = avformat_open_input(&m_pFormatContext, url, NULL, NULL);
    if (ret != 0)
    {
        delete m_pFormatContext;
        return ret;
    }

    ret = avformat_find_stream_info(m_pFormatContext, NULL);
    if (ret != 0)
    {
        delete m_pFormatContext;
        return ret;
    }

    m_nStreamIndex[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(m_pFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    m_nStreamIndex[AVMEDIA_TYPE_AUDIO] = av_find_best_stream(m_pFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

    return ret;
}

int openDecoder()
{
    int ret = -1;
    m_pCodecContext = m_pFormatContext->streams[m_nStreamIndex[AVMEDIA_TYPE_VIDEO]]->codec;
    if (m_pCodecContext)
    {
        m_pVideoCodec = avcodec_find_decoder(m_pCodecContext->codec_id);
        ret = avcodec_open2(m_pCodecContext, m_pVideoCodec, NULL);
        if (ret != 0)
        {
            return ret;
        }
        avcodec_flush_buffers(m_pCodecContext);
    }
    return ret;
}

void closeDecoder()
{
    if (m_pVideoCodec)
    {
        avcodec_close(m_pCodecContext);
        m_pCodecContext = NULL;
    }
}

int decodeOneFrame(AVFrame* pFrame)
{
    int ret = 0;

    bool frame_found = false;
    int  decoded_frame_count = 0;
    AVPacket pkt;
    do
    {
        int got_frame = 0;
        ret = av_read_frame(m_pFormatContext, &pkt);
        if (ret < 0)
        {
            break;
        }
        if (pkt.stream_index == m_nStreamIndex[AVMEDIA_TYPE_VIDEO])
        {
            ret = avcodec_decode_video2(m_pCodecContext, pFrame, &got_frame, &pkt);
            if (got_frame)
            {
                decoded_frame_count++;
                //skip black and white pitures
                uint32_t y_value = 0;
                uint32_t y_half = 0;
                int pixel_count = pFrame->width * pFrame->height;
                for (int i = 0; i < pixel_count; i++)
                {
                    uint8_t y_temp = (uint8_t)(*(uint8_t*)((uint8_t*)(pFrame->data[0]) + i));
                    y_value += y_temp;
                    if (i == (pixel_count / 2))
                    {
                        y_half = y_value / i;
                    }
                }

                y_value /= pixel_count;
                if (y_half == y_value)
                {
                    printf("decoded frame count = %d y_half=%d == y_value=%d, skip this frame!\n", decoded_frame_count, y_half, y_value);
                    continue;
                }
                if (y_value < BRIGHTNESS_VALUE && y_value > DARKNESS_VALUE)
                {
                    frame_found = true;
                    printf("frame_found = true -----------------------decoded frame count = %d\n", decoded_frame_count);
                }
            }
#ifdef SAVE_YUV_FRAME    
            char szName[128];
            sprintf(szName, "D:\\test_%d.yuv", frame_count);

            // save the yuv
            FILE *pFile = fopen(szName, "ab");
            if (pFile)
            {
                fwrite(pFrame->data[0], 1, pFrame->width * pFrame->height, pFile);
                fwrite(pFrame->data[1], 1, pFrame->width * pFrame->height * 1 / 4, pFile);
                fwrite(pFrame->data[2], 1, pFrame->width * pFrame->height * 1 / 4, pFile);
                fclose(pFile);
            }
#endif            
        }
        av_free_packet(&pkt);

    } while ((!frame_found) && (ret >= 0));

    av_free_packet(&pkt);

    return ret;
}

int getThumbnail(AVFrame* pInputFrame, AVFrame* pOutputFrame, int desW, int desH)
{
    if (pInputFrame == NULL || pOutputFrame == NULL)
    {
        return -1;
    }
    SwsContext* pSwsContext = NULL;
    pSwsContext = sws_getCachedContext(pSwsContext, pInputFrame->width, pInputFrame->height, (AVPixelFormat)pInputFrame->format,
        desW, desH, AV_PIX_FMT_RGB565, SWS_BICUBIC, NULL, NULL, NULL);

    if (pSwsContext == NULL)
    {
        return -1;
    }

    m_pThumbFrame->width = desW;
    m_pThumbFrame->height = desH;
    m_pThumbFrame->format = AV_PIX_FMT_RGB565;

    av_frame_get_buffer(m_pThumbFrame, 16);

    sws_scale(pSwsContext, pInputFrame->data, pInputFrame->linesize, 0, pInputFrame->height, m_pThumbFrame->data, m_pThumbFrame->linesize);

    sws_freeContext(pSwsContext);

    return 0;
}

int getFrameAt(int64_t timeUs, int width, int height)
{
    int ret = -1;
    AVFrame* pFrame = NULL;

    ret = avformat_seek_file(m_pFormatContext, -1, INT16_MIN, timeUs, INT16_MAX, 0);

    pFrame = av_frame_alloc();
    m_pThumbFrame = av_frame_alloc();
    ret = openDecoder();
    if (ret != 0)
    {
        av_frame_free(&pFrame);
        av_frame_free(&m_pThumbFrame);
        return ret;
    }

    ret = decodeOneFrame(pFrame);
    if (ret < 0)
    {
        av_frame_free(&pFrame);
        av_frame_free(&m_pThumbFrame);
        return ret;
    }    

    ret = getThumbnail(pFrame, m_pThumbFrame, width, height);
    if (ret < 0)
    {
        av_frame_free(&pFrame);
        av_frame_free(&m_pThumbFrame);
        return ret;
    }

    // save the rgb565
    FILE *pFile = fopen(strThumbFileName, "ab");
    if (pFile)
    {
        fwrite(m_pThumbFrame->data[0], 1, m_pThumbFrame->width * m_pThumbFrame->height * 2, pFile);
        fclose(pFile);
    }

    av_frame_free(&pFrame);
    av_frame_free(&m_pThumbFrame);

    closeDecoder();    

    return ret;
}



int _tmain(int argc, _TCHAR* argv[])
{
    int ret = -1;
    initFFmpegContext();

    int file_count = sizeof(strInputFileName) / sizeof(strInputFileName[0]);
    for (int i = 0; i < file_count; i++)
    {
        const char* pFileName = strInputFileName[i];
        ret = setDataSource(pFileName);

        getFrameAt(-1, THUMB_WIDTH, THUMB_HEIGHT);
    }

    //pause
    printf("finished, pause ....\n");
    getchar();

    return 0;
}

