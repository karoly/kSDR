#pragma once
#include "Env.h"

class WindowBlackman{

    int len = 0;
    float* weightArray;

public:

    WindowBlackman(int fftLen); //this->len = fftLen * 2; 

    float* getWeights();

    void init();
};