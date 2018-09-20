/*
  Reading multiple RFID tags, simultaneously!
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 3rd, 2016
  https://github.com/sparkfun/Simultaneous_RFID_Tag_Reader

  Constantly reads and outputs any tags heard

  If using the Simultaneous RFID Tag Reader (SRTR) shield, make sure the serial slide
  switch is in the 'SW-UART' position
*/

#include <SoftwareSerial.h> //Used for transmitting to the device

SoftwareSerial softSerial(2, 3); //RX, TX

#include "SparkFun_UHF_RFID_Reader.h" //Library for controlling the M6E Nano module
RFID nano; //Create instance

int id[14];
int lastid[14];

struct usertype {
     int serialno[12];
     char studentno[9];
};

struct usertype Users[3];

struct dbtype {
     struct usertype Users[3];
};

struct dbtype Namedb = {
      {{{0xE2,0x00,0x40,0x05,0x73,0x07,0x00,0x67,0x22,0x90,0x28,0xF7},"500732643"},    //1b
       {{0xE2,0x00,0x00,0x15,0x86,0x0E,0x01,0x31,0x15,0x40,0x7E,0xF2},"500737532"},    //2
       {{0xE2,0x00,0x20,0x00,0x20,0x00,0x00,0x00,0x15,0x64,0x46,0x3C},"500733099"}}    //paperclip
};

void setup()
{
  Serial.begin(115200);
  while (!Serial); //Wait for the serial port to come online

  if (setupNano(38400) == false) //Configure nano to run at 38400bps
  {
    while (1); //Freeze!
  }

  nano.setRegion(REGION_EUROPE); //Set to North America

  nano.setReadPower(500); //5.00 dBm. Higher values may caues USB port to brown out
  //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling
  
  nano.startReading(); //Begin scanning for tags
}

   
void loop()
{

  if (nano.check() == true) //Check to see if any new data has come in from module
  {
    if (nano.parseResponse() == RESPONSE_IS_TAGFOUND)
    {
      //byte tagEPCBytes = nano.getTagEPCBytes(); //Get the number of bytes of EPC from response

      for (uint8_t i=0; i<12; i++) {
        id[i]=nano.msg[i+31];
      }
      
      if(memcmp(lastid, id, sizeof(id)) != 0) {
        uint16_t dbsize = sizeof(Users)/sizeof(*Users);
        for(uint16_t i=0; i<dbsize; i++) {
          if(memcmp(id, (Namedb.Users[i].serialno), 12) == 0) {
            Serial.print(Namedb.Users[i].studentno);
            Serial.print("\r\n");
          }
        }
      }
      memcpy(&lastid, &id, sizeof(id));
    }
  }
}

//Gracefully handles a reader that is already configured and already reading continuously
//Because Stream does not have a .begin() we have to do this outside the library
boolean setupNano(long baudRate)
{
  nano.begin(softSerial); //Tell the library to communicate over software serial port

  //Test to see if we are already connected to a module
  //This would be the case if the Arduino has been reprogrammed and the module has stayed powered
  softSerial.begin(baudRate); //For this test, assume module is already at our desired baud rate
  while(!softSerial); //Wait for port to open

  //About 200ms from power on the module will send its firmware version at 115200. We need to ignore this.
  while(softSerial.available()) softSerial.read();
  
  nano.getVersion();

  if (nano.msg[0] == ERROR_WRONG_OPCODE_RESPONSE)
  {
    //This happens if the baud rate is correct but the module is doing a continuous read
    nano.stopReading();
    delay(1500);
  }
  else
  {
    //The module did not respond so assume it's just been powered on and communicating at 115200bps
    softSerial.begin(115200); //Start software serial at 115200

    nano.setBaud(baudRate); //Tell the module to go to the chosen baud rate. Ignore the response msg

    softSerial.begin(baudRate); //Start the software serial port, this time at user's chosen baud rate
  }

  //Test the connection
  nano.getVersion();
  if (nano.msg[0] != ALL_GOOD) return (false); //Something is not right

  //The M6E has these settings no matter what
  nano.setTagProtocol(); //Set protocol to GEN2

  nano.setAntennaPort(); //Set TX/RX antenna ports to 1

  return (true); //We are ready to rock
}

