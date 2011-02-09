
#pragma once

#ifdef WIN32
	#ifdef DLL_EXPORT
		#define FFMPEGWRAPPER_API __declspec(dllexport)
	#else
		#define FFMPEGWRAPPER_API __declspec(dllimport)
	#endif
#else
	#define FFMPEGWRAPPER_API
#endif
