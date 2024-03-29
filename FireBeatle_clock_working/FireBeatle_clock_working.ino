#include <GxEPD.h>
#include <GxGDEH0213B72/GxGDEH0213B72.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeMono24pt7b.h>
#include <Fonts/roboto36.h>

#include <DS3232RTC.h>      // https://github.com/JChristensen/DS3232RTC
#include <Streaming.h>      // http://arduiniana.org/libraries/streaming/
#include <Timezone.h>    // https://github.com/JChristensen/Timezone

DS3232RTC RTC;

TimeChangeRule myDST = {"PDT", Second, Sun, Mar, 2, -420};    //Daylight time = UTC - 4 hours
TimeChangeRule mySTD = {"PST", First, Sun, Nov, 2, -480};     //Standard time = UTC - 5 hours
Timezone myTZ(myDST, mySTD);
TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev

GxIO_Class io(SPI, /*CS=5*/ 25, /*DC=*/ 26, /*RST=*/ 27);
GxEPD_Class display(io, /*RST=*/ 27, /*BUSY=*/ 13);

#define NUM_PARTIALS 10 // Times to partial paint before full refresh
bool INVERT_COLORS;
uint16_t FOREGROUND_COLOR;
uint16_t BACKGROUND_COLOR;

RTC_DATA_ATTR bool firstBoot = true;
RTC_DATA_ATTR String prevHour = "";
RTC_DATA_ATTR String prevMinute = "";

void setup(void)
{
  Serial.begin(115200);
  Serial.println();
}

void loop() {
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
  display.init(115200); // enable diagnostic output on Serial

  setupSleep();

  setColors();
  showPartialUpdate();

  //uint8_t val = RTC.interruptWhileAsleep(true);
  firstBoot = false;
  digitalWrite(12, LOW);

  Serial.println("going to sleep");
  esp_deep_sleep_start();
}

// 250 px x 122 px
void showPartialUpdate()
{
  display.setTextColor(FOREGROUND_COLOR);
  display.setRotation(0);

  display.fillScreen(BACKGROUND_COLOR);
  
  showHour();
  showMinute();
  //showTemp();
  //showVoltage();
  if (shouldFullRefresh()) {
    display.update();
    Serial.println("Full Refresh");
  }
  display.powerDown();
}

void showHour() {
  uint16_t hourBox [2] = {8, 110};
  display.setFont(&Roboto_Bold_90);

  uint16_t hours = hour(myTZ.toLocal(now(), &tcr));
  //uint16_t hours = hour();
  hours = hours == 24 || hours == 12 ? 12 : hours % 12;
  String hourStr = (hours < 10 ? "0" : "") + String(hours);

  display.setCursor(hourBox[0], hourBox[1]);

  if (hourStr != prevHour || fullRefreshReached()) {  
    display.print(hourStr);
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

/*void showTemp() {
  uint16_t vBox [4] = {25, 240}; //250 height total
  display.setFont(&FreeMono24pt7b);
  display.setCursor(vBox[0], vBox[1]);

  int t = RTC.temperature();
  float celsius = t / 4.0;
  int fahrenheit = celsius * 9.0 / 5.0 + 32.0;
  String voltStr = String(fahrenheit);

  int16_t x, y;
  uint16_t w, h;
  display.getTextBounds("000", vBox[0], vBox[1], &x, &y, &w, &h);
  
  display.print(voltStr);
  if (!shouldFullRefresh()) {
    display.updateWindow(vBox[0], vBox[1], vBox[2], vBox[3], true);
    Serial.println("Partial voltage refresh");
  }
  Serial.println("Drew voltage");
}*/

/*void showVoltage() {
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
}*/

String getMinStr(uint16_t minutes) {
  String minuteStr = minutes < 10 ? "0" : "";
  minuteStr += String(minutes);
  return minuteStr;
}

void setupSleep() {
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
  pinMode(14, INPUT_PULLUP);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_14,LOW);

  // set alarm 2 for every minute
  RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 1);    // daydate parameter should be between 1 and 7
  RTC.alarm(ALARM_2);                   // ensure RTC interrupt flag is cleared
  RTC.alarmInterrupt(ALARM_2, true);
}

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
  return true;
  //return firstBoot || fullRefreshReached();
}

void print_wakeup_reason(){
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
}

void printDateTime(time_t t)
{
    Serial << ((day(t)<10) ? "0" : "") << _DEC(day(t));
    Serial << monthShortStr(month(t)) << _DEC(year(t)) << ' ';
    Serial << ((hour(t)<10) ? "0" : "") << _DEC(hour(t)) << ':';
    Serial << ((minute(t)<10) ? "0" : "") << _DEC(minute(t)) << ':';
    Serial << ((second(t)<10) ? "0" : "") << _DEC(second(t));
}
