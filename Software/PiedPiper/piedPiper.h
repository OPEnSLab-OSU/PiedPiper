#include <arduinoFFTFloat.h> // Floating point version of ArduinoFFT 1.6.1, pulled on August 18, 2023
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <ArduCAM.h> // Version 1.0.0, pulled on August 18, 2023
#include "memorysaver.h" // part of ArduCAM library
#include "RTClib.h" // Version 2.1.1, pulled on August 18, 2023

/****************************************************/

// Audio sampling settings:
#define AUD_IN_SAMPLE_FREQ 4096 // Sampling frequency of incoming audio, must be at least twice the target frequency
#define AUD_IN_DOWNSAMPLE_RATIO 2 // sinc filter downsample ratio, ideally should be a power of 2
#define AUD_IN_DOWNSAMPLE_FILTER_SIZE 8 // downsample sinc filter number of zero crossing, more crossings will produce a cleaner result but will also use more processor time 
#define AUD_OUT_SAMPLE_FREQ 4096
#define AUD_OUT_INTERP_RATIO 8
#define AUD_OUT_UPSAMPLE_RATIO 8 // sinc filter upsample ratio, ideally should be a power of 2
#define AUD_OUT_UPSAMPLE_FILTER_SIZE 5 // // upsample sinc filter number of zero crossings, more crossings will produce a cleaner result but will also use more processor time 
#define AUD_OUT_TIME 8
#define REC_TIME 8 // Number of seconds of audio to record when frequency test is positive
#define WIN_SIZE 512 // Size of window used when sampling incoming audio; must be a power of 2
// The product of sampleFreq and recordTime must be an integer multiple of winSize.


// Detection algorithm settings:
#define TGT_FREQ 150 // Primary (first harmonic) frequency of mating call to search for
#define FREQ_MARGIN 25  // Margin for error of target frequency
#define HARMONICS 1 // Number of harmonics to search for; looking for more than 3 is not recommended, because this can result in a high false-positive rate.
#define SIG_THRESH 1200 // Threshhold for magnitude of target frequency peak to be considered a positive detection
#define EXP_SIGNAL_LEN 5 // Expected length of the mating call
#define EXP_DET_EFF 1.0 // Minimum expected efficiency by which the detection algorithm will detect target frequency peaks
#define NOISE_FLOOR_MULT 1.0 // uh
#define TIME_AVG_WIN_COUNT 8 // Number of frequency windows used to average frequencies across time

#define SD_OPEN_ATTEMPT_COUNT 10
#define SD_OPEN_RETRY_DELAY_MS 100

//Hardware settings & information:
#define AUD_IN A3
#define AUD_OUT A0

#define HYPNOS_5VR 6
#define HYPNOS_3VR 5

#define SD_CS 11 // Chip select for SD
#define CAM_CS 10
#define AMP_SD 9

#define LOG_INT 3600000 // Miliseconds between status logs [3600000]
#define PLAYBACK_INT 120000 // Milliseconds between playback [900000]

#define CTRL_IMG_INT 3600000
#define IMG_TIME 300000
#define IMG_INT 30000

#define BEGIN_LOG_WAIT_TIME 30000 //3600000

#define SAVE_DETECTION_DELAY_TIME 2000

#define DEBUG 1

#define USE_CAMERA_MODULE 0

#define USE_DETECTION 1

volatile static bool pbs = false;

    // Volatile audio input buffer (LINEAR)
volatile static short inputSampleBuffer[WIN_SIZE];
volatile static int inputSampleBufferPtr = 0;
static const int inputSampleDelayTime = 1000000 / AUD_IN_SAMPLE_FREQ;

volatile static short outputSampleBuffer[AUD_OUT_SAMPLE_FREQ * AUD_OUT_TIME];
volatile static int outputSampleBufferPtr = 0;
volatile static int outputSampleInterpCount = 0;
//static const int outputSampleDelayTime = 1000000 / (AUD_OUT_SAMPLE_FREQ * AUD_OUT_INTERP_RATIO);
static const int outputSampleDelayTime = 1000000 / (AUD_OUT_SAMPLE_FREQ * AUD_OUT_UPSAMPLE_RATIO);
volatile static int interpCount = 0;
static volatile int playbackSampleCount = AUD_OUT_SAMPLE_FREQ * AUD_OUT_TIME;

static ArduCAM CameraModule(OV2640, CAM_CS );

static volatile int nextOutputSample = 0;
static volatile float interpCoeffA = 0;
static volatile float interpCoeffB = 0;
static volatile float interpCoeffC = 0;
static volatile float interpCoeffD = 0;

