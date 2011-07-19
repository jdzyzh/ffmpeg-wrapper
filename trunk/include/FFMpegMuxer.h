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
	
	void setRTPOutputAddr(const char* addr);
	void setRTPOutputPort(int port);

public:
	char rtpOutputAddr[64];
	int rtpOutputPort;

	AVFormatContext *formatCtx;
	AVFormatContext *rtpCtx;
	ByteIOContext *ioctx;
	uint8_t *mpegtsOutputBuf;
	AVOutputFormat *fmt;
	AVStream *videoStream;
	int videoFPS;
	
};
