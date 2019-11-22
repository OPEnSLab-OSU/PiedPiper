/*
 *Pied Piper
 */

#include "arduinoFFT.h"
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>



/****************************************************/
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03


#define CHANNEL A2 //sets analog input
#define samples 512 //set # of samples; must be a power of two
#define bufferSize 50
#define AMBIENCE 200 //set level of ambient noise
#define upperFreq 250 //set frequency bounds for insect
#define lowerFreq 130
#define upperPeak 5 //set peak bounds for insect
#define lowerPeak 2
#define peakThreshold 0.9  //set how sensitive the peak detection is
#define lowerThreshold 0.75
#define PLAYBACK_TIME 5    //set how long the system will playback mimc signal(seconds)
#define DEBUG_SIGNAL 1
#define DEBUG 0
/*****************************************************/
 

class piedPiper{
private:
		arduinoFFT FFT = arduinoFFT();  //object for FFT in frequency calcuation
    bool detected;  //flag for if an insect has been detected
		const double samplingFrequency = 10000; //samples per second
		unsigned int sampling_period_us;  //sampling period in microseconds
		unsigned long microseconds;
    int upper_bound = 250;  //upper frequency range
    int lower_bound = 130;  //lower frequency range 
    float peak; //peak value of waveform for peak counting
    int num_peaks;
    int sampleBuffer[bufferSize];
    int signalBuffer[500];
    int average = 0;
		double vReal[samples];
		double vImag[samples];
		double domFreq;
    
public:
		piedPiper();
		void sampleFreq();
		void printFreq();
    void getPeakVoltage();
    void countPeaks();
    void checkFrequency();
    void playback(int mimic, int clk, int latch, int data);
    double get_domFreq();
    void insectDetection();
    void print_all();
    bool checkSilence();
    bool getDetected();
    int smoothData();
    

};