static const int FFT_WIN_SIZE = int(WIN_SIZE) >> int(log(int(AUD_IN_DOWNSAMPLE_RATIO)) / log(2)); // Size of window used when performing fourier transform of incoming audio; must be a power of 2
static const int FFT_SAMPLE_FREQ = int(AUD_IN_SAMPLE_FREQ) >> int(log(int(AUD_IN_DOWNSAMPLE_RATIO)) / log(2));

static const int sincTableSizeDown = (2 * AUD_IN_DOWNSAMPLE_FILTER_SIZE + 1) * AUD_IN_DOWNSAMPLE_RATIO - AUD_IN_DOWNSAMPLE_RATIO + 1;
static const int sincTableSizeUp = (2 * AUD_OUT_UPSAMPLE_FILTER_SIZE + 1) * AUD_OUT_UPSAMPLE_RATIO - AUD_OUT_UPSAMPLE_RATIO + 1;

// tables holding values corresponding to sinc filter for band limited upsampling/downsampling
static volatile float sincFilterTableDownsample[sincTableSizeDown];
static volatile float sincFilterTableUpsample[sincTableSizeUp];

    // circular input buffer for up sampling
static volatile short downsampleInput[sincTableSizeDown];
static volatile int downsampleInputPtr = 0;
static volatile int downsampleInputPtrCpy = downsampleInputPtr;
static volatile int downsampleInputC = 0;

static volatile short downsampleOutput[FFT_WIN_SIZE];
static volatile int downsampleOutputPtr = 0;

    // circular input buffer for downsampling sampling
static volatile short upsampleInput[sincTableSizeUp];
static volatile int upsampleInputPtr = 0;
static volatile int upsampleInputPtrCpy = upsampleInputPtr;
static volatile int upsampleInputC = 0;

class piedPiper {
  private:
    arduinoFFT FFT = arduinoFFT();  //object for FFT in frequency calcuation
    
    File data;
    
    // This must be an integer multiple of the window size:
    static const int sampleCount = REC_TIME * AUD_IN_SAMPLE_FREQ + WIN_SIZE * TIME_AVG_WIN_COUNT; // [Number of samples required to comprise small + large frequency arrays]
    static const int freqWinCount = REC_TIME * FFT_SAMPLE_FREQ / FFT_WIN_SIZE;

    // Samples array (CIRCULAR)
    //short samples[sampleCount];
    int samplePtr = 0;

    static const int sampleDownsampledCount = sampleCount / AUD_IN_DOWNSAMPLE_RATIO;
    // downsampled samples (CIRCULAR)
    short samplesDownsampled[sampleDownsampledCount];
    int sampleDownsampledPtr = 0;

    // Time smoothing spectral buffer  (CIRCULAR ALONG TIME)
    float rawFreqs[TIME_AVG_WIN_COUNT][FFT_WIN_SIZE / 2];
    int rawFreqsPtr = 0;

    // Spectral data (CIRCULAR ALONG TIME)
    short freqs[freqWinCount][FFT_WIN_SIZE / 2];
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

    void StopAudio();
    void StartAudioOutput();
    void StartAudioInput();
    
    void SmoothFreqs(int winSize);
    bool CheckFreqDomain(int t);
    
    void ResetSPI();
    bool BeginSD();

    void(*ISRStopFn)();
    void(*ISRStartFn)(const unsigned long interval_us, void(*fnPtr)());
    uint8_t read_fifo_burst(ArduCAM CameraModule);
    
  public:
    piedPiper(void(*audStart)(const unsigned long interval_us, void(*fnPtr)()), void(*audStop)()) ;

    static void RecordSample();
    static void OutputSample();
    static void OutputUpsampledSample();
    bool InputSampleBufferFull();
    void ProcessData();
    void calculateDownsamplSincFilterTable();
    void calculateUpsampleSincFilterTable();
    bool InsectDetection();
    void Playback();
    void LoadSound();
    void LogAlive();
    bool TakePhoto(int n);
    void SaveDetection();
    bool InitializeAudioStream();
    bool LoadSound(char* fname);
    void ResetOperationIntervals();

    void SetPhotoNum(int n);
    void SetDetectionNum(int n);

    RTC_DS3231 rtc;
    
    unsigned long GetLastPlaybackTime();
    unsigned long GetLastPhotoTime();
    unsigned long GetLastDetectionTime();
    unsigned long GetLastLogTime();
    int GetDetectionNum();
    int GetPhotoNum();
};
