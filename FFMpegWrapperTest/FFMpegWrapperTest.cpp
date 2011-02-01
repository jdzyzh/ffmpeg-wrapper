// SDLWrapper.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <SDL.h>
#include <FFMpegDecoder.h>
#include <FFMpegEncoder.h>
#include <FFMpegCodecDecoder.h>
#include <FFMpegCodecEncoder.h>
#include <FFMpegConverter.h>
#include <FFMpegCanvas.h>
#include <vector>
#include <windows.h>
#include <SDLWrapper.h>
#include <FFMpegCodecEncoder.h>
#include <FFMpegAudioConverter.h>
#include <fstream>
#include <FFMpegCWrapper.h>
#undef main


extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}

	
	

	
	/*
	for (int i=0;i<50;i+=5)
	{

		AVFrame *pFrame = dec.getFrameAtSec(i);
		
		overlay->pitches = (Uint16*)pFrame->linesize;
		overlay->pixels = pFrame->data;
		SDL_Rect rect = {0,0,info->videoWidth,info->videoHeight};
		SDL_DisplayYUVOverlay(overlay,&rect);

		
		enc.encodeVideoFrame(pFrame);
		SimpleBuf frame = enc.getEncodedVideoBuf();
		codedBuf.insert(codedBuf.end(),frame.data,frame.data+frame.dataSize);
	}
	
	int codedBufSize = codedBuf.size();
	FILE *f = fopen("d:\\test.mp2","wb");
	int ret = fwrite(&codedBuf[0], 1, codedBufSize, f);
	fclose(f);
	bool quit = false;
	SDL_Event event;
	while( quit == false )
    {
        while( SDL_PollEvent( &event ) )
        {
            if( event.type == SDL_QUIT )
            {
                quit = true;
            }
        }
    }

	//Free the loaded image
	if (overlay)
		SDL_FreeYUVOverlay(overlay);
	if (screen)
		SDL_FreeSurface( screen );

    //Quit SDL
    SDL_Quit();
*/



static SDLPlayer *player = NULL;
static void video_encode_example(const char *filename,char* codecName,int w,int h)
{
    AVCodec *codec;
    AVCodecContext *c= NULL;
    int i=0;
	int out_size=0;
	int size=0;
	int x=0;
	int y=0;
	int outbuf_size=0;

    FILE *f = NULL;
    AVFrame *picture = NULL;
    uint8_t *outbuf = NULL, *picture_buf = NULL;

    printf("Video encoding\n");

  
	outbuf_size = 100000;
    outbuf = (uint8_t*)malloc(outbuf_size);
	codec = avcodec_find_encoder_by_name(codecName);
  
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

    c= avcodec_alloc_context();
    picture= avcodec_alloc_frame();

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = w;
    c->height = h;
    /* frames per second */
	c->time_base.den = 25;
	c->time_base.num = 1;
    c->gop_size = 10; /* emit one intra frame every ten frames */
    c->max_b_frames=1;
    c->pix_fmt = PIX_FMT_YUV420P;

    /* open it */
    if (avcodec_open(c, codec) < 0) {
        fprintf(stderr, "could not open codec\n");
        exit(1);
    }

    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "could not open %s\n", filename);
        exit(1);
    }


	FFMpegDecoder dec;
	dec.openFile("d:\\av\\Dolphins_720.wmv");
	while (true)
	{
		AVFrame *frame = dec.getNextVideoFrame();
		if (!frame)
			break;

		out_size = avcodec_encode_video(c,outbuf,outbuf_size,frame);
		fwrite(outbuf,1,out_size,f);
	}
	
    
    /* add sequence end code to have a real mpeg file */
	/*
    outbuf[0] = 0x00;
    outbuf[1] = 0x00;
    outbuf[2] = 0x01;
    outbuf[3] = 0xb7;
    fwrite(outbuf, 1, 4, f);
	*/
    fclose(f);
    free(picture_buf);
    free(outbuf);

    avcodec_close(c);
    av_free(c);
    av_free(picture);
    printf("\n");
}

