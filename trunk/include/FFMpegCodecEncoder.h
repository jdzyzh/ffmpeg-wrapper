#ifndef _H_FFMPEG_CODEC_ENCODER
#define _H_FFMPEG_CODEC_ENCODER

extern "C"
{
	#include <libavcodec/avcodec.h>
}
#include <FFMpegConverter.h>

int ffmpeg_jpeg_encode(unsigned char *srcBuf,unsigned char* dstBuf,int dstBufSize,PixelFormat srcPixFmt,int srcWidth,int srcHeight,int qvalue);

/*
typedef enum
{
	FFMPEG_PIXFMT_YUV420P,
	FFMPEG_PIXFMT_RGBA,
}FFMPEG_PIXFMT;
*/

class FFMpegCodecEncoder
{
public:
	FFMpegCodecEncoder();
	~FFMpegCodecEncoder();


	int InitCodec(const char* codecStr);
	int SetCodecParams(int w,int h,int qvalue,PixelFormat fmt); //q=1 (best) to 31 (worst)
	int Encode(AVFrame *pFrame);
	char* GetEncodeBuf();


protected:
	AVCodec *codec;
    AVCodecContext *c;
	int encBufSize;
	char *encBuf;
};

#endif
