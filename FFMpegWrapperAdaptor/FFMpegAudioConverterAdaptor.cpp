#include "FFMpegAudioConverterAdaptor.h"
#include <FFMpegAudioConverter.h>

#define GetInnerObj ((FFMpegAudioConverter*)pInnerObj)


void FFMpegAudioConverterAdaptor::GenWavHeader(char *buf, unsigned long ulPcmDataSize, int chNum, int sampleRate, int bitsPerSample)
{
	return FFMpegAudioConverter::GenWavHeader(buf,ulPcmDataSize,chNum,sampleRate,bitsPerSample);
}


FFMpegAudioConverterAdaptor::FFMpegAudioConverterAdaptor()
{
	pInnerObj = new FFMpegAudioConverter();
}

FFMpegAudioConverterAdaptor::~FFMpegAudioConverterAdaptor()
{
	delete (FFMpegAudioConverter*)pInnerObj;
}

int FFMpegAudioConverterAdaptor::DecodeAndResample(char *inBuf, int inBufLen, char *outBuf, int outBufLen)
{
	return GetInnerObj->DecodeAndResample(inBuf,inBufLen,outBuf,outBufLen);
}

int FFMpegAudioConverterAdaptor::GetBitrateIn()
{
	return GetInnerObj->GetBitrateIn();
}

int FFMpegAudioConverterAdaptor::GetBitsPerSampleIn()
{
	return GetInnerObj->GetBitsPerSampleIn();
}

int FFMpegAudioConverterAdaptor::GetChannelNumIn()
{
	return GetInnerObj->GetChannelNumIn();
}

int FFMpegAudioConverterAdaptor::SetupInputByWavHeader(char *wavHeader, int wavHeaderLen)
{
	return GetInnerObj->SetupInputByWavHeader(wavHeader,wavHeaderLen);
}