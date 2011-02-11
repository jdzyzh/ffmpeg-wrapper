#ifndef _H_FFMPEG_CODEC_ENCODER
#define _H_FFMPEG_CODEC_ENCODER

#include "FFMpegWrapperAPI.h"

extern "C"
{
	#include <libavcodec/avcodec.h>
}
#include <FFMpegConverter.h>

typedef struct
{
	unsigned short inputWidth;
	unsigned short inputHeight;
	unsigned short encodeWidth;
	unsigned short encodeHeight;
	unsigned char qmin;
	unsigned char qmax;
	unsigned short max_bframes;
	unsigned short gop_size;
	unsigned int bitrate;
	char inputPixelType[32];

}FFMpegCodecEncoderParam;

class FFMPEGWRAPPER_API FFMpegCodecEncoder
{
public:
	FFMpegCodecEncoder();
	~FFMpegCodecEncoder();


	int InitCodec(const char* codecStr, FFMpegCodecEncoderParam *param);
	int Encode(void* inputBuf);
	char* GetEncodeBuf();


protected:
	AVCodec *codec;
    AVCodecContext *c;
	int encBufSize;
	char *encBuf;

	FFMpegConverter *picConv;
	AVPicture *picSrc;
	AVFrame *frameSrc;

	bool bFirstIFrameFound;
};

#endif
