#include "Cardreader.h"
#include "Clock.h"
#include "Mp3Player.h"
#include "NeoPattern.h"

#define RST_PIN         9          // RFID
#define SS_PIN         10          // RFID
#define RX_PIN          2          // MP3
#define TX_PIN          3          // MP3
#define busyPin         4          // MP3
#define LED_PIN         8          // LED

void RaiseAlarm();
void NachAlarm();
void VorAlarm();
void SunriseComplete();

Cardreader mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
Mp3Player mp3(RX_PIN, TX_PIN, busyPin);        // create DFMiniMp3 instance
Clock clock(1, false, &VorAlarm, &RaiseAlarm, &NachAlarm); // type = 1, sync = false, alarm callbacks
NeoPattern ledring(24, LED_PIN, NEO_GRB + NEO_KHZ800, &SunriseComplete); // number LEDS, PIN, type, callback (sunrise)

void setup() {
	Serial.begin(115200);		// Initialize serial communications with the PC (baud rate != 9600, because that is used by mp3 player)
	while (!Serial);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

  Serial.println("Init LED ring");
  ledring.begin();
  ledring.Off();
  
	SPI.begin();			// Init SPI bus
	mfrc522.PCD_Init();		// Init MFRC522
	mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details

  clock.begin();
  //DateTime now = clock.now();
  /*Serial.print("Now: ");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.println(now.minute());*/
  /*clock.setAlarmTime(now.hour(), now.minute() + 1, now.hour(), now.minute() + 2, now.hour(), now.minute() + 3);
  clock.enableAlarm();*/

  mp3.begin();

  delay(3000);
  mp3.playCommandSound(Mp3Com_Start);

  //ledring.Sunup(100);
}

void loop() {
  mp3.loop();
  clock.update(); // alarms will be raised in callback
  ledring.Update(); 

  delay(500);

  /*********************************************/
	// Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
	if ( ! mfrc522.PICC_IsNewCardPresent()) {
		return;
	}
	// Select one of the cards
	if ( ! mfrc522.PICC_ReadCardSerial()) {
		return;
	}
 
	// read card
  if (mfrc522.readCard(&(mfrc522.myCard)) == true) {
    
    // Spezial
    //Serial.println(mfrc522.myCard.id);
    if (mfrc522.myCard.id == 483888059) {
      Serial.println("Spezial-Anweisung!");
      DateTime now = clock.now();
      if (clock.setAlarmTime(now.hour(), now.minute() + 1, now.hour(), now.minute() + 3, now.hour(), now.minute() + 5)) {
        clock.enableAlarm();
        //ledring.Off();
      }
    }
    
    if (mfrc522.myCard.cookie == 322417480) {
      Serial.println("bekannte Karte");
      mp3.playCommandSound(Mp3Com_KnownCard);

      //************** send commands to clock and leds *********************//
      // alarm sound
      switch (mfrc522.myCard.wakeup_sound) {
        case 0:
          clock.disableMusic();
          break;
        case 1:
          clock.enableMusic();
          break;
        case 99:
        default:
          // do nothing
          break;
      }
      // alarm time
      if (mfrc522.myCard.wakeup_hours < 24 && mfrc522.myCard.wakeup_minutes < 60) {
        clock.SetAlarmTime(mfrc522.myCard.wakeup_hours, mfrc522.myCard.wakeup_minutes, 1800, 1800);
        
      } else if (mfrc522.myCard.wakeup_hours < 24) {
        // change only hours
        uint8_t minutes = clock.alarm1min;
        clock.SetAlarmTime(mfrc522.myCard.wakeup_hours, minutes, 1800, 1800);
        
      } else if (mfrc522.myCard.wakeup_minutes < 60) {
        // only change minutes
        uint8_t hours = clock.alarm1hour;
        clock.SetAlarmTime(hours, mfrc522.myCard.wakeup_minutes, 1800, 1800);
      }
      // switch alarm on/off
      switch (mfrc522.myCard.wakeup_mode) {
        case WKMOD_OFF:
          clock.disableAlarm();
          break;
        case WKMOD_ON:
          clock.enableAlarm();
          break;
        case WKMOD_UNCHANGED:
        default:
          // do nothing
          break;
      }

      // light
      switch (mfrc522.myCard.light_pattern) {
        case PAT_OFF:
          ledring.Off();
          break;
        /*case PAT_SNRS:
          ledring.Sunup(100);
          break;*/
        case PAT_SNDWN:
          ledring.Sundown(500);  // TODO: interval?
          clock.showSunSymbol(true);
          clock.showStarSymbol(false);
          break;
        case PAT_SNDWN_SLP:
          ledring.SundownNight(500);  // TODO
          clock.showSunSymbol(true);
          clock.showStarSymbol(false);
          break;
        case PAT_RAINBOW:
          ledring.RainbowCycle(300);
          clock.showSunSymbol(false);
          clock.showStarSymbol(true);
          break;
        case PAT_MOOD_RND:
          ledring.Steady(ledring.Wheel(random(255)));
          clock.showSunSymbol(false);
          clock.showStarSymbol(true);
          break;
        case PAT_MOOD_COL:
          ledring.Steady(NeoPattern::Color(mfrc522.myCard.light_r, mfrc522.myCard.light_g, mfrc522.myCard.light_b));
          clock.showSunSymbol(false);
          clock.showStarSymbol(true);
          break;
        case PAT_UNCHANGED:
        default:
          // do nothing
          ledring.Off();
          break;
      }
      
      
    } else {
      Serial.print("unbekannte Karte (Cookie ");
      Serial.print(mfrc522.myCard.cookie);
      Serial.println(")");
      mp3.playCommandSound(Mp3Com_UnknownCard);
    }
  }
  
  // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
}

//------------------------------------------------------------
//Callback Routines - get called on completion of a routine
//------------------------------------------------------------

// Clock Callback
void RaiseAlarm() {
  Serial.println("     ALARM !!!!   ");
  // mp3 an
  if (clock.alarmMusic) {
    //mp3.begin();
    mp3.setFolder(2);
    mp3.play();
  }
  // Licht umstellen auf Dauer-an
  ledring.Steady(NeoPattern::Color(255,82,30));
    
}
void NachAlarm() {
  Serial.println("     Nach-Alarm!   ");
  mp3.stop();
  // Licht aus und Musik aus
  ledring.Off();
    
}
void VorAlarm() {
  Serial.println("     Vor-Alarm!   ");
  // Sonnenuntergang an
  long interval = 1000 * (long)clock.getSecsBeforeAlarm() / 240;   // interval = milliseconds / totalSteps of pattern
  Serial.print("Starte Sunrise mit intervall ");
  Serial.println(interval); // 60s = 250; 120s = 500
  ledring.Sunup(interval);
}
// NeoPattern Callback
void SunriseComplete() {
  Serial.println("Completion Callback"); 
  // Licht umstellen auf Dauer-an
  ledring.Steady(NeoPattern::Color(255,82,30));
}
