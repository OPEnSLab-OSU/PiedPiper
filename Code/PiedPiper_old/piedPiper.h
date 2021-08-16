/*
 *Pied Piper
 */

#include "arduinoFFT.h"
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
/****************************************************/
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03


#define CHANNEL A2 //sets analog input
#define samples 512 //set # of samples; must be a power of two
#define bufferSize 30
#define AMBIENCE 550 //set level of ambient noise
#define upperFreq 250 //set frequency bounds for insect
#define lowerFreq 110
#define upperPeak 4 //set peak bounds for insect
#define lowerPeak 2
#define peakThreshold 0.99 //set how sensitive the peak detection is
#define lowerThreshold 0.95
#define fShift 1.0         //set how much the frequency has to change for it to count as a frequency rise
#define DEBUG_SIGNAL 0
#define DEBUG 1
#define RECORD_SIZE 15000
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
    int num_peaks;  //number of peaks per second
    int sampleBuffer[bufferSize];
    int signalBuffer[500];  
    int average = 0;
    
    int recording[RECORD_SIZE];
    int rec_count;
    
		double vReal[samples];
		double vImag[samples];
		double domFreq;   //dominant frequency of the incoming signal
    bool debug;
    bool debug_signal;

    
public:  
		piedPiper();
    void calibrateGain(Adafruit_NeoPixel l,int clk, int latch, int data);
    void changeIndicator(Adafruit_NeoPixel l, int r, int g, int b);
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
    bool getDebug();
    bool getDebugSignal();
    void setDebugSetting(bool d);
    int* getRecord();
    int getRecordCount();
    


};
