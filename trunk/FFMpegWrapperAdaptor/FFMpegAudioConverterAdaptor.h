#pragma once

#ifdef WIN32
	#ifdef DLL_BUILD_FFMPEG_WRAPPER_ADAPTOR
		#define DLLPrefix   __declspec( dllexport )
	#else
		#define DLLPrefix	__declspec( dllimport )
	#endif
#endif

class DLLPrefix FFMpegAudioConverterAdaptor
{
public:
	FFMpegAudioConverterAdaptor();
	~FFMpegAudioConverterAdaptor();

	int SetupInputByWavHeader(char* wavHeader,int wavHeaderLen);
	int DecodeAndResample(char* inBuf,int inBufLen,char* outBuf,int outBufLen);

	int GetChannelNumIn();
	int GetBitsPerSampleIn();
	int GetSampleRateIn();
	int GetBitrateIn();

	static void GenWavHeader(char* buf, unsigned long ulPcmDataSize,int chNum,int sampleRate,int bitsPerSample);

protected:
	void*	pInnerObj;
};