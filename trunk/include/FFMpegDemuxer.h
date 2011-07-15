#pragma once

extern "C"
{
	#include <libavformat/avformat.h>
}

class FFMpegDemuxer
{
public:
	int init(const char* demuxerName);

protected:
	int pb_bufSize;
	char* pb_buf;

	static int MemReadFunc(void *opaque, uint8_t *buf, int buf_size);
	static int64_t MemSeekFunc(void *opaque, int64_t offset, int whence);
};

