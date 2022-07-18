#include "piedPiper.h"
#include "SAMDTimerInterrupt.h"

SAMDTimer ITimer0(TIMER_TC3);



void stopISRTimer()
{
  ITimer0.detachInterrupt();
  ITimer0.restartTimer();
}

void startISRTimer(const unsigned long interval_us, void(*fnPtr)())
{
  ITimer0.attachInterruptInterval(interval_us, fnPtr);
}

piedPiper p(startISRTimer, stopISRTimer);
unsigned long currentTime = 0;
unsigned long lastTime = 0;

void setup() {
  Serial.begin(2000000);
  analogReadResolution(12);
  analogWriteResolution(12);
  delay(4000);

  Serial.println("Initializing");

  pinMode(AUD_IN, INPUT);
  pinMode(AUD_OUT, OUTPUT);
  pinMode(HYPNOS_5VR, OUTPUT);
  pinMode(HYPNOS_3VR, OUTPUT);

  digitalWrite(HYPNOS_3VR, LOW);

  //Serial.println("Testing SD...");

  if (!SD.begin(SD_CS))
  {
    Serial.println("SD failed to initialize.");
    initializationFailFlash();
  }

  if ((SD.exists("/PBAUD/FPBX.PAD") && SD.exists("/DATA/DETS/DETS.txt") && SD.exists("/DATA/PHOTO/PHOTO.txt")))
  {
    //Serial.println("SD card has correct directory structure. Setting photo and detection numbers...");
    
    // Set current photo number
    int pn = 1;
    char str[10];

    while (1)
    {
      char dir[28];
      strcpy(dir, "/DATA/PHOTO/");
      itoa(pn, str, 10);
      strcat(dir, str);
      strcat(dir, ".jpg");
      
      //Serial.println(dir);
      
      if (SD.exists(dir))
      {
        pn++;
      }
      else
      {
        break;
      }
    }

    p.SetPhotoNum(pn - 1);
    

    // Set detection number
    int dn = 1;

    while (1)
    {
      char dir[28];
      strcpy(dir, "/DATA/DETS/");
      itoa(dn, str, 10);
      strcat(dir, str);
      strcat(dir, ".txt");

      if (SD.exists(dir))
      {
        dn++;
      }
      else
      {
        break;
      }
    }

    p.SetDetectionNum(dn - 1);

    //Serial.print("Photo number: ");
    //Serial.println(p.GetPhotoNum());
    //Serial.print("Detection number: ");
    //Serial.println(p.GetDetectionNum());
  }
  else
  {
    Serial.println("WARNING: SD card does not have the correct directory structure.");
    initializationFailFlash();
  }

  //Serial.println("Initializing RTC...");

  if (!p.rtc.begin())
  {
    Serial.println("RTC failed to begin.");
    initializationFailFlash();
  }

  if (p.rtc.lostPower())
  {
    p.rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  digitalWrite(HYPNOS_3VR, HIGH);

  SD.end();
  SPI.end();

  ITimer0.attachInterruptInterval(inputSampleDelayTime, p.RecordSample);

  p.LoadSound("FPBX.PAD");
  p.Playback();

  Serial.println("Send any character to skip the 30 minute delay.");
  long ct = millis();
  while (millis() - ct < BEGIN_LOG_WAIT_TIME)
  {
    if (Serial.available())
    {
      while (Serial.available())
      {
        Serial.read();
      }
      
      break;
    }
    delay(1000);
  }
}

void loop() {
  lastTime = currentTime;
  currentTime = millis();

  if (currentTime < lastTime)
  {
    p.ResetOperationIntervals();
  }

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
    //Intermittent playback
    if (currentTime - p.GetLastPlaybackTime() > PLAYBACK_INT)
    {
      p.Playback();
    }

    // Detection photos
    if ((currentTime - p.GetLastDetectionTime() < IMG_TIME) && (currentTime - p.GetLastPhotoTime() > IMG_INT))
    {
      p.TakePhoto(p.GetDetectionNum());
    }

    // Control photos
    else if (currentTime - p.GetLastPhotoTime() > CTRL_IMG_INT)
    {
      p.TakePhoto(0);
    }

    // Aliveness logging
    if (currentTime - p.GetLastLogTime() > LOG_INT)
    {
      p.LogAlive();
    }
  }
}

void initializationFailFlash()
{
  while (1)
  {
    digitalWrite(HYPNOS_3VR, LOW);
    delay(500);
    digitalWrite(HYPNOS_3VR, HIGH);
    delay(500);
  };
}
