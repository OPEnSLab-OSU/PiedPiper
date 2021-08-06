#include <Wire.h>
#include "piedPiper.h"


// Mode 1: Print samples to plotter
// Mode 2: Print info to monitor, use debug LED
// Blue: Waiting for positive initial frequency domain test
// Red: Initial frequency domain test positive; recording & analyzing samples
// Green: Positive detection

piedPiper p;

void setup() {
  analogReadResolution(12);
  pinMode(audIn, INPUT);
  Serial.begin(2000000);
  
  if (SD.begin(card_select))
  {
    Serial.println("SD initialized.");
  }
  else
  {
    Serial.println("SD failed to initialize.");
  }
}

void loop() {
  if (p.insectDetection())
  {
    p.takePhoto();
    p.playback();
  }
  else
  {
    // Take photos after a detection
    if ((millis() - p.lastPhotoTime > imgInt) && (millis() - p.lastDetectionTime < imgTime) && p.detectionNum != 0)
    {
      p.takePhoto(p.detectionNum);
    }

    // Take control photos
    if (millis() - p.lastPhotoTime > ctrlImgInt)
    {
      p.takePhoto(0);
    }
    
    // Report aliveness
    if ((millis() - p.lastLogTime) > logInt)
    {
      p.reportAlive();
    }
  }
}
