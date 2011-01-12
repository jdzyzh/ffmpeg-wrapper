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

#define PIXELSIZE3				3
#define PIXELSIZE4				4
#define TWO_PIXELSIZE4			8
#define TWO_BYTE				2

#define BY2U(BB, YY)			BY2Utable[(BB - YY) + 255] 
#define RY2V(RR, YY)			RY2Vtable[(RR - YY) + 255] 

unsigned char BY2Utable[511] =
{ 
	3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
	8, 8, 9, 9, 10, 10, 11, 11, 12, 12,
	13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
	18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
	23, 23, 24, 24, 25, 25, 26, 26, 27, 27,
	28, 28, 29, 29, 30, 30, 31, 31, 32, 32,
	33, 33, 34, 34, 34, 35, 35, 36, 36, 37,
	37, 38, 38, 39, 39, 40, 40, 41, 41, 42,
	42, 43, 43, 44, 44, 45, 45, 46, 46, 47,
	47, 48, 48, 49, 49, 50, 50, 51, 51, 52,
	52, 53, 53, 54, 54, 55, 55, 56, 56, 57,
	57, 58, 58, 59, 59, 60, 60, 61, 61, 62,
	62, 63, 63, 64, 64, 65, 65, 66, 66, 66,
	67, 67, 68, 68, 69, 69, 70, 70, 71, 71,
	72, 72, 73, 73, 74, 74, 75, 75, 76, 76,
	77, 77, 78, 78, 79, 79, 80, 80, 81, 81,
	82, 82, 83, 83, 84, 84, 85, 85, 86, 86,
	87, 87, 88, 88, 89, 89, 90, 90, 91, 91,
	92, 92, 93, 93, 94, 94, 95, 95, 96, 96,
	97, 97, 97, 98, 98, 99, 99, 100, 100, 101,
	101, 102, 102, 103, 103, 104, 104, 105, 105, 106,
	106, 107, 107, 108, 108, 109, 109, 110, 110, 111,
	111, 112, 112, 113, 113, 114, 114, 115, 115, 116,
	116, 117, 117, 118, 118, 119, 119, 120, 120, 121,
	121, 122, 122, 123, 123, 124, 124, 125, 125, 126,
	126, 127, 127, 128, 128, 128, 129, 129, 130, 130,
	131, 131, 132, 132, 133, 133, 134, 134, 135, 135,
	136, 136, 137, 137, 138, 138, 139, 139, 140, 140,
	141, 141, 142, 142, 143, 143, 144, 144, 145, 145,
	146, 146, 147, 147, 148, 148, 149, 149, 150, 150,
	151, 151, 152, 152, 153, 153, 154, 154, 155, 155,
	156, 156, 157, 157, 158, 158, 159, 159, 159, 160,
	160, 161, 161, 162, 162, 163, 163, 164, 164, 165,
	165, 166, 166, 167, 167, 168, 168, 169, 169, 170,
	170, 171, 171, 172, 172, 173, 173, 174, 174, 175,
	175, 176, 176, 177, 177, 178, 178, 179, 179, 180,
	180, 181, 181, 182, 182, 183, 183, 184, 184, 185,
	185, 186, 186, 187, 187, 188, 188, 189, 189, 190,
	190, 191, 191, 191, 192, 192, 193, 193, 194, 194,
	195, 195, 196, 196, 197, 197, 198, 198, 199, 199,
	200, 200, 201, 201, 202, 202, 203, 203, 204, 204,
	205, 205, 206, 206, 207, 207, 208, 208, 209, 209,
	210, 210, 211, 211, 212, 212, 213, 213, 214, 214,
	215, 215, 216, 216, 217, 217, 218, 218, 219, 219,
	220, 220, 221, 221, 222, 222, 222, 223, 223, 224,
	224, 225, 225, 226, 226, 227, 227, 228, 228, 229,
	229, 230, 230, 231, 231, 232, 232, 233, 233, 234,
	234, 235, 235, 236, 236, 237, 237, 238, 238, 239,
	239, 240, 240, 241, 241, 242, 242, 243, 243, 244,
	244, 245, 245, 246, 246, 247, 247, 248, 248, 249,
	249, 250, 250, 251, 251, 252, 252, 253, 253
};


