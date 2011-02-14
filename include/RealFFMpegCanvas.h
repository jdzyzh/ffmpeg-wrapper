#pragma once

extern "C"
{
	#include <libavcodec/avcodec.h>
}

class RealFFMpegCanvas
{
public:
	RealFFMpegCanvas(int w,int h,PixelFormat _fmt);
	virtual ~RealFFMpegCanvas(void);

	void clear();
	int draw(PixelFormat fmt,AVPicture *pPic, int x, int y, int imageW,int imageH);

	AVPicture* getPicture();
	AVPicture* copyPicture();

public:
	int m_w,m_h;
	PixelFormat m_fmt;
	AVPicture m_picture;
};
