#include "piedPiper.h"

// Class initialization
piedPiper::piedPiper(void(*audStart)(const unsigned long interval_us, void(*fnPtr)()), void(*audStop)()) {
  ISRStartFn = audStart;
  ISRStopFn = audStop;
}

// Used to stop the interrupt timer for sample recording, so that it does not interfere with SD and Serial communication
void piedPiper::StopAudio()
{
  (*ISRStopFn)();
  //Serial.println("Audio stream stopped");
}

// Used to restart the sample recording timer once SPI and Serial communications have completed.
void piedPiper::StartAudioOutput()
{
  //Serial.println("Restarting audio stream");
  (*ISRStartFn)(outputSampleDelayTime, OutputSample);
}

// Used to restart the sample recording timer once SPI and Serial communications have completed.
void piedPiper::StartAudioInput()
{
  //Serial.println("Restarting audio stream");
  (*ISRStartFn)(inputSampleDelayTime, RecordSample);
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

void piedPiper::OutputSample(void) {
  analogWrite(AUD_OUT, nextOutputSample);

  interpCount++;
  if (outputSampleBufferPtr < (playbackSampleCount - 2))
  {
    if (!(interpCount < AUD_OUT_INTERP_RATIO))
    {
      interpCount = 0;
      outputSampleBufferPtr++;

      interpCoeffA = -0.5 * outputSampleBuffer[outputSampleBufferPtr - 1] + 1.5 * outputSampleBuffer[outputSampleBufferPtr] - 1.5 * outputSampleBuffer[outputSampleBufferPtr + 1] + 0.5 * outputSampleBuffer[outputSampleBufferPtr + 2];
      interpCoeffB = outputSampleBuffer[outputSampleBufferPtr - 1] - 2.5 * outputSampleBuffer[outputSampleBufferPtr] + 2 * outputSampleBuffer[outputSampleBufferPtr + 1] - 0.5 * outputSampleBuffer[outputSampleBufferPtr + 2];
      interpCoeffC = -0.5 * outputSampleBuffer[outputSampleBufferPtr - 1] + 0.5 * outputSampleBuffer[outputSampleBufferPtr + 1];
      interpCoeffD = outputSampleBuffer[outputSampleBufferPtr];
    }

    float t = (interpCount * 1.0) / AUD_OUT_INTERP_RATIO;

    nextOutputSample = max(0, min(4095, round(interpCoeffA * t * t * t + interpCoeffB * t * t + interpCoeffC * t + interpCoeffD)));
    pbs = true;
  }
}

// Determines if the sample buffer is full, and is ready to have frequencies computed.
bool piedPiper::InputSampleBufferFull()
{
  return !(inputSampleBufferPtr < FFT_WIN_SIZE);
}

bool piedPiper::LoadSound(char* fname) {
  StopAudio();
  ResetSPI();
  digitalWrite(HYPNOS_3VR, LOW);

  char dir[28];

  strcpy(dir, "/PBAUD/");
  strcat(dir, fname);

  if (!BeginSD()) {
    //Serial.println("SD failed to initialize.");
    digitalWrite(HYPNOS_3VR, HIGH);
    return false;
  }

  //Serial.println("SD initialized successfully, checking directory...");

  if (!SD.exists(dir)) {
    //Serial.print("LoadSound failed: directory ");
    //Serial.print(dir);
    //Serial.println(" could not be found.");
    digitalWrite(HYPNOS_3VR, HIGH);
    StartAudioInput();
    return false;
  }

  //Serial.println("Directory found. Opening file...");

  data = SD.open(dir, FILE_READ);



  if (!data) {
    //Serial.print("LoadSound failed: file ");
    //Serial.print(dir);
    //Serial.println(" could not be opened.");
    digitalWrite(HYPNOS_3VR, HIGH);
    return false;
  }

  //Serial.println("Opened file. Loading samples...");

  int fsize = data.size();
  short buff;

  playbackSampleCount = fsize / 2;

  for (int i = 0; i < playbackSampleCount; i++) {
    if ((2 * i) < (fsize - 1)) {
      data.read((byte*)(&buff), 2);
      outputSampleBuffer[i] = buff;

    } else {
      break;
    }
  }

  data.close();
  SD.end();

  digitalWrite(HYPNOS_3VR, HIGH);

  StartAudioInput();

  return true;
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
    for (int f = 0; f < FFT_WIN_SIZE / 2; f++) {
      vReal[f] += rawFreqs[t][f] / TIME_AVG_WIN_COUNT;
    }
  }

  SmoothFreqs(4);

  for (int f = 0; f < FFT_WIN_SIZE / 2; f++)
  {
    freqs[freqsPtr][f] = round(vReal[f]);
  }

  SmoothFreqs(16);

  for (int f = 0; f < FFT_WIN_SIZE / 2; f++)
  {
    freqs[freqsPtr][f] = freqs[freqsPtr][f] - round(NOISE_FLOOR_MULT * vReal[f]);
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

  for (int i = 0; i < (FFT_WIN_SIZE / 2); i++)
  {
    inptDup[i] = vReal[i];
    vReal[i] = 0;
  }

  for (int i = 0; i < (FFT_WIN_SIZE / 2); i++)
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

  //Serial.println(count);
  // If the number of windows that contain target frequency peaks is at or above the expected number that would be present
  return (count >= EXP_SIGNAL_LEN * (freqWinCount / REC_TIME) * EXP_DET_EFF);
}

// Determines if there is a peak in the target frequency range at a specific time window (including harmonics).
bool piedPiper::CheckFreqDomain(int t)
{
  int lowerIdx = floor(((TGT_FREQ - FREQ_MARGIN) * 1.0) / AUD_IN_SAMPLE_FREQ * FFT_WIN_SIZE);
  int upperIdx = ceil(((TGT_FREQ + FREQ_MARGIN) * 1.0) / AUD_IN_SAMPLE_FREQ * FFT_WIN_SIZE);

  int count = 0;

  for (int h = 1; h <= HARMONICS; h++)
  {
    for (int i = (h * lowerIdx); i <= (h * upperIdx); i++)
    {
      if (((freqs[t][i + 1] - freqs[t][i]) < 0) && ((freqs[t][i] - freqs[t][i - 1]) > 0))
      {
        if (freqs[t][i] > SIG_THRESH)
        {
          count++;
          continue;
        }
      }
    }
  }

  return (count > 0);
}

// Records detection data to the uSD card, and clears all audio buffers (both sample & frequency data)
void piedPiper::SaveDetection()
{
  StopAudio();
  ResetSPI();
  digitalWrite(HYPNOS_3VR, LOW);

  lastDetectionTime = millis();
  detectionNum++;

  //Serial.println("Positive detection");

  if(!BeginSD())
  {
    digitalWrite(HYPNOS_3VR, HIGH);
    StartAudioInput();
    return;
  }

  Wire.begin();

  DateTime currentDT = rtc.now();
  data = SD.open("/DATA/DETS/DETS.txt", FILE_WRITE);
  data.print(detectionNum);
  data.print(",");
  data.print(currentDT.year(), DEC);
  data.print(",");
  data.print(currentDT.month(), DEC);
  data.print(",");
  data.print(currentDT.day(), DEC);
  data.print(",");
  data.print(currentDT.hour(), DEC);
  data.print(",");
  data.print(currentDT.minute(), DEC);
  data.print(",");
  data.println(currentDT.second(), DEC);
  data.close();

  Wire.end();

  char dir[24] = "/DATA/DETS/";
  char str[10];
  itoa(detectionNum, str, 9);
  strcat(dir, str);
  strcat(dir, ".txt");

  data = SD.open(dir, FILE_WRITE);

  for (int i = 0; i < sampleCount; i++)
  {
    data.println(samples[samplePtr], DEC);
    samples[samplePtr] = 0;
    samplePtr = IterateCircularBufferPtr(samplePtr, sampleCount);
  }

  samplePtr = 0;

  // DELETE ME LATER ==================================================================================================================

  data.close();

  char dir2[24] = "/DATA/DETS/F";
  char str2[10];
  itoa(detectionNum, str2, 9);
  strcat(dir2, str2);
  strcat(dir2, ".txt");

  data = SD.open(dir2, FILE_WRITE);

  for (int t = 0; t < freqWinCount; t++)
  {
    for (int f = 0; f < FFT_WIN_SIZE / 2; f++)
    {
      data.print(freqs[freqsPtr][f], DEC);
      data.print(",");

    }

    freqsPtr = IterateCircularBufferPtr(freqsPtr, freqWinCount);
    data.println();
  }

  // ===================================================================================================================================

  data.close();
  SD.end();
  SPI.end();

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
      rawFreqs[t][f] = 0;
    }
  }

  rawFreqsPtr = 0;

  //Serial.println("Done saving detection");
  StartAudioInput();
}

