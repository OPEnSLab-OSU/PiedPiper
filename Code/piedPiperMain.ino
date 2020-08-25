#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include "RTClib.h"
#include "piedPiper.h"
#include <Adafruit_NeoPixel.h>

#define sd 1
#define card_select 10
#define ampAddress 0x4B
#define clock_pin 11
#define latch_pin 10
#define signal_pin 12
#define SHTDWN 5  //active LOW shutdown pin for MAX9744 Amplifier
#define in1 6
#define in2 9
#define in3 13
#define FULL 0
#define TEST 1
#define PLAYBACK 1
#define PLAYBACK_TIME 10    //set how long the system will playback mimc signal(seconds)
#define MAX_ADD 0x4B
piedPiper p;
RTC_PCF8523 rtc;

File data;
Adafruit_NeoPixel indicator(1,8,NEO_GRB + NEO_KHZ800);
bool LED_set = 0;



void setup() {
  Wire.begin();
  pinMode(SHTDWN,OUTPUT);  //sets up MAX9744 amplifier shutdown pin
  pinMode(in1,INPUT_PULLUP);
  pinMode(in2,INPUT_PULLUP);
  pinMode(in3,INPUT_PULLUP);
  Serial.begin(115200);
  int8_t v = 31;
  setVolume(v);
  if(p.getDebug() or p.getDebugSignal()){   //turns on built in neopixel for visual debugging
    indicator.begin();
    LED_set = 1;
    indicator.setPixelColor(0,64,64,64);
    indicator.show();
  }

 
  
  #if sd  //initialize RTC and SD logger
    Serial.println("Setting up RTC and SD");
    if(!rtc.begin()){
      Serial.println("RTC missing");  //initializes rtc 
      while(1){
        delay(500);
      }
    }
    if (! rtc.initialized()) {
      Serial.println("RTC is NOT running!");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //set rtc to time of compile
    }
      
  
    while(!SD.begin(card_select)){
      Serial.println("Missing SD card");
      delay(100);
      if(p.getDebug() or p.getDebugSignal()){
        indicator.setPixelColor(0,0,0,128);
        indicator.show();
      }
    }
    DateTime now = rtc.now(); //set time for RTC
    data = SD.open("pPiper.txt",FILE_WRITE);
    data.print("-----Pied Piper Field Test ");
        data.print(now.month(), DEC);
        data.print('/');
        data.print(now.day(), DEC);
        data.print('/');
        data.print(now.year(), DEC);
        data.print(" ");
        data.print(now.hour(), DEC);
        data.print(':');
        data.print(now.minute(), DEC);
        data.print(':');
        data.print(now.second(), DEC); 
        data.println("-----");
        data.close();
  #endif

  
    pinMode(latch_pin,OUTPUT);
    pinMode(clock_pin,OUTPUT);    //initialize pins for shift register
    pinMode(signal_pin,OUTPUT);   //pin that will send serial data
   
    p.playback(10,clock_pin,latch_pin,signal_pin); //turn off all output from Audio Board
   
    
}

bool setVolume(int8_t vol){
  Wire.beginTransmission(MAX_ADD);
 if(vol > 63)
  vol = 63;
 else if(vol < 0);
  vol = 0;
  
  Wire.write(vol);
  if(Wire.endTransmission() == 0)
    return true;
  else
    return false;
}

int count = 0; //counting value for insects detected

void calibrateGain(){
    if(!LED_set){
      indicator.begin();
      LED_set = 1;
    }
    p.playback(1,clock_pin,latch_pin,signal_pin);
    int val = analogRead(CHANNEL);
    Serial.println(val);
    if(val >= 1020)
      indicator.setPixelColor(0,128,128,0);
    else
      indicator.setPixelColor(0,0,128,128);
    indicator.show();
    delay(10);
}

//duration of playback in seconds, delay "dly" = time of playback in millis, delay "dly_btw" = time inbetween playbacks in milliseconds, sound_num picks playback track
void playbackTime(int duration, int dly, int dly_btw, int sound_num){
  int t1 = millis();
  int t2 = t1;
   indicator.setPixelColor(0,32,0,128);
   indicator.show(); //turn indicator light dark purple
  while(t2-t1<=duration*1000){
    digitalWrite(SHTDWN,HIGH);
    p.playback(sound_num,clock_pin,latch_pin,signal_pin);
    delay(dly);
    t2 = millis();
    p.playback(10,clock_pin,latch_pin,signal_pin);
    digitalWrite(SHTDWN,LOW);
    delay(dly_btw);
  }
   p.playback(10,clock_pin,latch_pin,signal_pin);
   digitalWrite(SHTDWN,LOW);
}