unsigned char RY2Vtable[511] =
{
	161, 162, 163, 164, 165, 166, 167, 167, 168, 169, 170,
	171, 172, 173, 174, 174, 175, 176, 177, 178, 179,
	180, 181, 181, 182, 183, 184, 185, 186, 187, 188,
	189, 189, 190, 191, 192, 193, 194, 195, 196, 196,
	197, 198, 199, 200, 201, 202, 203, 203, 204, 205,
	206, 207, 208, 209, 210, 210, 211, 212, 213, 214,
	215, 216, 217, 217, 218, 219, 220, 221, 222, 223,
	224, 224, 225, 226, 227, 228, 229, 230, 231, 231,
	232, 233, 234, 235, 236, 237, 238, 239, 239, 240,
	241, 242, 243, 244, 245, 246, 246, 247, 248, 249,
	250, 251, 252, 253, 253, 254, 255, 0, 0, 1,
	2, 3, 3, 4, 5, 6, 7, 8, 9, 10,
	10, 11, 12, 13, 14, 15, 16, 17, 17, 18,
	19, 20, 21, 22, 23, 24, 24, 25, 26, 27,
	28, 29, 30, 31, 32, 32, 33, 34, 35, 36,
	37, 38, 39, 39, 40, 41, 42, 43, 44, 45,
	46, 46, 47, 48, 49, 50, 51, 52, 53, 53,
	54, 55, 56, 57, 58, 59, 60, 60, 61, 62,
	63, 64, 65, 66, 67, 67, 68, 69, 70, 71,
	72, 73, 74, 74, 75, 76, 77, 78, 79, 80,
	81, 82, 82, 83, 84, 85, 86, 87, 88, 89,
	89, 90, 91, 92, 93, 94, 95, 96, 96, 97,
	98, 99, 100, 101, 102, 103, 103, 104, 105, 106,
	107, 108, 109, 110, 110, 111, 112, 113, 114, 115,
	116, 117, 117, 118, 119, 120, 121, 122, 123, 124,
	124, 125, 126, 127, 128, 129, 130, 131, 132, 132,
	133, 134, 135, 136, 137, 138, 139, 139, 140, 141,
	142, 143, 144, 145, 146, 146, 147, 148, 149, 150,
	151, 152, 153, 153, 154, 155, 156, 157, 158, 159,
	160, 160, 161, 162, 163, 164, 165, 166, 167, 167,
	168, 169, 170, 171, 172, 173, 174, 174, 175, 176,
	177, 178, 179, 180, 181, 182, 182, 183, 184, 185,
	186, 187, 188, 189, 189, 190, 191, 192, 193, 194,
	195, 196, 196, 197, 198, 199, 200, 201, 202, 203,
	203, 204, 205, 206, 207, 208, 209, 210, 210, 211,
	212, 213, 214, 215, 216, 217, 217, 218, 219, 220,
	221, 222, 223, 224, 224, 225, 226, 227, 228, 229,
	230, 231, 232, 232, 233, 234, 235, 236, 237, 238,
	239, 239, 240, 241, 242, 243, 244, 245, 246, 246,
	247, 248, 249, 250, 251, 252, 253, 253, 254, 255,
	0, 1, 2, 3, 4, 4, 5, 6, 7, 8,
	9, 10, 11, 11, 12, 13, 14, 15, 16, 17,
	18, 18, 19, 20, 21, 22, 23, 24, 25, 26,
	26, 27, 28, 29, 30, 31, 32, 33, 33, 34,
	35, 36, 37, 38, 39, 40, 40, 41, 42, 43,
	44, 45, 46, 47, 47, 48, 49, 50, 51, 52,
	53, 54, 54, 55, 56, 57, 58, 59, 60, 61,
	61, 62, 63, 64, 65, 66, 67, 68, 68, 69,
	70, 71, 72, 73, 74, 75, 76, 76, 77, 78,
	79, 80, 81, 82, 83, 83, 84, 85, 86, 87,
	88, 89, 90, 90, 91, 92, 93, 94, 95
};

