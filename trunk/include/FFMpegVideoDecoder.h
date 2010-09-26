#ifndef _H_FFMPEG_VIDEO_DECODER
#define _H_FFMPEG_VIDEO_DECODER


extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}


class FFMpegVideoDecoder
{
public:
	FFMpegVideoDecoder(char* codecName);
	~FFMpegVideoDecoder();

	int decode(unsigned char* inBuf,int inBufSize,unsigned char* outBuf,int outBufSize);
protected:
	AVCodec *codec;
	AVCodecContext *ctx;
	AVFrame *decodedFrame;

};
#endif
