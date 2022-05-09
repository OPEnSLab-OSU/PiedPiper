#include <arduinoFFTFloat.h>
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

/****************************************************/

// Audio sampling settings:
#define SAMPLE_FREQ 2048 // Sampling frequency of incoming audio, must be at least twice the target frequency
#define REC_TIME 8 // Number of seconds of audio to record when frequency test is positive
#define FFT_WIN_SIZE 256 // Size of window used when performing fourier transform of incoming audio; must be a power of 2
// The product of sampleFreq and recordTime must be an integer multiple of winSize.

// Detection algorithm settings:
#define TGT_FREQ 175 // Primary (first harmonic) frequency of mating call to search for
#define FREQ_MARGIN 25 // Margin for error of target frequency
#define HARMONICS 2 // Number of harmonics to search for; looking for more than 3 is not recommended, because this can result in a high false-positive rate.
#define SIG_THRESH 18 // Threshhold for magnitude of target frequency peak to be considered a positive detection
#define EXP_SIGNAL_LEN 6 // Expected length of the mating call
#define EXP_DET_EFF 0.75 // Minimum expected efficiency by which the detection algorithm will detect target frequency peaks
#define NOISE_FLOOR_MULT 1.0 // uh
#define TIME_AVG_WIN_COUNT 4 // Number of frequency windows used to average frequencies across time

//Hardware settings & information:
#define AUD_IN A3
#define SD_CS 10 // Chip select for SD

//Camera
#define HYPNOS_5VR 6
#define HYPNOS_3VR 5
#define CAM_IMG_PIN 9 // Imaging control pin
#define IMG_INT 15000 // Miliseconds between detection photos [15000]
#define IMG_TIME 600000 // Time after a detection to be taking photos [600000]
#define CTRL_IMG_INT 900000 // Miliseconds between control photos [900000]

#define LOG_INT 3600000 // Miliseconds between status logs [3600000]
#define PLAYBACK_INT 900000 // Milliseconds between playback [900000]

    // Volatile audio input buffer (LINEAR)
volatile static short inputSampleBuffer[FFT_WIN_SIZE];
volatile static int inputSampleBufferPtr = 0;
static const int sampleDelayTime = 1000000 / SAMPLE_FREQ;

class piedPiper {
  private:
    arduinoFFT FFT = arduinoFFT();  //object for FFT in frequency calcuation
    File data;
    
    // This must be an integer multiple of the window size:
    static const int sampleCount = REC_TIME * SAMPLE_FREQ + FFT_WIN_SIZE * TIME_AVG_WIN_COUNT; // [Number of samples required to comprise small + large frequency arrays]
    static const int freqWinCount = REC_TIME * SAMPLE_FREQ / FFT_WIN_SIZE;
    

    // Samples array (CIRCULAR)
    // (Should I break this into time windows that directly correspond to the frequency array windows?)
    // (This would complicate things slightly, but it would let me know the frequency array positions just from the sample window position
    short samples[sampleCount];
    int samplePtr = 0;

    // Time smoothing spectral buffer  (CIRCULAR ALONG TIME)
    float rawFreqs[TIME_AVG_WIN_COUNT][FFT_WIN_SIZE / 2];
    int rawFreqsPtr = 0;

    // Spectral data (CIRCULAR ALONG TIME)
    float freqs[freqWinCount][FFT_WIN_SIZE / 2];
    int freqsPtr = 0; // Position of "current" time in buffer (specifically, the time that will be written to next; the data at this location is the oldest)

    // Scratchpad arrays used for calculating FFT's and frequency smoothing
    float vReal[FFT_WIN_SIZE];
    float vImag[FFT_WIN_SIZE];

    unsigned long lastLogTime = 0;
    unsigned long lastDetectionTime = 0;
    unsigned long lastImgTime = 0;
    unsigned long lastPlaybackTime = 0;

    int detectionNum = 0;
    int photoNum = 0;
    
    int IterateCircularBufferPtr(int currentVal, int arrSize);

    void StopAudioStream();
    void RestartAudioStream();

    
    void SmoothFreqs(int winSize);
    bool CheckFreqDomain(int t);
    
    void ResetSPI();

    void(*timerStart)();
    void(*timerStop)();

  public:
    piedPiper(void(*tStart)(), void(*tStop)());

    static void RecordSample();
    bool InputSampleBufferFull();
    void ProcessData();
    bool InsectDetection();
    void Playback();
    void LogAlive();
    void TakePhoto(int n);
    void SaveDetection();
    bool InitializeAudioStream();
    
    unsigned long GetLastPlaybackTime();
    unsigned long GetLastPhotoTime();
    unsigned long GetLastDetectionTime();
    unsigned long GetLastLogTime();
    int GetDetectionNum();
    
};
