#include "FFMpegEncoder.h"
//#include "fftrans/FFMpegFifo.h"

#ifndef errinfo
#define errinfo(str)		fprintf(stderr,"%s: %s\n",__FUNCTION__,str)
#endif

static AVFrame* allocDummyFrame(int w,int h)
{
	AVFrame *picture = avcodec_alloc_frame();
	int size = w*h;
	uint8_t *picture_buf = (uint8_t*)malloc((w*h * 3) / 2); /* size for YUV 420 */

    picture->data[0] = picture_buf;
    picture->data[1] = picture->data[0] + size;
    picture->data[2] = picture->data[1] + size / 4;
    picture->linesize[0] = w;
    picture->linesize[1] = w / 2;
    picture->linesize[2] = w / 2;

	return picture;
}
static void fillDummyFrame(AVFrame *picture,int w,int h,int i)
{
	/* prepare a dummy image */
	int x=0,y=0;

    /* Y */
    for(y=0;y<h;y++) {
        for(x=0;x<w;x++) {
            picture->data[0][y * picture->linesize[0] + x] = x + y + i * 3;
        }
    }

    /* Cb and Cr */
    for(y=0;y<h/2;y++) {
        for(x=0;x<w/2;x++) {
            picture->data[1][y * picture->linesize[1] + x] = 128 + y + i * 2;
            picture->data[2][y * picture->linesize[2] + x] = 64 + x + i * 5;
        }
    }

}
static AVCodecContext* createMP4Encoder()
{
	av_register_all();
	AVCodec *pVideoCodec = avcodec_find_encoder_by_name("mpeg1video");
	if (!pVideoCodec) 
		return NULL;

	AVCodecContext *pVideoCodecCtx = avcodec_alloc_context();
	//avcodec_get_context_defaults2(pVideoCodecCtx, CODEC_TYPE_VIDEO);
	
//	pVideoCodecCtx->codec_id = pVideoCodec->id;
 //   pVideoCodecCtx->codec_type = CODEC_TYPE_VIDEO;
    pVideoCodecCtx->bit_rate = 400000;
	pVideoCodecCtx->width = 352;
	pVideoCodecCtx->height = 288;
	pVideoCodecCtx->time_base.den = 25;
	pVideoCodecCtx->time_base.num = 1;
    pVideoCodecCtx->gop_size = 10; /* emit one intra frame every twelve frames at most */
    
	const PixelFormat *fmt = pVideoCodec->pix_fmts;
	pVideoCodecCtx->pix_fmt = *fmt;
	pVideoCodecCtx->max_b_frames = 1;

	int ret = avcodec_open(pVideoCodecCtx, pVideoCodec);
	if (ret == 0)
		return pVideoCodecCtx;
	else
		return NULL;
}
static void dumpAVCodecContext(AVCodecContext *p)
{
        FILE *file = fopen("avcodecCtx.dump","wb");
        for (int i=0;i<sizeof(AVCodecContext);i++)
		{
			
			fprintf(file,"%d: %x\n",i,*(p+i));
		}
		fclose(file);
}

FFMpegEncodeProfile* FFMpegEncoder::createDefaultProfile()
{
	FFMpegEncodeProfile *myProfile = (FFMpegEncodeProfile*)malloc(sizeof(FFMpegEncodeProfile));
	wcscpy(myProfile->outputFilename,L"test.avi");
	sprintf(myProfile->formatStr,"%s","avi");
	sprintf(myProfile->audioCodec,"%s","mp2");
	//sprintf(myProfile->videoCodec,"%s","mpeg1video");
	sprintf(myProfile->videoCodec,"%s","msmpeg4");

	myProfile->audioBitrate = 192000;
	myProfile->audioChannels = 2;
	myProfile->audioSampleRate = 44100;
	myProfile->audioBitsPerSample = 16;

	myProfile->videoFrameRate = 30;
	myProfile->videoWidth = 352;
	myProfile->videoHeight = 240;
	myProfile->videoBitrate = 400000;

	return myProfile;
}

FFMpegEncoder::FFMpegEncoder()
{
	FFMpegEncodeProfile *myProfile = FFMpegEncoder::createDefaultProfile();
	init(myProfile);
}

FFMpegEncoder::FFMpegEncoder(FFMpegEncodeProfile *myProfile)
{
	init(myProfile);
}

FFMpegEncoder::~FFMpegEncoder()
{
}


