#ifndef FFMPEG_ENCODER_H_
#define FFMPEG_ENCODER_H

#include <wchar.h>
#include "SimpleBuffer.h"
extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}
typedef struct
{
	char		formatStr[32];

	char		audioCodec[32];
	int			audioChannels;
	int			audioBitrate;
	int			audioSampleRate;
	int			audioBitsPerSample;

	char		videoCodec[32];
	int			videoWidth;
	int			videoHeight;
	double		videoFrameRate;
	int			videoBitrate;
	
	wchar_t		outputFilename[256];
}FFMpegEncodeProfile;

class FFMpegEncoder
{
public:
	static const int ERR_GUESS_FORMAT			= -1;
	static const int ERR_WRITE_AUDIO_FRAME		= -2;
	static const int ERR_FIND_AUDIO_CODEC		= -3;
	static const int ERR_OPEN_AUDIO_CODEC		= -4;
	static const int ERR_ADD_AUDIO_STREAM		= -5;
	static const int ERR_FIND_VIDEO_CODEC		= -6;
	static const int ERR_OPEN_VIDEO_CODEC		= -7;
	static const int ERR_ADD_VIDEO_STREAM		= -8;
	static const int ERR_WRITE_VIDEO_FRAME		= -9;
	static const int ERR_CREATE_VIDEO_STREAM	= -10;
	static const int ERR_ALLOC_VIDEO_BUF		= -11;
	static const int ERR_ALLOC_AUDIO_BUF		= -12;

	
	
public:
	static FFMpegEncodeProfile* createDefaultProfile();

	FFMpegEncoder();
	FFMpegEncoder(FFMpegEncodeProfile* profile);
	~FFMpegEncoder(void);

	int				init(FFMpegEncodeProfile *profile);
	int				openOutputFile();
	
	int				encodeVideoFrame(AVFrame *decodedFrame,unsigned char* imageBuf,int imageBufSize);
	int				encodeVideoFrame(AVFrame *decodedFrame);
	int				scaleAndEncodeVideoFrame(AVFrame *decodedFrame,unsigned char* encodeBuf,int encodeBufSize);
	int				encodeAudioFrame(short* audioBufIn,int audioBufInSize);
	
	SimpleBuf		getEncodedAudioBuf();
	SimpleBuf		getEncodedVideoBuf();
	double			getEncodedVideoPTS();
	AVFrame*		getCodedVideoFrame();
	PixelFormat		getPixelFormat();
	int				getAudioFrameSize();
	double			getAudioEncodeFrameSec();
	double			getAudioPtsSec();
	double			getVideoPtsSec();

	int				writePacket(AVPacket *pkt);
	int				writeToFileAudio();
	int				writeToFileVideo(double ptsSec);
	int				write_audio_frame(AVFormatContext *oc, AVStream *st,short *samples);
	int				write_video_frame(AVFormatContext *oc, AVStream *st,AVPicture *picture);
	void			finishEncode();
	
	//
	int				configOutput();
	int				configAudioStream();
	int				configVideoStream();


//protected:
public:

	FFMpegEncodeProfile profile;

	
	AVOutputFormat *fmt;
	AVFormatContext *pFormatCtx;
	AVStream *audioStream;
	AVStream *videoStream;
	
	//audio
	AVCodecContext *pAudioCodecCtx;
	unsigned char* audioEncodeBuf;
	int audioEncodeBufSize;
	int audioEncodedSize;
	double audioClock;
	
	//video
	AVCodecContext *pVideoCodecCtx;
	AVFrame *pDecodedFrame;
	int videoEncodeBufSize;
	unsigned char *videoEncodeBuf;
	int videoEncodedSize;
	double videoClock;

	FILE *outputFile;

};

#endif	//FFMPEG_ENCODER_H
