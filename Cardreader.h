#ifndef __CARDREADER__
#define __CARDREADER__
/*
 * Write personal data of a MIFARE RFID card using a RFID-RC522 reader
 * Uses MFRC522 - Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI W AND R BY COOQROBOT. 
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 * Hardware required:
 * Arduino
 * PCD (Proximity Coupling Device): NXP MFRC522 Contactless Reader IC
 * PICC (Proximity Integrated Circuit Card): A card or tag using the ISO 14443A interface, eg Mifare or NTAG203.
 * The reader can be found on eBay for around 5 dollars. Search for "mf-rc522" on ebay.com. 
 */
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

enum WAKEUPMODE : byte {
    WKMOD_OFF       = 0x00,
    WKMOD_ON        = 0x01,
    WKMOD_UNCHANGED = 0x63
  };
  enum WSOUND : byte {
    WSND_OFF       = 0x00,
    WSND_ON        = 0x01,
    WSND_UNCHANGED = 0x63
  };
  enum PATTERN : byte {
    PAT_OFF       = 0x00,
    PAT_SNRS      = 0x01,
    PAT_SNDWN     = 0x02,
    PAT_SNDWN_SLP = 0x03,
    PAT_RAINBOW   = 0x04,
    PAT_MOOD_RND  = 0x05,
    PAT_MOOD_COL  = 0x06,
    PAT_UNCHANGED = 0x63
  };
  
class Cardreader : public MFRC522 {
  public:

  // this object stores nfc tag data
  struct nfcTagObject {
    uint32_t id;
    uint32_t cookie;
    uint8_t wakeup_mode;
    uint8_t wakeup_sound;
    uint8_t wakeup_hours;
    uint8_t wakeup_minutes;
    uint8_t light_pattern;
    uint8_t light_r;
    uint8_t light_g;
    uint8_t light_b;
  };
  nfcTagObject myCard;

  private:
    MIFARE_Key key;
    bool successRead;
    byte sector;
    byte blockAddr;
    byte trailerBlock;
    StatusCode status;
  
  public:

  Cardreader (byte chipSelectPin, byte resetPowerDownPin, byte sector = 1, byte blockAddr = 4, byte trailerBlock = 7)
  : MFRC522(chipSelectPin, resetPowerDownPin),
  sector (sector), blockAddr(blockAddr), trailerBlock(trailerBlock)
  {
    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  }
  
  uint8_t readSerial(int maxZahl, String text) {
    byte buffer[34];
    byte len;
    int zahl;
    
    Serial.setTimeout(20000L) ;     // wait until 20 seconds for input from serial
    Serial.println(text);
    len = Serial.readBytesUntil('#', (char *) buffer, 30) ; // read from serial
    for (byte i = len; i < 30; i++) buffer[i] = ' ';     // pad with spaces
    zahl = atoi(buffer);
    if (zahl > maxZahl) return 99;
    return zahl;
  }
  
  void writeCard(nfcTagObject nfcTag) {
    MFRC522::PICC_Type mifareType;
    byte buffer[16] = {0x13, 0x37, 0xb3, 0x48, // 0x1337 0xb348 magic cookie to
                                               // identify our nfc tags
                       nfcTag.wakeup_mode,     // alarm clock status (active, inactive, unchanged)
                       nfcTag.wakeup_sound,    // alarm sound (--"-)
                       nfcTag.wakeup_hours,    // hour of alarm
                       nfcTag.wakeup_minutes,  // minutes of alarm
                       nfcTag.light_pattern,   // light pattern
                       nfcTag.light_r,         // light color (red)
                       nfcTag.light_g,         // light color (green)
                       nfcTag.light_b,         // light color (blue)
                       0x00, 0x00, 0x00, 0x00}; // free
  
    byte size = sizeof(buffer);
  
    mifareType = PICC_GetType(uid.sak);
  
    // Authenticate using key B
    Serial.println(F("Authenticating again using key B..."));
    status = (MFRC522::StatusCode)PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(GetStatusCodeName(status));
      return;
    }
  
