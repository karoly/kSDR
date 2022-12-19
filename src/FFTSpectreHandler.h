#pragma once

#include "Env.h"
#include "Config.h"
#include "Semaphore.h"
#include "WindowBlackman.h"
#include "WindowBlackmanHarris.h"
#include "Average.h"
#include "vector"
#include "queue"
#include "mutex"

class FFTSpectreHandler {
	
private:

	Config* config;
	

	//�������������� ������� ������ � ������������ ����� ������� ������ ������� (������������ ������ ������ +1)
	WindowBlackman* wb;
	WindowBlackmanHarris* wbh;

	float* dataBuffer;

	int savedBufferPos = -1;

	int complexLen;

	float* realInput;
	float* imInput;

	float* realOut;
	float* imOut;

	//float output[complexLen * 2];
	
	float* superOutput;

	Semaphore* semaphore = new Semaphore();

	bool firstRun = true;

	int spectreSpeed = 30;

	//Average average = Average(20);

	void processFFT();
	float average(float avg, float new_sample, int n);
	void dataPostprocess();
	float* getOutput();

	int spectreSize = 0;

	bool busy = false;
	bool readyToProcess = false;
	bool outputting = false;

	float psd(float re, float im);
	void prepareData();

	std::queue<std::vector<float>> getSpectreDataQueue();
public:

	FFTSpectreHandler(Config* config);
	Semaphore* getSemaphore();
	float* getOutputCopy(int startPos, int len);
	void putData(float* data);


	int getSpectreSize();
	void setSpectreSpeed(int speed);

	std::thread start();
	void run();



	



};