// Takes a single photo, and records what time and detection it corresponds to
bool piedPiper::TakePhoto(int n)
{
  StopAudio();


  lastImgTime = millis();

  uint8_t vid, pid;
  uint8_t temp;

  photoNum++;


  // Powers on and initializes SPI and I2C ===================================
  digitalWrite(HYPNOS_5VR, HIGH);
  digitalWrite(HYPNOS_3VR, LOW);
  digitalWrite(CAM_CS, HIGH);

  ResetSPI();

  SPI.begin();
  Wire.begin();
  //Wire.setClock(100000);
  //SPI.setClockDivider(32);
  //SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE1));


  delay(20);

  // Resets camera CPLD ===========================================
  CameraModule.write_reg(0x07, 0x80);
  delay(100);

  CameraModule.write_reg(0x07, 0x00);
  delay(100);

  // Verify that SPI communication with the camera has successfully begun.
  //Check if the ArduCAM SPI bus is OK
  CameraModule.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = CameraModule.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55)
  {
    Serial.println(F("SPI interface Error!"));
    digitalWrite(HYPNOS_5VR, LOW);
    digitalWrite(HYPNOS_3VR, HIGH);
    digitalWrite(CAM_CS, LOW);
    SPI.end();
    Wire.end();
    StartAudioInput();
    return false;
  }
  else
  {
    //Serial.println(F("SPI interface OK."));
  }

  // Verify that the camera module is alive and well ==========================

  CameraModule.wrSensorReg8_8(0xff, 0x01);
  CameraModule.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  CameraModule.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 )))
  {
    Serial.println(F("ACK CMD Can't find OV2640 module!"));
    digitalWrite(HYPNOS_5VR, LOW);
    digitalWrite(HYPNOS_3VR, HIGH);
    digitalWrite(CAM_CS, LOW);
    SPI.end();
    Wire.end();
    StartAudioInput();
    return false;
  }
  else
  {
    //Serial.println(F("ACK CMD OV2640 detected."));
  }

  // Reduce SPI clock speed for stability =======================================

  delay(10);

  //Serial.println(F("SD Card detected.\n"));

  // COnfigure camera module ==================================================
  CameraModule.set_format(JPEG);
  CameraModule.InitCAM();
  CameraModule.clear_fifo_flag();
  CameraModule.write_reg(ARDUCHIP_FRAMES, 0x00);
  CameraModule.OV2640_set_JPEG_size(OV2640_1600x1200);

  CameraModule.flush_fifo();
  delay(10);

  CameraModule.clear_fifo_flag();

  delay(4000);

  // Capture an image =======================================================
  CameraModule.start_capture();

  //Serial.println("Begun capture, waiting for reply.");

  //Serial.println(F("start capture."));
  long photoStartTime = millis();

  // Wait for the image capture to be complete =============================
  while ( !CameraModule.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
    delay(10);
    if (millis() - photoStartTime > 5000)
    {
      Serial.println("Failed to take photo.");
      digitalWrite(HYPNOS_5VR, LOW);
      digitalWrite(HYPNOS_3VR, HIGH);
      digitalWrite(CAM_CS, LOW);
      SPI.end();
      Wire.end();
      StartAudioInput();
      return false;
    }
  }

  //Serial.println("Recieved reply");

  //SPI.endTransaction();
  //SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));

  delay(10);

  // Initialize SD Card ========================================================
  if (!BeginSD())
  {
    Serial.println("SD failed to begin.");
    digitalWrite(HYPNOS_5VR, LOW);
    digitalWrite(HYPNOS_3VR, HIGH);
    digitalWrite(CAM_CS, LOW);
    Wire.end();
    SPI.end();
    StartAudioInput();
    return false;
  }

  //Serial.println(F("CAM Capture Done."));

  // Read image from camera and store on uSD card. ==========================
  read_fifo_burst(CameraModule);

  //Serial.println("Image saved");

  //Clear the capture done flag ====================================
  CameraModule.clear_fifo_flag();
  delay(10);

  // The format is: [detection #], [year], [month], [day], [hour], [minute], [second]
  DateTime currentDT = rtc.now();
  data = SD.open("/DATA/PHOTO/PHOTO.txt", FILE_WRITE);
  data.print(n);
  data.print(",");
  data.print(currentDT.year(), DEC);
  data.print(",");
  data.print(currentDT.month(), DEC);
  data.print(",");
  data.print(currentDT.day(), DEC);
  data.print(",");
  data.print(currentDT.hour(), DEC);
  data.print(",");
  data.print(currentDT.minute(), DEC);
  data.print(",");
  data.println(currentDT.second(), DEC);
  data.close();

  SD.end();
  SPI.end();
  Wire.end();

  digitalWrite(HYPNOS_5VR, LOW);
  digitalWrite(HYPNOS_3VR, HIGH);

  digitalWrite(CAM_CS, LOW);

  StartAudioInput();
  return true;
}

