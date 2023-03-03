#include "FFTSpectreHandler.h"

//#include "fft3.hpp"
//std::vector<KalmanFilter> kfArray;

float* tmpArray;
float* tmpArray2;
float* tmpArray3;

FFTSpectreHandler::~FFTSpectreHandler() {
	fftw_destroy_plan(fftwPlan);
}

FFTSpectreHandler::FFTSpectreHandler(Config* config) {
	this->config = config;

	spectreSize = config->fftLen / 2;

	wb = new WindowBlackman(config->fftLen);
	wbh = new WindowBlackmanHarris(config->fftLen);
	//windowArray = wb->init();

	dataBuffer = new float[config->fftLen];
	//memset(dataBuffer, 0, sizeof(float) * config->fftLen);

	complexLen = config->fftLen / 2 + 1;

	realInput = new float[spectreSize];
	//memset(realInput, 0, sizeof(float) * config->fftLen / 2);

	imInput = new float[spectreSize];
	//memset(imInput, 0, sizeof(float) * config->fftLen / 2);

	realOut = new float[complexLen];
	//memset(realOut, 0, sizeof(float) * complexLen);

	imOut = new float[complexLen];
	//memset(imOut, 0, sizeof(float) * complexLen);

	superOutput = new float[spectreSize];
	memset(superOutput, -100, sizeof(float) * spectreSize);

	/*for (int i = 0; i < spectreSize; i++) {
		kfArray.push_back(KalmanFilter(1.0f, 0.1f));
	}*/

	tmpArray = new float[spectreSize];
	memset(tmpArray, -100, sizeof(float) * spectreSize);
	tmpArray2 = new float[spectreSize];
	memset(tmpArray, -100, sizeof(float) * spectreSize);

	inData = new fftw_complex[config->fftLen];
	outData = new fftw_complex[spectreSize];

	fftwPlan = fftw_plan_dft_1d(spectreSize, inData, outData, FFTW_FORWARD, FFTW_ESTIMATE);

	speedDelta = new float[spectreSize];
	//memset(speedDelta, 1, sizeof(float) * spectreSize);
	//fft.init(config->fftLen);
}

//std::queue<std::vector<float>> spectreDataQueue;
std::mutex spectreDataMutex;

bool readyToCalculate = false;

void FFTSpectreHandler::run() {
	while (true) {
		//if (readyToCalculate) {
			/*if (spectreDataMutex.try_lock()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}*/
			spectreDataMutex.lock();
			printf("KAKA\n");
			processFFT();
			//readyToCalculate = false;
			//не забываем ставить unlock()!!!
			spectreDataMutex.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		//}
		//else {
		//}

		//else {
			//std::this_thread::sleep_for(std::chrono::milliseconds(1));
		//}
	}
}

void FFTSpectreHandler::putData(float* data) {
	if (!spectreDataMutex.try_lock()) {
		printf("blocked\n");
		return;
	}
	printf("sisya\n");

	for (int i = 0; i < config->fftLen; i++) {
		dataBuffer[i] = data[i];
	}

	//memcpy(dataBuffer, data, sizeof(float) * config->fftLen);

	spectreDataMutex.unlock(); // не забываем ставить unlock()!!!
	//readyToCalculate = true;
}

float* FFTSpectreHandler::getOutputCopy(int startPos, int len) {
	float* buffer = new float[spectreSize];
	float* output = getOutput();
	//spectreDataMutex.lock();

	memcpy(buffer, output + (spectreSize / 2), sizeof(output) * (spectreSize / 2));
	memcpy(buffer + (spectreSize / 2), output, sizeof(output) * (spectreSize / 2));

	//spectreDataMutex.unlock();

	float* dataCopy = new float[len];

	memcpy(dataCopy, buffer + startPos, sizeof(float) * len);

	delete[] buffer;

	return dataCopy;
}

void FFTSpectreHandler::processFFT() {
	
	//memcpy(output, dataBuffer, sizeof(output) * bufferLen);

	//memset(imOut, 0, sizeof(float) * bufferLen);

	//float* realIn = new float[bufferLen];
	//memset(realIn, 0, sizeof(float) * bufferLen);

	//prepareData();

	//std::vector<float> re(audiofft::AudioFFT::ComplexSize(FFT_LENGTH));
	//std::vector<float> im(audiofft::AudioFFT::ComplexSize(FFT_LENGTH));

	//cout << "im.size() " << audiofft::AudioFFT::ComplexSize(FFT_LENGTH) << "\r\n";

	//Apply window function
	for (int i = 0; i < spectreSize; i++) {
		//realInput[i] = dataBuffer[2 * i];
		//imInput[i] = dataBuffer[2 * i + 1];
		inData[i][0] = dataBuffer[2 * i] * wb->getWeights()[i];
		inData[i][1] = dataBuffer[2 * i + 1] * wb->getWeights()[i];
	}

	fftw_execute(fftwPlan);

	//auto begin = std::chrono::steady_clock::now();

	//fft3(realInput, imInput, config->fftLen / 2, realOut, imOut);

	/*auto end = std::chrono::steady_clock::now();
	auto elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
	std::cout << "The time: " << elapsed_ms.count() << " micros\n";*/

	//for (int i = 0; i < 32 * 1024; i++) printf("freq: %3d %+9.5f %+9.5f I\n", i, out[i][0], out[i][1]);

	/*for (int i = 0; i < config->fftLen / 2; i++) {
		realOut[i] = out[i][0];
		imOut[i] = out[i][1];
	}*/

	//fft3(realInput, imInput, config->fftLen / 2, realOut, imOut);

	//fft.fft(dataBuffer, realOut, imOut);


	//memcpy(output, realOut, sizeof(realOut));
	//memcpy(output + sizeof(realOut), imOut, sizeof(imOut));


	dataPostprocess();

	//Utils::printArray(inOut, bufferLen);

	//fft.process(1, 10, output, kakashka);
	//semaphore->unlock();
}


