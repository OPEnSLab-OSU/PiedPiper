#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include "RTClib.h"
#include "piedPiper.h"

#define sd 1
#define card_select 10
#define ampAddress 0x4B
#define clock_pin 11
#define latch_pin 10
#define signal_pin 9
piedPiper p;
RTC_PCF8523 rtc;
Adafruit_NeoPixel indicator(1,8,NEO_GRB + NEO_KHZ800);
File data;

void setup() {
  #if DEBUG or DEBUG_SIGNAL   //turns on built in neopixel for visual debugging
    indicator.begin();
    indicator.show();
  #endif
  if(!rtc.begin()){
    Serial.println("RTC missing");  //initializes rtc 
    while(1);
  }
  if (! rtc.initialized()) {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //set rtc to time of compile
  }
 
  
  #if sd
    while(!SD.begin(card_select)){
      Serial.println("Missing SD card");
      delay(100);
      #if DEBUG or DEBUG_SIGNAL
        indicator.begin();
        indicator.setPixelColor(0,0,0,128);
        indicator.show();
      #endif
    } 
  #endif
    pinMode(latch_pin,OUTPUT);
    pinMode(clock_pin,OUTPUT);    //initialize pins for shift register
    pinMode(signal_pin,OUTPUT);   //pin that will send serial data
 
    Serial.begin(115200); 
}

int count = 0;

void loop(){
  //Serial.println(analogRead(CHANNEL));
  
  int t1;
  DateTime now = rtc.now();
 if(p.getDetected()==1){
  t1=millis();  //set t1 time for mimic signal timing, resets each time a bug is detected
  p.playback(0,clock_pin,latch_pin,signal_pin);
  count++;
  Serial.println("Playing Mimic Signal");
  #if DEBUG or DEBUG_SIGNAL
    indicator.setPixelColor(0,0,128,0);
    indicator.show(); //turn indicator light green
  #endif
  digitalWrite(10,LOW); //start playing mimic signal
  #if sd
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
  #endif
 }
 else{
  indicator.setPixelColor(0,128,0,0); //turn indicator light red
  indicator.show();
 }

  p.insectDetection(); //run insect detection algorithm
 #if DEBUG
  p.print_all(); //prints all gathered data from audio signal
 #endif
 if(t1 - millis() > PLAYBACK_TIME*1000)
  p.playback(10,clock_pin,latch_pin,signal_pin);
}
