// FFMpegWrapperAdaptorTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include "../FFMpegWrapperAdaptor/FFMpegAudioConverterAdaptor.h"

#pragma comment( lib, "FFMpegWrapperAdaptor" )

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	char buf[65536];
	int bufSize = 65536;
	char bufOut[65536];
	int bufOutSize = 65536;

	FFMpegAudioConverterAdaptor conv;
	ifstream is;
	is.open("d:\\temp\\test_s32.wav",ios::binary | ios::in);
	is.seekg(0,ios::end);
	long isLen = is.tellg();
	is.seekg(0,ios::beg);
	
	
	ofstream os;
	os.open("d:\\temp\\convertResult.wav",ios::binary | ios::out);
	
	//read arbitrary length, make sure it is long enough for wave parsing.	
	is.read(buf,bufSize);
	conv.SetupInputByWavHeader(buf,bufSize);
	long osLen = isLen / conv.GetBitrateIn() * 2 * 2 * 44100;
	//


	//write header first
	FFMpegAudioConverterAdaptor::GenWavHeader(bufOut,osLen,2,44100,16);
	os.write(bufOut,44);

	//the decode==>convert==>write loop
	while (true)
	{
		int bytesRead = is.read(buf,bufSize).gcount();
		if (bytesRead <= 0)
			break;
		int bytesConverted = conv.DecodeAndResample(buf,bytesRead,bufOut,bufOutSize);
		os.write(bufOut,bytesConverted);
	}
	//

	is.close();
	os.close();
	return 0;
}

