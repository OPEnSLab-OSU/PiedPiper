#include "piedPiper.h"

// Class initialization
piedPiper::piedPiper(void(*tStart)(), void(*tStop)()) {
  timerStart = tStart;
  timerStop = tStop;
}

// Used to stop the interrupt timer for sample recording, so that it does not interfere with SD and Serial communication
void piedPiper::StopAudioStream()
{
  (*timerStop)();
  Serial.println("Audio stream stopped");
}

// Used to restart the sample recording timer once SPI and Serial communications have completed.
void piedPiper::RestartAudioStream()
{
  Serial.println("Restarting audio stream");
  (*timerStart)();
}

// This records new audio samples into the sample buffer asyncronously using a hardware timer.
// Before recording a sample, it first checks if the buffer is full.
// If it is full, it does not record a sample.
// If it is not full, it records a sample and moves the pointer over.
void piedPiper::RecordSample(void)
{
  if (inputSampleBufferPtr < FFT_WIN_SIZE)
  {
    inputSampleBuffer[inputSampleBufferPtr] = analogRead(AUD_IN);
    inputSampleBufferPtr++;
  }
}

// Determines if the sample buffer is full, and is ready to have frequencies computed.
bool piedPiper::InputSampleBufferFull()
{
  return !(inputSampleBufferPtr < FFT_WIN_SIZE);
}

// Called when the input sample buffer is full, to incorperate the new data into the large "cold" frequency array
// It clears the input buffer (by just resetting the pointer to the start of the array)
// It performs time and frequency smoothing on the new data
// It adds the new frequency block to the large frequency array.
void piedPiper::ProcessData()
{
  //Quickly pull data from volatile buffer into sample buffer, iterating pointer as needed.
  for (int i = 0; i < FFT_WIN_SIZE; i++)
  {
    samples[samplePtr] = inputSampleBuffer[i];
    vReal[i] = inputSampleBuffer[i];
    vImag[i] = 0.0;
    samplePtr = IterateCircularBufferPtr(samplePtr, sampleCount);
    //Serial.println(inputSampleBuffer[i]);
  }

  inputSampleBufferPtr = 0;

  //Calculate FFT of new data and put into time smoothing buffer
  FFT.DCRemoval(vReal, FFT_WIN_SIZE); // Remove DC component of signal
  FFT.Windowing(vReal, FFT_WIN_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD); // Apply windowing function to data
  FFT.Compute(vReal, vImag, FFT_WIN_SIZE, FFT_FORWARD); // Compute FFT
  FFT.ComplexToMagnitude(vReal, vImag, FFT_WIN_SIZE); // Compute frequency magnitudes

  for (int i = 0; i < FFT_WIN_SIZE / 2; i++)
  {
    rawFreqs[rawFreqsPtr][i] = vReal[i];
    vReal[i] = 0.0;
  }

  rawFreqsPtr = IterateCircularBufferPtr(rawFreqsPtr, TIME_AVG_WIN_COUNT);
  

  for (int t = 0; t < TIME_AVG_WIN_COUNT; t++)
  {
    for (int f = 0; f < FFT_WIN_SIZE / 2; f++){
      vReal[f] += rawFreqs[t][f] * (1.0 / TIME_AVG_WIN_COUNT);
    }
  }

  SmoothFreqs(4);

  for (int f = 0; f < FFT_WIN_SIZE / 2; f++)
  {
    freqs[freqsPtr][f] = vReal[f];
  }

  SmoothFreqs(16);

  for (int f = 0; f < FFT_WIN_SIZE / 2; f++)
  {
    freqs[freqsPtr][f] = freqs[freqsPtr][f] - max(0, NOISE_FLOOR_MULT * vReal[f]);
  }

  freqsPtr = IterateCircularBufferPtr(freqsPtr, freqWinCount);
}

// Performs rectangular smoothing on frequency data stored in [vReal]
void piedPiper::SmoothFreqs(int winSize)
{
  float inptDup[FFT_WIN_SIZE / 2];
  int upperBound = 0;
  int lowerBound = 0;
  int avgCount = 0;

  for (int i = 0; i < winSize; i++)
  {
    inptDup[i] = vReal[i];
    vReal[i] = 0;
  }

  for (int i = 0; i < FFT_WIN_SIZE / 2; i++)
  {
    lowerBound = max(0, i - winSize / 2);
    upperBound = min((FFT_WIN_SIZE / 2) - 1, i + winSize / 2);
    avgCount = 0;

    for (int v = lowerBound; v < upperBound + 1; v++)
    {
      vReal[i] += inptDup[v];
      avgCount++;
    }

    vReal[i] /= avgCount;
  }
}

// This tests the cold frequency array for the conditions required for a positive detection.
// A positive detection is made when at least [THRESHHOLD] frequency peaks of [THRESHHOLD] magnitudes are found in the target frequency harmonics.
bool piedPiper::InsectDetection()
{
  int count = 0;

  // Determine if there are target frequency peaks at every window in the frequency data array
  for (int t = 0; t < freqWinCount; t++)
  {
    if (CheckFreqDomain(t))
    {
      count++;
    }
  }
  
  Serial.println(count);
  // If the number of windows that contain target frequency peaks is at or above the expected number that would be present
  return (count >= EXP_SIGNAL_LEN * (freqWinCount / REC_TIME) * EXP_DET_EFF);
}

