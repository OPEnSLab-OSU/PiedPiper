#include "piedPiper.h"

piedPiper::piedPiper() {

}

bool piedPiper::insectDetection()
{
  //Serial.println("Running detection algorithm");
  recordSamples(winSize);

  if (initFreqTest())
  {
    //Serial.println("Positive initial frequency test; recording & processing for full test...");
    recordSamples(sampleCount);

    processSignal();

    if (fullSignalTest())
    {
      //Serial.println("!POSITIVE DETECTION!");
      detectionNum++;
      saveDetection();
      lastDetectionTime = millis();
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

bool piedPiper::initFreqTest()
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

void piedPiper::processSignal()
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
bool piedPiper::fullSignalTest()
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
void piedPiper::smoothFreq(int avgWinSize)
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
void piedPiper::timeSmoothFreq(int avgWinSize)
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
bool piedPiper::checkFreqDomain(int t)
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
void piedPiper::recordSamples(int samples)
{
  for (int i = 0; i < samples; i++)
  {
    sampleBuffer[i] = analogRead(audIn);
    delayMicroseconds(delayTime);
  }
}

void piedPiper::saveDetection()
{
  //Serial.println("Saving detection data...");
  data = SD.open("DETS.txt", FILE_WRITE);

  while (!data)
  {
    data = SD.open("DETS.txt", FILE_WRITE);
  }

  data.println("NUMBER");
  data.println(detectionNum, DEC);

  data.println("TIME");
  data.println(millis(), DEC);

  data.println("AUDIO");

  for (int i = 0; i < sampleCount; i++)
  {
    data.println(sampleBuffer[i], DEC);
  }

  data.close();
}

void piedPiper::takePhoto(int n)
{
  //Serial.println("Taking photo...");
  lastPhotoTime = millis();

  digitalWrite(camImg, HIGH);
  digitalWrite(camPow, HIGH); // Turn on the camera
  delay(2000);                    // Wait for the camera to initialize
  digitalWrite(camImg, LOW);  // Write low to tell the camera to take a photo
  delay(50);                      // Wait 50ms to ensure that the camera responds
  digitalWrite(camImg, HIGH); // Write high to the camera to ensure that only a single image is taken
  delay(2000);                    // Wait for the camera to finish writing the image to storage
  digitalWrite(camPow, LOW);  // Power off the camera
  delay(50);                      // Wait for the camera to power off
  digitalWrite(camImg, LOW);

  photoNum++;

  data = SD.open("PHOTO.txt", FILE_WRITE);

  while (!data)
  {
    data = SD.open("PHOTO.txt", FILE_WRITE);
  }

  data.println("PHOTO#");
  data.println(photoNum, DEC);

  data.println("DETECTION#");
  data.println(n, DEC);

  data.println("TIME");
  data.println(lastPhotoTime, DEC);

  data.close();
}

void piedPiper::playback()
{
  //Serial.println("Beginning playback");
  const byte soundNum = 0b11111110;

  digitalWrite(SHTDWN, HIGH);
  digitalWrite(latch, LOW); //turns off register
  digitalWrite(clk, LOW); //sets clock low indicating the start of a byte
  shiftOut(dataPin, clk, MSBFIRST, soundNum); //sends data to shift register
  digitalWrite(latch, HIGH); //turns on output

  delay(6300);

  digitalWrite(SHTDWN, LOW);
  digitalWrite(latch, LOW); //turns off register
  digitalWrite(clk, LOW); //sets clock low indicating the start of a byte
  shiftOut(dataPin, clk, MSBFIRST, 0b00000000); //sends data to shift register
  digitalWrite(latch, HIGH); //turns on output

  lastPlaybackTime = millis();
}

void piedPiper::reportAlive()
{
  //Serial.println("Logging aliveness...");
  lastLogTime = millis();

  data = SD.open("LOG.txt", FILE_WRITE);

  while (!data)
  {
    data = SD.open("LOG.txt", FILE_WRITE);
  }

  data.println(millis(), DEC);
  data.close();
}
