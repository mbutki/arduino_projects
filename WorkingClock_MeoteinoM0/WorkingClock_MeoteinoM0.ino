#include <GxEPD.h>
#include <GxGDEH0213B72/GxGDEH0213B72.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/roboto36.h>

//#include <avr/io.h>
//#include <avr/pgmspace.h>

#include <DS3232RTC.h>      // https://github.com/JChristensen/DS3232RTC
#include <Streaming.h>      // http://arduiniana.org/libraries/streaming/

#undef RTC
extern DS3232RTC RTC;

using namespace std;

GxIO_Class io(SPI, /*CS=*/ 0, /*DC=*/ 1, /*RST=*/ 2);
GxEPD_Class display(io, /*RST=*/ 2, /*BUSY=*/ 3);

#define NUM_PARTIALS 10 // Times to partial paint before full refresh
bool INVERT_COLORS;
uint16_t FOREGROUND_COLOR;
uint16_t BACKGROUND_COLOR;

bool firstBoot = true;
String prevHour = "";
String prevMinute = "";

const uint8_t SQW_PIN(4);   // connect this pin to DS3231 INT/SQW pin

void setup(void)
{
  Serial.begin(115200);
  Serial.println();
}

void loop() {
  display.init(115200); // enable diagnostic output on Serial

  //setupSleep();

  setColors();
  showPartialUpdate();

  firstBoot = false;
  Serial.println("going to sleep");
  delay(60000);
  //esp_deep_sleep_start();
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

  uint16_t hours = hour();
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
  String minuteStr = getMinStr(minute());

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

void showVoltage() {
  uint16_t vBox [4] = {25, 240}; //250 height total
  display.setFont(&FreeMono12pt7b);
  display.setCursor(vBox[0], vBox[1]);

  String voltStr = String(getBatteryVoltage()) + "v";

  int16_t x, y;
  uint16_t w, h;
  display.getTextBounds("0.00v", vBox[0], vBox[1], &x, &y, &w, &h);
  
  display.print(voltStr);
  if (!shouldFullRefresh()) {
    display.updateWindow(vBox[0], vBox[1], vBox[2], vBox[3], true);
    Serial.println("Partial voltage refresh");
  }
  Serial.println("Drew voltage");
}

String getMinStr(uint16_t minutes) {
  String minuteStr = minutes < 10 ? "0" : "";
  minuteStr += String(minutes);
  return minuteStr;
}

/*void setupSleep() {
  //print_wakeup_reason();

  // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
  RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.alarmInterrupt(ALARM_2, true);
  RTC.squareWave(SQWAVE_NONE);

  setInternalClock();

  // configure an interrupt on the falling edge from the SQW pin
  pinMode(SQW_PIN, INPUT_PULLUP);
  //esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);

  // set alarm 2 for every minute
  RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 1);    // daydate parameter should be between 1 and 7
  RTC.alarm(ALARM_2);                   // ensure RTC interrupt flag is cleared
  RTC.alarmInterrupt(ALARM_2, true);
}*/

bool fullRefreshReached() {
  return minute() % NUM_PARTIALS == 0;
}

float getBatteryVoltage() {
  // 4.2v is fully charged, 3.7v is normal, 3.2v is around cuttoff
  return (analogRead(35)/4095.0)*2*3.3*1.1;
}

void setColors() {
  //INVERT_COLORS = hour() < 20 ? false : true;
  INVERT_COLORS = false;
  FOREGROUND_COLOR = !INVERT_COLORS ? GxEPD_BLACK : GxEPD_WHITE;
  BACKGROUND_COLOR = !INVERT_COLORS ? GxEPD_WHITE : GxEPD_BLACK;
}

void setInternalClock() {
  // setSyncProvider() causes the Time library to synchronize with the
  // external RTC by calling RTC.get() every five minutes by default.
  setSyncProvider(RTC.get);
  Serial << "RTC Sync";
  if (timeStatus() != timeSet)
  {
     Serial << "Unable to sync with the RTC";
  } else {
    Serial << "RTC has set the system time";
  }
  Serial << endl;

  printDateTime(RTC.get());
  Serial << " --> Current RTC time\n";
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

/*void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}*/

void printDateTime(time_t t)
{
    Serial << ((day(t)<10) ? "0" : "") << _DEC(day(t));
    Serial << monthShortStr(month(t)) << _DEC(year(t)) << ' ';
    Serial << ((hour(t)<10) ? "0" : "") << _DEC(hour(t)) << ':';
    Serial << ((minute(t)<10) ? "0" : "") << _DEC(minute(t)) << ':';
    Serial << ((second(t)<10) ? "0" : "") << _DEC(second(t));
}
