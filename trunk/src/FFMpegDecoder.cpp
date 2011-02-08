#include "FFMpegDecoder.h"
//#include <stdint.h>


//#include <windows.h>

#ifndef MAX_AUDIO_PACKET_SIZE
#define MAX_AUDIO_PACKET_SIZE (128 * 1024)
#endif
uint64_t	my_video_pkt_pts = AV_NOPTS_VALUE;

#ifndef errinfo
#define errinfo(str)		fprintf(stderr,"%s: %s\n",__FUNCTION__,str)
#endif

FFMpegDecoder::FFMpegDecoder()
{
	av_register_all();
	avcodec_register_all();
	pDecodedFrame = avcodec_alloc_frame();
	encodedPacket.data = NULL;
	
	audioBuf.totalSize = 4 * MAX_AUDIO_PACKET_SIZE;
	audioBuf.dataSize = 0;
	audioBuf.data = (char*)malloc(audioBuf.totalSize);

	my_video_pkt_pts = AV_NOPTS_VALUE;

	pFormatCtx = NULL;
	resampler = NULL;
	fifoResampleResult = NULL;
	resampleChunk = NULL;

	audioStream = -1;
	videoStream = -1;
}

FFMpegDecoder::~FFMpegDecoder()
{
	if (pFormatCtx == NULL)
		return;

	if (audioStream >= 0)
		avcodec_close(pFormatCtx->streams[audioStream]->codec);

	if (videoStream >= 0)
		avcodec_close(pFormatCtx->streams[videoStream]->codec);

	av_close_input_file(pFormatCtx);

	free(audioBuf.data);
	if (resampleChunk)
		free(resampleChunk);
	if (fifoResampleResult)
		delete fifoResampleResult;
	if (resampler)
		delete resampler;

	if (pDecodedFrame)
		av_free(pDecodedFrame);
}

AVInfo* FFMpegDecoder::openFileW(wchar_t *fileName)
{
	int mbcsSize = 512;
	char mbcs[512];
	memset(mbcs,0,mbcsSize);
	//TODO: ray: convert to UTF8!!
	/*
	int ret = WideCharToMultiByte(CP_UTF8,0,fileName,wcslen(fileName)+1,mbcs,mbcsSize,NULL,NULL);
	return openFile(mbcs);
	*/
	return openFile((char*)fileName);
}
AVInfo* FFMpegDecoder::openFile(char* fileName)
{
	
	int ret = av_open_input_file(&pFormatCtx, fileName, NULL, 0, NULL);

	FILE *f = fopen(fileName,"rb");
	if (f != NULL)
	{
		fseek(f,0,SEEK_END);
		long offset = ftell(f);
		printf("file size=%d\n",offset);
		fclose(f);
	}
	if (ret != 0)
	{
		errinfo("av_open_input_file failed!");
		return NULL;
	}
		

	ret = av_find_stream_info(pFormatCtx);
	/*
    if( ret < 0 )
        return NULL;
	*/
	dump_format(pFormatCtx, 0, (char*)fileName, false);

    for(unsigned int i=0; i<pFormatCtx->nb_streams; i++)
	{
        if( pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) 
            videoStream = i;
        else if( pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) 
            audioStream = i;
	}
    if( videoStream == -1 && audioStream == -1 )
        return NULL;
	//
	

	if( videoStream >= 0 )
	{
		pVideoCodecCtx = pFormatCtx->streams[videoStream]->codec;
		pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
		if( pVideoCodec == NULL )
			return NULL;
/*
		if( pVideoCodec->capabilities & CODEC_CAP_TRUNCATED )
			pVideoCodecCtx->flags |= CODEC_FLAG_TRUNCATED;
*/
		if(avcodec_open(pVideoCodecCtx, pVideoCodec)<0)
			return NULL;

		pVideoCodecCtx->get_buffer = &FFMpegDecoder::my_get_buffer;
		pVideoCodecCtx->release_buffer = &FFMpegDecoder::my_release_buffer;
	}
	if( audioStream >= 0 )
	{
		pAudioCodecCtx = pFormatCtx->streams[audioStream]->codec;
		pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);
		if( pAudioCodec == NULL )
		{
			errinfo("audio stream found but no decoder");
			audioStream = -1;
		}
		else
		{
			if(avcodec_open(pAudioCodecCtx, pAudioCodec)<0)
			return NULL;
		}
	}

	mediaInfo.videoStreamIdx = videoStream;
	mediaInfo.audioStreamIdx = audioStream;
	mediaInfo.fileSize = pFormatCtx->file_size;
	mediaInfo.durationSec = pFormatCtx->duration / 1000000;
	
	

	if (audioStream >= 0)
	{
		mediaInfo.audioBitrate = pAudioCodecCtx->bit_rate;
		mediaInfo.audioChannels = pAudioCodecCtx->channels;
		mediaInfo.audioSampleRate = pAudioCodecCtx->sample_rate;
		mediaInfo.audioTimeBase = av_q2d(pFormatCtx->streams[audioStream]->time_base);
		mediaInfo.audioBitsPerSample = 16;

		audioBytesPerSec = pAudioCodecCtx->sample_rate * pAudioCodecCtx->channels * 2;
		audioPtsSec = 0;
	}

	if (videoStream >= 0)
	{
		mediaInfo.videoBitrate = pFormatCtx->file_size / mediaInfo.durationSec * 8;
		mediaInfo.videoWidth = pVideoCodecCtx->width;
		mediaInfo.videoHeight = pVideoCodecCtx->height;
		//mediaInfo.videoFrameRate = av_q2d(pFormatCtx->streams[videoStream]->r_frame_rate);
		mediaInfo.videoTimeBase = av_q2d(pFormatCtx->streams[videoStream]->time_base);
		mediaInfo.videoFrameCount = pFormatCtx->streams[videoStream]->nb_frames;
		mediaInfo.videoPixFmt = pFormatCtx->streams[videoStream]->codec->pix_fmt;
	}

	
	return &mediaInfo;
}

