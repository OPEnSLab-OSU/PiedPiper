#include <Wire.h>
#include "piedPiper.h"


// Mode 1: Print samples to plotter
// Mode 2: Print info to monitor, use debug LED
// Blue: Waiting for positive initial frequency domain test
// Red: Initial frequency domain test positive; recording & analyzing samples
// Green: Positive detection

piedPiper p;
int t = 0;

void setup() {
  Serial.begin(2000000);
  delay(1000);

  Serial.println("Initializing");
  analogReadResolution(12);

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

  digitalWrite(camPow, HIGH);
  digitalWrite(camImg, HIGH);

  Serial.println("Pins configured, performing playback...");
  
  p.playback();
  
  Serial.println("Testing SD...");
  
  if (SD.begin(card_select))
  {
    Serial.println("SD initialized.");
  }
  else
  {
    Serial.println("SD failed to initialize.");
  }

  digitalWrite(camPow, LOW);
  digitalWrite(camImg, LOW);

  //SD.end();
}

void loop() {
  if (p.insectDetection())
  {
    p.takePhoto(p.detectionNum);
    p.playback();
  }
  else
  {
    t = millis();

    // Take photos after a detection
    if ((t - p.lastPhotoTime > imgInt) && (t - p.lastDetectionTime < imgTime) && p.detectionNum != 0)
    {
      //Serial.println("Taking detection photo");
      p.takePhoto(p.detectionNum);
    }

    //INtermittent playback
    if ((t - p.lastPlaybackTime) > playbackInt)
    {
      //Serial.println("Performing intermittent playback");
      p.playback();
    }
    
    // Take control photos
    if (t - p.lastPhotoTime > ctrlImgInt)
    {
      //Serial.println("Taking control photo");
      p.takePhoto(0);
    }
    
    // Report aliveness
    if ((t - p.lastLogTime) > logInt)
    {
      p.reportAlive();
    }
  }
}