// Determines if there is a peak in the target frequency range at a specific time window (including harmonics).
bool piedPiper::CheckFreqDomain(int t)
{
  int lowerIdx = floor(((TGT_FREQ - FREQ_MARGIN) * 1.0) / SAMPLE_FREQ * FFT_WIN_SIZE);
  int upperIdx = ceil(((TGT_FREQ + FREQ_MARGIN) * 1.0) / SAMPLE_FREQ * FFT_WIN_SIZE);

  int count = 0;

  for (int h = 1; h <= HARMONICS; h++)
  {
    for (int i = (h * lowerIdx); i < (h * upperIdx); i++)
    {
      if (((freqs[t][i + 1] - freqs[t][i]) < 0) && ((freqs[t][i] - freqs[t][i - 1]) > 0))
      {
        if (freqs[t][i] > SIG_THRESH)
        {
          count++;
        }
      }
    }
  }

  return (count > 0);
}

// Records detection data to the uSD card, and clears all audio buffers (both sample & frequency data)
void piedPiper::SaveDetection()
{
  StopAudioStream();
  lastDetectionTime = millis();
  
  ResetSPI();
  digitalWrite(HYPNOS_3VR, LOW);
  detectionNum++;

  Serial.println("Positive detection");
  
  for (int i = 0; i < 200; i++)
  {
    if (SD.begin(SD_CS))
    {
      break;
    }
    Serial.print("SD failed to initialize in SaveDetection on try #");
    Serial.println(i);
    delay(10);
  }

  data = SD.open("DETS.TXT", FILE_WRITE);

  data.println("NUMBER");
  data.println(detectionNum, DEC);

  data.println("TIME");
  data.println(lastDetectionTime, DEC);

  data.println("AUDIO");

  for (int i = 0; i < sampleCount; i++)
  {
    data.println(samples[samplePtr], DEC);
    samples[samplePtr] = 0;
    samplePtr = IterateCircularBufferPtr(samplePtr, sampleCount);
  }

  samplePtr = 0;

  data.close();
  SD.end();
  digitalWrite(HYPNOS_3VR, HIGH);

  for (int t = 0; t < freqWinCount; t++)
  {
    for (int f = 0; f < FFT_WIN_SIZE / 2; f++)
    {
      freqs[t][f] = 0;
    }
  }

  freqsPtr = 0;

  for (int t = 0; t < TIME_AVG_WIN_COUNT; t++)
  {
    for (int f = 0; f < FFT_WIN_SIZE / 2; f++)
    {
      rawFreqs[f][t] = 0;
    }
  }

  rawFreqsPtr = 0;

  RestartAudioStream();
}

// Takes a single photo, and records what time and detection it corresponds to
void piedPiper::TakePhoto(int n)
{
  StopAudioStream();
  lastImgTime = millis();
  ResetSPI();
  
  digitalWrite(CAM_IMG_PIN, HIGH);
  digitalWrite(HYPNOS_5VR, HIGH);
  delay(1000);
  digitalWrite(HYPNOS_3VR, LOW);
  delay(1000);
  digitalWrite(CAM_IMG_PIN, LOW);
  delay(50);
  digitalWrite(CAM_IMG_PIN, HIGH);
  delay(2000);
  digitalWrite(HYPNOS_5VR, LOW);
  delay(50);
  digitalWrite(CAM_IMG_PIN, LOW);

  for (int i = 0; i < 200; i++)
  {
    if (SD.begin(SD_CS))
    {
      break;
    }
    Serial.print("SD failed to initialize in TakePhoto on try #");
    Serial.println(i);
    delay(10);
  }

  data = SD.open("PHOTO.txt", FILE_WRITE);

  data.println("PHOTO#");
  data.println(photoNum, DEC);

  data.println("DETECTION#");
  data.println(n, DEC);

  data.println("TIME");
  data.println(lastImgTime, DEC);

  data.close();
  SD.end();
  digitalWrite(HYPNOS_3VR, HIGH);
  RestartAudioStream();
}

// Plays back a female mating call using the vibration exciter
void piedPiper::Playback()
{
  StopAudioStream();
  Serial1.begin(9600);
  digitalWrite(HYPNOS_5VR, HIGH);
  delay(500);
  Serial1.println("#00");
  delay(6300);
  digitalWrite(HYPNOS_5VR, LOW);
  Serial1.end();
  lastPlaybackTime = millis();
  RestartAudioStream();
}

// Records the most recent time that the unit is alive.
void piedPiper::LogAlive()
{
  StopAudioStream();
  lastLogTime = millis();
  ResetSPI();
  digitalWrite(HYPNOS_3VR, LOW);

  for (int i = 0; i < 200; i++)
  {
    if (SD.begin(SD_CS))
    {
      break;
    }

    Serial.print("SD failed to initialize in LogAlive on try #");
    Serial.println(i);
    delay(10);
  }

  data = SD.open("LOG.TXT", FILE_WRITE);

  data.println(lastLogTime, DEC);

  data.close();
  
  SD.end();
  digitalWrite(HYPNOS_3VR, HIGH);
  RestartAudioStream();
}

//+
int piedPiper::IterateCircularBufferPtr(int currentVal, int arrSize)
{
  if ((currentVal + 1) == arrSize)
  {
    return 0;
  }
  else
  {
    return (currentVal + 1);
  }
}


unsigned long piedPiper::GetLastLogTime()
{
  return lastLogTime;
}

unsigned long piedPiper::GetLastDetectionTime()
{
  return lastDetectionTime;
}

unsigned long piedPiper::GetLastPhotoTime()
{
  return lastImgTime;
}

unsigned long piedPiper::GetLastPlaybackTime()
{
  return lastPlaybackTime;
}

int piedPiper::GetDetectionNum()
{
  return detectionNum;
}

void piedPiper::ResetSPI()
{
  pinMode(23, INPUT);
  pinMode(24, INPUT);
  pinMode(10, INPUT);

  delay(20);

  pinMode(23, OUTPUT);
  pinMode(24, OUTPUT);
  pinMode(10, OUTPUT);
}
