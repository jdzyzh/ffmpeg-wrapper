#include "FFMpegFifo.h"
#include <stdio.h>


FFMpegFifo::FFMpegFifo(int _size)
{
	size = _size;
	fifo = av_fifo_alloc(size);
	/*
	mutex = SDL_CreateMutex();
	condRead = SDL_CreateCond();
	condWrite = SDL_CreateCond();
	*/
}

FFMpegFifo::~FFMpegFifo(void)
{
}

int FFMpegFifo::read(unsigned char* dst,int readSize)
{
	//SDL_LockMutex(mutex);
	if (av_fifo_size(fifo) < readSize)
	{
		waitReadBytes = readSize;
		//SDL_CondWait(condRead,mutex);
	}
	int ret = av_fifo_generic_read(fifo,dst,readSize,NULL);
	if (av_fifo_space(fifo) >= waitWriteBytes)
		//SDL_CondSignal(condWrite);
	//SDL_UnlockMutex(mutex);
	return readSize;
}
int FFMpegFifo::write(unsigned char* src,int writeSize)
{
	//SDL_LockMutex(mutex);
	if (av_fifo_space(fifo) < writeSize)
	{
		waitWriteBytes = writeSize;
		//SDL_CondWait(condWrite,mutex);
	}
	int ret = av_fifo_generic_write(fifo,src,writeSize,NULL);
	/*
	if (av_fifo_size(fifo) >= waitReadBytes)
		SDL_CondSignal(condRead);
	*/
	//SDL_UnlockMutex(mutex);
	return ret;
}

int FFMpegFifo::getSpace()
{
	//SDL_LockMutex(mutex);
	int ret = av_fifo_space(fifo);
	//SDL_UnlockMutex(mutex);
	return ret;
}

int FFMpegFifo::getSize()
{
	//SDL_LockMutex(mutex);
	int ret = av_fifo_size(fifo);
	//SDL_UnlockMutex(mutex);
	return ret;
}