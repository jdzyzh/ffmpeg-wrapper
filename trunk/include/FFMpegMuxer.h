class FFMpegMuxer
{
public:
	int init(const char* muxerName,const char* outputName);
	int addVideoStream(int streamIndex,int width,int height,int bitrate,int timebaseDen,int timebaseNum);
	int beginMux();
	int addVideoFrame(void* encodedData,int encodedDataSize,int64_t pts,bool isKeyFrame);
	int endMux();
	
public:
	AVFormatContext *formatCtx;
	AVOutputFormat *fmt;
	AVStream *videoStream;
	
};
