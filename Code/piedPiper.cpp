#include "piedPiper.h"
#include "arduinoFFT.h"

piedPiper::piedPiper(){
	sampling_period_us = round(1000000*(1.0/samplingFrequency));
  debug = 1;
  debug_signal = 0;
 }

void piedPiper::sampleFreq(){
	microseconds = micros();
  for(int i=0; i<samples; i++)
  {
      vReal[i] = analogRead(CHANNEL);
      vImag[i] = 0;
      while(micros() - microseconds < sampling_period_us){
        //empty loop
      }
      microseconds += sampling_period_us;
  }
  /* Print the results of the sampling according to time */
  FFT.Windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);	/* Weigh data */
  FFT.Compute(vReal, vImag, samples, FFT_FORWARD); /* Compute FFT */
  FFT.ComplexToMagnitude(vReal, vImag, samples); /* Compute magnitudes */
 
  domFreq = FFT.MajorPeak(vReal, samples, samplingFrequency);
}

void piedPiper::printFreq(){/*
  Serial.println("Computed magnitudes:");
  PrintVector(vReal, (samples >> 1), SCL_FREQUENCY);*/
 // Serial.println(domFreq); //Print out what frequency is the most dominant.
//  for(int i=0; i<samples;i++)
  //  Serial.println(vReal[i]);
}

double piedPiper::get_domFreq(){
  return domFreq;
}

bool piedPiper::getDetected(){
  return detected;
}

/*****************************************************************************
 * smoothData
 * performs a low pass filter on incoming signal to highlight magnitude peaks
 * utilizes circular buffer and running average to sharpen peaks and reduce noise
 ****************************************************************************/
int piedPiper::smoothData(){
  float ave = 0;
  for(int i=bufferSize-1; i>=0; i--){ //moves the values in the buffer forward an index
    if(i==0)
      sampleBuffer[i] = analogRead(CHANNEL); //inputs new value to buffer
    else   
      sampleBuffer[i]=sampleBuffer[i-1];  //sets buffer value to next index
  }
  if(sampleBuffer[0]>average){  //check if new value exceeds the average value
    for(int i=0; i<bufferSize; i++)
      sampleBuffer[i] = sampleBuffer[0];  //sets all values in buffer to new value
  }
  for(int i=0; i<bufferSize; i++){ //average the values in the buffer
    ave+=sampleBuffer[i]; 
  }
  average = round(ave/bufferSize);
  delay(1);
  return average;
}

/**********************************************************
 * getPeakVoltage
 * takes a one second sample and calculates the peak value
 * Passes peak value to thresholds for peak counting
 *********************************************************/
//sets values in the signal buffer and determines peak value
void piedPiper::getPeakVoltage(){
  peak = 0;
  //collect one second of the incoming signal
  for(int i=0; i<500; i++){
    signalBuffer[i] = smoothData(); //set the values in the signal buffer to the smoothed analog readings
    if(signalBuffer[i]>peak)
      peak = signalBuffer[i]; //if the current reading is greater than peak, set peak to current reading
  }
  //Serial.println("Data Set");
  if(debug_signal){
    for(int i=0; i<500;i++){ //For visualizing the peak detection parameters
      Serial.print(peak*peakThreshold); //prints upper value signal must pass to be declared a peak
      Serial.print(',');
      Serial.print(peak*lowerThreshold); //prints the value signal must pass below to count the peak
      Serial.print(',');
      Serial.println(signalBuffer[i]);
    }
  }
}

/***********************************************************
 * countPeaks
 * analyzes one second of signal to count peaks in amplitude
 ***********************************************************/
void piedPiper::countPeaks(){
  getPeakVoltage();
  bool count_flag = false;
  int peak_count = 0;
  for(int i=0; i<500; i++){   
   if(signalBuffer[i] > peak*peakThreshold && !count_flag) //Checks if signal has passed abouve peak threshold
       {
             peak_count++;
             count_flag = true; //sets flag true as to not count repeated peaks from noise
       }
     
       if(signalBuffer[i] <= peak*lowerThreshold) //set count_flag to false only when below the threshold
             count_flag = false; //sets flag false so next peak can be counted
  }

  num_peaks = peak_count;
}

/******************************************************************
 * checkSilence
 * returns true if there is no incoming signal above ambient noise
 ******************************************************************/
bool piedPiper::checkSilence(){
  int val = 0;
  for(int i=0; i<512; i++){
    val+=analogRead(CHANNEL); //takes an average reading of the signal to check for ambient conditions
  }
  val = val/512;
  if(val > AMBIENCE)
    return false; //if there is external noise, return false
  else
    return true; //if it is quiet enough, return true: mating call has ended
}

