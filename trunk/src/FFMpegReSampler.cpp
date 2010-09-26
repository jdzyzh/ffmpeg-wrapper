#include "FFMpegReSampler.h"


FFMpegReSampler::FFMpegReSampler(int ch1,int rate1,SampleFormat fmt1,int ch2,int rate2,SampleFormat fmt2)
{
	chIn = ch1;
	rateIn = rate1;
	fmtIn = fmt1;
	sampleSizeIn = av_get_bits_per_sample_format(fmtIn) / 8;

	chOut = ch2;
	rateOut = rate2;
	fmtOut = fmt2;
	sampleSizeOut = av_get_bits_per_sample_format(fmtOut) / 8;

	ctx = av_audio_resample_init(chOut,chIn,rateOut,rateIn,fmtOut,fmtIn,16,10,0,0.8);
}

FFMpegReSampler::~FFMpegReSampler(void)
{
	audio_resample_close(ctx);
}


int FFMpegReSampler::resample(short *bufSrc, short *bufDst, int bufSrcSize)
{
	//nb_samples = number of samples = bytesIn / (channelsIn * sampleSizeIn)

	int nb_samples = bufSrcSize / (chIn * sampleSizeIn);
    int nb_samplesOut = audio_resample(ctx,bufDst,bufSrc,nb_samples);
	int bytesOut = nb_samplesOut * sampleSizeOut * chOut;
	return bytesOut;
}
