#pragma once

#include "Env.h"

class WindowBlackmanHarris {

    int len = 0;
    float* weightArray;

public:

    WindowBlackmanHarris(int fftLen); //this->len = fftLen * 2; 

    float* getWeights();

    void init();

};