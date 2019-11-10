#include <GxEPD.h>
#include <GxGDEH029A1/GxGDEH029A1.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/URW_Gothic_L_Demi_100.h>

#include <ArduinoLowPower.h>
#include <Adafruit_SleepyDog.h>

#include <RTCZero.h>
RTCZero zerortc;

#include <Wire.h>
#include "RTClib.h"
RTC_DS3231 rtc;

using namespace std;

GxIO_Class io(SPI, /*CS=*/ 2, /*DC=*/ 3, /*RST=*/ 4);
GxEPD_Class display(io, /*RST=*/ 4, /*BUSY=*/ 5);

uint16_t hourBox [2] = {8, 130};
uint16_t minuteBox [2] = {8, 225};
uint16_t vBox [4] = {28, 280}; //250 height total
DateTime now;

uint16_t EINK_PIN = 6;
uint16_t RTC_PIN = 7;

void setup(void)
{
  //pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RTC_PIN, OUTPUT);
  pinMode(EINK_PIN, OUTPUT);
}

void loop() {
  //digitalWrite(LED_BUILTIN, HIGH);
  zerortc.begin();

  // DS3231: turn on --> read --> turn off
  
  digitalWrite(RTC_PIN, HIGH);
  delay(50);
  rtc.begin();
  now = rtc.now();
  digitalWrite(RTC_PIN, LOW);
  ////////////////////////////////////////
  
  zerortc.setTime(now.hour(), now.minute(), now.second());
  zerortc.setDate(now.day(), now.month(), now.year());

  // eInk: turn on --> update --> turn off
  digitalWrite(EINK_PIN, HIGH);
  //delay(50);
  display.init();
  showClock();
  digitalWrite(EINK_PIN, LOW);
  ////////////////////////////////////////
  
  int sec = 60 - zerortc.getSeconds();
  sec = sec < 1 ? 1 : sec;
  //digitalWrite(LED_BUILTIN, LOW);
  LowPower.sleep(sec*1000);
}

// 250 px x 122 px
void showClock()
{
  display.setFont(&URW_Gothic_L_Demi_100);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setRotation(0);
  
  showHour();
  showMinute();
  showVoltage();

  display.update();
}

void showHour() {
  uint16_t hours = now.hour();
  hours = hours == 00 | hours == 12 ? 12 : hours % 12;
  String hourStr = (hours < 10 ? "0" : "") + String(hours);

  display.setCursor(hourBox[0], hourBox[1]);
  display.print(hourStr);
}

void showMinute() {
  String minuteStr = getMinStr(now.minute());

  display.setCursor(minuteBox[0], minuteBox[1]);
  display.print(minuteStr);
}

String getMinStr(uint16_t minutes) {
  String minuteStr = minutes < 10 ? "0" : "";
  minuteStr += String(minutes);
  return minuteStr;
}

void showVoltage(void) {
  display.setFont(&FreeMono12pt7b);
  display.setCursor(vBox[0], vBox[1]);

  String voltStr = String(getBatteryVoltage()) + "v";
  display.print(voltStr);
}

float getBatteryVoltage() {
  unsigned int readings=0;
  for (byte i=0; i<10; i++) //take 10 samples, and average
    readings+=analogRead(A5);
  float batteryVolts = (readings / 10.0) * 2 * 3.3 / 1024;
  return batteryVolts;
}