int mode(){
  bool i1 = digitalRead(in1);
  bool i2 = digitalRead(in2);
  bool i3 = digitalRead(in3);
  int mode = 0;
  if(i2 && i3)
    mode = 0;
  else if(i2 && !i3)
    mode = 1;
  else if(i1 && !i2 && i3)
    mode = 2;
  else if(!i2 && !i3)
    mode = 3;
  else if(!i1 && !i2 && i3)
    mode = 6;
  else if(!i1 && !i2 && !i3)
    mode = 7;
  else
    mode = 0;
  return mode;
}

void printInsectDetected(){
  DateTime now = rtc.now(); //set time for RTC
  data = SD.open("pPiper.txt",FILE_WRITE);
      if(data){
        data.print(now.month(), DEC);
        data.print('/');
        data.print(now.day(), DEC);
        data.print('/');
        data.print(now.year(), DEC);
        data.print(" ");
        data.print(now.hour(), DEC);
        data.print(':');
        data.print(now.minute(), DEC);
        data.print(':');
        data.print(now.second(), DEC);
        data.print(" ");
        data.print("Insect detected");
        data.print(" Total Count: ");
        data.print(count);
        data.println();
        data.close();
      }
}


void PiedPiper_Test(){
  PiedPiper_NoPlayback();
  indicator.setPixelColor(0,255,0,170);
  indicator.show();
  DateTime now = rtc.now();
  Serial.print("Min: ");
  Serial.println(now.minute());
  if(now.minute()%5 == 0){
    p.playback(1,clock_pin,latch_pin,signal_pin);
    Serial.println("Playing");
  }
  if(now.minute()%5 == 2)
    p.playback(10,clock_pin,latch_pin,signal_pin);
}

void PiedPiper_NoPlayback(){
  if(p.getDetected()==1){ //insect has been detected
    count++;
    Serial.println("Playing Mimic Signal");
    
    
    if(p.getDebug() or p.getDebugSignal()){
      indicator.setPixelColor(0,0,128,0);
      indicator.show(); //turn indicator light green
      delay(500);
    }
    
    #if sd  //if using an SD card write info to it
      printInsectDetected();
    #endif
   }
  
    if(p.getDebug() or p.getDebugSignal()){
      indicator.setPixelColor(0,128,32,0);
      indicator.show(); //turn indicator light red orange
    }
    
    p.insectDetection(); //run insect detection algorithm
   
   if(p.getDebug()){
    p.print_all(); //prints all gathered data from audio signal
   }
}

void PiedPiperFull(){
  if(p.getDetected()==1){ //insect has been detected
    count++;
    Serial.println("Playing Mimic Signal");
    
    
    if(p.getDebug() or p.getDebugSignal()){
      indicator.setPixelColor(0,0,128,0);
      indicator.show(); //turn indicator light green
    }
    
    #if sd  //if using an SD card write info to it
      printInsectDetected();
    #endif
    digitalWrite(SHTDWN,LOW);
    playbackTime(600,6000,7000,2);  //play signal for 10 minutes
    p.playback(10,clock_pin,latch_pin,signal_pin); //stop playing signal
    digitalWrite(SHTDWN,HIGH);
   }
  
    if(p.getDebug() or p.getDebugSignal()){
      indicator.setPixelColor(0,128,0,0);
      indicator.show(); //turn indicator light red
    }
    p.insectDetection(); //run insect detection algorithm
   
   if(p.getDebug());
    p.print_all(); //prints all gathered data from audio signal
   
}

void loop(){
  digitalWrite(SHTDWN,HIGH);
  bool d = digitalRead(in1); 
  p.setDebugSetting(d);
  switch(mode()){
      case 0: PiedPiperFull(); break; //insect detection and active playback
      case 1: PiedPiper_NoPlayback(); break;  //insect detection only
      case 2: calibrateGain(); break; //calibrate the gain setting 
      case 3: playbackTime(13,6000,7000,2); break; //only playback
      case 6: PiedPiper_Test(); break; //simulate insect  
  }
}
