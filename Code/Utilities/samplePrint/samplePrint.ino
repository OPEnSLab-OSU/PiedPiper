#include <arduinoFFTFloat.h>
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "SAMDTimerInterrupt.h"

// Audio sampling settings:
#define SAMPLE_FREQ 4096 // Sampling frequency of incoming audio, must be at least twice the target frequency
#define REC_TIME 8 // Number of seconds of audio to record when frequency test is positive
#define FFT_WIN_SIZE 256 // Size of window used when performing fourier transform of incoming audio; must be a power of 2
// The product of sampleFreq and recordTime must be an integer multiple of winSize.

// Detection algorithm settings:
#define TGT_FREQ 175 // Primary (first harmonic) frequency of mating call to search for
#define FREQ_MARGIN 25 // Margin for error of target frequency
#define HARMONICS 2 // Number of harmonics to search for; looking for more than 3 is not recommended, because this can result in a high false-positive rate.
#define SIG_THRESH 15 // Threshhold for magnitude of target frequency peak to be considered a positive detection
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

//Hardware settings & information:
#define audIn A3

int opmode = 0;

arduinoFFT FFT = arduinoFFT();  //object for FFT in frequency calcuation
// This must be an integer multiple of the window size:
static const int delayTime = round(1000000 / SAMPLE_FREQ - 23);

const int sampleCount = 500;
short sampleBuffer[sampleCount];

File dat;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(2000000);
  analogReadResolution(12);
  Serial.println("Beginning");
  Serial1.begin(9600);
  //indicator.setPixelColor(0, 64, 0, 0);
  //indicator.show();
  
  pinMode(audIn, INPUT);
  pinMode(HYPNOS_5VR, OUTPUT);
  
  digitalWrite(HYPNOS_5VR, HIGH);
  digitalWrite(HYPNOS_3VR, HIGH);

  unsigned long lastPlaybackTime = 0;
  
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
      //analogRead(audIn);
      
      for (int i = 0; i < sampleCount; i++) {
        sampleBuffer[i] = analogRead(audIn);

        delayMicroseconds(delayTime);

      }

      for (int i = 0; i < sampleCount; i++) {
        Serial.println(sampleBuffer[i]);
      }
    }
    else if (opmode == 1)
    {
      //insectDetection();
    }
    
    else if (opmode == 2)
    {
      /*/
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
      /*/
    }
    else
    {
      opmode = 0;
    }

    if (millis() - lastPlaybackTime > 7000)
    {
      Serial1.println("#00");
      lastPlaybackTime = millis();
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  

}