uint8_t piedPiper::read_fifo_burst(ArduCAM CameraModule)
{
  bool is_header = false;
  uint8_t temp = 0, temp_last = 0;
  uint32_t length = 0;
  static int i = 0;
  static int k = 0;
  char dir[24] = "/DATA/PHOTO/";
  File outFile;
  byte buf[256];



  length = CameraModule.read_fifo_length();

  //Serial.print(F("The fifo length is :"));
  //Serial.println(length, DEC);

  if (length >= MAX_FIFO_SIZE) //8M
  {
    //Serial.println("Over size.");
    return 0;
  }

  if (length == 0 ) //0 kb
  {
    //Serial.println(F("Size is 0."));
    return 0;
  }

  digitalWrite(CAM_CS, LOW);
  CameraModule.set_fifo_burst();//Set fifo burst mode
  i = 0;

  while ( length-- )
  {
    temp_last = temp;
    temp =  SPI.transfer(0x00);

    //End of the image transmission
    if ( (temp == 0xD9) && (temp_last == 0xFF) ) //If find the end ,break while,
    {
      buf[i++] = temp;  //save the last  0XD9

      //Write the remain bytes in the buffer
      digitalWrite(CAM_CS, HIGH);
      outFile.write(buf, i);

      //Close the file
      outFile.close();
      //Serial.print(F("OK: "));
      //Serial.println(length);

      is_header = false;
      digitalWrite(CAM_CS, LOW);
      CameraModule.set_fifo_burst();
      i = 0;
    }

    if (is_header == true)
    {
      //Write image data to buffer if not full
      if (i < 256)
        buf[i++] = temp;
      else
      {
        //Write 256 bytes image data to file
        digitalWrite(CAM_CS, HIGH);
        outFile.write(buf, 256);
        i = 0;
        buf[i++] = temp;
        digitalWrite(CAM_CS, LOW);
        CameraModule.set_fifo_burst();
      }
    }

    // Beginning of the image transmission
    else if ((temp == 0xD8) & (temp_last == 0xFF)) // This is the start of a jpg file header
    {
      is_header = true;
      digitalWrite(CAM_CS, HIGH);

      //Assembles filename
      char str[10];
      itoa(photoNum, str, 10);
      strcat(dir, str);
      strcat(dir, ".jpg");

      //Open the new file
      outFile = SD.open(dir, O_WRITE | O_CREAT | O_TRUNC);

      if (! outFile)
      {
        Serial.println(F("File open failed"));
        return 0;
      }

      digitalWrite(CAM_CS, LOW);
      CameraModule.set_fifo_burst();
      buf[i++] = temp_last;
      buf[i++] = temp;
    }
  }

  digitalWrite(CAM_CS, HIGH);
  return 1;
}

