#include <GxEPD.h>
#include <GxGDEH0213B72/GxGDEH0213B72.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/roboto36.h>

// #include <MemoryFree.h>;

#include <RTCZero.h>
RTCZero zerortc;

#include <Wire.h>
#include "RTClib.h"
RTC_DS3231 rtc;

#if defined (MOTEINO_M0)
  #if defined(SERIAL_PORT_USBVIRTUAL)
    #define Serial SERIAL_PORT_USBVIRTUAL // Required for Serial on Zero based boards
  #endif
#endif

using namespace std;

GxIO_Class io(SPI, /*CS=*/ 2, /*DC=*/ 3, /*RST=*/ 4);
GxEPD_Class display(io, /*RST=*/ 4, /*BUSY=*/ 5);

bool INVERT_COLORS;
uint16_t FOREGROUND_COLOR;
uint16_t BACKGROUND_COLOR;

uint16_t hourBox [2] = {8, 110};
uint16_t minuteBox [2] = {8, 195};

DateTime now;


void setup(void)
{
  
}

void loop() {
  Serial.begin(115200);
  delay(3000);

  Serial.println("------------------------");
  Serial.println("");
  Serial.println("Top of loop");
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);

  Serial.println("Init Display");
  display.init(115200);
  Serial.println("Init RTCs");
  rtc.begin();
  zerortc.begin();

  now = rtc.now();
  zerortc.setTime(now.hour(), now.minute(), now.second());
  zerortc.setDate(now.day(), now.month(), now.year());
  
  setColors();
  Serial.println("Start Update");
  showClock();

  Serial.println("Reset Alarm");
  resetAlarm();
  digitalWrite(6, LOW);

  //Serial.print("SRAM free: ");
  //Serial.println(freeMemory(), DEC);

  const int sec = 60 - rtc.now().second();
  Serial.print("Going to Sleep for ");
  Serial.print(sec);
  Serial.println(" seconds");
  //zerortc.standbyMode();
  delay(sec * 1000);
  Serial.println("Woke up");
}

void resetAlarm(void) {
  zerortc.setAlarmTime(0, 0, 0);
  zerortc.enableAlarm(zerortc.MATCH_SS);
  //zerortc.attachInterrupt(match);
}

void showVoltage(void) {
  uint16_t vBox [4] = {25, 240}; //250 height total
  display.setFont(&FreeMono12pt7b);
  display.setCursor(vBox[0], vBox[1]);

  String voltStr = String(getBatteryVoltage()) + "v";
  Serial.println("About to draw voltage");
  display.print(voltStr);
  Serial.println("Drew voltage");
}

float getBatteryVoltage() {
  unsigned int readings=0;
  for (byte i=0; i<10; i++) //take 10 samples, and average
    readings+=analogRead(A5);
  float batteryVolts = (readings / 10.0) * 2 * 3.3 / 1024;
  return batteryVolts;
}

// 250 px x 122 px
void showClock()
{
  display.setFont(&Roboto_Bold_90);
  display.fillScreen(BACKGROUND_COLOR);
  display.setTextColor(FOREGROUND_COLOR);
  display.setRotation(0);
  
  showHour();
  showMinute();
  showVoltage();

  display.update();
  Serial.println("Full Refresh");
  //display.powerDown();
}

void showHour() {
  uint16_t hours = now.hour();
  hours = hours == 00 | hours == 12 ? 12 : hours % 12;
  String hourStr = (hours < 10 ? "0" : "") + String(hours);

  display.setCursor(hourBox[0], hourBox[1]);
  Serial.println("About to draw hour");
  display.print(hourStr);
  Serial.println("Drew hour");
}

void showMinute() {
  String minuteStr = getMinStr(now.minute());

  display.setCursor(minuteBox[0], minuteBox[1]);
  Serial.println("About to draw minute");
  display.print(minuteStr);
  Serial.println("Drew minute");
}

String getMinStr(uint16_t minutes) {
  String minuteStr = minutes < 10 ? "0" : "";
  minuteStr += String(minutes);
  return minuteStr;
}

void setColors() {
  //INVERT_COLORS = now.hour() < 12 ? false : true;
  INVERT_COLORS = false;
  FOREGROUND_COLOR = !INVERT_COLORS ? GxEPD_BLACK : GxEPD_WHITE;
  BACKGROUND_COLOR = !INVERT_COLORS ? GxEPD_WHITE : GxEPD_BLACK;
}