static void video_decode_example(const char *filename,char* codecName)
{
#define INBUF_SIZE 4096

    AVCodec *codec;
    AVCodecContext *c= NULL;
    int frame, got_picture, len;
    FILE *f;
    AVFrame *picture;
    uint8_t inbuf[INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];
    char buf[1024];
    AVPacket avpkt;

    av_init_packet(&avpkt);

    /* set end of buffer to 0 (this ensures that no overreading happens for damaged mpeg streams) */
    memset(inbuf + INBUF_SIZE, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    printf("Video decoding\n");

    /* find the mpeg1 video decoder */
    //codec = avcodec_find_decoder(CODEC_ID_MPEG1VIDEO);
	codec = avcodec_find_decoder_by_name(codecName);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

    c= avcodec_alloc_context();
	
    picture= avcodec_alloc_frame();

	
    if(codec->capabilities&CODEC_CAP_TRUNCATED)
        c->flags|= CODEC_FLAG_TRUNCATED; // we do not send complete frames
	

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */

    /* open it */
    if (avcodec_open(c, codec) < 0) {
        fprintf(stderr, "could not open codec\n");
        exit(1);
    }

    /* the codec gives us the frame size, in samples */

    f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "could not open %s\n", filename);
        exit(1);
    }

    frame = 0;
    for(;;) {
        avpkt.size = fread(inbuf, 1, INBUF_SIZE, f);
        if (avpkt.size == 0)
            break;

        /* NOTE1: some codecs are stream based (mpegvideo, mpegaudio)
           and this is the only method to use them because you cannot
           know the compressed data size before analysing it.

           BUT some other codecs (msmpeg4, mpeg4) are inherently frame
           based, so you must call them with all the data for one
           frame exactly. You must also initialize 'width' and
           'height' before initializing them. */

        /* NOTE2: some codecs allow the raw parameters (frame size,
           sample rate) to be changed at any frame. We handle this, so
           you should also take care of it */

        /* here, we use a stream based decoder (mpeg1video), so we
           feed decoder and see if it could decode a frame */
        avpkt.data = inbuf;
        while (avpkt.size > 0) {
            len = avcodec_decode_video2(c, picture, &got_picture, &avpkt);
            if (len < 0) {
                fprintf(stderr, "Error while decoding frame %d\n", frame);
                exit(1);
            }
            if (got_picture) {
                printf("saving frame %3d\n", frame);
                fflush(stdout);

                /* the picture is allocated by the decoder. no need to
                   free it */
				/*
                snprintf(buf, sizeof(buf), outfilename, frame);
                pgm_save(picture->data[0], picture->linesize[0],
                         c->width, c->height, buf);
				 */
				int w = c->width;
				int h = c->height;
				player->showPixels(picture->data,picture->linesize);
                frame++;
            }
            avpkt.size -= len;
            avpkt.data += len;
        }
    }

    /* some codecs, such as MPEG, transmit the I and P frame with a
       latency of one frame. You must do the following to have a
       chance to get the last frame of the video */
    avpkt.data = NULL;
    avpkt.size = 0;
    len = avcodec_decode_video2(c, picture, &got_picture, &avpkt);
    if (got_picture) {
        printf("saving last frame %3d\n", frame);
        fflush(stdout);

        /* the picture is allocated by the decoder. no need to
           free it */
		/*
        snprintf(buf, sizeof(buf), outfilename, frame);
        pgm_save(picture->data[0], picture->linesize[0],
                 c->width, c->height, buf);
		*/
        frame++;
    }

    fclose(f);

    avcodec_close(c);
    av_free(c);
    av_free(picture);
    printf("\n");
}
int video_decode_test(char* filename)
{
	FFMpegDecoder dec;
	AVInfo *info = dec.openFile(filename);
	
	while (true)
	{
		AVPacket *pkt = dec.readPacket();
		if (pkt == NULL)
			break;

		if (pkt->stream_index != info->videoStreamIdx)
			continue;

		AVFrame *frame = dec.decodeVideo();
		player->showPixels(frame->data,frame->linesize);
	}
	return 0;
}



