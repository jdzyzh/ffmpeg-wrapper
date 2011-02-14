
#pragma once

#ifdef WIN32
	#if defined (DLL_EXPORT)
		#define FFMPEGWRAPPER_API __declspec(dllexport)
	#elif defined (DLL_IMPORT)
		#define FFMPEGWRAPPER_API __declspec(dllimport)
	#else
		#define FFMPEGWRAPPER_API
	#endif
#else
	#define FFMPEGWRAPPER_API
#endif

typedef struct 
{
	char* data[4];
	int linesize[4];
	char frameType;	//I=1,P=2,B=3, refer to avcodec.h for other types.
}FFMpegFrame;
