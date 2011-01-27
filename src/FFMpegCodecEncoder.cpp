#include "FFMpegCodecEncoder.h"


FFMpegCodecEncoder::FFMpegCodecEncoder()
{
	c = NULL;
	codec = NULL;
	picConv = NULL;
	encBufSize = 1024*1024;
	encBuf = (char*)malloc(encBufSize);
	avcodec_register_all();
}

int FFMpegCodecEncoder::InitCodec(const char *codecStr,FFMpegCodecEncoderParam *param)
{
	codec = avcodec_find_encoder_by_name(codecStr);
    if (!codec) 
	{
        fprintf(stderr, "codec not found\n");
        return -1;
    }

	c = avcodec_alloc_context();
	c->qmin = param->qmin;
	c->qmax = param->qmax;
    
	/* resolution must be a multiple of two */
	c->width = param->encodeWidth;
    c->height = param->encodeHeight;

	c->max_b_frames = param->max_bframes;
	c->pix_fmt = codec->pix_fmts[0];
	c->time_base.den = 24;
	c->time_base.num = 1;

	if (codec->id == CODEC_ID_H264)
	{
		c->gop_size = 200;
		c->bit_rate = 1024*1024*5;
		c->max_qdiff = 4;
		c->me_range = 16;
		c->qcompress = 0.6;
		c->keyint_min = 10;
		c->trellis = 0;
		c->level = 13;
		c->me_threshold = 7;
		c->thread_count = 2;
		c->qblur = 0.5;
		c->profile = FF_PROFILE_H264_BASELINE;
	}
    /* open it */
    if (avcodec_open(c, codec) < 0) {
        fprintf(stderr, "could not open codec\n");
        return -2;
    }

	//setup conversion context
	if (0 == strcmp("rgb888",param->inputPixelType))
	{
		picConv = new FFMpegConverter(
			param->inputWidth,param->inputHeight,PIX_FMT_RGB24,
			c->width,c->height,c->pix_fmt);
	}
	else if (0 == strcmp("bgr888",param->inputPixelType))
	{
		picConv = new FFMpegConverter(
			param->inputWidth,param->inputHeight,PIX_FMT_BGR24,
			c->width,c->height,c->pix_fmt);
	}

	//setup input buffers
	picSrc = (AVPicture*)malloc(sizeof(AVPicture));
	frameSrc = avcodec_alloc_frame();
	frameSrc->pts = 0;
	return 0;
}



FFMpegCodecEncoder::~FFMpegCodecEncoder()
{
	avcodec_close(c);
    av_free(c);
	free(encBuf);
}

int FFMpegCodecEncoder::Encode(void* inputBuf)
{
	avpicture_fill(picSrc,(uint8_t*)inputBuf,picConv->f1,picConv->w1,picConv->h1);
	AVPicture *picDst = picConv->convertVideo(picSrc);

	for (int i=0;i<3;i++)
	{
		frameSrc->data[i] = picDst->data[i];
		frameSrc->linesize[i] = picDst->linesize[i];
	}

	printf("encode begin! pts=%d\n",frameSrc->pts);
	int out_size = avcodec_encode_video(c, (uint8_t*)encBuf, encBufSize, frameSrc);
	printf("encode finish: out_size=%d\n",out_size);
	frameSrc->pts++;

	static FILE *fout = fopen("codecEncoder_output.raw","wb");
	fwrite(encBuf,1,out_size,fout);
	fflush(fout);

	return out_size;
}

char* FFMpegCodecEncoder::GetEncodeBuf()
{
	return encBuf;
}

int ffmpeg_jpeg_encode(unsigned char *srcBuf,unsigned char* dstBuf,int dstBufSize,PixelFormat srcPixFmt,int srcWidth,int srcHeight,int qvalue)
{
    AVCodec *codec;
    AVCodecContext *c= NULL;
    
    printf("Video encoding\n");

    /* find the mpeg1 video encoder */
    codec = avcodec_find_encoder(CODEC_ID_MJPEG);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        return -1;
    }

    c= avcodec_alloc_context();
	c->qmin = qvalue;
	c->qmax = qvalue;
    /* resolution must be a multiple of two */
    c->width = srcWidth;
    c->height = srcHeight;
    
	c->time_base.den = 25;
	c->time_base.num = 1;
    c->max_b_frames=0;
    c->pix_fmt = PIX_FMT_YUVJ420P;

    /* open it */
    if (avcodec_open(c, codec) < 0) {
        fprintf(stderr, "could not open codec\n");
        return -2;
    }

	//prepare colorspace conversion
	//TODO: factor to util.
	AVPicture *pPicSrc = (AVPicture*)malloc(sizeof(AVPicture));
	int srcBufSize =  avpicture_get_size(srcPixFmt,srcWidth,srcHeight);
	avpicture_fill(pPicSrc,srcBuf,srcPixFmt,srcWidth,srcHeight);

	AVFrame *pPicScaled = (AVFrame*)malloc(sizeof(AVFrame));
	int scaleBufSize =  avpicture_get_size(c->pix_fmt,srcWidth,srcHeight);
	unsigned char *scaleBuf = (unsigned char*)malloc(scaleBufSize);
	avpicture_fill((AVPicture*)pPicScaled,scaleBuf,c->pix_fmt,srcWidth,srcHeight);

    SwsContext *img_convert_ctx = sws_getContext(
		srcWidth, srcHeight, srcPixFmt,
		srcWidth, srcHeight, c->pix_fmt,
		SWS_BICUBIC, NULL, NULL, NULL);

	if (img_convert_ctx == NULL)
	{
		printf("can not create colorspace converter!\n");
		return -3;
	}

	int ret = sws_scale(img_convert_ctx,
		pPicSrc->data, pPicSrc->linesize,
		0, srcHeight,
		pPicScaled->data, pPicScaled->linesize);

	if (ret < 0)
	{
		printf("color space conversion failed!\n");
		return -4;
	}

	//encode
	int out_size = avcodec_encode_video(c, dstBuf, dstBufSize, pPicScaled);
    
	if (out_size < 0)
	{
		printf("encode failed!\n");
		return -5;
	}

    avcodec_close(c);
    av_free(c);

	av_free(pPicSrc);
    av_free(pPicScaled);
	free(scaleBuf);
 
	return out_size;
}

