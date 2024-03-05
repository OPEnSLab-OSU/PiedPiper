#include "piedPiper.h"
#include "SAMDTimerInterrupt.h"

// Timer object for managing ISR's for audio recording and playback
SAMDTimer ITimer0(TIMER_TC3);

// Functions to start and stop the ISR timer (these are passed to the main Pied Piper class)
void stopISRTimer()
{
  ITimer0.detachInterrupt();
  ITimer0.restartTimer();
}

void startISRTimer(const unsigned long interval_us, void(*fnPtr)())
{
  ITimer0.attachInterruptInterval(interval_us, fnPtr);
}

// Create the Pied Piper class, and initialize any necessary global variables
piedPiper p(startISRTimer, stopISRTimer);
unsigned long currentTime = 0;
unsigned long lastTime = 0;

void setup() {
  // Begin serial communication and configure the analog read and write resolutions to their maximum possible values.
  Serial.begin(2000000);
  analogReadResolution(ANALOG_RES);
  analogWriteResolution(ANALOG_RES);
  delay(4000);

  Serial.println("Initializing");

  // Configure pins
  pinMode(AUD_IN, INPUT);
  pinMode(AUD_OUT, OUTPUT);
  pinMode(HYPNOS_5VR, OUTPUT);
  pinMode(HYPNOS_3VR, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  pinMode(CAM_CS, OUTPUT);
  pinMode(AMP_SD, OUTPUT);

  // Enable the 3 volt rail.
  digitalWrite(HYPNOS_3VR, LOW);

  Serial.println("Testing SD...");

  delay(1000);

  // Verify that SD can be initialized; stop the program if it can't.
  if (!SD.begin(SD_CS))
  {
    Serial.println("SD failed to initialize.");
    initializationFailFlash();
  }

  // Verify that the SD card has all the required files and the correct directory structure.
  if ((SD.exists("/PBAUD/FPBX.PAD") && SD.exists("/DATA/DETS/DETS.txt") && SD.exists("/DATA/PHOTO/PHOTO.txt")))
  {
    Serial.println("SD card has correct directory structure. Setting photo and detection numbers...");

    // Set current photo number
    int pn = 1;
    char str[10];

    // Count the number of images inside /DATA/PHOTO/
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

    // Set the photo number to match the number of photos inside the directory.
    p.SetPhotoNum(pn - 1);


    // Set detection number
    int dn = 1;

    // Count the number of images inside /DATA/DETS/
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
    
    // Set the detection number to match the number of detections inside the directory.
    p.SetDetectionNum(dn - 1);

    // Print out the current photo and detection numbers.
    Serial.print("Photo number: ");
    Serial.println(p.GetPhotoNum());
    Serial.print("Detection number: ");
    Serial.println(p.GetDetectionNum());
  }
  else
  {
    // Stop the program if the incorrect directory structure was found.
    Serial.println("WARNING: SD card does not have the correct directory structure.");
    initializationFailFlash();
  }

  SD.end();
  Wire.begin();
  
  Serial.println("Initializing RTC...");

  // Initialize RTC
  if (!p.rtc.begin())
  {
    Serial.println("RTC failed to begin.");
    initializationFailFlash();
  }

  // Check if RTC lost power; adjust the clock to the compilation time if it did.
  if (p.rtc.lostPower())
  {
    Serial.println("RTC lost power, adjusting...");
    p.rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Disable the 3-volt rail.
  digitalWrite(HYPNOS_3VR, HIGH);

  Wire.end();

  // Enable audio sampling
  ITimer0.attachInterruptInterval(inputSampleDelayTime, p.RecordSample);

  // Load the prerecorded mating call to play back
  p.LoadSound("FPBX.PAD");
  p.Playback();

  // Test the camera module by taking 3 test images
  for (int i = 0; i < 3; i++)
  {
    if (!p.TakePhoto(0))
    {
      // Stop the program if the camera module failed to take an image
      Serial.println("Camera module failure!");
      //initializationFailFlash();
    }
    else
    {
      Serial.println("Successfully took test photo.");
    }
  }

  // Wait here for 30 minutes (or until the user sends a character over)
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

  // If the time of the last loop is later than the time of the current loop, then a clock rollover has occured.
  // Deal with this by resetting the times of all the last intermittent operations (playback, photos, etc) to 0.
  if (currentTime < lastTime)
  {
    p.ResetOperationIntervals();
  }

  // Check if the input sample buffer is full
  if (p.InputSampleBufferFull())
  {
    // Move the data out of the buffer, reset the volatile sample pointer, and process the audio samples.
    p.ProcessData();

    // Check if the newly recorded audio contains a mating call
    if (p.InsectDetection())
    {
      // Continue recording audio for SAVE_DETECTION_DELAY_TIME milliseconds before taking photos and performing playback,
      // to make sure that all (or at least most) of the mating call is captured.
      
      while (millis() - currentTime < SAVE_DETECTION_DELAY_TIME)
      {
        if (p.InputSampleBufferFull())
        {
          p.ProcessData();
        }
      }

      // Save the recorded detection to the SD card, play back an artificial mating call, and take a photo when a detection occurs.
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

    // If a detection has recently occured, then take images at a rapid pace (with interval IMG_INT milliseconds)
    if ((currentTime - p.GetLastDetectionTime() < IMG_TIME) && (currentTime - p.GetLastPhotoTime() > IMG_INT))
    {
      p.TakePhoto(p.GetDetectionNum());
    }

    // If there have been no recent detections, then take photos with interval CTRL_IMG_INT
    else if (currentTime - p.GetLastPhotoTime() > CTRL_IMG_INT)
    {
      p.TakePhoto(0);
    }

    // Occasionally log the current millisecond timer value to a text file to show that the trap is still alive.
    if (currentTime - p.GetLastLogTime() > LOG_INT)
    {
      p.LogAlive();
    }
  }
}

// If something fails to initialize, stop the program and execute this function that repeatedly blinks
// the camera flash to show that there's a problem.
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
