#ifndef _H_FFMPEG_CODEC_DECODER
#define _H_FFMPEG_CODEC_DECODER

#ifdef DLL_EXPORT
#define FFMPEGWRAPPER_API __declspec(dllexport)
#else
#define FFMPEGWRAPPER_API __declspec(dllimport)
#endif

extern "C"
{
	#include <libavcodec/avcodec.h>
}
#include <FFMpegConverter.h>

unsigned char* makeBMPFromRGB888(unsigned char* rgbData,int rgbDataSize,int w,int h);

class FFMPEGWRAPPER_API FFMpegCodecDecoder
{

public:
	FFMpegCodecDecoder(char* _codecName);
	virtual ~FFMpegCodecDecoder();

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


protected:
	AVCodec *codec;
	AVCodecContext *ctx;
	AVFrame *picture;

	FFMpegConverter *converter;
};
#endif
