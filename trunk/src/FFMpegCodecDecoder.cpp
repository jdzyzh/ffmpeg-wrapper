#include <FFMpegCodecDecoder.h>
#include <RealFFMpegCodecDecoder.h>

unsigned char* RealFFMpegCodecDecoder::genBMPHeader(int w,int h)
{
	unsigned char header[54] = 
	{
    0x42,        // identity : B
    0x4d,        // identity : M
    0, 0, 0, 0,  // file size
    0, 0,        // reserved1
    0, 0,        // reserved2
    54, 0, 0, 0, // RGB data offset
    40, 0, 0, 0, // struct BITMAPINFOHEADER size
    0, 0, 0, 0,  // bmp width
    0, 0, 0, 0,  // bmp height
    1, 0,        // planes
    24, 0,       // bit per pixel
    0, 0, 0, 0,  // compression
    0, 0, 0, 0,  // data size
    0, 0, 0, 0,  // h resolution
    0, 0, 0, 0,  // v resolution 
    0, 0, 0, 0,  // used colors
    0, 0, 0, 0   // important colors
  };

	memcpy(header+18,&w,4);
	memcpy(header+22,&h,4);
	
	unsigned char* header_ret = (unsigned char*)malloc(54);
	memcpy(header_ret,header,54);
	return header_ret;
}
RealFFMpegCodecDecoder::RealFFMpegCodecDecoder(char *_codecName)
{
	picture = avcodec_alloc_frame();
	ctx = NULL;
	converter = NULL;
	int ret = InitCodec(_codecName);
	fpsCounter.SetName(_codecName);
}

RealFFMpegCodecDecoder::~RealFFMpegCodecDecoder()
{
	avcodec_close(ctx);
	av_free(ctx);
	av_free(picture);

	if (converter)
		delete converter;

}

int RealFFMpegCodecDecoder::InitCodec(char *_codecName)
{
	avcodec_register_all();
	if (ctx)
	{
		avcodec_close(ctx);
		av_free(ctx);
	}
	
	codec = avcodec_find_decoder_by_name(_codecName);
	ctx = avcodec_alloc_context();
    
	int ret = avcodec_open(ctx, codec);

	
	if(codec->capabilities&CODEC_CAP_TRUNCATED)
        ctx->flags|= CODEC_FLAG_TRUNCATED;
	
	strcpy(m_codecName,codec->name);
	return ret;
}
AVFrame* RealFFMpegCodecDecoder::decode(unsigned char *encData, int encDataSize, int *encDataConsumed)
{

	unsigned char* inbuf_ptr = encData;
	int len = -1;
	int got_picture = 0;

	if (encDataConsumed)
		*encDataConsumed = 0;

    while (encDataSize > 0) 
	{
		fpsCounter.BeforeProcess();
        len = avcodec_decode_video(ctx, picture, &got_picture,
                                   inbuf_ptr, encDataSize);
		fpsCounter.AfterProcess();

		videoWidth = ctx->width;
		videoHeight = ctx->height;
		videoPixFormat = ctx->pix_fmt;

        if (len < 0) 
		{
            fprintf(stderr, "Error while decoding frame");
            return NULL;
        }

		encDataSize -= len;
		if (encDataConsumed)
			*encDataConsumed += len;
        inbuf_ptr += len;
		
        if (got_picture) 
		{
			fpsCounter.FrameFinished();
            return picture;
        }   
    }
	return NULL;
}

unsigned char* RealFFMpegCodecDecoder::decodeAsBGR888(unsigned char *encData, int encDataSize, int *encDataConsumed)
{
	AVFrame *pFrame = decode(encData,encDataSize,encDataConsumed);
	if (pFrame == NULL)
		return NULL;


	if (converter == NULL)
		converter = new RealFFMpegBitmapConverter(videoWidth,videoHeight,videoPixFormat,videoWidth,videoHeight,PIX_FMT_BGR24);
	else
		converter->initContext(videoWidth,videoHeight,videoPixFormat,videoWidth,videoHeight,PIX_FMT_BGR24);

	AVPicture *frameConverted = converter->convertVideo((AVPicture*)pFrame);

	int rgbDataSize = videoWidth*videoHeight*3;
	unsigned char* rgbData = (unsigned char*)malloc(rgbDataSize);
	memcpy(rgbData,frameConverted->data[0],rgbDataSize);
	return rgbData;
}