int testColorSpaceConversion(const void *pRGBA, int width, int height, void *pY, void *pU, void *pV)
{
	int i, j;	
	int widthStep;
	unsigned char *movRGBA;
	unsigned char R, G, B;	
	unsigned char *cpY, *cpU, *cpV;
	unsigned char Y;

	cpY = (unsigned char*)pY;			

	widthStep = ((width*PIXELSIZE4 + 3)/PIXELSIZE4 )*PIXELSIZE4;
		
	for(j = 0; j< height; j++){		
		movRGBA = (unsigned char*)pRGBA + j * widthStep;

		for(i = 0; i< width; i++){
			R = *movRGBA; G = *(movRGBA + 1); B = *(movRGBA + 2);	

			*cpY = (R*38 + G*75 + B*15) >> 7;
			movRGBA += PIXELSIZE4; 
			cpY++;			

		}/*for i */		
	}/*for j*/

	
	cpU = (unsigned char*)pU;	
	cpV = (unsigned char*)pV;

	for(j = 0; j< height; j+= 2){		
		movRGBA = (unsigned char*)pRGBA + j * widthStep;
		cpY = (unsigned char*)pY + j * width ;	
		for(i = 0; i< width; i+= 2){
			R = *movRGBA; G = *(movRGBA + 1); B = *(movRGBA + 2);							
			Y = *cpY;
			*cpU = BY2U(B, Y);
			*cpV = RY2V(R, Y);	


			movRGBA += TWO_PIXELSIZE4;
			cpV++; 			
			cpU++;			
			cpY+= TWO_BYTE;
		}/*for i */		
	}/*for j*/

	return 0;
	return 0;
}

using namespace std;
int main(int argc, _TCHAR* argv[])
{
	av_register_all();
	avcodec_register_all();
	

/*
	FILE *frgb = fopen("d:\\temp\\test.bin","rb");
	fseek(frgb,0,SEEK_END);
	int fsize = ftell(frgb);
	fseek(frgb,0,SEEK_SET);
	unsigned char *srcBuf = (unsigned char*)malloc(fsize);
	fread(srcBuf,1,fsize,frgb);
	fclose(frgb);

	int dstBufSize = 1024*1024*1;
	unsigned char *dstBuf = (unsigned char*)malloc(dstBufSize);

	int ret = ffmpeg_jpeg_encode(srcBuf,dstBuf,dstBufSize,PIX_FMT_RGB565LE,1280,720,1);
	FILE *fjpg = fopen("d:\\temp\\test.jpg","wb");
	fwrite(dstBuf,1,dstBufSize,fjpg);
	fclose(fjpg);
	return ret;

	return playVideoFile("d:\\av\\StarTrekTrailer_720p_H.264_AAC_Stereo.mkv");
	//return myffmpeg_main();
	//FILE *f = fopen("d:\\av\\jpegcd_firstFrame.jpg","rb");
	FILE *f = fopen("d:\\temp\\frame_jpegcd_0831.jpg","rb");
	fseek(f,0,SEEK_END);
	int size = ftell(f);
	fseek(f,0,SEEK_SET);
	unsigned char* encData = (unsigned char*)malloc(size);
	fread(encData,1,size,f);
	fclose(f);

*/

	int picW = 800;
	int picH = 480;
	const char* picFile = "d:\\av\\800x480.rgba";

	//Prepare RGBA data
	fstream fs;
	fs.open(picFile,ios::binary | ios::in);
	fs.seekg(0,fstream::end);
	unsigned long fsize = fs.tellg();
	fs.seekg(0,fstream::beg);
	char *rgbaBuf = (char*)malloc(fsize);
	fs.read(rgbaBuf,fsize);
	fs.close();
	//

	//ColorSpace Conversion 1
	AVPicture frame420;
	avpicture_alloc(&frame420,PIX_FMT_YUV420P,picW,picH);
	testColorSpaceConversion(rgbaBuf, picW,picH, frame420.data[0],frame420.data[1],frame420.data[2]);
	//


	//ColorSpace Conversion 2
	AVPicture picRGBA;
	picRGBA.data[0] = (uint8_t*)rgbaBuf;
	picRGBA.linesize[0] = picW * 4;
	FFMpegConverter convRGB2YUV(picW,picH,PIX_FMT_RGBA,picW,picH,PIX_FMT_YUV420P);
	AVPicture *pFrameYUV420 = convRGB2YUV.convertVideo(&picRGBA);
	//

	//encode
	AVFrame frameInput;
	frameInput.pts = 0;
	//memcpy(&frameInput,pFrameYUV420,sizeof(AVPicture));
	memcpy(&frameInput,&frame420,sizeof(AVPicture));
	FFMpegCodecEncoder enc;
	enc.InitCodec("mjpeg");
	enc.SetCodecParams(picW,picH,1,PIX_FMT_YUVJ420P);

	int out_size = -1;
	for (int i=0;i<10;i++)
	{
		testColorSpaceConversion(rgbaBuf, picW,picH, frame420.data[0],frame420.data[1],frame420.data[2]);
		frameInput.pts++;
		out_size = enc.Encode(&frameInput);
	}
	
	FILE *f = fopen("d:\\temp\\out.jpg","wb");
	fwrite(enc.GetEncodeBuf(),1,out_size,f);
	fclose(f);
	
}

