#include <GxEPD.h>
#include <GxGDEH0213B72/GxGDEH0213B72.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/roboto36.h>

#include <Streaming.h>      // http://arduiniana.org/libraries/streaming/

#include <RTCZero.h>

RTCZero zerortc;

// Set how often alarm goes off here
const byte alarmSeconds = 0;
const byte alarmMinutes = 0;
const byte alarmHours = 0;

#if defined (MOTEINO_M0)
  #if defined(SERIAL_PORT_USBVIRTUAL)
    #define Serial SERIAL_PORT_USBVIRTUAL // Required for Serial on Zero based boards
  #endif
#endif

using namespace std;

GxIO_Class io(SPI, /*CS=*/ 2, /*DC=*/ 3, /*RST=*/ 4);
GxEPD_Class display(io, /*RST=*/ 4, /*BUSY=*/ 5);

#define NUM_PARTIALS 10 // Times to partial paint before full refresh
bool INVERT_COLORS;
uint16_t FOREGROUND_COLOR;
uint16_t BACKGROUND_COLOR;

bool firstBoot = true;
String prevHour = "";
String prevMinute = "";

void setup(void)
{
  Serial.begin(115200);
  Serial.println();
  
  zerortc.begin(); // Set up clocks and such

  byte seconds = 55;
  byte minutes = 9;
  byte hours = 22;
  byte day = 15;
  byte month = 8;
  byte year = 2019;
  
  zerortc.setTime(hours, minutes, seconds);
  zerortc.setDate(day, month, year);
  
  resetAlarm();  // Set alarm
}

void loop() {
  //Serial.begin(115200);
  //Serial.println();
  display.init(115200); // enable diagnostic output on Serial

  setColors();
  showPartialUpdate();

  firstBoot = false;
  resetAlarm();  // Reset alarm before returning to sleep
  zerortc.standbyMode();    // Sleep until next alarm match
}

void resetAlarm(void) {
  zerortc.setAlarmTime(alarmHours, alarmMinutes, alarmSeconds);
  zerortc.enableAlarm(zerortc.MATCH_SS);
}

// 250 px x 122 px
void showPartialUpdate()
{
  display.setTextColor(FOREGROUND_COLOR);
  display.setRotation(0);

  display.fillScreen(BACKGROUND_COLOR);
  /*if (shouldFullRefresh()) {
    display.update();
  }*/
  
  showHour();
  showMinute();
  //showVoltage();
  if (shouldFullRefresh()) {
    display.update();
    //display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);
    Serial.println("Full Refresh");
  }
  display.powerDown();
}

void showHour() {
  uint16_t hourBox [2] = {8, 110};
  display.setFont(&Roboto_Bold_90);

  uint16_t hours = zerortc.getHours();
  hours = hours == 24 ? 12 : hours % 12;
  String hourStr = (hours < 10 ? "0" : "") + String(hours);

  display.setCursor(hourBox[0], hourBox[1]);

  if (hourStr != prevHour || fullRefreshReached()) {  
    display.print(hourStr);
    // hours never do partial refresh
    /*int16_t x, y;
    uint16_t w, h;
    display.getTextBounds("00", hourBox[0], hourBox[1], &x, &y, &w, &h);
    display.updateWindow(x, y, w, h, true);
    Serial.println("Drew hour");*/
  }
  prevHour = hourStr;
}

void showMinute() {
  uint16_t minuteBox [2] = {8, 195};
  display.setFont(&Roboto_Bold_90);
  display.setCursor(minuteBox[0], minuteBox[1]);
  String minuteStr = getMinStr(zerortc.getMinutes());

  int16_t x, y;
  uint16_t w, h;
  display.getTextBounds("00", minuteBox[0], minuteBox[1], &x, &y, &w, &h);

  if (minuteStr != prevMinute) {
    display.print(minuteStr);
    
    if (!shouldFullRefresh()) {
      display.updateWindow(x, y, w, h, true);
      Serial.println("Partial minute refresh");
    }
    Serial.println("Drew minute");
  }
  prevMinute = minuteStr;
}

String getMinStr(uint16_t minutes) {
  String minuteStr = minutes < 10 ? "0" : "";
  minuteStr += String(minutes);
  return minuteStr;
}

bool fullRefreshReached() {
  return zerortc.getMinutes() % NUM_PARTIALS == 0;
}

void setColors() {
  //INVERT_COLORS = hour() < 20 ? false : true;
  INVERT_COLORS = false;
  FOREGROUND_COLOR = !INVERT_COLORS ? GxEPD_BLACK : GxEPD_WHITE;
  BACKGROUND_COLOR = !INVERT_COLORS ? GxEPD_WHITE : GxEPD_BLACK;
}


void fillTextBackground(String text) {
  int16_t x, y;
  uint16_t w, h;
  display.getTextBounds(text, display.getCursorX(), display.getCursorY(), &x, &y, &w, &h);
  display.fillRect(x, y, w, h, FOREGROUND_COLOR);
  display.updateWindow(x, y, w, h, true);
  display.fillRect(x-5, y-5, w+10, h+10, BACKGROUND_COLOR);
  display.updateWindow(x-5, y-5, w+10, h+10, true);
}

bool shouldFullRefresh() {
  return firstBoot || fullRefreshReached();
}
