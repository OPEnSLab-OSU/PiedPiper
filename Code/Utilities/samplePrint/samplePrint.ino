#include <arduinoFFTFloat.h>
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

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
#define camPow 4 // Power control pin [CHANGE]
#define camImg 1 // Imaging control pin [CHANGE]

#define hypnos 0

int opmode = 0;

arduinoFFT FFT = arduinoFFT();  //object for FFT in frequency calcuation
// This must be an integer multiple of the window size:
static const short sampleCount = recordTime * sampleFreq;
static const short winCount = sampleCount / winSize;
static const int delayTime = round(1000000 / sampleFreq - analogReadTime);

short sampleBuffer[sampleCount];
int printBuffSize = 500;
float vReal[winSize];
float vImag[winSize];
float freqs[winCount][winSize / 2];

File dat;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(2000000);
  analogReadResolution(12);
  Serial.println("Beginning");
  //indicator.setPixelColor(0, 64, 0, 0);
  //indicator.show();

  if (hypnos)
  {
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    digitalWrite(5, LOW);
    digitalWrite(6, HIGH);
  }
  
  pinMode(audIn, INPUT);
  pinMode(camPow, OUTPUT);
  pinMode(camImg, OUTPUT);
  pinMode(clk, OUTPUT);
  pinMode(latch, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(SHTDWN, OUTPUT);

  digitalWrite(camPow, LOW);
  
  playback();
  
  while (1)
  {

    while (Serial.available())
    {
      byte dat = Serial.read();

      if (dat == 's')
      {
        opmode = 0;
      }
      else if (dat == 'd')
      {
        opmode = 1;
      }
      else if (dat == 'r')
      {
        opmode = 2;
      }
      else
      {
        continue;
      }
    }


    if (opmode == 0)
    {
      analogRead(audIn);
      
      for (int i = 0; i < printBuffSize; i++) {
        sampleBuffer[i] = analogRead(audIn);

        delayMicroseconds(delayTime);

      }

      for (int i = 0; i < printBuffSize; i++) {
        Serial.println(sampleBuffer[i]);
      }
    }
    else if (opmode == 1)
    {
      insectDetection();
    }
    else if (opmode == 2)
    {
      Serial.println("Opening SD...");

      while (!SD.begin(card_select))
      {
        Serial.println("Failed; trying again...");
      }

      Serial.println("Success");
      Serial.println("Opening file...");
      //indicator.setPixelColor(0, 64, 0, 0);
      //indicator.show();

      dat = SD.open("READTEST.txt", FILE_WRITE);

      while (!dat)
      {
        Serial.println("Failed; trying again...");
        dat = SD.open("READTEST.txt", FILE_WRITE);
      }

      Serial.println("Success");
      Serial.println("Recording...");

      for (int i = 0; i < sampleCount; i++)
      {
        sampleBuffer[i] = analogRead(audIn);

        delayMicroseconds(delayTime);
      }

      Serial.println("Finished");
      Serial.println("Writing...");

      for (int i = 0; i < sampleCount; i++)
      {
        dat.println(sampleBuffer[i], DEC);
      }

      Serial.println("Finished writing.");

      dat.close();

      break;
    }
    else
    {
      opmode = 0;
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:


}

bool insectDetection()
{
  Serial.println("Running detection algorithm");
  recordSamples(winSize);

  if (initFreqTest())
  {
    Serial.println("Positive initial frequency test; recording & processing for full test...");
    recordSamples(sampleCount);

    processSignal();

    if (fullSignalTest())
    {
      Serial.println("!POSITIVE DETECTION!");
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}

bool initFreqTest()
{
  for (int i = 0; i < winSize; i++)
  {
    vReal[i] = sampleBuffer[i];
    vImag[i] = 0.0;
  }

  FFT.DCRemoval(vReal, winSize); // Remove DC component of signal
  FFT.Windowing(vReal, winSize, FFT_WIN_TYP_HAMMING, FFT_FORWARD); // Apply windowing function to data
  FFT.Compute(vReal, vImag, winSize, FFT_FORWARD); // Compute FFT
  FFT.ComplexToMagnitude(vReal, vImag, winSize); // Compute frequency magnitudes

  for (int i = 0; i < winSize / 2; i++)
  {
    freqs[0][i] = vReal[i];
  }

  smoothFreq(16);

  for (int i = 0; i < winSize / 2; i++)
  {
    freqs[0][i] = max(0, freqs[0][i] - noiseFloorThresh * vReal[i]);
  }

  return checkFreqDomain(0);
}

void processSignal()
{
  for (int t = 0; t < winCount; t++)
  {
    for (int v = 0; v < winSize; v++)
    {
      vReal[v] = sampleBuffer[winSize * t + v];
      vImag[v] = 0.0;
    }

    FFT.DCRemoval(vReal, winSize); // Remove DC component of signal
    FFT.Windowing(vReal, winSize, FFT_WIN_TYP_HAMMING, FFT_FORWARD); // Apply windowing function to data
    FFT.Compute(vReal, vImag, winSize, FFT_FORWARD); // Compute FFT
    FFT.ComplexToMagnitude(vReal, vImag, winSize); // Compute frequency magnitudes

    for (int v = 0; v < winSize / 2; v++)
    {
      freqs[t][v] = vReal[v];
    }

  }

  //Perform time smoothing
  timeSmoothFreq(16);

  //Perform frequency smoothing
  for (int t = 0; t < winCount; t++)
  {
    for (int v = 0; v < winSize / 2; v++)
    {
      vReal[v] = freqs[t][v];
    }

    smoothFreq(4);

    for (int v = 0; v < winSize / 2; v++)
    {
      freqs[t][v] = vReal[v];
    }
  }

  // Subtract noise background
  for (int t = 0; t < winCount; t++)
  {
    for (int v = 0; v < winSize / 2; v++)
    {
      vReal[v] = freqs[t][v];
    }

    smoothFreq(16);

    for (int v = 0; v < winSize / 2; v++)
    {
      freqs[t][v] = max(0, freqs[t][v] - noiseFloorThresh * vReal[v]);
    }
  }
}

// Tests all of the frequency data stored in [freqs], and determines if an insect is present.
bool fullSignalTest()
{
  int count = 0;

  // Determine if there are target frequency peaks at every window in the frequency data array
  for (int t = 0; t < winCount; t++)
  {
    if (checkFreqDomain(t))
    {
      count++;
    }
  }

  // If the number of windows that contain target frequency peaks is at or above the expected number that would be present
  return (count >= signalLen * (winCount / recordTime) * detectionEfficiency);
}

// Performs rectangular smoothing on frequency data stored in [vReal]
void smoothFreq(int avgWinSize)
{
  float inptDup[winSize];
  int upperBound = 0;
  int lowerBound = 0;
  int avgCount = 0;

  for (int i = 0; i < winSize; i++)
  {
    inptDup[i] = vReal[i];
    vReal[i] = 0;
  }

  for (int i = 0; i < winSize; i++)
  {
    lowerBound = max(0, i - avgWinSize / 2);
    upperBound = min(winSize - 1, i + avgWinSize / 2);
    avgCount = 0;

    for (int v = lowerBound; v < upperBound + 1; v++)
    {
      vReal[i] += inptDup[v];
      avgCount++;
    }

    vReal[i] /= avgCount;
  }
}

// Performs rectangular smoothing on frequency data stored in [freqs] over time to reduce transient noise
void timeSmoothFreq(int avgWinSize)
{
  float inptDup[winCount];
  int upperBound = 0;
  int lowerBound = 0;
  int avgCount = 0;

  for (int f = 0; f < winSize / 2; f++)
  {
    for (int t = 0; t < winCount; t++)
    {
      inptDup[t] = freqs[t][f];
      freqs[t][f] = 0;
    }

    for (int t = 0; t < winCount; t++)
    {
      lowerBound = max(0, t - avgWinSize / 2);
      upperBound = min(winCount, t + avgWinSize / 2);
      avgCount = 0;

      for (int i = lowerBound; i < upperBound + 1; i++)
      {
        freqs[t][f] += inptDup[i];
        avgCount++;
      }

      freqs[t][f] /= avgCount;
    }
  }
}

// Determines if there is a peak in the target frequency range (including harmonics).
bool checkFreqDomain(int t)
{
  int lowerIdx = floor(((targetFreq - freqMargin) * 1.0) / sampleFreq * winSize);
  int upperIdx = ceil(((targetFreq + freqMargin) * 1.0) / sampleFreq * winSize);

  int count = 0;

  for (int h = 1; h <= harms; h++)
  {
    for (int i = (h * lowerIdx); i < (h * upperIdx); i++)
    {
      if (((freqs[t][i + 1] - freqs[t][i]) < 0) && ((freqs[t][i] - freqs[t][i - 1]) > 0))
      {
        if (freqs[t][i] > significanceThresh)
        {
          count++;
        }
      }
    }
  }

  return (count > 0);
}

// Records [samples] samples into the sample buffer, starting from 0.
void recordSamples(int samples)
{
  for (int i = 0; i < samples; i++)
  {
    sampleBuffer[i] = analogRead(audIn);
    delayMicroseconds(delayTime);
  }
}


void playback()
{
  //Serial.println("Beginning playback");
  const byte soundNum = 0b11111110;

  digitalWrite(SHTDWN, HIGH);
  digitalWrite(latch, LOW); //turns off register
  digitalWrite(clk, LOW); //sets clock low indicating the start of a byte
  shiftOut(dataPin, clk, MSBFIRST, soundNum); //sends data to shift register
  digitalWrite(latch, HIGH); //turns on output
}
