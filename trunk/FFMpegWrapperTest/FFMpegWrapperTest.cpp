// SDLWrapper.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <SDL.h>
#include <FFMpegDecoder.h>
#include <FFMpegEncoder.h>
#include <FFMpegCodecDecoder.h>
#include <FFMpegConverter.h>
#include <FFMpegCanvas.h>
#include <vector>
#include <windows.h>
#include <SDLWrapper.h>

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

DWORD WINAPI encodeThread(void *param)
{
	bool bWriteToFile = false;
	bool bWriteToFifo = true;
	char* vCodec = "mpeg4";
	FILE *encFile = NULL;
	FFMpegFifo *fifo = (FFMpegFifo*)param;

	if (bWriteToFile)
	{
		encFile = fopen("d:\\temp\\encTest.mpg1","wb");
	}
	int codedBufSize = 1024000;
	unsigned char* codedBuf = (unsigned char*)malloc(codedBufSize);

	FFMpegDecoder dec;
	AVInfo *info = dec.openFile("d:\\av\\dolphins_720.wmv");

	FFMpegEncodeProfile *encProfile = FFMpegEncoder::createDefaultProfile();
	strcpy(encProfile->audioCodec,"mp2");
	strcpy(encProfile->videoCodec,vCodec);
	encProfile->videoWidth = info->videoWidth;
	encProfile->videoHeight = info->videoHeight;
	encProfile->videoFrameRate = 24;
	wcscpy(encProfile->outputFilename,L"d:\\test.avi");
	FFMpegEncoder enc(encProfile);

	
	long decCount = 0;
	while (true)
	{
		decCount++;
		AVPacket *pkt = dec.readPacket();
		if (pkt == NULL)
			break;
		if (pkt->stream_index != info->videoStreamIdx)
			continue;

		printf("progress: %.2f\n",info->videoTimeBase * pkt->pts);
		AVFrame *pFrame = dec.decodeVideo();
		int encRet = enc.encodeVideoFrame(pFrame,codedBuf,codedBufSize);
		
		if (bWriteToFile)
			fwrite(codedBuf,1,encRet,encFile);
		if (bWriteToFifo)
			fifo->write(codedBuf,encRet);

		printf("%s: write %d bytes\n",__FUNCTION__,encRet);
	}

	if (encFile)
		fclose(encFile);
	return 0;
}

class FFMpegVideoDecoder
{
//protected:
public:

	AVFrame *frame;
	AVCodecContext *ctx;
	AVCodec *codec;
	bool init;
	int m_w;
	int m_h;

public:
	FFMpegVideoDecoder(char* codecName)
	{
		m_w = -1;
		m_h = -1;
		avcodec_register_all();
		init = false;
		codec = avcodec_find_decoder_by_name(codecName);
		if (!codec)
		{
			fprintf(stderr,"can not find decoder for: %s!\n");
			return;
		}

		ctx = avcodec_alloc_context();
		frame = avcodec_alloc_frame();

		if(codec->capabilities & CODEC_CAP_TRUNCATED)
			ctx->flags|= CODEC_FLAG_TRUNCATED; // we do not send complete frames
		if (avcodec_open(ctx, codec) < 0) 
		{
			fprintf(stderr, "could not open codec\n");
			return;
		}

		init = true;
	}
	~FFMpegVideoDecoder()
	{
		if (ctx)
		{
			avcodec_close(ctx);
			av_free(ctx);
		}
		if (frame)
			av_free(frame);

	}

	int Decode(AVPacket *pkt,AVFrame **decodedFrame)
	{
		int got_pic = 0;
		int decRet = avcodec_decode_video2(ctx,frame,&got_pic,pkt);
		if (got_pic)
			*decodedFrame = frame;
		else
			*decodedFrame = NULL;
		return decRet;
	}

	int GetW()
	{
		return m_w;
	}
	int GetH()
	{
		return m_h;
	}
};

