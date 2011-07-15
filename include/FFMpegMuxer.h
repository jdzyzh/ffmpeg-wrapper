#pragma once

extern "C"
{
	#include <libavformat/avformat.h>
}

class FFMpegMuxer
{
public:
	int init(const char* muxerName,const char* outputName);
	int addVideoStream(int streamIndex,int width,int height,int bitrate,int fps);
	int beginMux();
	int addVideoFrame(void* encodedData,int encodedDataSize,int64_t pts,bool isKeyFrame);
	int endMux();
	
public:
	AVFormatContext *formatCtx;
	AVFormatContext *rtpCtx;
	ByteIOContext *ioctx;
	AVOutputFormat *fmt;
	AVStream *videoStream;
	int videoFPS;
	
};
