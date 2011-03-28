#pragma once

#include <RealFFMpegBitmapConverter.h>

extern "C"
{
	#include <libavcodec/avcodec.h>
}

class RealFFMpegCodecEncoder
{
public:
	RealFFMpegCodecEncoder();
	~RealFFMpegCodecEncoder();


	int InitCodec(const char* codecStr, FFMpegCodecEncoderParam *param);
	int EncodeFrame(FFMpegFrame *pFrame);
	int Encode(void* inputBuf);
	char* GetEncodeBuf();


protected:
	AVCodec *codec;
    AVCodecContext *c;
	int encBufSize;
	char *encBuf;

	RealFFMpegBitmapConverter *picConv;
	AVPicture *picSrc;
	AVFrame *frameSrc;

	bool bFirstIFrameFound;
};
