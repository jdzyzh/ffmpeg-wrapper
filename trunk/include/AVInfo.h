#ifndef AVINFO_H
#define AVINFO_H

#include <libavutil/avutil.h>

typedef struct
{
	int64_t fileSize;
	double durationSec;

	int audioStreamIdx;
	int audioBitrate;
	int audioChannels;
	int audioSampleRate;
	int audioBitsPerSample;
	double audioTimeBase;

	int videoStreamIdx;
	int videoBitrate;
	double videoFrameRate;
	double videoTimeBase;
	int videoWidth;
	int videoHeight;
	long videoFrameCount;
	PixelFormat videoPixFmt;

}AVInfo;
#endif	//AVINFO_H
