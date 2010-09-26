#ifndef FFMPEG_FIFO_H
#define FFMPEG_FIFO_H


extern "C"
{
	#include <libavutil/fifo.h>
	//#include <SDL.h>
}

class FFMpegFifo
{
public:
	FFMpegFifo(int size);
	~FFMpegFifo(void);

	int read(unsigned char* dst,int readSize);
	int write(unsigned char* src,int writeSize);
	int getSpace();
	int getSize();


protected:
	AVFifoBuffer		*fifo;
	int					size;
/*
	SDL_mutex			*mutex;
	SDL_cond			*condRead;
	SDL_cond			*condWrite;
*/
	int					waitReadBytes;
	int					waitWriteBytes;
};
#endif	//#ifdef FFMPEG_FIFO_H
