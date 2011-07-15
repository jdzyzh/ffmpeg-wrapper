#include "FFMpegMuxer.h"

int FFMpegMuxer::init(const char* muxerName,const char* outputName)
{
	av_register_all();
	formatCtx = avformat_alloc_context();
	fmt = av_guess_format(muxerName,NULL,NULL);
	formatCtx->oformat = fmt;
	strcpy(formatCtx->filename,outputName);

	return 0;
}

int FFMpegMuxer::addVideoStream(int streamIndex,int width,int height,int bitrate,int fps)
{
	videoFPS = fps;
	int timebaseDen = 90000;
	int timebaseNum = 1;

	videoStream = av_new_stream(formatCtx,streamIndex);
	avcodec_get_context_defaults2(videoStream->codec, CODEC_TYPE_VIDEO);
	videoStream->stream_copy = 1;
	videoStream->avg_frame_rate.den = timebaseNum;
	videoStream->avg_frame_rate.num = timebaseDen;
	videoStream->time_base.den = timebaseDen;
	videoStream->time_base.num = timebaseNum;
	//videoStream->first_dts = 0;
	//videoStream->start_time = 0;
	videoStream->r_frame_rate.den = timebaseNum;
	videoStream->r_frame_rate.num = timebaseDen;
	AVCodecContext *codecCtx = videoStream->codec;
	codecCtx->codec_id = CODEC_ID_H264;
	codecCtx->codec_type = AVMEDIA_TYPE_VIDEO;

	codecCtx->time_base.den = timebaseDen;
	codecCtx->time_base.num = timebaseNum;
	codecCtx->width = width;
	codecCtx->height = height;
	codecCtx->bit_rate = bitrate;
	return 0;
}

int FFMpegMuxer::beginMux()
{
	int ret = av_set_parameters(formatCtx, NULL);
	dump_format(formatCtx,0,formatCtx->filename,1);

    if (!(fmt->flags & AVFMT_NOFILE)) 
	{
	
		url_open_dyn_buf(&ioctx);
		formatCtx->pb = ioctx;

		rtpCtx = av_alloc_format_context();
		rtpCtx->oformat = guess_format("rtp",0,0);
		AVStream *fakeVideo = 0;
		fakeVideo = av_new_stream(rtpCtx,0);
		avcodec_get_context_defaults(fakeVideo->codec);
		fakeVideo->codec->codec_id = CODEC_ID_MPEG2TS;
		rtpCtx->audio_codec_id = CODEC_ID_NONE;
		rtpCtx->video_codec_id = CODEC_ID_MPEG2TS;
		av_set_parameters(rtpCtx,0);
		url_fopen(&rtpCtx->pb,"rtp://127.0.0.1:4000",URL_WRONLY);
		av_write_header(rtpCtx);
		

		/*
		if (url_fopen(&formatCtx->pb, formatCtx->filename, URL_WRONLY) < 0) 
		{
			fprintf(stderr, "Could not open '%s'\n", formatCtx->filename);
            return -1;
        }
		*/
    }
	ret = av_write_header(formatCtx);
	return ret;
}

int FFMpegMuxer::addVideoFrame(void* encodedData,int encodedDataSize,int64_t pts,bool isKeyFrame)
{
	int streamIdx = 0;	//hardcode
	AVPacket pkt;
    av_init_packet(&pkt);

	static int64_t dts = 0;
	static int duration = 90000 / videoFPS;
	//pkt.pts = pts;
	pkt.pts = AV_NOPTS_VALUE;
	pkt.dts = dts;
	dts += duration;
	pkt.duration = duration;
	//printf("muxer: pts=%lld\n",pts);
	/*
    if (pts != AV_NOPTS_VALUE)
        pkt.pts= av_rescale_q(pts, videoCodecTimebase, videoStream->time_base);
		*/
    if(isKeyFrame)
        pkt.flags |= AV_PKT_FLAG_KEY;
    pkt.stream_index = streamIdx;
    pkt.data= (uint8_t*)encodedData;
    pkt.size= encodedDataSize;

    int ret = av_interleaved_write_frame(formatCtx, &pkt);

	
	uint8_t *destbuff;
	int len = url_close_dyn_buf(ioctx,&destbuff);
	
	AVPacket tspkt;
	av_init_packet(&tspkt);
	tspkt.size = len;
	tspkt.data = destbuff;
	av_interleaved_write_frame(rtpCtx,&tspkt);
	
	av_free(destbuff);
	url_open_dyn_buf(&ioctx);
	formatCtx->pb = ioctx;
	printf("write rtp packet: %d bytes\n",len);
	
	return ret;
}

int FFMpegMuxer::endMux()
{
	return av_write_trailer(formatCtx);
}
