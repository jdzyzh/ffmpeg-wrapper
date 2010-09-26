#include "FFMpegVideoDecoder.h"


FFMpegVideoDecoder::FFMpegVideoDecoder(char* codecName)
{
	codec =avcodec_find_decoder_by_name(codecName);
	if (!codec)
	{
		fprintf(stderr,"failed to find decoder: %s\n",codecName);
	}

	ctx = avcodec_alloc_context();
    decodedFrame = avcodec_alloc_frame();
	avcodec_open(ctx,codec);

	AVPacket avpkt;

    av_init_packet(&avpkt);

}

int FFMpegVideoDecoder::decode(unsigned char* inBuf,int inBufSize,unsigned char* outBuf,int outBufSize)
{
	int gotPic = 0;
	int ret = avcodec_decode_video(ctx,decodedFrame,&gotPic,inBuf,inBufSize);
	return ret;

}