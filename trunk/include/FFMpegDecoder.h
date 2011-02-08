#ifndef FFMPEG_DECODER_H
#define FFMPEG_DECODER_H

#include "FFMpegWrapper.h"

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}

#include "AVInfo.h"
#include "SimpleBuffer.h"
#include "FFMpegFifo.h"
#include "FFMpegReSampler.h"

class FFMPEGWRAPPER_API FFMpegDecoder
{
public:
	FFMpegDecoder();
	~FFMpegDecoder();

	AVInfo*			openFile(char* filename);
	AVInfo*			openFileW(wchar_t* filename);
	AVPacket*		readPacket();
	AVFrame*		decodeVideo();
	AVFrame*		decodeVideo(AVPacket *encodedPacket);
	SimpleBuf*		decodeAudio();
	int				decodeAudio(AVPacket *pkt,unsigned char* outputBuf,int outputBufSize);
	double			getVideoPtsSec();
	double			getAudioPtsSec();
	PixelFormat		getPixelFormat();
	int				seekToSec(double sec);
	AVFrame*		getFrameAtSec(double sec);
	AVFrame*		getNextVideoFrame();


	FFMpegReSampler*	createReSampler(int ch2,int rate2,SampleFormat fmt2);
	unsigned char*		getReSampledAudioSamples(int audio_size);
	bool			isOpened();

protected:
	static int			my_get_buffer(struct AVCodecContext *c, AVFrame *pic);
	static void			my_release_buffer(struct AVCodecContext *c, AVFrame *pic);
	void				syncVideoPTS();

public:
//protected:
	int					videoBufSize;
	SimpleBuf			audioBuf;			//audio decode result
	unsigned char*		resampleBuf;		//resample result
	
	int					videoStream,audioStream;
	AVFormatContext*	pFormatCtx;
	AVCodecContext*		pVideoCodecCtx;
	AVCodecContext*		pAudioCodecCtx;
	AVCodec*			pVideoCodec;
	AVCodec*			pAudioCodec;
	AVFrame*			pDecodedFrame;

	AVPacket			encodedPacket;
	AVInfo				mediaInfo;

	double				videoPTS;
	double				videoClock;

	
	FFMpegReSampler*	resampler;
	FFMpegFifo*			fifoResampleResult;
	unsigned char*		resampleChunk;		//this will be returned by getReSampledAudio()

	int					audioBytesPerSec;
	double				audioPtsSec;
	
	bool				opened;

public:
	static const int ERR_OPENFILE		= -1;
	static const int ERR_FIND_STREAM	= -2;
	static const int ERR_NOAVSTREAM		= -3;
	static const int ERR_FINDDECODER_V	= -4;
	static const int ERR_OPENDECODER_V	= -5;
	static const int ERR_FINDDECODER_A	= -6;
	static const int ERR_OPENDECODER_A	= -7;
	static const int ERR_NO_STREAM		= -8;
	
};
#endif	//FFMPEG_DECODER_H
