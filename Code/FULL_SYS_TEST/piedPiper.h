/*
  Pied Piper
*/

#include <arduinoFFTFloat.h>
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
/****************************************************/

// Audio sampling settings:
#define sampleFreq 2048 // Sampling frequency of incoming audio, must be at least twice the target frequency
#define recordTime 8 // Number of seconds of audio to record when frequency test is positive
#define winSize 512 // Size of window used when performing fourier transform of incoming audio; must be a power of 2
// The product of sampleFreq and recordTime must be an integer multiple of winSize.

// Detection algorithm settings:
#define targetFreq 175 // Primary (first harmonic) frequency of mating call to search for
#define freqMargin 25 // Margin for error of target frequency
#define harms 2 // Number of harmonics to search for; looking for more than 3 is not recommended, because this can result in a high false-positive rate.
#define significanceThresh 20 // Threshhold for magnitude of target frequency peak to be considered a positive detection
#define signalLen 6 // Expected length of the mating call
#define detectionEfficiency 0.75 // Minimum expected efficiency by which the detection algorithm will detect target frequency peaks

// Signal processing settings:
#define analogReadTime 23 // Number of microseconds required to execute analogRead()
#define noiseFloorThresh 1.1 // uh

//Hardware settings & information:
#define audIn A3
#define in1 6
#define in2 9 //input pins for changing runtime mode
#define in3 13
#define card_select 10 // Chip select for SD
#define clk 11
#define latch 10
#define dataPin 12
#define SHTDWN 5

//Camera [Camera power and image control to be changed to pins #1 (Tx) and #4 (D4)]
#define camPow 4 // Power control pin [CHANGE]
#define camImg 1 // Imaging control pin [CHANGE]
#define imgInt 15000 // Miliseconds between detection photos [15000]
#define imgTime 600000 // Time after a detection to be taking photos [600000]
#define ctrlImgInt 900000 // Miliseconds between control photos [900000]

#define logInt 3600000 // Miliseconds between status logs [3600000]
#define playbackInt 900000 // Milliseconds between playback [900000]

#define hypnos 0

class piedPiper {
  private:
    arduinoFFT FFT = arduinoFFT();  //object for FFT in frequency calcuation
    // This must be an integer multiple of the window size:
    static const short sampleCount = recordTime * sampleFreq;
    static const short winCount = sampleCount / winSize;
    static const int delayTime = round(1000000 / sampleFreq - analogReadTime);

    short sampleBuffer[sampleCount];
    float vReal[winSize];
    float vImag[winSize];
    float freqs[winCount][winSize / 2];
    
    bool initFreqTest();
    bool fullSignalTest();
    bool checkFreqDomain(int t);

    void processSignal();
    void timeSmoothFreq(int avgWinSize);
    void smoothFreq(int avgWinSize);

    void recordSamples(int samples);
    void saveBuffer();
    void saveDetection();
    File data;

  public:
    piedPiper();

    unsigned long lastDetectionTime;
    unsigned long lastPhotoTime;
    unsigned long lastLogTime;
    unsigned long lastPlaybackTime;
    int detectionNum;
    int photoNum;
    
    
    bool insectDetection();
    void playback();
    void takePhoto(int n);
    void reportAlive();
};