DWORD WINAPI decodeThread(void* param)
{
	FFMpegVideoDecoder vdec("mpeg4");
	FFMpegFifo *fifo = (FFMpegFifo*)param;
	FILE *decFile = NULL;
	bool bReadFromFifo = true;

	if (!bReadFromFifo)
		decFile = fopen("d:\\temp\\encTest.mpg1","rb");

	AVPacket avpkt;
	AVFrame *pFrame = NULL;
    av_init_packet(&avpkt);
	int inbuf_size = 4096+FF_INPUT_BUFFER_PADDING_SIZE;
	uint8_t *inbuf = (uint8_t*)malloc(inbuf_size);
	
	while (true)
	{
		if (bReadFromFifo)
			avpkt.size = fifo->read(inbuf,inbuf_size);
		else
			avpkt.size = fread(inbuf,1,inbuf_size,decFile);
		//fprintf(stderr,"%s: read %d bytes\n","decodeThread",avpkt.size);
		
		if (avpkt.size == 0)
			break;
		
		avpkt.data = inbuf;
		
		while (avpkt.size > 0)
		{

			int bytesConsumed = vdec.Decode(&avpkt,&pFrame);
			if (pFrame)
			{
				if (player->overlay == NULL)
					player->CreateOverlay(pFrame->linesize[0],720);
				player->showPixels(pFrame->data,pFrame->linesize);
			}
				

			avpkt.size -= bytesConsumed;
			avpkt.data += bytesConsumed;
		}
		

		
	}
	if (decFile)
		fclose(decFile);
	return 0;
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

int ffmpeg_main(int argc, char **argv);

int myffmpeg_main()
{
	int i=0;
	char **ffmpeg_opts = (char**)malloc(sizeof(char*) * 30);
	for (i=0;i<30;i++)
		ffmpeg_opts[i] = (char*)malloc(64);
	
	i = 0;
	ffmpeg_opts[i++] = "ffmpeg";
	ffmpeg_opts[i++] = "-i";
	/*
	ffmpeg_opts[i++] = "d:\\av\\720.jpg";
	ffmpeg_opts[i++] = "-an";
	ffmpeg_opts[i++] = "-vcodec";
	ffmpeg_opts[i++] = "mjpeg";
	ffmpeg_opts[i++] = "-cropleft";
	ffmpeg_opts[i++] = "300";
	ffmpeg_opts[i++] = "-cropright";
	ffmpeg_opts[i++] = "300";
	ffmpeg_opts[i++] = "-croptop";
	ffmpeg_opts[i++] = "300";
	ffmpeg_opts[i++] = "-cropbottom";
	ffmpeg_opts[i++] = "300";
	ffmpeg_opts[i++] = "d:\\temp\\test.jpg";
	*/

	ffmpeg_opts[i++] = "d:\\av\\256x192.jpg";
	ffmpeg_opts[i++] = "-an";
	ffmpeg_opts[i++] = "-vcodec";
	ffmpeg_opts[i++] = "mjpeg";
	ffmpeg_opts[i++] = "-padleft";
	ffmpeg_opts[i++] = "300";
	ffmpeg_opts[i++] = "-padtop";
	ffmpeg_opts[i++] = "300";
	ffmpeg_opts[i++] = "d:\\temp\\test.jpg";

	return ffmpeg_main(i,ffmpeg_opts);

}


int main(int argc, _TCHAR* argv[])
{
	//return myffmpeg_main();
	//FILE *f = fopen("d:\\av\\jpegcd_firstFrame.jpg","rb");
	FILE *f = fopen("d:\\temp\\frame_jpegcd_0831.jpg","rb");
	fseek(f,0,SEEK_END);
	int size = ftell(f);
	fseek(f,0,SEEK_SET);
	unsigned char* encData = (unsigned char*)malloc(size);
	fread(encData,1,size,f);
	fclose(f);


	avcodec_register_all();
	FFMpegCodecDecoder dec("mjpeg");
	int sizeConsumed = 0;
	
	/*
	unsigned char* bmpData = dec.decodeAsBMP(encData,size,&sizeConsumed);
	int rgbDataSize = dec.videoWidth*dec.videoHeight*3;
	f = fopen("d:\\temp\\test.bmp","wb");
	fwrite(bmpData,1,rgbDataSize+54,f);
	fclose(f);
	*/

	int x = 0;
	int y = 0;
	
	int contextW = 1024;
	int contextH = 768;
	PixelFormat contextFMT = PIX_FMT_NONE;

	AVFrame *pContextFrame = avcodec_alloc_frame();
	avpicture_alloc((AVPicture*)pContextFrame,contextFMT,contextW,contextH);

	AVFrame *pFrame = dec.decode(encData,size,&sizeConsumed);
	contextFMT = dec.videoPixFormat;
	FFMpegCanvas canvas(contextW,contextH,contextFMT);
	canvas.draw(dec.videoPixFormat,(AVPicture*)pFrame,x,y,dec.videoWidth,dec.videoHeight);

	FFMpegConverter converter(contextW,contextH,contextFMT,contextW,contextH,PIX_FMT_YUV420P);
	AVPicture *pFrameConverted = converter.convertVideo(canvas.getPicture());


	SDLPlayer player;
	player.CreateOverlay(1024,768);
	player.showPixels(pFrameConverted->data,pFrameConverted->linesize);
	player.StartEventLoop();
		
}

