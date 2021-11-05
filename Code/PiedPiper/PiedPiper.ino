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
  analogReadResolution(12);
  delay(1000);

  Serial.println("Initializing");
  
  pinMode(AUD_IN, INPUT);
  pinMode(HYPNOS_5VR, OUTPUT);
  pinMode(HYPNOS_3VR, OUTPUT);
  pinMode(CAM_IMG_PIN, OUTPUT);
  
  digitalWrite(HYPNOS_3VR, LOW);
  digitalWrite(CAM_IMG_PIN, HIGH);

  Serial.println("Pins configured, performing playback...");

  Serial.println("Testing SD...");

  if (SD.begin(SD_CS))
  {
    Serial.println("SD initialized.");
  }
  else
  {
    Serial.println("SD failed to initialize.");
  }

  digitalWrite(HYPNOS_3VR, HIGH);
  SD.end();

  p.InitializeAudioStream();
}

void loop() {
  if (p.InputSampleBufferFull())
  {
    p.ProcessData();

    if (p.InsectDetection())
    {
      p.SaveDetection();
      p.Playback();
      p.TakePhoto(p.GetDetectionNum());
    }
  }
  else
  {
    unsigned long t = millis();

    //Intermittent playback
    if (t - p.GetLastPlaybackTime() > PLAYBACK_INT)
    {
      p.Playback();
    }

    // Detection photos
    if ((t - p.GetLastDetectionTime() < IMG_TIME) && (t - p.GetLastPhotoTime() > IMG_INT))
    {
      p.TakePhoto(p.GetDetectionNum());
    }

    // Control photos
    else if (t - p.GetLastPhotoTime() > CTRL_IMG_INT)
    {
      p.TakePhoto(0);
    }

    // Aliveness logging
    if (t - p.GetLastLogTime() > LOG_INT)
    {
      p.LogAlive();
    }
  }

  //public:
  //functions:
  //p.InputSampleBufferFull() => bool
  //p.ProcessNewData() => void
  //p.InsectDetection() => bool
  //p.Playback() => void
  //p.LogAlive() => void
  //p.TakePhoto() => void
  //p.Playback() => void
  //p.GetLastPlaybackTime() => unsigned long
  //p.GetLastPhotoTime() => unsigned long
  //p.GetLastDetectionTime() => unsigned long
  //p.GetLastLogTime() => unsigned long
  //p.GetDetectionNum() => int
  //private:
}
