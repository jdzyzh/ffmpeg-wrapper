
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
