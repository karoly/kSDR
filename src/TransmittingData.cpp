#include "TransmittingData.h"
#include "random"
#include "iostream"

//std::random_device rd; // obtain a random number from hardware
//std::mt19937 gen(rd()); // seed the generator
//std::uniform_int_distribution<> distr(0, 100); // define the range

#define hilbertTransformLen 4096

TransmittingData::TransmittingData(Config* config, int freq, int samplingRate) {
    this->config = config;
    carierSignal = new ComplexOscillator(5000, samplingRate);
    //complexOscilator = new ComplexOscillator(freq, samplingRate);

    sourceSignal = new CosOscillator(500000, samplingRate);

    mixer1 = new Mixer(samplingRate);
    mixer1->setFreq(-freq - 500000);

    //xuixer = new ComplexOscillator(freq, samplingRate);

    /*mixer2 = new Mixer(samplingRate);
    mixer2->setFreq(freq);*/
    
    hilbertTransformFFTW = new HilbertTransformFFTW(hilbertTransformLen);
    hilbertTransformFFTW1 = new HilbertTransformFFTW(hilbertTransformLen);
    
    hilbertTransform = new HilbertTransform(samplingRate, 127);
    delay = new Delay((127 - 1) / 2);
    
    hilbertTransform2 = new HilbertTransform(samplingRate, hilbertTransformLen);
    delay2 = new Delay((hilbertTransformLen - 1) / 2);

    windowBlackman = new WindowBlackman(2048);
    //firI.init(firI.LOWPASS, firI.BARTLETT, 65, 10000, 0, samplingRate);
    //firQ.init(firQ.LOWPASS, firQ.BARTLETT, 65, 10000, 0, samplingRate);
}

TransmittingData::~TransmittingData() {
    delete carierSignal;
    delete sourceSignal;
    delete mixer1;
    //delete mixer2;
    delete hilbertTransform;
    delete hilbertTransformFFTW;
    delete delay;
    delete delay2;
    delete windowBlackman;
}

long count = 0;
//long count2 = 0;

Signal* TransmittingData::nextBuffer() {
    
    /*double x[4] = {1, 2, 3, 4};
    fftw_complex* complex = hilbertTransformFFTW->process(x);

    for (int i = 0; i < hilbertTransformLen; i++) {
        if (complex[i][IMAG] < 0)
            std::cout << complex[i][REAL] << "-" << abs(complex[i][IMAG]) << "i" << std::endl;
        else
            std::cout << complex[i][REAL] << "+" << complex[i][IMAG] << "i" << std::endl;
    }

    exit(0);*/

    double* arr1 = new double[hilbertTransformLen];
    double* arr2 = new double[hilbertTransformLen];

    Signal* out = new Signal[131072];

    for (int j = 0; j < 32; j++) {
        for (int i = 0; i < hilbertTransformLen; i++) {
            auto val = 0.5 * sourceSignal->nextSample();
            arr1[i] = val;
        }

        fftw_complex* complex = hilbertTransformFFTW->process(arr1);

        for (int i = 0; i < hilbertTransformLen; i++) {
            ComplexSignal carier = carierSignal->next();

            float I = (float) complex[i][IMAG];
            float Q = (float) complex[i][REAL];

            float ssb = 0;
            if (config->receiver.modulation == USB) ssb = carier.I * Q + carier.Q * I;
            if (config->receiver.modulation == LSB) ssb = carier.I * Q - carier.Q * I;

            arr2[i] = ssb;
        }

        fftw_complex* complex2 = hilbertTransformFFTW->process(arr2);

        for (int i = 0; i < hilbertTransformLen; i++) {
            float I = (float)complex2[i][IMAG];
            float Q = (float)complex2[i][REAL];
            float dither = ((float)rand() / (float)(RAND_MAX)) / 30.0f;
            auto gugka = mixer1->mix(I, Q);
            out[j * 4096 + i] = Signal{ gugka.I + dither , gugka.Q + dither };

            /*count++;
            if (count % 500000 == 0) {
                if (sourceSignal->freq > 3000.0f) sourceSignal->setFreq(0.0f);
                else sourceSignal->setFreq(sourceSignal->freq + 100.0f);
            }*/
        }

        

    }

    delete[] arr1;
    delete[] arr2;

    return out;
    /*ComplexSignal complexSignal = carierSignal->next();
    float dither = ((float)rand() / (float)(RAND_MAX)) / 100.0f;
    Signal signal = Signal { complexSignal.I + dither, complexSignal.Q + dither };
    return signal;*/

    //Signal outputSignal;

    //���������� �� 8000
    //if (count % 100 == 0) {

        /*if (count % 500000 == 0) {
            if (sourceSignal->freq > 3000.0f) sourceSignal->setFreq(0.0f);
            else sourceSignal->setFreq(sourceSignal->freq + 100.0f);
        }*/

        /*ComplexSignal carier = carierSignal->next();

        auto val = sourceSignal->nextSample();
        auto s1 = hilbertTransform->filter(val);
        auto s2 = delay->filter(val);
        
        float ssb = 0;
        if (config->receiver.modulation == USB) ssb = carier.I * s2 + carier.Q * s1;
        if (config->receiver.modulation == LSB) ssb = carier.I * s2 - carier.Q * s1;

        //ComplexSignal piska { (carier.I * cs1.I - carier.Q * cs2.I), (carier.I * cs1.Q - carier.Q * cs2.Q) };

        outputSignal = Signal{ ssb, 0 };*/
        //outputSignal = Signal{ ssb * 16, 0 };
        
        //outputSignal = Signal { piska.I,  piska.Q };
    //} else {
    //    outputSignal = Signal { 0.0f,  0.0f };
    //}

    //count++;

    //float dither = ((float)rand() / (float)(RAND_MAX)) / 500.0f;

    //float kaka = firI.proc(outputSignal.I);

    //auto xuixerina = xuixer->next();

    //ComplexSignal signal { outputSignal.I, outputSignal.Q };

    //return Signal{ signal.I + dither, signal.Q + dither };
    //
    //auto baseSignal = 


    
    //    auto signal = mixer1->mix(pipka.I, pipka.Q);
    //printf("%f %f\r\n", signal.I, signal.Q);

}

void TransmittingData::setFreq(int freq) {
    //carierSignal->setFreq(freq);
    //complexOscilator->setFreq(freq);
    mixer1->setFreq(-freq - 500000);
}