#include "EncodeProfile.h"
#include <string.h>

EncodeProfile::EncodeProfile()
{
}

EncodeProfile::~EncodeProfile()
{
}

EncodeProfile* EncodeProfile::parse(char* filename)
{
	EncodeProfile *result = new EncodeProfile();
	char line[512];

	FILE *fp = fopen(filename,"r");
	if (fp == NULL)
	{
		return NULL;
	}

	while (true)
	{
		char* ret = fgets(line,512,fp);
		if (ret == NULL)
			break;

		line[strlen(line)-1] = '\0';
		if (strncmp("input_file",line,10) == 0)
			result->addInputFile(line+11);
	
		if (strncmp("output_file",line,11) == 0)
			result->setOutputFile(line+12);

		if (strncmp("ffmpeg_param",line,12) == 0)
			result->setFFMpegParam(line+13);
	}
	
	return result;
}

void EncodeProfile::addInputFile(char *file)
{
	inputFiles.push_back(string(file));	
}

void EncodeProfile::setOutputFile(char *file)
{
	outputFile = string(file);
}

void EncodeProfile::setFFMpegParam(char *param)
{

	ffmpegParams.push_back(string(param));
}

int EncodeProfile::getFFMpegEncodeIterations()
{
	return ffmpegParams.size();
}

char* EncodeProfile::getFFMpegEncodeParam(int idx)
{
	return (char*)ffmpegParams.at(idx).c_str();
}

char* EncodeProfile::getOutputFile()
{
	return (char*)outputFile.c_str();
}