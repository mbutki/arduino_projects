#include <GxEPD.h>
#include <GxGDEH0213B72/GxGDEH0213B72.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/roboto36.h>

#include <vector>

#include <DS3232RTC.h>      // https://github.com/JChristensen/DS3232RTC
#include <Streaming.h>      // http://arduiniana.org/libraries/streaming/
DS3232RTC RTC;

using namespace std;
GxIO_Class io(SPI, /*CS=5*/ 25, /*DC=*/ 26, /*RST=*/ 27);
GxEPD_Class display(io, /*RST=*/ 27, /*BUSY=*/ 13);

const bool INVERT_COLORS = true;
const uint16_t FOREGROUND_COLOR = !INVERT_COLORS ? GxEPD_BLACK : GxEPD_WHITE;
const uint16_t BACKGROUND_COLOR = !INVERT_COLORS ? GxEPD_WHITE : GxEPD_BLACK;

const uint8_t SQW_PIN(5);   // connect this pin to DS3231 INT/SQW pin

void setup(void)
{
  Serial.begin(115200);
  Serial.println();
   // enable diagnostic output on Serial

  //esp_deep_sleep_start();
  //ESP.deepSleep(10000000);
}

void loop() {
  display.init(115200);
  showPartialUpdate();
  Serial.println("going to sleep");
  delay(10000);
}

// 250 px x 122 px
void showPartialUpdate()
{
  display.setTextColor(FOREGROUND_COLOR);
  display.setRotation(0);

  display.fillScreen(BACKGROUND_COLOR);
  display.update();
  Serial.println("1st Full Update");

  uint16_t hourBox [2] = {8, 110};
  display.setFont(&Roboto_Bold_90);
  display.setCursor(hourBox[0], hourBox[1]);
  display.print("99");
  display.update();
  Serial.println("2nd Full Update");;
  display.powerDown();
}