unsigned char* RealFFMpegCodecDecoder::decodeAsBMP(unsigned char *encData, int encDataSize, int *encDataConsumed)
{
	unsigned char* rgbData = this->decodeAsBGR888(encData,encDataSize,encDataConsumed);
	if (rgbData == NULL)
		return NULL;
	
	unsigned char header[54] = 
	{
    0x42,        // identity : B
    0x4d,        // identity : M
    0, 0, 0, 0,  // file size
    0, 0,        // reserved1
    0, 0,        // reserved2
    54, 0, 0, 0, // RGB data offset
    40, 0, 0, 0, // struct BITMAPINFOHEADER size
    0, 0, 0, 0,  // bmp width
    0, 0, 0, 0,  // bmp height
    1, 0,        // planes
    24, 0,       // bit per pixel
    0, 0, 0, 0,  // compression
    0, 0, 0, 0,  // data size
    0, 0, 0, 0,  // h resolution
    0, 0, 0, 0,  // v resolution 
    0, 0, 0, 0,  // used colors
    0, 0, 0, 0   // important colors
  };

	memcpy(header+18,&videoWidth,4);
	memcpy(header+22,&videoHeight,4);
	
	int rgbDataSize = videoWidth*videoHeight*3;
	unsigned char* bmpData = (unsigned char*)malloc(54+rgbDataSize);
	memcpy(bmpData,header,54);

	int rowSize = videoWidth*3;
	unsigned char* ptr = bmpData+54+rgbDataSize-rowSize;
	for (int i=0;i<videoHeight;i++)
	{
		memcpy(ptr,rgbData,rowSize);
		rgbData += rowSize;
		ptr -= rowSize;
	}
	return bmpData;

}
unsigned char* makeBMPFromRGB888(unsigned char* rgbData,int rgbDataSize,int w,int h)
{
	unsigned char header[54] = {
    0x42,        // identity : B
    0x4d,        // identity : M
    0, 0, 0, 0,  // file size
    0, 0,        // reserved1
    0, 0,        // reserved2
    54, 0, 0, 0, // RGB data offset
    40, 0, 0, 0, // struct BITMAPINFOHEADER size
    0, 0, 0, 0,  // bmp width
    0, 0, 0, 0,  // bmp height
    1, 0,        // planes
    24, 0,       // bit per pixel
    0, 0, 0, 0,  // compression
    0, 0, 0, 0,  // data size
    0, 0, 0, 0,  // h resolution
    0, 0, 0, 0,  // v resolution 
    0, 0, 0, 0,  // used colors
    0, 0, 0, 0   // important colors
  };

	memcpy(header+18,&w,4);
	memcpy(header+22,&h,4);
	
	unsigned char* bmpData = (unsigned char*)malloc(54+rgbDataSize);
	memcpy(bmpData,header,54);

	int rowSize = w*3;
	unsigned char* ptr = bmpData+54+rgbDataSize-rowSize;
	for (int i=0;i<h;i++)
	{
		memcpy(ptr,rgbData,rowSize);
		rgbData += rowSize;
		ptr -= rowSize;
	}
	//memcpy(bmpData+54,rgbData,rgbDataSize);
	return bmpData;
}

FFMpegCodecDecoder::FFMpegCodecDecoder(char *_codecName)
{
	_delegate = new RealFFMpegCodecDecoder(_codecName);
}

FFMpegCodecDecoder::~FFMpegCodecDecoder()
{
	delete ((RealFFMpegCodecDecoder*) _delegate);
}

int FFMpegCodecDecoder::InitCodec(char *_codecName)
{
	return ((RealFFMpegCodecDecoder*) _delegate)->InitCodec(_codecName);
}

FFMpegFrame FFMpegCodecDecoder::decode(unsigned char *encData, int encDataSize, int *encDataConsumed)
{
	AVFrame *pFrame = ((RealFFMpegCodecDecoder*) _delegate)->decode(encData,encDataSize,encDataConsumed);
	FFMpegFrame frame;
	for (int i=0;i<4;i++)
	{
		frame.data[i] = (char*)pFrame->data[i];
		frame.linesize[i] = pFrame->linesize[i];
	}
	frame.frameType = pFrame->type;

	return frame;
}

unsigned char* FFMpegCodecDecoder::decodeAsBGR888(unsigned char *encData, int encDataSize, int *encDataConsumed)
{
	return ((RealFFMpegCodecDecoder*) _delegate)->decodeAsBGR888(encData,encDataSize,encDataConsumed);
}

unsigned char* FFMpegCodecDecoder::decodeAsBMP(unsigned char *encData, int encDataSize, int *encDataConsumed)
{
	return ((RealFFMpegCodecDecoder*) _delegate)->decodeAsBMP(encData,encDataSize,encDataConsumed);
}


const char* FFMpegCodecDecoder::getFmtStr()
{
	PixelFormat fmt = ((RealFFMpegCodecDecoder*) _delegate)->videoPixFormat;
	return avcodec_get_pix_fmt_name(fmt);
}

int FFMpegCodecDecoder::getWidth()
{
	return ((RealFFMpegCodecDecoder*) _delegate)->videoWidth;
}

int FFMpegCodecDecoder::getHeight()
{
	return ((RealFFMpegCodecDecoder*) _delegate)->videoHeight;
}