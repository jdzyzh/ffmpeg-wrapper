#ifndef ENCODE_PROFILE_H
#define ENCODE_PROFILE_H

#include <vector>
#include <string>
/*
input_file=abc.wmv
input_file=123.wmv
output_file=output.wmv
ffmpeg_param=-i xxx -acodec wmav2 -vcodec msmpeg4 -ss 30 -y xxx.wmv
ffmpeg_param=-i xxx -acodec wmav2 -vcodec msmpeg4 -ss 30 -y xxx.wmv
*/
using namespace std;

class EncodeProfile
{
protected:
	EncodeProfile();
public:
	static EncodeProfile* parse(char *filename);
	~EncodeProfile();

public:
	void addInputFile(char *file);
	void setOutputFile(char *file);
	void setFFMpegParam(char *param);
	int getFFMpegEncodeIterations();
	char* getFFMpegEncodeParam(int idx);
	char* getOutputFile();

protected:
	vector<string> inputFiles;
	string outputFile;
	vector<string> ffmpegParams;
};
#endif	//ENCODE_PROFILE_H