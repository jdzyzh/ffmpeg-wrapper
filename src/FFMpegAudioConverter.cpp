#include "FFMpegAudioConverter.h"

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}

typedef struct 
{
	int size;
	char* dataBegin;
	char* data;
}MyMemBuf;

static int ReadFunc(void* opaque,uint8_t *buf,int buf_size)
{
	MyMemBuf *memBuf = (MyMemBuf*)opaque;
	int bytesAvailable = memBuf->size - (memBuf->data - memBuf->dataBegin);
	int copySize = bytesAvailable >= buf_size ? buf_size : bytesAvailable;

	memcpy(buf,memBuf->data,copySize);
	memBuf->data += copySize;
	return copySize;
}

static int64_t SeekFunc(void* opaque,int64_t offset,int whence)
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

void FFMpegAudioConverter::GenWavHeader(char *buf, unsigned long ulPcmDataSize, int chNum, int sampleRate, int bitsPerSample)
{
	WAVHEADER header;
	header.ChunkID[0] = 'R';
	header.ChunkID[1] = 'I';
	header.ChunkID[2] = 'F';
	header.ChunkID[3] = 'F';

	header.ChunkSize = ulPcmDataSize + 44 - 8;
	header.Format[0] = 'W';
	header.Format[1] = 'A';
	header.Format[2] = 'V';
	header.Format[3] = 'E';

	header.SubChunk1ID[0] = 'f';
	header.SubChunk1ID[1] = 'm';
	header.SubChunk1ID[2] = 't';
	header.SubChunk1ID[3] = ' ';

	header.SubChunk1Size = 16;
	header.AudioFormat = 1;
	header.NumChannels = chNum;
	header.SampleRate = sampleRate;
	header.ByteRate = sampleRate * chNum * bitsPerSample / 8;
	header.BlockAlign = chNum * bitsPerSample / 8;
	header.BitsPerSample = bitsPerSample;
	header.SubChunk2ID[0] = 'd';
	header.SubChunk2ID[1] = 'a';
	header.SubChunk2ID[2] = 't';
	header.SubChunk2ID[3] = 'a';
	header.SubChun2Size = ulPcmDataSize;

	memcpy(buf,&header,44);
}

FFMpegAudioConverter::FFMpegAudioConverter() : decCtx(NULL),dec(NULL),pResampleCtx(NULL)
{
	avcodec_register_all();
	av_register_all();
	decodeBuf = (char*)malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
}

FFMpegAudioConverter::~FFMpegAudioConverter()
{
	delete decodeBuf;
	if (decCtx)
	{
		avcodec_close(decCtx);
		av_free(decCtx);
	}
	if (pResampleCtx)
			audio_resample_close(pResampleCtx);
}
int FFMpegAudioConverter::SetupInputByWavHeader(char *wavHeader,int wavHeaderLen)
{
	if (decCtx != NULL)
	{
		avcodec_close(decCtx);
		av_free(decCtx);
		decCtx = NULL;
	}

	ByteIOContext ioCtx;
	AVInputFormat *fmt = av_find_input_format("wav");
	
	MyMemBuf memBuf;
	memBuf.data = wavHeader;
	memBuf.dataBegin = wavHeader;
	memBuf.size = wavHeaderLen;

	unsigned char* buffer = (unsigned char*)malloc(65536 + FF_INPUT_BUFFER_PADDING_SIZE);
	init_put_byte(&ioCtx,buffer,65536,0,&memBuf,ReadFunc,NULL,SeekFunc);

	av_open_input_stream(&pFormatCtx,&ioCtx,"dummy",fmt,NULL);
	av_find_stream_info(pFormatCtx);
	dump_format(pFormatCtx, 0, "dummy", false);
	
	decCtx = pFormatCtx->streams[0]->codec;
	dec = avcodec_find_decoder(decCtx->codec_id);
	avcodec_open(decCtx,dec);


	Setup(2,decCtx->channels,44100,decCtx->sample_rate,SAMPLE_FMT_S16,decCtx->sample_fmt);
	delete buffer;
	return 0;
}

int FFMpegAudioConverter::DecodeAndResample(char* inBuf,int inBufLen,char* outBuf,int outBufLen)
{
	AVPacket pkt;
	av_init_packet(&pkt);

	int decodeBuf_size =  AVCODEC_MAX_AUDIO_FRAME_SIZE;
	int sampleSize = av_get_bits_per_sample_format(decCtx->sample_fmt) / 8;
	
	pkt.data = (uint8_t*)inBuf;
	pkt.size = inBufLen;
		
	int bytesDecoded = avcodec_decode_audio3(decCtx,(int16_t*)decodeBuf,&decodeBuf_size,&pkt);
	int nSamplesDecoded = bytesDecoded / decCtx->channels / sampleSize;
	int nSamplesResampled = Resample((short*)outBuf,(short*)decodeBuf,nSamplesDecoded);
	int bytesResampled = nSamplesResampled * 2 * 2;	//2 channels, 2 bytes per sample
	
	return bytesResampled;
}

void FFMpegAudioConverter::Setup(int chOut, int chIn, int samplerateOut, int samplerateIn, SampleFormat sampleFormatOut, SampleFormat sampleFormatIn)
{
	if (pResampleCtx)
		audio_resample_close(pResampleCtx);

	pResampleCtx = av_audio_resample_init(chOut,chIn,samplerateOut,samplerateIn,sampleFormatOut,sampleFormatIn,16,10,0,0.8);
}

int FFMpegAudioConverter::Resample(short *out, short *in, int nb_sample)
{
	return audio_resample(pResampleCtx,out,in,nb_sample);
}

int FFMpegAudioConverter::GetBitrateIn()
{
	return GetChannelNumIn() * GetBitsPerSampleIn() / 8 * GetSampleRateIn();
}

int FFMpegAudioConverter::GetChannelNumIn()
{
	if (decCtx)
		return  decCtx->channels;
	else
		return -1;
}

int FFMpegAudioConverter::GetBitsPerSampleIn()
{
	if (decCtx)
		return av_get_bits_per_sample_format(decCtx->sample_fmt);
	else
		return -1;
}

int FFMpegAudioConverter::GetSampleRateIn()
{
	if (decCtx)
		return decCtx->sample_rate;
	else
		return -1;
}