#include "FFMpegCodecEncoder.h"

int FFMpegCodecEncoder::SetCodecParams(int w,int h,int qvalue,PixelFormat fmt)
{
	if (c != NULL)
		avcodec_close(c);
	
	c= avcodec_alloc_context();
	c->qmin = qvalue;
	c->qmax = qvalue;
	/* resolution must be a multiple of two */
	c->width = w;
	c->height = h;
    
	c->time_base.den = 25;
	c->time_base.num = 1;
	c->max_b_frames=0;
	c->pix_fmt = fmt;

    /* open it */
    if (avcodec_open(c, codec) < 0) 
	{
        fprintf(stderr, "could not open codec\n");
        return -2;
    }

	return 0;
}

int FFMpegCodecEncoder::InitCodec(const char *codecStr)
{
	codec = avcodec_find_encoder_by_name(codecStr);
    if (!codec) 
	{
        fprintf(stderr, "codec not found\n");
        return -1;
    }

	return 0;
}

FFMpegCodecEncoder::FFMpegCodecEncoder()
{
	c = NULL;
	codec = NULL;
	encBufSize = 1024*1024;
	encBuf = (char*)malloc(encBufSize);
}

FFMpegCodecEncoder::~FFMpegCodecEncoder()
{
	avcodec_close(c);
    av_free(c);
	free(encBuf);
}

int FFMpegCodecEncoder::Encode(AVFrame *pFrame)
{
	int out_size = avcodec_encode_video(c, (uint8_t*)encBuf, encBufSize, pFrame);
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

//wait to be implemented
/*
FFMpegCodecEncoder::FFMpegCodecEncoder(char *_codecName)
{
	picture = avcodec_alloc_frame();
	ctx = NULL;
	converter = NULL;
	int ret = InitCodec(_codecName);
}

FFMpegCodecEncoder::~FFMpegCodecEncoder()
{
	avcodec_close(ctx);
	av_free(ctx);
	//av_free(picture);

	if (converter)
		delete converter;

}

int FFMpegCodecEncoder::InitCodec(char *_codecName)
{
	avcodec_register_all();
	if (ctx)
	{
		avcodec_close(ctx);
		av_free(ctx);
	}
	
	codec = avcodec_find_decoder_by_name(_codecName);
	ctx = avcodec_alloc_context();
    
	int ret = avcodec_open(ctx, codec);
	strcpy(m_codecName,codec->name);
	return ret;
}
unsigned char* FFMpegCodecEncoder::encode(AVPicture *pPic)
{
	return NULL;
}
*/
