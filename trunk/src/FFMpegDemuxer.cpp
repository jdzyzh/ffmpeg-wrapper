#include "FFMpegDemuxer.h"

int FFMpegDemuxer::MemReadFunc(void *opaque, uint8_t *buf, int buf_size)
{
	FFMpegDemuxer *pSelf = (FFMpegDemuxer*)opaque;
	
}

int FFMpegDemuxer::init(const char* demuxerName)
{

	AVInputFormat *inputFmt = av_find_input_format(demuxerName);
	if (inputFmt == NULL)
		return -1;

	
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	pFormatCtx->iformat = inputFmt;

	ByteIOContext *pb = (ByteIOContext*)malloc(sizeof(ByteIOContext);
	init_put_byte(pb,pb_buf,pb_bufSize,0,opaque,pb_readFunc,pb_writeFunc,pb_seekFunc);
	int ret = av_open_input_stream(&pFormatCtx,pb,NULL,inputFmt,NULL);

	return 0;
}