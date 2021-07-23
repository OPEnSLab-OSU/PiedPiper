#include <Wire.h>
#include "piedPiper.h"


// Mode 1: Print samples to plotter
// Mode 2: Print info to monitor, use debug LED
// Blue: Waiting for positive initial frequency domain test
// Red: Initial frequency domain test positive; recording & analyzing samples
// Green: Positive detection

piedPiper p;

void setup() {
  //analogReadResolution(12);
  pinMode(audIn, INPUT);
  Serial.begin(2000000);
  SD.begin(card_select);
}

void loop() {
  if (p.insectDetection())
  {
    p.takePhoto();
    p.playback();
  }
  else
  {
    if ((millis() - p.lastPhotoTime > imgInt) && (millis() - p.lastDetectionTime < imgTime))
    {
      p.takePhoto();
    }
  }
}