    // Write data to the block
    Serial.print(F("Writing data into block "));
    Serial.print(blockAddr);
    Serial.println(F(" ..."));
    dump_byte_array(buffer, 16);
    Serial.println();
    status = (MFRC522::StatusCode)MIFARE_Write(blockAddr, buffer, 16);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(GetStatusCodeName(status));
    }
    Serial.println();
    delay(100);
  }
  
  bool readCard(nfcTagObject *nfcTag) {
    bool returnValue = true;
    // Show some details of the PICC (that is: the tag/card)
    Serial.print(F("Card UID:"));
    dump_byte_array(uid.uidByte, uid.size);
    Serial.println();
    /*Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = PICC_GetType(uid.sak);
    Serial.println(PICC_GetTypeName(piccType));*/
  
    byte buffer[18];
    byte size = sizeof(buffer);
  
    // Authenticate using key A
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode)PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(uid));
    if (status != MFRC522::STATUS_OK) {
      returnValue = false;
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(GetStatusCodeName(status));
      return;
    }
  
    /*// Show the whole sector as it currently is
    Serial.println(F("Current data in sector:"));
    PICC_DumpMifareClassicSectorToSerial(&(uid), &key, sector);
    Serial.println();*/
  
    // Read data from the block
    Serial.print(F("Reading data from block "));
    Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode)MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
      returnValue = false;
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(GetStatusCodeName(status));
    } else {
      Serial.print(F("Data in block "));
      Serial.print(blockAddr);
      Serial.println(F(":"));
      dump_byte_array(buffer, 16);
      Serial.println();
      Serial.println();
    
      uint32_t tempCookie;
      tempCookie = (uint32_t)buffer[0] << 24;
      tempCookie += (uint32_t)buffer[1] << 16;
      tempCookie += (uint32_t)buffer[2] << 8;
      tempCookie += (uint32_t)buffer[3];
      
      uint32_t tempID;
      tempID = (uint32_t)uid.uidByte[0] << 24;
      tempID += (uint32_t)uid.uidByte[1] << 16;
      tempID += (uint32_t)uid.uidByte[2] << 8;
      tempID += (uint32_t)uid.uidByte[3];

      nfcTag->id = tempID;
      nfcTag->cookie = tempCookie;
      nfcTag->wakeup_mode = buffer[4];
      nfcTag->wakeup_sound = buffer[5];
      nfcTag->wakeup_hours = buffer[6];
      nfcTag->wakeup_minutes = buffer[7];
      nfcTag->light_pattern = buffer[8];
      nfcTag->light_r = buffer[9];
      nfcTag->light_g = buffer[10];
      nfcTag->light_b = buffer[11];
    }
    return returnValue;
  }
  
  void setupCard() {
    Serial.println(F("Neue Karte konfigurieren"));
  
    // read in data  
    myCard.wakeup_mode = readSerial(99, "New status of alarm clock:   (0 = inactive, 1 = active, 99 = unchanged)  ---> end with #");
    Serial.println(myCard.wakeup_mode);
  
    if (myCard.wakeup_mode == 1) {
      myCard.wakeup_sound = readSerial(99, "Alarm sound:   (0 = inactive, 1 = active, 99 = unchanged)  ---> end with #");
      Serial.println(myCard.wakeup_sound);
  
      myCard.wakeup_hours = readSerial(99, "Alarm time (hours):   (0 - 23, 99 = unchanged)  ---> end with #");
      Serial.println(myCard.wakeup_hours);
  
      myCard.wakeup_minutes = readSerial(99, "Alarm time (minutes):   (0 - 59)  ---> end with #");
      Serial.println(myCard.wakeup_minutes);
    } else {
      myCard.wakeup_sound = 99;
      myCard.wakeup_hours = 99;
      myCard.wakeup_minutes = 99;
    }
  
    myCard.light_pattern = readSerial(99, "Light pattern:   \n\t(0 - off, 1 - sunrise, 2 - sundown, \n\t3 - sundown with sleep light, \n\t4 - rainbow, 5 - moodlight random, \n\t6 - moodlight color, 99 - unchanged)  ---> end with #");
    Serial.println(myCard.light_pattern);
  
    if (myCard.light_pattern == 6) {
      myCard.light_r = readSerial(255, "Light color (red):   (0 - 255)  ---> end with #");
      myCard.light_g = readSerial(255, "Light color (green):   (0 - 255)  ---> end with #");
      myCard.light_b = readSerial(255, "Light color (blue):   (0 - 255)  ---> end with #");
    } else {
      myCard.light_r = 99;
      myCard.light_g = 99;
      myCard.light_b = 99;
    }
  
    // Karte ist konfiguriert -> speichern
    writeCard(myCard);
  }
  
  /**
   * Helper routine to dump a byte array as hex values to Serial.
   */
  void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
      Serial.print(buffer[i] < 0x10 ? " 0" : " ");
      Serial.print(buffer[i], HEX);
    }
  }
};
#endif
