/*
 * Sketch for Tally Shield
 */

// Including libraries: 
#include <SPI.h>
#include <Ethernet.h>
#include <Streaming.h>
#include <MemoryFree.h>
#include <SkaarhojPgmspace.h>
#include <Wire.h>

// MAC address and IP address for this *particular* Arduino / Ethernet Shield!
// The MAC address is printed on a label on the shield or on the back of your device
// The IP address should be an available address you choose on your subnet where the switcher is also present
byte mac[] = { 0x90, 0xA2, 0xDA, 0x10, 0x63, 0x8E };
IPAddress clientIp(192, 168, 10, 99);
IPAddress switcherIp(192, 168, 10, 240);

// Include ATEMbase library and make an instance:
// The port number is chosen randomly among high numbers.
#include <ATEMbase.h>
#include <ATEMstd.h>
ATEMstd AtemSwitcher;

const byte  mcp_addr=0x20;      // I2C Address of MCP23017 Chip
const byte  GPIOA=0x12;            // Register Address of Port A
const byte  GPIOB=0x13; 

// Selects which Switcher Input is routed to each tally channel
const int channel_inputs[8]={1,2,3,4,4,5,6,7};

void setup() { 

  // initialize digital outputs.
  pinMode(8, OUTPUT);//MCP23017 Reset Pin
  digitalWrite(8,LOW);//Hold MCP in Reset

  randomSeed(analogRead(5));  // For random port selection
  
  // Start Ethernet:
  Ethernet.begin(mac,clientIp);

  // Initialize a connection to the switcher:
  AtemSwitcher.begin(switcherIp);
  AtemSwitcher.serialOutput(0x80);
  AtemSwitcher.connect();
  

  digitalWrite(8,HIGH); //Take MCP out of Reset

  // Fire up I2C
  Wire.begin();

  //Initial MCP23017 Config
  Wire.beginTransmission(mcp_addr);
  Wire.write((byte)0x00); // IODIRA register
  Wire.write((byte)0x00); // set all of bank A to outputs
  Wire.write((byte)0x00); // set all of bank B to outputs 
  Wire.endTransmission();
}

void loop() {
  // Check for packets, respond to them etc. Keeping the connection alive!
  // VERY important that this function is called all the time - otherwise connection might be lost because packets from the switcher is
  // overlooked and not responded to.
    AtemSwitcher.runLoop();

  //Reset out output variables to 0
  byte outa=0x00;
  byte outb=0x00;
  int state;

  // Get Tally Info
  if (AtemSwitcher.hasInitialized())
  {
    //Get Status for tally channels 1 thru 8
    for (uint8_t i=0;i<=7;i++)
    {
      uint8_t flag = AtemSwitcher.getTallyByIndexTallyFlags(channel_inputs[i]-1);
      switch(flag)
      {
        case 1://Program
          state=1;
          break;
        case 2://Preview
          state=2;
          break;
        case 3://Both(Transition)
          state=1;
          break;
        default://Neither (Off)
          state=0;
          break;
      }
      //Encode output states
      if(i<4)
        outa |= state<<((i%4)*2);
      else
        outb |= state<<((i%4)*2);
    }
    //Update outputs
    Wire.beginTransmission(mcp_addr);
    Wire.write(GPIOA); // IODIRA register
    Wire.write(outa); // set bank A
    Wire.endTransmission();
    Wire.beginTransmission(mcp_addr);
    Wire.write(GPIOB);
    Wire.write(outb); // set bank B
    Wire.endTransmission();
  }
  else
  {
    // Set all outputs to 0
    Wire.beginTransmission(mcp_addr);
    Wire.write(GPIOA); // IODIRA register
    Wire.write((byte)0x00); // set all of bank A 0
    Wire.endTransmission();
    Wire.beginTransmission(mcp_addr);
    Wire.write(GPIOB);
    Wire.write((byte)0x00); // set all of bank B to 0
    Wire.endTransmission();
  }
}
