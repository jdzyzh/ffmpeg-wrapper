#ifndef _H_FFMPEG_CODEC_DECODER
#define _H_FFMPEG_CODEC_DECODER


#include "FFMpegWrapperAPI.h"


unsigned char* makeBMPFromRGB888(unsigned char* rgbData,int rgbDataSize,int w,int h);



class FFMPEGWRAPPER_API FFMpegCodecDecoder
{

public:
	FFMpegCodecDecoder(char* _codecName);
	virtual ~FFMpegCodecDecoder();

	int InitCodec(char* _codecName);
	FFMpegFrame decode(unsigned char *encData, int encDataSize, int *encDataConsumed);
	unsigned char* decodeAsBGR888(unsigned char *encData, int encDataSize, int *encDataConsumed);
	unsigned char* decodeAsBMP(unsigned char *encData,int encDataSize,int *encDataConsumed);

	static unsigned char* genBMPHeader(int w,int h);

public:
	void* _delegate;
};
#endif
