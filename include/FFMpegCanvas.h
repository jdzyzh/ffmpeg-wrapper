#pragma once

#include "FFMpegWrapperAPI.h"


class FFMPEGWRAPPER_API FFMpegCanvas
{
public:
	FFMpegCanvas(int w,int h,char* _fmt);
	~FFMpegCanvas(void);

	void clear();
	int draw(char *fmtStr, FFMpegFrame *pFrame, int x, int y, int imageW,int imageH);
	FFMpegFrame getPicture();

	const char* getFmtStr();
	int getWidth();
	int getHeight();

public:
	void* _delegate;
};