AVPacket* FFMpegDecoder::readPacket()
{
    if( encodedPacket.data != NULL )
        av_free_packet(&encodedPacket);


	if( av_read_frame(pFormatCtx, &encodedPacket) < 0 )
		return NULL;
	else
		return &encodedPacket;
}

AVFrame* FFMpegDecoder::decodeVideo()
{
	return decodeVideo(&encodedPacket);
}
AVFrame* FFMpegDecoder::decodeVideo(AVPacket *encodedPacket)
{
	int gotPic = 0;
	int bytesDecoded = -1;
	
	my_video_pkt_pts = encodedPacket->pts;
	fpsCounter.BeforeProcess();
	bytesDecoded = avcodec_decode_video2(pVideoCodecCtx,pDecodedFrame,&gotPic,encodedPacket);
	fpsCounter.AfterProcess();


	if (encodedPacket->dts == AV_NOPTS_VALUE && pDecodedFrame->opaque && *(uint64_t*)pDecodedFrame->opaque != AV_NOPTS_VALUE)
		videoPTS = *(uint64_t *)pDecodedFrame->opaque;
	else if(encodedPacket->dts != AV_NOPTS_VALUE)
		videoPTS = encodedPacket->dts;
	else
		videoPTS = 0;

	//videoPTS *= pVideoCodecCtx->time_base;
	videoPTS *= av_q2d(pFormatCtx->streams[videoStream]->time_base);
    
	if (bytesDecoded < 0)
	{
		errinfo("decode error");
		return NULL;
	}	
	if (gotPic)
	{
		fpsCounter.FrameFinished();
		syncVideoPTS();
		return pDecodedFrame;
	}	
	else
		return NULL;
}

int FFMpegDecoder::decodeAudio(AVPacket *pkt,unsigned char* outputBuf,int outputBufSize)
{
	//decode 1 or more audio frames to outputBuf, from 1 packet
	unsigned char *bufPtr = outputBuf;
	int availableSize = outputBufSize;
	while (true)
	{
		int bufSize = availableSize;
		int bytesDecoded = avcodec_decode_audio3(pAudioCodecCtx,(int16_t*)bufPtr,&bufSize,pkt);
		if (bytesDecoded < 0)
			return -1;

		if (bufSize > availableSize)
		{
			printf("warning! audio buffer too small!\n");
			return 0;
		}

		availableSize -= bufSize;
		bufPtr += bufSize;
		if (availableSize <= 0)
			return (outputBufSize - availableSize);
	}
}
SimpleBuf* FFMpegDecoder::decodeAudio()
{	
	int frameSize = audioBuf.totalSize;
	int bytesDecoded = avcodec_decode_audio3(pAudioCodecCtx,(int16_t*)audioBuf.data,&frameSize,&encodedPacket);
	
	if( bytesDecoded <= 0 || frameSize < 0)		//error, or no more frames
		return NULL;

	audioBuf.dataSize = frameSize;
	audioPtsSec += (double)frameSize / audioBytesPerSec;

	return &audioBuf;
}

