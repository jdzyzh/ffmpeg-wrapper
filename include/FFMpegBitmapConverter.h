#ifndef _H_FFMPEG_CONVERTER
#define _H_FFMPEG_CONVERTER

#include "FFMpegWrapperAPI.h"


class FFMPEGWRAPPER_API FFMpegBitmapConverter
{
public:
	FFMpegBitmapConverter(int w1,int h1,char *fmt1,int w2,int h2, char* fmt2,unsigned char* dstBuf=0,int dstBufSize=0);
	~FFMpegBitmapConverter();

	FFMpegFrame convert(FFMpegFrame *pSrcFrame);

	void* _delegate;
};



#endif
