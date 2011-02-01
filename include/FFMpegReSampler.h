#pragma once

#ifdef DLL_EXPORT
#define FFMPEGWRAPPER_API __declspec(dllexport)
#else
#define FFMPEGWRAPPER_API __declspec(dllimport)
#endif

extern "C" 
{
	#include <libavcodec/avcodec.h>
}
class FFMPEGWRAPPER_API FFMpegReSampler
{
public:
	FFMpegReSampler(int ch1,int rate1,SampleFormat fmt1,int ch2,int rate2,SampleFormat fmt2);
	~FFMpegReSampler(void);

	int resample(short* bufSrc,short* bufDst,int bufSrcSize);

public:
	int chIn;
	int chOut;
	int rateIn;
	int rateOut;
	int sampleSizeIn;
	int sampleSizeOut;
	SampleFormat fmtIn;
	SampleFormat fmtOut;


	ReSampleContext *ctx;
};
