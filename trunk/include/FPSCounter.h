#pragma once

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}

class FPSCounter
{
public:

	FPSCounter() 
	{
		SetUpdateInterval(1000);
		Reset();
	};
	~FPSCounter() {};

	void SetUpdateInterval(int msec)
	{
		msUpdateInterval = msec;
	}

	void BeforeProcess()
	{
		t1 = av_gettime();
	}

	void AfterProcess()
	{
		t2 = av_gettime();
		msTimeElapsed += t2-t1;
		fps = (float)frameCount * 1000000 / msTimeElapsed;

		if (msTimeElapsed > msUpdateInterval)
		{
			Report();
			Reset();
		}
	}
	void FrameFinished()
	{
		frameCount++;
	}
	void Reset()
	{
		msTimeElapsed = 0;
		frameCount = 0;
		fps = 0;
	}
	
	void Report()
	{
		printf("FPS:%.2f\n",fps);
	}
protected:
	int64_t t1,t2,msTimeElapsed,msUpdateInterval,frameCount;
	float fps;
};
