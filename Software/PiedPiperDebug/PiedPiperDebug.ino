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
  pinMode(SD_CS, OUTPUT);
  pinMode(CAM_CS, OUTPUT);
  pinMode(AMP_SD, OUTPUT);

  digitalWrite(HYPNOS_3VR, LOW);

  //Serial.println("Testing SD...");

  if (!SD.begin(SD_CS))
  {
    Serial.println("SD failed to initialize.");
    initializationFailFlash();
  }

  Wire.begin();
  SPI.begin();

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

    Serial.print("Photo number: ");
    Serial.println(p.GetPhotoNum());
    Serial.print("Detection number: ");
    Serial.println(p.GetDetectionNum());
  }
  else
  {
    Serial.println("WARNING: SD card does not have the correct directory structure.");
    initializationFailFlash();
  }

  Serial.println("Initializing RTC...");

  if (!p.rtc.begin())
  {
    Serial.println("RTC failed to begin.");
    initializationFailFlash();
  }

  if (p.rtc.lostPower())
  {
    Serial.println("RTC lost power, adjusting...");
    p.rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  digitalWrite(HYPNOS_3VR, HIGH);

  SD.end();

  ITimer0.attachInterruptInterval(inputSampleDelayTime, p.RecordSample);

  Serial.println("Nominal state (3VR and 5VR off, audio input enabled) established.");

  p.LoadSound("FPBX.PAD");

  /*/
  p.Playback();
/*/

/*/
  for (int i = 0; i < 3; i++)
  {
    if (!p.TakePhoto(0))
    {
      Serial.println("Camera module failure!");
      //initializationFailFlash();
    }
    else
    {
      Serial.println("Successfully took test photo.");
    }
  }
  /*/

  Serial.println("Main loop begin");
  Serial.println("d - Test detection algorithm\np - Take photo\ns - Perform playback");
}

bool enableDetectionAlg = false;

void loop() {
  lastTime = currentTime;
  currentTime = millis();

  while (Serial.available())
  {
    char ch = Serial.read();

    // Photo, playback, detection algorithm

    if (ch == 'd')
    {
      if (enableDetectionAlg)
      {
        Serial.println("Disabling detection algorithm");
        enableDetectionAlg = false;
      }
      else
      {
        Serial.println("Enabling detection algorithm");
        enableDetectionAlg = true;
      }
    }

    else if (ch == 's')
    {
      Serial.println("Performing playback");
      p.Playback();
    }

    else if (ch == 'p')
    {
      Serial.println("Taking photo");
      p.TakePhoto(0);
    }

    else if (ch == 'm')
    {
      Serial.print("Memory remaining: ");

      Serial.println(FreeRAM());
    }
  }

  if (enableDetectionAlg)
  {
    if (p.InputSampleBufferFull())
    {
      p.ProcessData();

      if (p.InsectDetection())
      {
        Serial.println("Positive detection");
      }
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

extern "C" char *sbrk(int i);

int FreeRAM () {
  char stack_dummy = 0;
  return &stack_dummy - sbrk(0);
}