// Plays back a female mating call using the vibration exciter
void piedPiper::Playback() {
  StopAudio();

  //Serial.println("Enabling amplifier...");

  digitalWrite(HYPNOS_5VR, HIGH);
  analogWrite(AUD_OUT, 2048);
  digitalWrite(AMP_SD, HIGH);
  delay(100);

  //Serial.println("Amplifier enabled. Beginning playback ISR...");

  StartAudioOutput();

  while (outputSampleBufferPtr < (playbackSampleCount - 2)) {}

  StopAudio();

  //Serial.println("Finished playback ISR.\nShutting down amplifier...");

  outputSampleBufferPtr = 2;
  outputSampleInterpCount = 0;

  digitalWrite(AMP_SD, LOW);
  analogWrite(AUD_OUT, 0);
  digitalWrite(HYPNOS_5VR, LOW);


  //Serial.println("Amplifer shut down.");
  lastPlaybackTime = millis();

  StartAudioInput();
}

// Records the most recent time that the unit is alive.
void piedPiper::LogAlive()
{
  StopAudio();
  lastLogTime = millis();
  ResetSPI();
  digitalWrite(HYPNOS_3VR, LOW);

  if (!BeginSD())
  {
    digitalWrite(HYPNOS_3VR, HIGH);
    StartAudioInput();
    return;
  }

  data = SD.open("LOG.TXT", FILE_WRITE);

  data.println(lastLogTime, DEC);

  data.close();

  SD.end();
  digitalWrite(HYPNOS_3VR, HIGH);
  StartAudioInput();
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

bool piedPiper::BeginSD()
{
  for (int i = 0; i < SD_OPEN_ATTEMPT_COUNT; i++)
  {
    if (SD.begin(SD_CS))
    {
      return true;
    }
    //Serial.print("SD failed to initialize in SaveDetection on try #");
    //Serial.println(i);
    delay(SD_OPEN_RETRY_DELAY_MS);
  }

  return false;
}

void piedPiper::ResetOperationIntervals()
{
  lastLogTime = 0;
  lastDetectionTime = 0;
  lastImgTime = 0;
  lastPlaybackTime = 0;
}

void piedPiper::SetPhotoNum(int n)
{
  photoNum = n;
}

int piedPiper::GetPhotoNum()
{
  return photoNum;
}

void piedPiper::SetDetectionNum(int n)
{
  detectionNum = n;
}
