#include "FFMpegConverter.h"
#include <algorithm>



FFMpegConverter::FFMpegConverter(
								 int _w1,int _h1,PixelFormat _f1,
								 int _w2,int _h2,PixelFormat _f2,unsigned char* dstBuf,int dstBufSize)
{
	img_convert_ctx = NULL;
	
	if (dstBuf == NULL)
	{
		scaleBufSize =  avpicture_get_size(_f2,_w2,_h2);
		scaleBuf = (unsigned char*)malloc(scaleBufSize);
		m_bAllocBuf = true;
	}
	else 
	{
		scaleBufSize = dstBufSize;
		scaleBuf = dstBuf;
		m_bAllocBuf = false;
	}

	pPicScaled = (AVPicture*)malloc(sizeof(AVPicture));
	avpicture_fill(pPicScaled,scaleBuf,_f2,_w2,_h2);

	initContext(_w1,_h1,_f1,_w2,_h2,_f2);
}

int FFMpegConverter::initContext(int _w1,int _h1,PixelFormat _f1,
								 int _w2,int _h2,PixelFormat _f2)
{
	w1 = _w1;
	h1 = _h1;
	f1 = _f1;
	w2 = _w2;
	h2 = _h2;
	f2 = _f2;

	img_convert_ctx = sws_getCachedContext(img_convert_ctx,
		w1, h1, f1,
		w2, h2, f2,SWS_BICUBIC, NULL, NULL, NULL);

	if (!img_convert_ctx)
		return -1;
	
	if (scaleBufSize < avpicture_get_size(f2, w2, h2))
	{
		free(scaleBuf);
		scaleBufSize = avpicture_get_size(f2,w2,h2);
		scaleBuf = (unsigned char*)malloc(scaleBufSize);
	}

	return 0;
}
FFMpegConverter::~FFMpegConverter(void)
{
	if (m_bAllocBuf)
		free(scaleBuf);
	av_free(pPicScaled);
	sws_freeContext(img_convert_ctx);
	
}


AVPicture *FFMpegConverter::convertVideo(AVPicture *pic)
{
	if (img_convert_ctx == NULL)
	{
		fprintf(stderr,"img_convert_ctx NULL!\n");
		return NULL;
	}
    int ret = sws_scale(img_convert_ctx,
		pic->data, pic->linesize,
		0, h1,
		pPicScaled->data, pPicScaled->linesize);

	if (ret > 0)
		return pPicScaled;
	else
		return NULL;
}

