#pragma once

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libswscale/swscale.h>
}

#include "FPSCounter.h"

class RealFFMpegBitmapConverter
{
public:
	RealFFMpegBitmapConverter(int w1,int h1,PixelFormat f1,
					int w2,int h2,PixelFormat f2,unsigned char* dstBuf=NULL,int dstBufSize=0);
	~RealFFMpegBitmapConverter(void);

	int initContext(int w1,int h1,PixelFormat f1,
					int w2,int h2,PixelFormat f2);
	AVPicture* convertVideo(AVPicture *picSrc);


	SwsContext		*img_convert_ctx;
	int				scaleBufSize;
	unsigned char	*scaleBuf;
	AVPicture		*pPicScaled;

	bool m_bAllocBuf;
	int w1,w2,h1,h2;
	PixelFormat f1,f2;
	
	FPSCounter fpsCounter;

};
