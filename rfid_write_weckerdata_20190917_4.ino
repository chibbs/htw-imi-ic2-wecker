#include "RFID.h"

RFID rfid(9,10,1,4,7); // SS-PIN, RST-PIN, SECTOR, BLOCK, TRAILERBLOCK

void setup() {
  Serial.begin(9600);        // Initialize serial communications with the PC

  // NFC Leser initialisieren
  SPI.begin();               // Init SPI bus
  rfid.PCD_Init();        // Init MFRC522 card
  rfid.PCD_DumpVersionToSerial(); // Show details of PCD - MFRC522 Card Reader
  Serial.println(F("Write personal data on a MIFARE PICC "));
}

void loop() {
  do {
    // Buttons, MP3 and stuff here
  } while (!rfid.PICC_IsNewCardPresent());

  // restart loop, while no card is present
  if (!rfid.PICC_ReadCardSerial())
    return;

  // RFID Karte wurde aufgelegt
  if (rfid.readCard(&rfid.myCard) == true) {
    rfid.setupCard();
  }
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

}