int playVideoFile(const char* path)
{
	FFMpegDecoder dec;
	AVInfo *info = dec.openFile((char*)path);
	while (true)
	{
		AVPacket *pkt = dec.readPacket();
		if (pkt == NULL)
			break;
		if (pkt->stream_index != info->videoStreamIdx)
			continue;

		printf("flag=%d\n",pkt->flags);
		AVFrame *frame = dec.decodeVideo(pkt);
		if (frame != NULL)
		{
			printf("got frame,frame type=%d\n",frame->pict_type);
		}
		else
			printf("no frame available\n");

		getchar();
	}
	return 0;
}

int testAudioConvert()
{
	FFMpegAudioConverter conv_32to16;
	conv_32to16.Setup(2,2,44100,44100,SAMPLE_FMT_S16,SAMPLE_FMT_S32);
	
	FILE *s32pcm = fopen("d:\\temp\\test_s32.pcm","rb");
	FILE *s16pcm = fopen("d:\\temp\\result_s16.pcm","wb");

	short samples_in[65536];
	short samples_out[65536];

	while (true)
	{
		int bytesRead = fread(samples_in,1,65536,s32pcm);
		if (bytesRead <= 0)
			break;

		int nSamples = bytesRead / (2 * 32 / 8);
		conv_32to16.Resample(samples_out,samples_in,nSamples);
		fwrite(samples_out,1,nSamples * 4,s16pcm);
	}
	fclose(s32pcm);
	fclose(s16pcm);

	return 0;
}

typedef struct 
{
	int size;
	char* dataBegin;
	char* data;
}MyMemBuf;

int ReadFunc(void* opaque,uint8_t *buf,int buf_size)
{
	MyMemBuf *memBuf = (MyMemBuf*)opaque;
	int bytesAvailable = memBuf->size - (memBuf->data - memBuf->dataBegin);
	int copySize = bytesAvailable >= buf_size ? buf_size : bytesAvailable;

	memcpy(buf,memBuf->data,copySize);
	memBuf->data += copySize;
	return copySize;
}

int64_t SeekFunc(void* opaque,int64_t offset,int whence)
{
	MyMemBuf *memBuf = (MyMemBuf*)opaque;

	if (whence == AVSEEK_SIZE)
	{
		return memBuf->size;
	}
	else if (whence == SEEK_SET)
	{
		if (offset <= memBuf->size)
			memBuf->data = memBuf->dataBegin + offset;
		else
			return -1;
	}
	else if (whence == SEEK_END)
	{
		char* ptr = memBuf->dataBegin + memBuf->size + offset;
		if (ptr >= memBuf->dataBegin && ptr <= memBuf->dataBegin + memBuf->size)
			memBuf->data = ptr;
		else
			return -1;
	}
	else if (whence == SEEK_CUR)
	{
		char* ptr = memBuf->data + offset;
		if (ptr >= memBuf->dataBegin && ptr <= memBuf->dataBegin + memBuf->size)
			memBuf->data = ptr;
		else
			return -1;
	}
	else
		return -1;

	return memBuf->data - memBuf->dataBegin;
}

int testWavParser()
{
	FILE *fout = fopen("d:\\temp\\convertResule.wav","wb");
	FILE *f = fopen("d:\\temp\\test_s32.wav","rb");
	
	fseek(f,0,SEEK_END);
	int fsize = ftell(f);
	fseek(f,0,SEEK_SET);
	
	FFMpegAudioConverter conv;
	
	int bufInLen = 65536;
	char *bufIn = (char*)malloc(bufInLen);
	int bufOutLen = bufInLen * 4;
	char *bufOut = (char*)malloc(bufOutLen);

	fread(bufIn,1,bufInLen,f);
	conv.SetupInputByWavHeader(bufIn,128);

	//write wav header
	char wavHeader[44];
	unsigned long ulSizeFake = 1024*1024*50;
	FFMpegAudioConverter::GenWavHeader(wavHeader,ulSizeFake,2,44100,16);
	fwrite(wavHeader,1,44,fout);
	//

	//write pcm data
	while(true)
	{
		int bytesRead = fread(bufIn,1,bufInLen,f);
		if (bytesRead <= 0)
			break;

		int bytesConverted = conv.DecodeAndResample(bufIn,bytesRead,bufOut,bufOutLen);
		fwrite(bufOut,1,bytesConverted,fout);
	}
	fclose(f);
	fclose(fout);
	
	return 0;
}