int FFMpegEncoder::init(FFMpegEncodeProfile *myProfile)
{
	av_register_all();
	profile = *myProfile;
	
	videoEncodeBufSize = 1024000;
	videoEncodeBuf = (unsigned char*)malloc(videoEncodeBufSize);
	if (videoEncodeBuf == NULL)
		return ERR_ALLOC_VIDEO_BUF;

	audioEncodeBufSize = 4*128*1024;
	audioEncodeBuf = (unsigned char*)malloc(audioEncodeBufSize);
	if (audioEncodeBuf == NULL)
		return ERR_ALLOC_AUDIO_BUF;
	    
	pFormatCtx = avformat_alloc_context();
	
	int ret = -1;
	ret = configOutput();
	if (ret) 
	{
		printf("error configuring output!\n");
		return ret;
	}
	
	videoStream = NULL;
	ret = configVideoStream();
	if (ret) 
	{
		printf("error configuring video!\n");
		return ret;
	}
	
	audioStream = NULL;
	ret = configAudioStream();
	if (ret) 
	{
		printf("error configuring audio!\n");
		return ret;
	}

	av_set_parameters(pFormatCtx, NULL);
	dump_format(pFormatCtx, 0, (char*)profile.outputFilename, 1);
    
	av_write_header(pFormatCtx);
	audioClock = 0;
	videoClock = 0;
	return 0;
	//
}


int FFMpegEncoder::configOutput()
{
	AVOutputFormat *fmt = guess_format(profile.formatStr,NULL,NULL);
	
	if (fmt == NULL)
		return ERR_GUESS_FORMAT;

	pFormatCtx->oformat = fmt;
	sprintf(pFormatCtx->filename, "%s", profile.outputFilename);

	int ret = url_fopen(&pFormatCtx->pb, (char*)profile.outputFilename, URL_WRONLY);
	/*
	fifo_open(&pFormatCtx->pb);
	pFormatCtx->pb->write_packet = fifo_write;
	pFormatCtx->pb->seek = fifo_seek;
	AVFifoBuffer *fifo = getFifo();
	*/
	return ret;
}

SimpleBuf FFMpegEncoder::getEncodedAudioBuf()
{
	SimpleBuf buf;
	buf.data = (char*)this->audioEncodeBuf;
	buf.dataSize = this->audioEncodedSize;
	buf.totalSize = this->audioEncodeBufSize;
	return buf;
}

SimpleBuf FFMpegEncoder::getEncodedVideoBuf()
{
	SimpleBuf buf;
	buf.data = (char*)this->videoEncodeBuf;
	buf.dataSize = this->videoEncodedSize;
	buf.totalSize = this->videoEncodeBufSize;
	return buf;
}
int FFMpegEncoder::encodeAudioFrame(short* audioBufIn,int audioBufInSize)
{
	audioEncodedSize = avcodec_encode_audio(pAudioCodecCtx,audioEncodeBuf,audioEncodeBufSize,audioBufIn);

	if (audioEncodedSize > 0)
		audioClock += ((double)audioEncodedSize / getAudioFrameSize());

	return audioEncodedSize;
}

int FFMpegEncoder::encodeVideoFrame(AVFrame *decodedFrame)
{	
	return encodeVideoFrame(decodedFrame,this->videoEncodeBuf,this->videoEncodeBufSize);
}

int FFMpegEncoder::encodeVideoFrame(AVFrame *decodedFrame,unsigned char* encodeBuf,int encodeBufSize)
{
	if (pVideoCodecCtx->me_threshold == 0)
		decodedFrame->pict_type = 0;

	videoEncodedSize = avcodec_encode_video(pVideoCodecCtx,encodeBuf,encodeBufSize, decodedFrame);
	if (videoEncodedSize < 0)
		errinfo("encode error!");

	return videoEncodedSize;
}

