#pragma once

#ifdef DLL_EXPORT
#define FFMPEGWRAPPER_API __declspec(dllexport)
#else
#define FFMPEGWRAPPER_API __declspec(dllimport)
#endif

extern "C"
{

#include <libavcodec/avcodec.h>
}
class FFMPEGWRAPPER_API FFMpegCanvas
{
public:
	FFMpegCanvas(int w,int h,PixelFormat _fmt);
	virtual ~FFMpegCanvas(void);

	void clear();
	int draw(PixelFormat fmt,AVPicture *pPic, int x, int y, int imageW,int imageH);

	AVPicture* getPicture();
	AVPicture* copyPicture();

public:
	int m_w,m_h;
	PixelFormat m_fmt;
	AVPicture m_picture;
};