void FFMpegDecoder::syncVideoPTS()
{
	double frame_delay;
	if (videoPTS != 0) 
		videoClock = videoPTS;
	else
		videoPTS = videoClock;

	frame_delay = av_q2d(pFormatCtx->streams[videoStream]->codec->time_base);
	frame_delay += pDecodedFrame->repeat_pict * (frame_delay * 0.5);
	videoClock += frame_delay;
}

double FFMpegDecoder::getVideoPtsSec()
{
	return videoPTS;
}
int FFMpegDecoder::my_get_buffer(struct AVCodecContext *c, AVFrame *pic) 
{
  int ret = avcodec_default_get_buffer(c, pic);
  if (ret)
	  errinfo("avcodec_default_get_buffer failed!\n");

  uint64_t *pts = (uint64_t*)av_malloc(sizeof(uint64_t));
  *pts = my_video_pkt_pts;
  pic->opaque = pts;
  return ret;
}
void FFMpegDecoder::my_release_buffer(struct AVCodecContext *c, AVFrame *pic)
{
  if(pic) av_freep(&pic->opaque);
  avcodec_default_release_buffer(c, pic);
}

PixelFormat FFMpegDecoder::getPixelFormat()
{
	return pVideoCodecCtx->pix_fmt;
}

int FFMpegDecoder::seekToSec(double sec)
{
	//TODO: implement
	/*
	AV_TIME_BASE
	avformat_seek_file(pFormatCtx,this->videoStream,
	*/
	int64_t seekTarget = sec*AV_TIME_BASE;
	int ret = avformat_seek_file(pFormatCtx,-1,INT64_MIN,seekTarget,INT64_MAX,0);
	return ret;
}
FFMpegReSampler* FFMpegDecoder::createReSampler(int ch2,int rate2, SampleFormat fmt2)
{
	if (resampler != NULL)
	{
		delete resampler;
		free(fifoResampleResult);
		free(resampleChunk);
	}
		
	resampler = new FFMpegReSampler(
		pAudioCodecCtx->channels,
		pAudioCodecCtx->sample_rate,
		pAudioCodecCtx->sample_fmt,
		ch2,
		rate2,
		fmt2);

	if (resampler)
	{
		fifoResampleResult = new FFMpegFifo(4* MAX_AUDIO_PACKET_SIZE);
		resampleChunk = (unsigned char*)malloc(2*MAX_AUDIO_PACKET_SIZE);
	}
	

	return resampler;
}

unsigned char* FFMpegDecoder::getReSampledAudioSamples(int dstSize)
{
	if (resampler == NULL)
	{
		errinfo("resampler is NULL!");
		return NULL;
	}
		

	//write enough samples into fifo
	while (fifoResampleResult->getSize() < dstSize)
	{
		
		AVPacket *pkt = readPacket();
		if (pkt == NULL)
		{
			int fifoSize = fifoResampleResult->getSize();
			fifoResampleResult->read(resampleChunk,fifoSize);
			memset(resampleChunk+fifoSize,0,dstSize-fifoSize);
			break;
		}
		
		if (pkt->stream_index != audioStream)
			continue;
		
		//decode,resample,and write to fifo
		SimpleBuf *samples = decodeAudio();
		if (samples == NULL)	//decode error
			continue;

		int nb_resampled = resampler->resample((short*)samples->data,(short*)resampleChunk,samples->dataSize);
		fifoResampleResult->write(resampleChunk,nb_resampled);
	}	
	
	fifoResampleResult->read(resampleChunk,dstSize);
	return  resampleChunk;
}


double FFMpegDecoder::getAudioPtsSec()
{
	return audioPtsSec;
}

AVFrame *FFMpegDecoder::getNextVideoFrame()
{
	if (videoStream < 0)
	{
		errinfo("no video stream!");
		return NULL;
	}

	AVFrame *pic = NULL;
	while (true)
	{
		AVPacket *pkt = readPacket();
		if (pkt == NULL)
			break;

		if (pkt->stream_index != videoStream)
			continue;

		pic = decodeVideo();
		if (pic != NULL)
			break;
	}
	return pic;
}
AVFrame *FFMpegDecoder::getFrameAtSec(double sec)
{
	if (videoStream < 0)
	{
		errinfo("no video stream!");
		return NULL;
	}

	AVFrame *pic = NULL;
	seekToSec(sec);
	while (true)
	{
		AVPacket *pkt = readPacket();
		if (pkt == NULL)
			break;

		if (pkt->stream_index != videoStream)
			continue;

		pic = decodeVideo();
		if (pic != NULL)
			break;
	}
	return pic;
}

