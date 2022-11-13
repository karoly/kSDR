#pragma once

#include "Config.h"
#include "Mixer.h"
#include "HilbertTransform.h"
#include "Delay.h"
#include "PolyPhaseFilter.h"
#include "SoundCard.h"
#include "CircleBuffer.h"
#include "AGC.h"
#include "ReceiverLogic.h"
#include "Display.h"

using namespace std;

class SoundProcessorThread {

	//float* data = nullptr;
	//int dataLen = 0;

	float* outputData = nullptr;

	Mixer* mixer;

	HilbertTransform* hilbertTransform;
	Delay* delay;

	PolyPhaseFilter* firFilterI;
	PolyPhaseFilter* firFilterQ;

	FirFilter* audioFilter;

	double* decimateBufferI;
	double* decimateBufferQ;

	vector<vector<float>> soundDataBuffer;

	CircleBuffer* soundProcessorCircleBuffer;
	CircleBuffer* soundWriterCircleBuffer;
	FFTSpectreHandler* fftSpectreHandler;

	Config* config;

public: 

	AGC* agc;

	int len; //������ ���������� �� 1 ��� �� ��������� ������

	SoundProcessorThread(Config* config, CircleBuffer* sPCB, CircleBuffer* sWCB, FFTSpectreHandler* fftSpectreHandler);

	void initFilters(int filterWidth);

	void putData(float* data, int len);

	void process();

	std::thread start();
};