/*******************************************************************
 * checkFrequency
 * checks end of signal frequency compared to an earlier reading
 * Sets detected variable true if end frequency > start frequency
 ******************************************************************/
void piedPiper::checkFrequency(){
  int above = 0;
  int below = 0;
  Serial.println("Check Frequency is Being Run");
  //sampleFreq(); //samples signal to calculate frequency
  int init_freq = domFreq; //sets start of signal frequency
  int average = 0;
  int t1, t2;
  t1 = millis();
  t2=t1;
  while(t2-t1 < 2000 && !checkSilence()){
    t2 = millis();
    sampleFreq(); //samples signal while silence conditions haven't been met
    if(init_freq*fShift<domFreq)
      above++;
    else if(domFreq < init_freq) 
      below++;
     average+=domFreq;
    if(domFreq>700){
       break;
    }
      
    if(debug){
      Serial.print("Initial Frequency: ");
      Serial.print(init_freq);
      Serial.print(" Dominant Frequency: ");
      Serial.println(domFreq);
    }
  }
  average = average/(above+below);
  
  if(debug){
    Serial.print("Above: ");
    Serial.print(above);
    Serial.print(" Below: ");
    Serial.print(below);
    Serial.print(" Average Frequency: ");
    Serial.println(average);
  }
  
 if(above>=below && below!=0 || average > init_freq){ //if the end frequency > start frequency, bug has been detected
    detected = true;
    return;
 }

  else
    detected = false;
    return;
}

/******************************************************************************
 * insectDetection
 * Runs full insect detection algorithm
 * Check frequency range->Count amplitude peaks/second->check end frequency
 * sets detected variable true when insect has been determined
 *****************************************************************************/
void piedPiper::insectDetection(){
  sampleFreq(); 
  float store_freq = domFreq;
  if(domFreq<lowerFreq or domFreq>upperFreq){ //checks incoming signal until a signal appears in the set frequency range
    detected = false;
    return;
  } //passes checkpoint if signal is between upper and lower frequency bounds
  countPeaks();
  if(num_peaks < lowerPeak or num_peaks > upperPeak){ //checks if incoming signal matches peak amplitude counts of insect
    detected = false;
    Serial.println("Outside of Peak Bounds");
    return;
  } //passes checkpoint if signal has peaks/s in  peak bounds
  
  domFreq = store_freq;   //set frequency to initial frequency that triggered the detection algorithm
  checkFrequency(); //checks if signal frequency increases at end of call
  
}

void piedPiper::playback(int mimic, int clk, int latch, int data){
    byte bitsToSend = 0;  //byte variable for outputting to shift register
    Serial.print("Playing Sound #"); Serial.println(mimic);
    switch(mimic){  //maps the inputted audio # to the matching shift register #
      case 1: mimic = 7; break;
      case 2: mimic = 0; break;
      case 3: mimic = 6; break;
      case 4: mimic = 1; break;
      case 5: mimic = 2; break;
      case 6: mimic = 5; break;
      case 7: mimic = 3; break;
      case 8: mimic = 4; break;
      default: mimic = 10; 
        Serial.println("Playback Stopped");
        digitalWrite(latch,LOW); //turns off register
        digitalWrite(clk,LOW); //sets clock low indicating the start of a byte
        shiftOut(data,clk,LSBFIRST,0); //sends data to shift register
        digitalWrite(latch,HIGH); //turns on outputbreak;
        return;
    }
    
    int mimic_byte = pow(2,mimic);    //sets byte to only turn on the selected pin
    
    digitalWrite(latch,LOW); //turns off register
    digitalWrite(clk,LOW);  //sets clock low indicating the start of a byte
    shiftOut(data,clk,LSBFIRST,mimic_byte); //sends data to shift register
    digitalWrite(latch,HIGH); //turns on output 
}



void piedPiper::print_all(){
  Serial.print("Dominant Frequency: ");
  Serial.print(domFreq);
  Serial.print("  Peak Value: ");
  Serial.print(peak);
  Serial.print("  Peaks/second: ");
  Serial.print(num_peaks);
  Serial.print("  Detected: ");
  Serial.println(detected);
}

bool piedPiper::getDebug(){
  return debug; 
}

bool piedPiper::getDebugSignal(){
  return debug_signal;
}

void piedPiper::setDebugSetting(bool d){
  if(d){
    debug = 1;
    debug_signal = 0;
  }
  else{
    debug = 0;
    debug_signal = 1;
  } 
  //Serial.print("Debug: "); Serial.print(debug); Serial.print(" Debug Signal: "); Serial.println(debug_signal);
}