float FFTSpectreHandler::average(float avg, float new_sample, int n) {
	float tmp = avg;
	tmp -= avg / (float)n;
	tmp += new_sample / (float)n;
	return tmp;
}

void FFTSpectreHandler::dataPostprocess() {
	for (int i = 0; i < spectreSize; i++) {
		float psd = this->psd(outData[i][0], outData[i][1]) - 100;
		if (firstRun) {
			tmpArray[i] = psd;
			firstRun = false;
		} else {
			tmpArray[i] = average(tmpArray[i], psd, config->spectre.spectreSpeed);
		}
	}

	//memcpy(tmpArray2, tmpArray, sizeof(float) * spectreSize);
	//superOutput[j] = (tmpArray[j - 1] + tmpArray[j] + tmpArray[j + 1]) / 3;
	//tmpArray2[j] = (tmpArray[j - 3] + 2 * tmpArray[j - 2] + 3 * tmpArray[j - 1] + 4 * tmpArray[j] + 3 * tmpArray[j + 1] + 2 * tmpArray[j + 2] + tmpArray[j + 3]) / 16.0;

	for (int n = 0; n <= config->spectre.smoothingDepth; n++) {
		for (int j = 0; j < spectreSize; j++) {
			if (j >= 2 && j < spectreSize - 2) {
				tmpArray2[j] = (tmpArray[j - 2] + 2.0f * tmpArray[j - 1] + 3.0f * tmpArray[j] + 2.0f * tmpArray[j + 1] + tmpArray[j + 2]) / 9.0f;
			}
			else {
				tmpArray2[j] = tmpArray[j];
			}
			if (n == config->spectre.smoothingDepth) {
				if (config->spectre.hangAndDecay) {
					if (superOutput[j] < tmpArray2[j]) {
						superOutput[j] = average(superOutput[j], tmpArray2[j], config->spectre.spectreSpeed2);
						speedDelta[j] = 1.0f;
					}
					else {
						superOutput[j] -= config->spectre.decaySpeed * speedDelta[j];
						speedDelta[j] += config->spectre.decaySpeedDelta;
					}
				} else {
					superOutput[j] = average(superOutput[j], tmpArray2[j], config->spectre.spectreSpeed2);
				}
			}
			//Utils::printFloat(superOutput[j]);
		}
		if (n != config->spectre.smoothingDepth) memcpy(tmpArray, tmpArray2, sizeof(float) * spectreSize);
	}

	/*for (int j = 0; j < spectreSize; j++) {
		//if (j >= 1 && j < spectreSize - 1) pipiska(((tmpArray[j - 1] + tmpArray[j] + tmpArray[j + 1]) / 3.0f), j);
		//if (j >= 2 && j < spectreSize - 2) pipiska((tmpArray2[j - 2] + 2 * tmpArray2[j - 1] + 3 * tmpArray2[j] + 2 * tmpArray2[j + 1] + tmpArray2[j + 2]) / 9.0, j);
		//else pipiska(tmpArray2[j], j);
		if (j >= 3 && j < spectreSize - 3) {
			//pipiska((tmpArray[i - 2] + 2 * tmpArray[i - 1] + 3 * tmpArray[i] + 2 * tmpArray[i + 1] + tmpArray[i + 2]) / 9.0, i);
			pipiska((tmpArray2[j - 3] + 2 * tmpArray2[j - 2] + 3 * tmpArray2[j - 1] + 4 * tmpArray2[j] + 3 * tmpArray2[j + 1] + 2 * tmpArray2[j + 2] + tmpArray2[j + 3]) / 16.0, j);
		}
		else pipiska(tmpArray2[j], j);
	}*/
}

Semaphore* FFTSpectreHandler::getSemaphore() {
	return semaphore;
}


float* FFTSpectreHandler::getOutput() {
	return superOutput;
}

int FFTSpectreHandler::getSpectreSize() {
	return spectreSize;
}

float FFTSpectreHandler::psd(float re, float im) {
	return 10 * log(re * re + im * im);
}

void FFTSpectreHandler::prepareData() {
	//Применения окна Блэкмона к исходным данным
	float* weights = wbh->getWeights();
	for (int i = 0; i < spectreSize; i++) {
		dataBuffer[2 * i] = dataBuffer[2 * i] * weights[i];
		dataBuffer[2 * i + 1] = dataBuffer[2 * i + 1] * weights[i];
	}
}

std::thread FFTSpectreHandler::start() {
	std::thread p(&FFTSpectreHandler::run, this);
	return p;
}