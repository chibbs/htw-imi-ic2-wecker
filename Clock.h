#ifndef __CLOCK__
#define __CLOCK__

#include <Arduino.h>
#include <Wire.h>     // I2C
#include <U8x8lib.h>
#include "RTClib.h"

static const char *weekday[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

class Clock {
  protected:
  U8X8 u8x8;
  RTC_DS3231 rtc;
  boolean syncOnFirstStart;
  uint8_t lastShownMinute;
  boolean showSun;
  boolean showStar;

  public:
  uint8_t alarm0hour; // needed to switch of alarm after 30 minutes
  uint8_t alarm0min;
  uint8_t alarm1hour;
  uint8_t alarm1min;
  uint8_t alarm2hour; // needed to switch of alarm after 30 minutes
  uint8_t alarm2min;
  boolean alarm;
  boolean alarmMusic;
  void (*OnAlarm1)();  // Callback for alarm 1
  void (*OnAlarm2)();  // Callback for alarm 2
  void (*OnAlarm0)();  // Callback for alarm 0

  // constructor
  Clock(byte oledtype, boolean sync, void (*callback0)(), void (*callback1)(), void (*callback2)()) {
    if (oledtype == 0)
      u8x8 = U8X8_SSD1306_128X64_NONAME_HW_I2C(/* reset=*/ U8X8_PIN_NONE);    // small display 0.96
    else
      u8x8 = U8X8_SH1106_128X64_NONAME_HW_I2C(/* reset=*/ U8X8_PIN_NONE);      // bigger display 1.33

    syncOnFirstStart = sync;
    // alarm 7:00, with 30mins before and after
    alarm1hour = alarm2hour = 7;
    alarm0hour = 6;
    alarm1min = 0;
    alarm2min = alarm0min = 30;
    
    lastShownMinute = 60;       // offset > 59 for beginning
    alarm = false;
    showSun = false;
    showStar = false;
    alarmMusic = true;
    
    OnAlarm1 = callback1;
    OnAlarm2 = callback2;
    OnAlarm0 = callback0;
  }
  void printTime(DateTime now) {
    String datum = String(weekday[now.dayOfTheWeek()]) + ", ";
    datum = (now.day()<10) ? datum+"0"+now.day()+"." : datum+now.day()+".";
    datum = (now.month()<10) ? datum+"0"+now.month()+"." : datum+now.month()+".";
    datum += now.year();
  
    String zeit = (now.hour()<10) ? String("0")+now.hour()+":" : String("")+now.hour()+":";
    zeit = (now.minute()<10) ? zeit+"0"+now.minute() : zeit+now.minute();
    String zeits = (now.second()<10) ? zeit+":0"+now.second() : zeit+":"+now.second(); // time with seconds (used for serial monitor)
    
    String weckzeit = (alarm1hour <10) ? String("0")+alarm1hour : String("")+alarm1hour;
    weckzeit = (alarm1min < 10) ? weckzeit+":0"+alarm1min : weckzeit+":"+alarm1min;
    
    Serial.println(datum + ", " + zeits);
    //printLCD("Mo, 11.02.2019", "12:35", true, "06:45", true, true);
    printClock(datum.c_str(), zeit.c_str(), weckzeit.c_str());
  }
  
  void updateDisplay() {
    DateTime now = rtc.now();
    updateDisplay(now);
  }
  void updateDisplay(DateTime now) {
    printTime(now);
  }

  void begin() {
    Serial.println("Initialize display.");
    u8x8.begin();
    u8x8.clear();
    u8x8.setFlipMode(1);

    Serial.println("Initialize RTC...");
    if (! rtc.begin()) {
      Serial.println("Kann RTC nicht finden");
      while (1);
    }
  
    if (rtc.lostPower() || syncOnFirstStart) {
      Serial.println("Die RTC war vom Strom getrennt. Die Zeit wird neu synchronisiert.");
      // Über den folgenden Befehl wird die die RTC mit dem Zeitstempel versehen, zu dem der
      // Kompilierungsvorgang gestartet wurde, beginnt aber erst mit dem vollständigen Upload
      // selbst mit zählen. Daher geht die RTC von Anfang an wenige Sekunden nach.
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));  // (DateTime(Jahr,Tag,Monat,Stunde,Minute,Sekunde))
      //printTime(rtc.now());
      updateDisplay();
    }
  }

  void pre2() {
    u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);    
    u8x8.clear();
    u8x8.setFont(u8x8_font_chroma48medium8_r);  
    //u8x8.setCursor(0,1);
  }

  void printClock(const char *datum, const char *zeit, const char *weckzeit) {
    //u8x8.clear();
    u8x8.setFont(u8x8_font_artossans8_r); 
    u8x8.drawString(1, 0, datum);
    u8x8.setFont(u8x8_font_inb21_2x4_n);
    u8x8.drawString(3, 2, zeit);
    if (alarm) {
      if (alarmMusic) {
        u8x8.setFont(u8x8_font_open_iconic_embedded_2x2);
        u8x8.drawGlyph(1, 6, '@'+1);      // Alarm
      }
      u8x8.setFont(u8x8_font_artossans8_r); 
      u8x8.drawString(4, 7, weckzeit);
    }
    if (showStar) {
      //u8x8.setFont(u8x8_font_open_iconic_thing_2x2);
      //u8x8.drawGlyph(14, 6, '@'+14);     // Flame
      u8x8.setFont(u8x8_font_open_iconic_weather_2x2);
      u8x8.drawGlyph(10, 6, '@'+4);      // Star
    }
    if (showSun) {
      u8x8.setFont(u8x8_font_open_iconic_weather_2x2);
      u8x8.drawGlyph(14, 6, '@'+5);      // Sun
    }
  }

  boolean checkAlarm1(DateTime now) {
    if (now.hour() == alarm1hour && now.minute() == alarm1min) {
      //Serial.println("     ALARM!!!!   ");
      return true;
    }
    return false;
  }

  boolean checkAlarm2(DateTime now) {
    if (now.hour() == alarm2hour && now.minute() == alarm2min) {
      //Serial.println("     ALARM2!!!!   ");
      return true;
    }
    return false;
  }

  boolean checkAlarm0(DateTime now) {
    if (now.hour() == alarm0hour && now.minute() == alarm0min) {
      return true;
    }
    return false;
  }

  void update() {
    DateTime now = rtc.now();
    /*Serial.println("update");
    Serial.print("last shown minute: ");
    Serial.println(lastShownMinute);
    Serial.println(now.minute());*/
    if (lastShownMinute != now.minute()) {
      lastShownMinute = now.minute();
      updateDisplay(now);
      //checkAlarm(now);
      if (checkAlarm1(now) && OnAlarm1 != NULL) {
          OnAlarm1(); // call the callback
      }
      if (checkAlarm2(now) && OnAlarm2 != NULL) {
          OnAlarm2(); // call the callback
      }
      if (checkAlarm0(now) && OnAlarm0 != NULL) {
          OnAlarm0(); // call the callback
      }
    }
  }

  // for testing: set all three alarms freely
  boolean setAlarmTime(uint8_t hours0, uint8_t mins0, uint8_t hours1, uint8_t mins1, uint8_t hours2, uint8_t mins2) {
    boolean correct = true;
    if (hours0 < 24 && mins0 < 60 && hours1 < 24 && mins1 < 60 && hours2 < 24 && mins2 < 60) {
      alarm1hour = hours1;
      alarm1min = mins1;
      alarm2hour = hours2;
      alarm2min = mins2;
      alarm0hour = hours0;
      alarm0min = mins0;
      
      Serial.print("Set alarm to ");
      Serial.print(alarm1hour);
      Serial.print(":");
      Serial.print(alarm1min);
      Serial.print(" (before ");
      Serial.print(alarm0hour);
      Serial.print(":");
      Serial.print(alarm0min);
      Serial.print(", after ");
      Serial.print(alarm2hour);
      Serial.print(":");
      Serial.print(alarm2min);
      Serial.println(")");

      if (alarm) updateDisplay(); // only show if alarm is active
    } else {
      Serial.println("Alarm time could not be changed, because incorrect time given.");
      correct = false;
    }
    return correct;
  }

  // main alarm and seconds before and after (max 18h)
  boolean SetAlarmTime(uint8_t hours1, uint8_t minutes1, uint16_t secsBefore, uint16_t secsAfter) {
    boolean correct = true;
    if (hours1 < 24 && minutes1 < 60) {
      alarm1hour = hours1;
      alarm1min = minutes1;

      // calculate alarm0 (before alarm 1)
      uint8_t hb = secsBefore / 3600;           // hours before
      uint8_t mb = (secsBefore % 3600) / 1800;  // minutes before
      alarm0hour = (hours1 >= hb) ? hours1 - hb : hours1 + 24 - hb;
      if (minutes1 >= mb) {
        alarm0min = minutes1 - mb;
      } else {
        alarm0min = minutes1 + 60 - mb;
        alarm0hour = (alarm0hour == 0) ? 23 : alarm0hour - 1;
      }

      // calculate alarm2 (after alarm 1)
      uint8_t ha = secsAfter / 3600;
      uint8_t ma = (secsAfter % 3600) / 1800;
      alarm2hour = hours1 + ha;
      alarm2min = minutes1 + ma;
      if (alarm2min > 59) {
        alarm2min -= 60;
        alarm2hour += 1;
      }
      if (alarm2hour > 23) {
        alarm2hour -= 24;
      }

      Serial.print("Set alarm to ");
      Serial.print(alarm1hour);
      Serial.print(":");
      Serial.print(alarm1min);
      Serial.print(" (before ");
      Serial.print(alarm0hour);
      Serial.print(":");
      Serial.print(alarm0min);
      Serial.print(", after ");
      Serial.print(alarm2hour);
      Serial.print(":");
      Serial.print(alarm2min);
      Serial.println(")");

      if (alarm) updateDisplay(); // only show if alarm is active
      
    } else {
      Serial.println("Alarm time could not be changed, because incorrect time given.");
      correct = false;
    }
    return correct;
  }

  uint16_t getSecsBeforeAlarm() {
    uint16_t diff = 0;
    long millis1 = 0;
    long millis0 = 0;
    millis1 = (alarm1min * 60000) + (alarm1hour * 3600000);
    millis0 = (alarm0min * 60000) + (alarm0hour * 3600000);
    if (millis1 >= millis0) {
      diff = (millis1 - millis0) / 1000;
    } else {
      Serial.println("Fehler: alarm0 ist größer als alarm1");
    }
    //Serial.println(millis0);
    //Serial.println(millis1);
    Serial.println(diff);
    return diff;
  }
  uint16_t getSecsAfterAlarm() {
    uint16_t diff = 0;
    long millis1 = 0;
    long millis2 = 0;
    millis1 = (alarm1min * 60000) + (alarm1hour * 3600000);
    millis2 = (alarm2min * 60000) + (alarm2hour * 3600000);
    if (millis2 >= millis1) {
      diff = (millis2 - millis1) / 1000;
    } else {
      Serial.println("Fehler: alarm1 ist größer als alarm2");
    }
    return diff;
  }

  void showSunSymbol(boolean sun) {
    showSun = sun;
    updateDisplay();
  }
  void showStarSymbol(boolean star) {
    showStar = star;
    updateDisplay();
  }
  void enableAlarm() {
    alarm = true;
    updateDisplay();
  }
  void disableAlarm() {
    alarm = false;
    updateDisplay();
  }

  void enableMusic() {
    alarmMusic = true;
    updateDisplay();
  }
  void disableMusic() {
    alarmMusic = false;
    updateDisplay();
  }

  DateTime now() {
    return rtc.now();
  }
};
#endif
