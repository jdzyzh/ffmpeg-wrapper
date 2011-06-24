#include <RealFFMpegBitmapConverter.h>
#include <FFMpegBitmapConverter.h>

RealFFMpegBitmapConverter::RealFFMpegBitmapConverter(
								 int _w1,int _h1,PixelFormat _f1,
								 int _w2,int _h2,PixelFormat _f2,unsigned char* dstBuf,int dstBufSize)
{
	img_convert_ctx = NULL;
	
	if (dstBuf == NULL)
	{
		scaleBufSize =  avpicture_get_size(_f2,_w2,_h2);
		scaleBuf = (unsigned char*)malloc(scaleBufSize);
		if (scaleBuf == NULL)
		{
			printf("oops!!! scaleBuf == NULL!\n");
		}
		printf("FFMpegBitMapConverter alloc: dstBuf=%d bytes\n",scaleBufSize);
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

int RealFFMpegBitmapConverter::initContext(int _w1,int _h1,PixelFormat _f1,
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
RealFFMpegBitmapConverter::~RealFFMpegBitmapConverter(void)
{
	if (m_bAllocBuf)
		free(scaleBuf);
	free(pPicScaled);
	sws_freeContext(img_convert_ctx);
	
}


AVPicture *RealFFMpegBitmapConverter::convertVideo(AVPicture *pic)
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


FFMpegBitmapConverter::FFMpegBitmapConverter(int w1,int h1,char* fmt1,int w2,int h2, char* fmt2,unsigned char *dstBuf,int dstBufSize)
{
	PixelFormat f1 = avcodec_get_pix_fmt(fmt1);
	PixelFormat f2 = avcodec_get_pix_fmt(fmt2);

	_delegate = new RealFFMpegBitmapConverter(w1,h1,f1,w2,h2,f2,dstBuf,dstBufSize);
}

FFMpegBitmapConverter::~FFMpegBitmapConverter()
{
	delete ((RealFFMpegBitmapConverter*) _delegate);
}

FFMpegFrame FFMpegBitmapConverter::convert(FFMpegFrame *pSrcFrame)
{
	FFMpegFrame frame;
	
	AVPicture *picDst = ((RealFFMpegBitmapConverter*) _delegate)->convertVideo((AVPicture*)pSrcFrame);
	for (int i=0;i<4;i++)
	{
		frame.data[i] = (char*)picDst->data[i];
		frame.linesize[i] = picDst->linesize[i];
	}

	return frame;
}