using namespace std;

#define LOGD printf
#define LOGE printf


int testDecode()
{

	
	//Setup JPEG codec
	AVCodec *codec = avcodec_find_decoder_by_name("mjpeg");
	AVCodecContext *ctx = avcodec_alloc_context();
	int ret = avcodec_open(ctx, codec);
	if (codec->capabilities & CODEC_CAP_TRUNCATED)
		ctx->flags |= CODEC_FLAG_TRUNCATED;

	int got_pic = -1;
	AVFrame picDecoded;
	AVPacket pkt;
	
	//
	const char* jpgFiles[1] = 
	{
		//"d:\\av\\720.jpg",
		"d:\\av\\desktop_800x600.jpg",
		//"d:\\av\\love001.jpg"
	};
	
	for (int j=0;j<1;j++)
	{
		//Prepare Jpg Buffer
		FILE *jpgFile = fopen(jpgFiles[j],"rb");
		fseek(jpgFile,0,SEEK_END);
		long jpgFileSize = ftell(jpgFile);
		fseek(jpgFile,0,SEEK_SET);
		void *jpgBuf = malloc(jpgFileSize);
		fread(jpgBuf,1,jpgFileSize,jpgFile);
		fclose(jpgFile);
		//

		//decode: jpgBuf ==> picDecoded->data[]
		pkt.data = (uint8_t*)jpgBuf;
		pkt.size = jpgFileSize;
		avcodec_decode_video2(ctx,&picDecoded,&got_pic,&pkt);
		
		
		//

		//output YUV image, 
		PixelFormat yuvFormat = ctx->pix_fmt;
		const char* yuvFormatName = avcodec_get_pix_fmt_name(yuvFormat);
		char yuvFilename[256];
		sprintf(yuvFilename,"d:\\temp\\ffmpeg_decode_result_%d_%dx%d.%s",j,picDecoded.linesize[0],ctx->height,yuvFormatName);
		FILE *yuvFile = fopen(yuvFilename,"wb");

		
		for (int i=0;i<3;i++)
		{
			int ret = fwrite(picDecoded.data[i],1,picDecoded.linesize[i] * ctx->height,yuvFile);
			printf("writing plane [%d]: %d bytes\n",i,ret/*picDecoded.linesize[i] * ctx->height*/);
		}
		
		/*
		AVCodec *encCodec = avcodec_find_encoder_by_name("rawvideo");
		AVCodecContext *encCtx = avcodec_alloc_context();
		encCtx->pix_fmt = ctx->pix_fmt;
		ret = avcodec_open(encCtx,encCodec);
		int encBufSize = ctx->width * ctx->height * 4;
		char *encBuf = (char*)malloc(encBufSize);
		encCtx->width = ctx->width;
		encCtx->height = ctx->height;
		int encRet = avcodec_encode_video(encCtx,(uint8_t*)encBuf,encBufSize,&picDecoded);
		fwrite(encBuf,1,encRet,yuvFile);
		*/

		fclose(yuvFile);
		//
	}

	return 0;
}

int main(int argc, _TCHAR* argv[])
{
	av_register_all();
	avcodec_register_all();
	
	//return myffmpeg_main();
	//return testDecode();

	FFMpegDecoder dec;
	AVInfo *pInfo = dec.openFile("d:\\av\\StarTrekTrailer_720p_H.264_AAC_Stereo.mkv");
	SDLPlayer *sdl = NULL;
	while (true)
	{
		AVPacket *pPacket = dec.readPacket();
		if (pPacket == NULL)
			break;
		if (pPacket->stream_index != pInfo->videoStreamIdx)
			continue;

		AVFrame *pFrame = dec.decodeVideo();
		if (pFrame)
		{
			printf("frame decoded!\n");
			if (sdl == NULL)
			{
				sdl = new SDLPlayer();
				sdl->CreateOverlay(/*pInfo->videoWidth*/1312,pInfo->videoHeight);
			}
			sdl->showPixels(pFrame->data,pFrame->linesize);
		}
	}


	
}

