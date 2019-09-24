#ifndef __MP3PLAYER__
#define __MP3PLAYER__

#include <Arduino.h>
#include <DFMiniMp3.h>
#include <SoftwareSerial.h>

// implement a notification class,
// its member methods will get called 
//
class Mp3Notify {
public:
  static void OnError(uint16_t errorCode) {
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print("Com Error ");
    Serial.println(errorCode);
  }

  static void OnPlayFinished(uint16_t globalTrack) {
    Serial.println();
    Serial.print("Play finished for #");
    Serial.println(globalTrack);   
  }

  static void OnCardOnline(uint16_t code) {
    Serial.println();
    Serial.print("Card online ");
    Serial.println(code);     
  }

  static void OnUsbOnline(uint16_t code) {
    Serial.println();
    Serial.print("USB Disk online ");
    Serial.println(code);     
  }

  static void OnCardInserted(uint16_t code) {
    Serial.println();
    Serial.print("Card inserted ");
    Serial.println(code); 
  }

  static void OnUsbInserted(uint16_t code) {
    Serial.println();
    Serial.print("USB Disk inserted ");
    Serial.println(code); 
  }

  static void OnCardRemoved(uint16_t code) {
    Serial.println();
    Serial.print("Card removed ");
    Serial.println(code);  
  }

  static void OnUsbRemoved(uint16_t code) {
    Serial.println();
    Serial.print("USB Disk removed ");
    Serial.println(code);  
  }
};

enum Mp3VoiceCommand : byte {
  Mp3Com_Start           = 0x00,
  Mp3Com_KnownCard       = 0x01,
  Mp3Com_UnknownCard     = 0x02,
  // bing am start, "Diese Karte ist unbekannt" oder möööp, anderes bing für erkannte Karte
};

class Mp3Player: public DFMiniMp3<SoftwareSerial, Mp3Notify> {
  private:
  SoftwareSerial mySoftwareSerial;
  //uint16_t lastTrackFinished;

  public:
  uint16_t numTracksInFolder;
  uint8_t currentTrack;
  uint8_t currentFolder;
  byte busyPin;
  Mp3Player(byte rx, byte tx, byte busy) :  busyPin(busy), mySoftwareSerial(rx, tx), DFMiniMp3<SoftwareSerial, Mp3Notify>(mySoftwareSerial)
  {}

  void begin() {  // overrides begin() of base class
    Serial.println("Initialize mp3 player");
    DFMiniMp3::begin(); // caution: uses 9600 for software serial connection
    setVolume(20);
  }

  void playCommandSound(Mp3VoiceCommand com) {
    playMp3FolderTrack(com);
  }

  void setFolder(uint8_t folder) {
    currentFolder = folder;
    numTracksInFolder = getFolderTrackCount(folder);
    currentTrack = 1;
    Serial.print("Set Folder to ");
    Serial.print(folder);
    Serial.print(" (");
    Serial.print(numTracksInFolder);
    Serial.println(" tracks in folder)!");
  }

  void play() {
    playFolderTrack(currentFolder, currentTrack);
  }

  /*bool isPlaying() { 
    return !digitalRead(busyPin); 
  }

  // override
  void loop() {
    DFMiniMp3::loop();
    //Serial.println("Extra!");
    //listenForReply(0x00);
    uint8_t replyCommand = 0;
    uint16_t replyArg = 0;
    
  }*/
  
};
#endif
