/*
 *  FFMpegWrapper.h
 *  iScreenReceiver
 *
 *  Created by Hung Raymond on 2011/2/8.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

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
