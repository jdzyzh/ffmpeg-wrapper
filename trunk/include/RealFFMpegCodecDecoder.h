#pragma once

#include "RealFFMpegBitmapConverter.h"

extern "C"
{
	#include <libavcodec/avcodec.h>
}

class RealFFMpegCodecDecoder
{

public:
	RealFFMpegCodecDecoder(char* _codecName);
	virtual ~RealFFMpegCodecDecoder();

	int InitCodec(char* _codecName);
	AVFrame* decode(unsigned char *encData, int encDataSize, int *encDataConsumed);
	unsigned char* decodeAsBGR888(unsigned char *encData, int encDataSize, int *encDataConsumed);
	unsigned char* decodeAsBMP(unsigned char *encData,int encDataSize,int *encDataConsumed);

	static unsigned char* genBMPHeader(int w,int h);

public:
	int videoWidth;
	int videoHeight;
	PixelFormat videoPixFormat;
	char m_codecName[64];


public:
	AVCodec *codec;
	AVCodecContext *ctx;
	AVFrame *picture;

	RealFFMpegBitmapConverter *converter;
	FPSCounter fpsCounter;
};