int FFMpegEncoder::writeToFileAudio()
{
	static double myPTS = 0;
	myPTS += audioEncodedSize / (double)pAudioCodecCtx->bit_rate;

	AVPacket pkt;
    av_init_packet(&pkt);

	pkt.size= this->audioEncodedSize;

	if (pAudioCodecCtx->coded_frame && pAudioCodecCtx->coded_frame->pts != AV_NOPTS_VALUE)
        pkt.pts= av_rescale_q(pAudioCodecCtx->coded_frame->pts, pAudioCodecCtx->time_base,audioStream->time_base);
	
    pkt.flags |= PKT_FLAG_KEY;
    pkt.stream_index= audioStream->index;
	pkt.data= audioEncodeBuf;

	int ret = av_interleaved_write_frame(pFormatCtx, &pkt);
	if (ret < 0 )
		return ERR_WRITE_AUDIO_FRAME;
	else
		return 0;
}
int FFMpegEncoder::writeToFileVideo(double ptsSec)
{
	//ray: should the pts be used?
	AVPacket pkt;
    av_init_packet(&pkt);

    if (pVideoCodecCtx->coded_frame->pts != AV_NOPTS_VALUE)
        pkt.pts= av_rescale_q(pVideoCodecCtx->coded_frame->pts, pVideoCodecCtx->time_base, videoStream->time_base);

	pkt.pts = ptsSec / av_q2d(videoStream->time_base);

	if(pVideoCodecCtx->coded_frame->key_frame/* || pVideoCodecCtx->coded_frame->coded_picture_number % 12 == 0*/)
        pkt.flags |= PKT_FLAG_KEY;
	
    pkt.stream_index= videoStream->index;
	pkt.data= this->videoEncodeBuf;
	pkt.size= this->videoEncodedSize;
	
	
/*
	pkt.pts = pVideoCodecCtx->coded_frame->display_picture_number;
	pkt.dts = pVideoCodecCtx->coded_frame->coded_picture_number;
	*/
    if (-1 > av_interleaved_write_frame(pFormatCtx, &pkt))
		return ERR_WRITE_VIDEO_FRAME;
	else
	{
		videoClock += av_q2d(pVideoCodecCtx->time_base);
		return 0;
	}
		
}


int FFMpegEncoder::configAudioStream()
{
	if (strcmp(profile.audioCodec,"NONE") == 0)
		return 0;

    AVStream *st;
    st = av_new_stream(pFormatCtx, 1);
    if (!st)
		return ERR_ADD_AUDIO_STREAM;
	audioStream = st;

	AVCodec *pAudioCodec = avcodec_find_encoder_by_name(profile.audioCodec);
    if (!pAudioCodec) 
		return ERR_FIND_AUDIO_CODEC;

	pAudioCodecCtx = st->codec;
	pAudioCodecCtx->codec_id = pAudioCodec->id;
    pAudioCodecCtx->codec_type = CODEC_TYPE_AUDIO;
	pAudioCodecCtx->bit_rate = profile.audioBitrate;
    pAudioCodecCtx->sample_rate = profile.audioSampleRate;
	pAudioCodecCtx->channels = profile.audioChannels;
	pAudioCodecCtx->sample_fmt = SAMPLE_FMT_S16;

	if (avcodec_open(pAudioCodecCtx, pAudioCodec) < 0)
		return ERR_OPEN_AUDIO_CODEC;
	
	return 0;
}


int FFMpegEncoder::write_audio_frame(AVFormatContext *oc, AVStream *st,short *samples)
{
    AVCodecContext *c;
    AVPacket pkt;
    av_init_packet(&pkt);

    c = st->codec;

	pkt.size= this->audioEncodedSize;

    if (c->coded_frame->pts != AV_NOPTS_VALUE)
        pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
    pkt.flags |= PKT_FLAG_KEY;
    pkt.stream_index= st->index;
    pkt.data= (uint8_t*)samples;

    if (av_interleaved_write_frame(oc, &pkt) != 0)
		return ERR_WRITE_AUDIO_FRAME;
	else
		return 0;
}


int FFMpegEncoder::configVideoStream()
{
	if (strcmp(profile.videoCodec,"NONE") == 0)
		return 0;

    AVStream *st = av_new_stream(pFormatCtx, 0);
    if (!st) return ERR_CREATE_VIDEO_STREAM;
	videoStream = st;
	avcodec_get_context_defaults2(st->codec, CODEC_TYPE_VIDEO);

	AVCodec *pVideoCodec = avcodec_find_encoder_by_name(profile.videoCodec);
	if (!pVideoCodec) 
		return ERR_FIND_VIDEO_CODEC;
	
	pVideoCodecCtx = st->codec;
	pVideoCodecCtx->codec_id = pVideoCodec->id;
    pVideoCodecCtx->codec_type = CODEC_TYPE_VIDEO;
    pVideoCodecCtx->bit_rate = profile.videoBitrate;
	//pVideoCodecCtx->bit_rate_tolerance = profile.videoBitrate * profile.videoFrameRate;
	pVideoCodecCtx->width = profile.videoWidth;
	pVideoCodecCtx->height = profile.videoHeight;
	pVideoCodecCtx->time_base = av_d2q(1/profile.videoFrameRate,65535);
    pVideoCodecCtx->gop_size = 12; /* emit one intra frame every twelve frames at most */
    
	const PixelFormat *fmt = pVideoCodec->pix_fmts;
	pVideoCodecCtx->pix_fmt = *fmt;
	

	//ray: CBR
	
	/*
	pVideoCodecCtx->rc_min_rate = profile.videoBitrate;
	pVideoCodecCtx->rc_max_rate = profile.videoBitrate;  
	pVideoCodecCtx->bit_rate_tolerance = profile.videoBitrate;
	pVideoCodecCtx->rc_buffer_size = profile.videoBitrate;
	pVideoCodecCtx->rc_initial_buffer_occupancy = pVideoCodecCtx->rc_buffer_size*3/4;
	pVideoCodecCtx->rc_buffer_aggressivity= (float)1.0;
	pVideoCodecCtx->rc_initial_cplx= 0.5; 
*/
	//

	//ray: constant quality
//	pVideoCodecCtx->idct_algo = FF_IDCT_IPP;
	//pVideoCodecCtx->qmin = 51;
	//pVideoCodecCtx->qmax = 51;
	//
	if (pVideoCodecCtx->codec_id == CODEC_ID_MPEG4)
	{
		pVideoCodecCtx->max_b_frames = 2;
	}
    if (pVideoCodecCtx->codec_id == CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B frames */
        //pVideoCodecCtx->max_b_frames = 2;
    }
    if (pVideoCodecCtx->codec_id == CODEC_ID_MPEG1VIDEO){
        /* Needed to avoid using macroblocks in which some coeffs overflow.
           This does not happen with normal video, it just happens here as
           the motion of the chroma plane does not match the luma plane. */
       // pVideoCodecCtx->mb_decision=2;

    }
	if (pVideoCodecCtx->codec_id == CODEC_ID_MSMPEG4V3)
	{
		pVideoCodecCtx->max_b_frames = 0;
		pVideoCodecCtx->mb_decision = 2;
		pVideoCodecCtx->trellis = 2;
	}
    // some formats want stream headers to be separate
    if(pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        pVideoCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

	int ret = avcodec_open(pVideoCodecCtx, pVideoCodec);
	
	if ( ret < 0)
		return ERR_OPEN_VIDEO_CODEC;
	else
		return 0;
}
int FFMpegEncoder::write_video_frame(AVFormatContext *oc, AVStream *st,AVPicture *picture)
{
    int out_size, ret;
    AVCodecContext *c;
    static struct SwsContext *img_convert_ctx;

    c = st->codec; 
    AVPacket pkt;
    av_init_packet(&pkt);

    if (c->coded_frame->pts != AV_NOPTS_VALUE)
        pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
    if(c->coded_frame->key_frame)
        pkt.flags |= PKT_FLAG_KEY;
    pkt.stream_index= st->index;
    pkt.data= (uint8_t*)picture;
    
	
    ret = av_interleaved_write_frame(oc, &pkt);
       
    if (ret != 0) 
		return ERR_WRITE_VIDEO_FRAME;
	else
		return 0;
   
}
void FFMpegEncoder::finishEncode()
{
    av_write_trailer(pFormatCtx);

    if (videoStream)
		avcodec_close(videoStream->codec);
    if (audioStream)
		avcodec_close(audioStream->codec);
        
	unsigned int i;
    for(i = 0; i < pFormatCtx->nb_streams; i++) {
        av_freep(&pFormatCtx->streams[i]->codec);
        av_freep(&pFormatCtx->streams[i]);
    }

    url_fclose(pFormatCtx->pb);
    av_free(pFormatCtx);

	free(audioEncodeBuf);
	free(videoEncodeBuf);
}


int FFMpegEncoder::getAudioFrameSize()
{
	return pAudioCodecCtx->frame_size*pAudioCodecCtx->channels*2;
}

double FFMpegEncoder::getEncodedVideoPTS()
{
	if (pVideoCodecCtx->coded_frame->pts < 0)
		return -1;
	double timebase = av_q2d(pVideoCodecCtx->time_base);
	double pts = pVideoCodecCtx->coded_frame->pts * timebase;
	return pts;
}
/*
SimpleBuf FFMpegEncoder::getCodedBuffer()
{
	SimpleBuf buf;
	buf.data = this->videoEndodeBuf;
	return buf;
}
*/
AVFrame* FFMpegEncoder::getCodedVideoFrame()
{
	return pVideoCodecCtx->coded_frame;
}

int FFMpegEncoder::writePacket(AVPacket *pkt)
{
	int ret = av_interleaved_write_frame(pFormatCtx, pkt);
	return ret;
}

double FFMpegEncoder::getAudioEncodeFrameSec()
{
	return av_q2d(audioStream->time_base);
}


PixelFormat FFMpegEncoder::getPixelFormat()
{
	return pVideoCodecCtx->pix_fmt;
}

double FFMpegEncoder::getAudioPtsSec()
{
	return audioClock;
}

double FFMpegEncoder::getVideoPtsSec()
{
	return videoClock;
}
