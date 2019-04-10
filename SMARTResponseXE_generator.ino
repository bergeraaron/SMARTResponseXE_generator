//#include <SmartResponseXE.h>
//move this internally
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
//#include "SmartResponseXE.h"
//#include <SPI.h>

// The power button is connected to PD2 (RX1) and signals INT2
#define POWER_BUTTON 0xd2
// Font sizes (9x8, 6x8, 12x16, 15x16)
#define FONT_NORMAL 0
#define FONT_SMALL 1
#define FONT_MEDIUM 2
#define FONT_LARGE 3

// Keyboard info
#define ROWS 6
#define COLS 10

// Display info
#define LCD_WIDTH 384
#define LCD_HEIGHT 136

//my defines
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

#define MAX_CHANNEL 26
#define MIN_CHANNEL 11

#define PKT_TYPE_BEACON 0x8000
typedef struct
{
  uint16_t frame_control;//2 bytes
  uint8_t  seq;
  uint16_t source_pan;//2 bytes
  uint16_t source_address;//2 bytes
  uint16_t surperrf_spec;//2 bytes
  uint16_t gts;//2 bytes
}beacon_pkt;

#define PKT_TYPE_DATA 0x8841
typedef struct
{
  uint16_t frame_control;//2 bytes
  uint8_t  seq;
  uint16_t dest_pan;//2 bytes
  uint16_t dest_address;//2 bytes
  uint16_t source_address;//2 bytes
  unsigned char payload[12];//make the max size?
}data_pkt;

#define PKT_TYPE_CMD 0x8023
typedef struct
{
  uint16_t frame_control;//2 bytes
  uint8_t  seq;
  uint16_t source_pan;//2 bytes
  uint16_t source_address;//2 bytes
  uint8_t  cmd;//cmd frame id?
}cmd_pkt;

#define PKT_TYPE_SIMPLICI_PING 0x8801
typedef struct
{
  uint16_t frame_control;//2 bytes
  uint8_t  seq;
  uint32_t dest_address;//4 bytes
  uint32_t source_address;//4 bytes
  uint8_t port;
  uint8_t device_info;
  uint8_t trans_id;
  unsigned char payload[12];//make the max size?
}simpliciTI_ping_packet;

#define num_pkt_types 4

//globals
int8_t selected_chan = -1;//-1 is to hop
uint8_t cur_chan = 0;//index of below
uint8_t chan_list = {11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26};

uint8_t time_btwn_pkts = 0;//time to wait between sending packets

uint8_t sel_pkt_type = 1;

uint8_t state = 0;

uint32_t num_pkt_sent = 0;

uint8_t last_pkt_sent[128];
uint8_t last_pkt_sent_len = 0;

enum states {
  waiting_for_input,
  running
};


void setup()
{
  screen_setup();
  //need to setup the radio
  rfBegin(17);
}

void draw_menu()
{
  //draw the menu system
  char lcd_out[64];

  uint8_t yoffset = 0;
  uint8_t dist_btw_btn = 20;
  //pkt type
  memset(lcd_out,0x00,64);
  sprintf(lcd_out,"Pkt Type:%d\n",sel_pkt_type);
  SRXEWriteString(0,yoffset,lcd_out, FONT_NORMAL, 3, 0);
  yoffset = yoffset + dist_btw_btn;
  //channel
  memset(lcd_out,0x00,64);
  sprintf(lcd_out,"Channel:%d\n",selected_chan);
  SRXEWriteString(0,yoffset,lcd_out, FONT_NORMAL, 3, 0);
  yoffset = yoffset + dist_btw_btn;
  //time_btwn_pkts
  memset(lcd_out,0x00,64);
  sprintf(lcd_out,"Time Btwn Pkt:%d\n",time_btwn_pkts);
  SRXEWriteString(0,yoffset,lcd_out, FONT_NORMAL, 3, 0);
  yoffset = yoffset + dist_btw_btn;
  //start
  memset(lcd_out,0x00,64);
  sprintf(lcd_out,"Start\n");
  SRXEWriteString(0,yoffset,lcd_out, FONT_NORMAL, 3, 0);
  yoffset = yoffset + dist_btw_btn;
  //stop
  memset(lcd_out,0x00,64);
  sprintf(lcd_out,"Stop\n");
  SRXEWriteString(0,yoffset,lcd_out, FONT_NORMAL, 3, 0);
  yoffset = yoffset + dist_btw_btn;

  //print the status running or not


  //print the last packet out
  

}

void loop()
{
  char lcd_out[64];
  byte keypressed = 0x00;
  uint8_t pkt_len = 0x00;
  uint8_t * pkt;
  //declare each packet a head of time
  beacon_pkt beacon;
  data_pkt   data;
  cmd_pkt    cmd;
  simpliciTI_ping_packet ping;

  while(1)
  {
      //delay(5);
      if(ctr > 10000)
      {
        keypressed = SRXEGetKey();
        sprintf(lcd_out,"button pressed:%d %02X",keypressed,keypressed);
        SRXEWriteString(0,10,lcd_out, FONT_NORMAL, 3, 0);
/**
 * Keys
 * Left Side
 * 1 - F0
 * 2 - F1
 * 3 - F2
 * 4 - F3
 * 5 - F4
 * Right Side
 * 1 - F5
 * 2 - F6
 * 3 - F7
 * 4 - F8
 * 5 - F9
 */
        if(keypressed == 0xF0)//pkt type
        {
          sel_pkt_type++;
          if(sel_pkt_type > num_pkt_types)//roll around
            sel_pkt_type = 1;

          if(sel_pkt_type == 1)
          {
            beacon.frame_control = 0x8000;//2 bytes
            beacon.seq = 0x01;
            beacon.source_pan = 0x2010;//2 bytes
            beacon.source_address = 0xAAAA;//2 bytes
            beacon.surperrf_spec = 0x664F;//2 bytes
            beacon.gts = 0x8000;//2 bytes
            pkt_len = 0x0D-2;
            pkt = (uint8_t*)&beacon;
          }
          else if(sel_pkt_type == 2)
          {

          }
          else if(sel_pkt_type == 3)
          {

          }
          else if(sel_pkt_type == 4)
          {

          }
        }
        else if(keypressed == 0xF1)//channel
        {
          selected_chan++;
          if(selected_chan > 16)
            selected_chan = -1;//roll back to hop
          if(selected_chan == -1)
            cur_chan = 0; 
        }
        else if(keypressed == 0xF2)//time_btwn_pkts
        {
          time_btwn_pkts++;
          if(time_btwn_pkts > 10)
            time_btwn_pkts=0;
        }
        else if(keypressed == 0xF3)//start
        {
          state = 1;
        }
        else if(keypressed == 0xF4)//stop
        {
          state = 0;
        }
      }//end of key pressed
    //look and see what state we are in
    if(state == 1)//running
    {
      //send the packet
      rfPrint(pkt, pkt_len);
      sprintf(lcd_out,"packet sent \n");
      SRXEWriteString(0,30,lcd_out, FONT_NORMAL, 3, 0);
      num_pkt_sent++;
      for(int i=0;i < pkt_len;i++)
        last_pkt_sent[i] = pkt[i];
      last_pkt_sent_len=pkt_len;
    }
    else
    {
      //draw the screen?
      draw_menu();
    }

  }//main loop
/**/
}

// Initialize the RFA1's low-power 2.4GHz transciever.
// Sets up the state machine, and gets the radio into
// the RX_ON state. Interrupts are enabled for RX
// begin and end, as well as TX end.
uint8_t rfBegin(uint8_t channel)
{
  // Transceiver Pin Register -- TRXPR.
  // This register can be used to reset the transceiver, without
  // resetting the MCU.
  TRXPR |= (1<<TRXRST);   // TRXRST = 1 (Reset state, resets all registers)

  // Transceiver Interrupt Enable Mask - IRQ_MASK
  // This register disables/enables individual radio interrupts.
  // First, we'll disable IRQ and clear any pending IRQ's
  IRQ_MASK = 0;  // Disable all IRQs

  // Transceiver State Control Register -- TRX_STATE
  // This regiseter controls the states of the radio.
  // First, we'll set it to the TRX_OFF state.
  TRX_STATE = (TRX_STATE & 0xE0) | TRX_OFF;  // Set to TRX_OFF state
  delay(1);

  // Transceiver Status Register -- TRX_STATUS
  // This read-only register contains the present state of the radio transceiver.
  // After telling it to go to the TRX_OFF state, we'll make sure it's actually
  // there.
  if ((TRX_STATUS & 0x1F) != TRX_OFF) // Check to make sure state is correct
    return 0;   // Error, TRX isn't off

  // Transceiver Control Register 1 - TRX_CTRL_1
  // We'll use this register to turn on automatic CRC calculations.
  TRX_CTRL_1 |= (1<<TX_AUTO_CRC_ON);  // Enable automatic CRC calc.
  // Enable RX start/end and TX end interrupts
  IRQ_MASK = (1<<RX_START_EN) | (1<<RX_END_EN) | (1<<TX_END_EN);

  // Transceiver Clear Channel Assessment (CCA) -- PHY_CC_CCA
  // This register is used to set the channel. CCA_MODE should default
  // to Energy Above Threshold Mode.
  // Channel should be between 11 and 26 (2405 MHz to 2480 MHz)
  if ((channel < 11) || (channel > 26)) channel = 11;
  PHY_CC_CCA = (PHY_CC_CCA & 0xE0) | channel;

  // Finally, we'll enter into the RX_ON state. Now waiting for radio RX's, unless
  // we go into a transmitting state.
  TRX_STATE = (TRX_STATE & 0xE0) | RX_ON; // Default to receiver

  return 1;
}

// This function sends a string of characters out of the radio.
// Given a string, it'll format a frame, and send it out.
void rfPrint(uint8_t * frame, uint8_t pkt_len)
{

  // Transceiver State Control Register -- TRX_STATE
  // This regiseter controls the states of the radio.
  // Set to the PLL_ON state - this state begins the TX.
  TRX_STATE = (TRX_STATE & 0xE0) | PLL_ON;  // Set to TX start state
  while(!(TRX_STATUS & PLL_ON))
    ;  // Wait for PLL to lock

  // Start of frame buffer - TRXFBST
  // This is the first byte of the 128 byte frame. It should contain
  // the length of the transmission.
  TRXFBST = pkt_len + 2;
  memcpy((void *)(&TRXFBST+1), frame, pkt_len);
  // Transceiver Pin Register -- TRXPR.
  // From the PLL_ON state, setting SLPTR high will initiate the TX.
  TRXPR |= (1<<SLPTR);   // SLPTR high
  TRXPR &= ~(1<<SLPTR);  // SLPTR low

  // After sending the byte, set the radio back into the RX waiting state.
  TRX_STATE = (TRX_STATE & 0xE0) | RX_ON;
}

// This interrupt is called when radio TX is complete. We'll just
// use it to turn off our TX LED.
ISR(TRX24_TX_END_vect)
{
  //done writing, increase the pkt counter?
}

uint8_t rfChannel(uint8_t channel)
{
  // Transceiver Clear Channel Assessment (CCA) -- PHY_CC_CCA
  // This register is used to set the channel. CCA_MODE should default
  // to Energy Above Threshold Mode.
  // Channel should be between 11 and 26 (2405 MHz to 2480 MHz)
  if ((channel < 11) || (channel > 26)) channel = 11;
  PHY_CC_CCA = (PHY_CC_CCA & 0xE0) | channel; // Set the channel to 11
  
  return 1;
}

void screen_setup()
{
  // CS, D/C, RESET
  SRXEInit(0xe7, 0xd6, 0xa2); // initialize display
}

//EVERYTHING FROM THE SmartReponseXE library
//
// SMART Response XE library
//
// LCD and keyboard routines for the SRXE handheld classroom communicator
//
// Copyright (c) 2018 BitBank Software, Inc.
// written by Larry Bank
// email: bitbank@pobox.com
// Project started 8/4/2018
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

// Mapping of keyboard to GPIO pins
//static byte rowPins[ROWS] = {6,35,34,8,9,0};
//static byte colPins[COLS] = {4,A1,A3,2,1,25,16,19,23,22};
const uint8_t rowPins[ROWS] = {0xe6, 0xb7, 0xb6, 0xb5, 0xb4, 0xe0};
const uint8_t colPins[COLS] = {0xe4, 0xf1, 0xf3, 0xe2, 0xe1, 0xd7, 0xa0, 0xa5, 0xd5, 0xd4};
static byte bKeyMap[COLS]; // bits indicating pressed keys
static byte bOldKeyMap[COLS]; // previous map to look for pressed/released keys
static byte bColorToByte[4] = {0, 0x4a, 0x92, 0xff};
static byte iCSPin, iDCPin, iResetPin;

static int iScrollOffset;
void SRXEFill(byte ucData);
static void SRXEWriteCommand(unsigned char c);

typedef enum
{
 MODE_DATA = 0,
 MODE_COMMAND
} DC_MODE;

#ifndef HIGH
#define HIGH 1
#define LOW 0
#define INPUT 0
#define INPUT_PULLUP 1
#define OUTPUT 2
#endif

// Chip select for the external 1Mb flash module
#define CS_FLASH 0xd3

//Keyboard
//Logical Layout (SK# are screen keys: top to bottom 1-5 on left, 6-10 on right):
//                ROW1|ROW2|ROW3|ROW4|ROW5|ROW6|ROW7|ROW8|ROW9|ROW10
//           COL1    1|   2|   3|   4|   5|   6|   7|   8|   9|    0
//           COL2    Q|   W|   E|   R|   T|   Y|   U|   I|   O|    P
//           COL3    A|   S|   D|   F|   G|   H|   J|   K|   L| Bksp
//           COL4 Shft|   Z|   X|   C|   V|   B|   N|   M|Entr|   Up
//           COL5  Sym|Frac|Root| Exp| Spc|   ,|   .|Down|Left|Right
//           COL6  SK1| SK2| SK3| SK4| SK5| SK6| SK7| SK8| SK9| SK10
byte OriginalKeys[] = {'1','2','3','4','5','6','7','8','9','0',
                       'q','w','e','r','t','y','u','i','o','p',
                       'a','s','d','f','g','h','j','k','l',8,
                       0  ,'z','x','c','v','b','n',0x5,0,0x4, // 5 = down, 4 = up
                       0  ,0xd,  0,  0,' ',',','.','m',  2,  3, // 2 = left, 3 = right
                       0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9};
byte ShiftedKeys[] =  {'1','2','3','4','5','6','7','8','9','0',
                       'Q','W','E','R','T','Y','U','I','O','P',
                       'A','S','D','F','G','H','J','K','L',8,
                       0  ,'Z','X','C','V','B','N',0x5,0,0x4, // 5 = down
                       0  ,0xd,  0,  0,'_',',','.','M',  2,  3,
                       0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9};
byte SymKeys[] =       {'!','2','3','$','%','6','\'','\"','(',')',
                       'q','w','e','r','t','y','u','i','[',']',
                       '=','+','-','f','g','h','j',':','?',8,
                        0 ,'z','x','c','v','b','n',0x5,0,0x4,
                        0 ,0xd, 0 , 0 ,0x1,'<','>','m', 2, 3, // 1 = menu
                       0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9};

//
// Power on the LCD
//
const char powerup[] PROGMEM = {
1, 0x01, // soft reset
99, 120, // 120ms delay
1, 0x11,  // sleep out
1, 0x28,  // display off
99, 50, // 50ms delay
3, 0xc0, 0xf0, 0x00, // Vop = 0xF0
2, 0xc3, 0x04, // BIAS = 1/14
2, 0xc4, 0x05, // Booster = x8
2, 0xd0, 0x1d, // Enable analog circuit
2, 0xb3, 0x00, // Set FOSC divider
2, 0xb5, 0x8b, // N-Line = 0
1, 0x38,       // Set grayscale mode (0x39 = monochrome mode)
2, 0x3a, 0x02, // Enable DDRAM interface
2, 0x36, 0x00, // Scan direction setting
2, 0xB0, 0x9f, // Duty setting (0x87?)
5, 0xf0, 0x12,0x12,0x12,0x12, // 77Hz frame rate in all temperatures
1, 0x20, // Display inversion off
1, 0x29, // Display ON
0};

// small font
const byte ucFont[]PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x5f,0x5f,0x06,0x00,0x00,
  0x00,0x07,0x07,0x00,0x07,0x07,0x00,0x00,0x14,0x7f,0x7f,0x14,0x7f,0x7f,0x14,0x00,
  0x24,0x2e,0x2a,0x6b,0x6b,0x3a,0x12,0x00,0x46,0x66,0x30,0x18,0x0c,0x66,0x62,0x00,
  0x30,0x7a,0x4f,0x5d,0x37,0x7a,0x48,0x00,0x00,0x04,0x07,0x03,0x00,0x00,0x00,0x00,
  0x00,0x1c,0x3e,0x63,0x41,0x00,0x00,0x00,0x00,0x41,0x63,0x3e,0x1c,0x00,0x00,0x00,
  0x08,0x2a,0x3e,0x1c,0x1c,0x3e,0x2a,0x08,0x00,0x08,0x08,0x3e,0x3e,0x08,0x08,0x00,
  0x00,0x00,0x80,0xe0,0x60,0x00,0x00,0x00,0x00,0x08,0x08,0x08,0x08,0x08,0x08,0x00,
  0x00,0x00,0x00,0x60,0x60,0x00,0x00,0x00,0x60,0x30,0x18,0x0c,0x06,0x03,0x01,0x00,
  0x3e,0x7f,0x59,0x4d,0x47,0x7f,0x3e,0x00,0x40,0x42,0x7f,0x7f,0x40,0x40,0x00,0x00,
  0x62,0x73,0x59,0x49,0x6f,0x66,0x00,0x00,0x22,0x63,0x49,0x49,0x7f,0x36,0x00,0x00,
  0x18,0x1c,0x16,0x53,0x7f,0x7f,0x50,0x00,0x27,0x67,0x45,0x45,0x7d,0x39,0x00,0x00,
  0x3c,0x7e,0x4b,0x49,0x79,0x30,0x00,0x00,0x03,0x03,0x71,0x79,0x0f,0x07,0x00,0x00,
  0x36,0x7f,0x49,0x49,0x7f,0x36,0x00,0x00,0x06,0x4f,0x49,0x69,0x3f,0x1e,0x00,0x00,
  0x00,0x00,0x00,0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x80,0xe6,0x66,0x00,0x00,0x00,
  0x08,0x1c,0x36,0x63,0x41,0x00,0x00,0x00,0x00,0x14,0x14,0x14,0x14,0x14,0x14,0x00,
  0x00,0x41,0x63,0x36,0x1c,0x08,0x00,0x00,0x00,0x02,0x03,0x59,0x5d,0x07,0x02,0x00,
  0x3e,0x7f,0x41,0x5d,0x5d,0x5f,0x0e,0x00,0x7c,0x7e,0x13,0x13,0x7e,0x7c,0x00,0x00,
  0x41,0x7f,0x7f,0x49,0x49,0x7f,0x36,0x00,0x1c,0x3e,0x63,0x41,0x41,0x63,0x22,0x00,
  0x41,0x7f,0x7f,0x41,0x63,0x3e,0x1c,0x00,0x41,0x7f,0x7f,0x49,0x5d,0x41,0x63,0x00,
  0x41,0x7f,0x7f,0x49,0x1d,0x01,0x03,0x00,0x1c,0x3e,0x63,0x41,0x51,0x33,0x72,0x00,
  0x7f,0x7f,0x08,0x08,0x7f,0x7f,0x00,0x00,0x00,0x41,0x7f,0x7f,0x41,0x00,0x00,0x00,
  0x30,0x70,0x40,0x41,0x7f,0x3f,0x01,0x00,0x41,0x7f,0x7f,0x08,0x1c,0x77,0x63,0x00,
  0x41,0x7f,0x7f,0x41,0x40,0x60,0x70,0x00,0x7f,0x7f,0x0e,0x1c,0x0e,0x7f,0x7f,0x00,
  0x7f,0x7f,0x06,0x0c,0x18,0x7f,0x7f,0x00,0x1c,0x3e,0x63,0x41,0x63,0x3e,0x1c,0x00,
  0x41,0x7f,0x7f,0x49,0x09,0x0f,0x06,0x00,0x1e,0x3f,0x21,0x31,0x61,0x7f,0x5e,0x00,
  0x41,0x7f,0x7f,0x09,0x19,0x7f,0x66,0x00,0x26,0x6f,0x4d,0x49,0x59,0x73,0x32,0x00,
  0x03,0x41,0x7f,0x7f,0x41,0x03,0x00,0x00,0x7f,0x7f,0x40,0x40,0x7f,0x7f,0x00,0x00,
  0x1f,0x3f,0x60,0x60,0x3f,0x1f,0x00,0x00,0x3f,0x7f,0x60,0x30,0x60,0x7f,0x3f,0x00,
  0x63,0x77,0x1c,0x08,0x1c,0x77,0x63,0x00,0x07,0x4f,0x78,0x78,0x4f,0x07,0x00,0x00,
  0x47,0x63,0x71,0x59,0x4d,0x67,0x73,0x00,0x00,0x7f,0x7f,0x41,0x41,0x00,0x00,0x00,
  0x01,0x03,0x06,0x0c,0x18,0x30,0x60,0x00,0x00,0x41,0x41,0x7f,0x7f,0x00,0x00,0x00,
  0x08,0x0c,0x06,0x03,0x06,0x0c,0x08,0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
  0x00,0x00,0x03,0x07,0x04,0x00,0x00,0x00,0x20,0x74,0x54,0x54,0x3c,0x78,0x40,0x00,
  0x41,0x7f,0x3f,0x48,0x48,0x78,0x30,0x00,0x38,0x7c,0x44,0x44,0x6c,0x28,0x00,0x00,
  0x30,0x78,0x48,0x49,0x3f,0x7f,0x40,0x00,0x38,0x7c,0x54,0x54,0x5c,0x18,0x00,0x00,
  0x48,0x7e,0x7f,0x49,0x03,0x06,0x00,0x00,0x98,0xbc,0xa4,0xa4,0xf8,0x7c,0x04,0x00,
  0x41,0x7f,0x7f,0x08,0x04,0x7c,0x78,0x00,0x00,0x44,0x7d,0x7d,0x40,0x00,0x00,0x00,
  0x60,0xe0,0x80,0x84,0xfd,0x7d,0x00,0x00,0x41,0x7f,0x7f,0x10,0x38,0x6c,0x44,0x00,
  0x00,0x41,0x7f,0x7f,0x40,0x00,0x00,0x00,0x7c,0x7c,0x18,0x78,0x1c,0x7c,0x78,0x00,
  0x7c,0x78,0x04,0x04,0x7c,0x78,0x00,0x00,0x38,0x7c,0x44,0x44,0x7c,0x38,0x00,0x00,
  0x84,0xfc,0xf8,0xa4,0x24,0x3c,0x18,0x00,0x18,0x3c,0x24,0xa4,0xf8,0xfc,0x84,0x00,
  0x44,0x7c,0x78,0x4c,0x04,0x0c,0x18,0x00,0x48,0x5c,0x54,0x74,0x64,0x24,0x00,0x00,
  0x04,0x04,0x3e,0x7f,0x44,0x24,0x00,0x00,0x3c,0x7c,0x40,0x40,0x3c,0x7c,0x40,0x00,
  0x1c,0x3c,0x60,0x60,0x3c,0x1c,0x00,0x00,0x3c,0x7c,0x60,0x30,0x60,0x7c,0x3c,0x00,
  0x44,0x6c,0x38,0x10,0x38,0x6c,0x44,0x00,0x9c,0xbc,0xa0,0xa0,0xfc,0x7c,0x00,0x00,
  0x4c,0x64,0x74,0x5c,0x4c,0x64,0x00,0x00,0x08,0x08,0x3e,0x77,0x41,0x41,0x00,0x00,
  0x00,0x00,0x00,0x77,0x77,0x00,0x00,0x00,0x41,0x41,0x77,0x3e,0x08,0x08,0x00,0x00,
  0x02,0x03,0x01,0x03,0x02,0x03,0x01,0x00,0x70,0x78,0x4c,0x46,0x4c,0x78,0x70,0x00};

  // 5x7 font (in 6x8 cell)
const unsigned char ucSmallFont[]PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x5f,0x06,0x00,0x00,0x07,0x03,0x00,
  0x07,0x03,0x00,0x24,0x7e,0x24,0x7e,0x24,0x00,0x24,0x2b,0x6a,0x12,0x00,0x00,0x63,
  0x13,0x08,0x64,0x63,0x00,0x36,0x49,0x56,0x20,0x50,0x00,0x00,0x07,0x03,0x00,0x00,
  0x00,0x00,0x3e,0x41,0x00,0x00,0x00,0x00,0x41,0x3e,0x00,0x00,0x00,0x08,0x3e,0x1c,
  0x3e,0x08,0x00,0x08,0x08,0x3e,0x08,0x08,0x00,0x00,0xe0,0x60,0x00,0x00,0x00,0x08,
  0x08,0x08,0x08,0x08,0x00,0x00,0x60,0x60,0x00,0x00,0x00,0x20,0x10,0x08,0x04,0x02,
  0x00,0x3e,0x51,0x49,0x45,0x3e,0x00,0x00,0x42,0x7f,0x40,0x00,0x00,0x62,0x51,0x49,
  0x49,0x46,0x00,0x22,0x49,0x49,0x49,0x36,0x00,0x18,0x14,0x12,0x7f,0x10,0x00,0x2f,
  0x49,0x49,0x49,0x31,0x00,0x3c,0x4a,0x49,0x49,0x30,0x00,0x01,0x71,0x09,0x05,0x03,
  0x00,0x36,0x49,0x49,0x49,0x36,0x00,0x06,0x49,0x49,0x29,0x1e,0x00,0x00,0x6c,0x6c,
  0x00,0x00,0x00,0x00,0xec,0x6c,0x00,0x00,0x00,0x08,0x14,0x22,0x41,0x00,0x00,0x24,
  0x24,0x24,0x24,0x24,0x00,0x00,0x41,0x22,0x14,0x08,0x00,0x02,0x01,0x59,0x09,0x06,
  0x00,0x3e,0x41,0x5d,0x55,0x1e,0x00,0x7e,0x11,0x11,0x11,0x7e,0x00,0x7f,0x49,0x49,
  0x49,0x36,0x00,0x3e,0x41,0x41,0x41,0x22,0x00,0x7f,0x41,0x41,0x41,0x3e,0x00,0x7f,
  0x49,0x49,0x49,0x41,0x00,0x7f,0x09,0x09,0x09,0x01,0x00,0x3e,0x41,0x49,0x49,0x7a,
  0x00,0x7f,0x08,0x08,0x08,0x7f,0x00,0x00,0x41,0x7f,0x41,0x00,0x00,0x30,0x40,0x40,
  0x40,0x3f,0x00,0x7f,0x08,0x14,0x22,0x41,0x00,0x7f,0x40,0x40,0x40,0x40,0x00,0x7f,
  0x02,0x04,0x02,0x7f,0x00,0x7f,0x02,0x04,0x08,0x7f,0x00,0x3e,0x41,0x41,0x41,0x3e,
  0x00,0x7f,0x09,0x09,0x09,0x06,0x00,0x3e,0x41,0x51,0x21,0x5e,0x00,0x7f,0x09,0x09,
  0x19,0x66,0x00,0x26,0x49,0x49,0x49,0x32,0x00,0x01,0x01,0x7f,0x01,0x01,0x00,0x3f,
  0x40,0x40,0x40,0x3f,0x00,0x1f,0x20,0x40,0x20,0x1f,0x00,0x3f,0x40,0x3c,0x40,0x3f,
  0x00,0x63,0x14,0x08,0x14,0x63,0x00,0x07,0x08,0x70,0x08,0x07,0x00,0x71,0x49,0x45,
  0x43,0x00,0x00,0x00,0x7f,0x41,0x41,0x00,0x00,0x02,0x04,0x08,0x10,0x20,0x00,0x00,
  0x41,0x41,0x7f,0x00,0x00,0x04,0x02,0x01,0x02,0x04,0x00,0x80,0x80,0x80,0x80,0x80,
  0x00,0x00,0x03,0x07,0x00,0x00,0x00,0x20,0x54,0x54,0x54,0x78,0x00,0x7f,0x44,0x44,
  0x44,0x38,0x00,0x38,0x44,0x44,0x44,0x28,0x00,0x38,0x44,0x44,0x44,0x7f,0x00,0x38,
  0x54,0x54,0x54,0x08,0x00,0x08,0x7e,0x09,0x09,0x00,0x00,0x18,0xa4,0xa4,0xa4,0x7c,
  0x00,0x7f,0x04,0x04,0x78,0x00,0x00,0x00,0x00,0x7d,0x40,0x00,0x00,0x40,0x80,0x84,
  0x7d,0x00,0x00,0x7f,0x10,0x28,0x44,0x00,0x00,0x00,0x00,0x7f,0x40,0x00,0x00,0x7c,
  0x04,0x18,0x04,0x78,0x00,0x7c,0x04,0x04,0x78,0x00,0x00,0x38,0x44,0x44,0x44,0x38,
  0x00,0xfc,0x44,0x44,0x44,0x38,0x00,0x38,0x44,0x44,0x44,0xfc,0x00,0x44,0x78,0x44,
  0x04,0x08,0x00,0x08,0x54,0x54,0x54,0x20,0x00,0x04,0x3e,0x44,0x24,0x00,0x00,0x3c,
  0x40,0x20,0x7c,0x00,0x00,0x1c,0x20,0x40,0x20,0x1c,0x00,0x3c,0x60,0x30,0x60,0x3c,
  0x00,0x6c,0x10,0x10,0x6c,0x00,0x00,0x9c,0xa0,0x60,0x3c,0x00,0x00,0x64,0x54,0x54,
  0x4c,0x00,0x00,0x08,0x3e,0x41,0x41,0x00,0x00,0x00,0x00,0x77,0x00,0x00,0x00,0x00,
  0x41,0x41,0x3e,0x08,0x00,0x02,0x01,0x02,0x01,0x00,0x00,0x3c,0x26,0x23,0x26,0x3c};

uint8_t getPinInfo(uint8_t pin, volatile uint8_t **iDDR, volatile uint8_t **iPort, int bInput)
{
  uint8_t port, bit;
  
  port = (pin & 0xf0); // hex port (A,B,D,E,F)
  bit = pin & 0x7;
  switch (port)
  {
    case 0xA0: // really port G
      *iPort = (bInput) ? &PING : &PORTG;
      *iDDR = &DDRG;
      break;
    case 0xB0:
      *iPort = (bInput) ? &PINB : &PORTB;
      *iDDR = &DDRB;
      break;
    case 0xD0:
      *iPort = (bInput) ? &PIND : &PORTD;
      *iDDR = &DDRD;
      break;
    case 0xE0:
      *iPort = (bInput) ? &PINE : &PORTE;
      *iDDR = &DDRE;
      break;
    case 0xF0:
      *iPort = (bInput) ? &PINF : &PORTF;
      *iDDR = &DDRF;
      break;
  }
  return bit;
} /* getPinInfo() */
//
// Simplified pin numbering scheme uses a hex number to specify the port number
// and bit. Top 4 bits = port (B/D/E/F/G), bottom 3 bits specify the bit of the port
// e.g. 0xB4 = PORTB, bit 4, 0Ax is for port G
//
void mypinMode(uint8_t pin, uint8_t mode)
{
  uint8_t bit;
  volatile uint8_t *iPort, *iDDR;
  
  bit = getPinInfo(pin, &iDDR, &iPort, 0);
  switch (mode)
  {
    case INPUT:
      *iDDR &= ~(1<<bit);
      break;
    case INPUT_PULLUP:
      *iDDR |= (1<<bit);
      *iPort |= (1<<bit); // set the output high, then set it as an input
      *iDDR &= ~(1<<bit);
      break;
    case OUTPUT:
      *iDDR |= (1<<bit);
      break;
  }
} /* mypinMode() */

void mydigitalWrite(uint8_t pin, uint8_t value)
{
  uint8_t bit;
  volatile uint8_t *iPort, *iDDR;
  
  bit = getPinInfo(pin, &iDDR, &iPort, 0);
  if (value == LOW)
  {
    *iPort &= ~(1<<bit);
  }
  else
  {
    *iPort |= (1<<bit);
  }
} /* mydigitalWrite() */

uint8_t mydigitalRead(uint8_t pin)
{
  uint8_t bit;
  volatile uint8_t *iPort, *iDDR;
  
  bit = getPinInfo(pin, &iDDR, &iPort, 1);
  if (*iPort & (1<<bit))
    return HIGH;
  else
    return LOW;
} /* mydigitalRead() */
//
// Called when the power button is pressed to wake up the system
// Power up the display
//
ISR (INT2_vect)
{
  // cancel sleep as a precaution
  sleep_disable();
} 
//
// Put the device in a deep sleep to save power
// Wakes up when pressing the "power" button
//
void SRXESleep(void)
{
  // Turn off the LCD
  SRXEPowerDown();

  TRXPR = 1 << SLPTR; // send transceiver to sleep

  // disable ADC
  ADCSRA = 0;  
  DDRD &= ~(1 << PORTD2);  //PIN INT2 as input
  PORTD |= (1 << PORTD2); // pull-up resistor, the pin is forced to 1 if nothing is connected
  EIMSK &= ~(1 << INT2); //disabling interrupt on INT2
  EICRA &= ~((1<<ISC21) | (1<<ISC20)); // low level triggers interrupt
  EIFR |= (1 << INTF2); //clear interrupt flag
  EIMSK |= (1 << INT2); //enabling interrupt flag on INT2

  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();
 
  // turn off brown-out enable in software
  // BODS must be set to one and BODSE must be set to zero within four clock cycles
//  MCUCR = bit (BODS) | bit (BODSE);
  // The BODS bit is automatically cleared after three clock cycles
//  MCUCR = bit (BODS); 
  
  // We are guaranteed that the sleep_cpu call will be done
  // as the processor executes the next instruction after
  // interrupts are turned on.
  sleep_cpu ();   // one cycle
  SRXEPowerUp();
} /* SRXESleep() */

//
// Initialize SPI using direct register access
//
void SPI_Init(void)
{
  uint8_t temp;
  // Initialize SPI
  // Set SS to high so a connected chip will be "deselected" by default
//  digitalWrite(SS, HIGH);
  mydigitalWrite(0xb0, HIGH);
  
  // When the SS pin is set as OUTPUT, it can be used as
  // a general purpose output port (it doesn't influence
  // SPI operations).
//  pinMode(SS, OUTPUT);
  mypinMode(0xb0, OUTPUT);

  // SPCR = 01010000
  //interrupt disabled,spi enabled,msb 1st,master,clk low when idle,
  //sample on leading edge of clk,system clock/16 rate
  SPCR = (1<<SPE)|(1<<MSTR) | 1;
  temp=SPSR; // clear old data
  temp=SPDR;
  if (temp != 0) {}; // suppress compiler warning
  // Set SCK as output
  //pinMode(13, OUTPUT);
  mypinMode(0xb1, OUTPUT);
  // set MOSI as output
  //pinMode(11, OUTPUT);
  mypinMode(0xb2, OUTPUT);

} /* SPI_Init() */

uint8_t SPI_transfer(volatile char data)
{
  SPDR = data;                    // Start the transmission
  while (!(SPSR & (1<<SPIF)))     // Wait for the end of the transmission
  {
  };
  return SPDR;                    // return the received byte
} /* SPI_transfer() */

// Sets the D/C pin to data or command mode
static void SRXESetMode(int iMode)
{
  mydigitalWrite(iDCPin, (iMode == MODE_DATA));
} /* SRXESetMode() */
// Write a block of pixel data to the LCD
// Length can be anything from 1 to 504 (whole display)
void SRXEWriteDataBlock(unsigned char *ucBuf, int iLen)
{
  int i;
  
  mydigitalWrite(iCSPin, LOW);
  for (i=0; i<iLen; i++)
    SPI_transfer(ucBuf[i]);
  mydigitalWrite(iCSPin, HIGH);
}
//
// Command sequence to power ip the LCD controller
//
void SRXEPowerUp(void)
{
uint8_t ucTemp[4];
const char *pList = powerup;
uint8_t val, count, len = 1;

   while (len != 0)
   {
      len = pgm_read_byte(pList++);
      if (len == 99) // delay
      {
         val = pgm_read_byte(pList++);
         delay(val);
      }
      else if (len != 0) // send command with optional data
      {
         val = pgm_read_byte(pList++); // command
         SRXEWriteCommand(val);
         count = len-1;
         if (count != 0)
         {
            memcpy_P(ucTemp, pList, count);
            pList += count;
            SRXEWriteDataBlock(ucTemp, count);
         }
      }
   }
} /* SRXEPowerUp() */
//
// Initializes the LCD controller 
// Parameters: GPIO pin numbers used for the CS/DC/RST control lines
//
int SRXEInit(int iCS, int iDC, int iReset)
{
byte uc, ucTemp[8];

  iCSPin = iCS;
  iDCPin = iDC;
  iResetPin = iReset;

  SPI_Init();
  mypinMode(iCSPin,OUTPUT);
  mypinMode(CS_FLASH, OUTPUT); // in case we want to use the SPI flash
  mydigitalWrite(iCSPin, HIGH);
  
  mypinMode(iDCPin, OUTPUT);
  mypinMode(iResetPin, OUTPUT);

  // Start by reseting the LCD controller
  mydigitalWrite(iResetPin, HIGH);
  delay(50);
  mydigitalWrite(iResetPin, LOW);
  delay(5);
  mydigitalWrite(iResetPin, HIGH); // take it out of reset
  delay(150); // datasheet says it must be at least 120ms

//  mydigitalWrite(iCSPin, LOW); // leave CS low forever

  SRXEPowerUp(); // turn on and initialize the display
  
  SRXEFill(0); // erase memory (it's already cleared by resetting it)
  return 0;

} /* SRXEInit() */

//
// Turn off the LCD display (lowest power mode)
//
void SRXEPowerDown()
{
    SRXEFill(0); // fill memory with zeros to go to lowest power mode
    SRXEWriteCommand(0x28); // Display OFF
    SRXEWriteCommand(0x10); // Sleep in
} /* SRXEPowerDown() */
//
// Write a 1 byte command to the LCD controller
//
static void SRXEWriteCommand(unsigned char c)
{
  mydigitalWrite(iCSPin, LOW);
  SRXESetMode(MODE_COMMAND);
  SPI_transfer(c);
  SRXESetMode(MODE_DATA);
  mydigitalWrite(iCSPin, HIGH);
} /* SRXEWriteCommand() */
//
// Send commands to position the "cursor" to the given
// row and column
//
void SRXESetPosition(int x, int y, int cx, int cy)
{
byte ucTemp[4];

  if (x > 383 || y > 159 || cx > 384 || cy > 160)
     return; // invalid
  SRXEWriteCommand(0x2a); // set column address
  ucTemp[0] = 0; // start column high byte
  ucTemp[1] = x/3; // start column low byte
  ucTemp[2] = 0; // end col high byte
  ucTemp[3] = (x+cx-1)/3; // end col low byte
  SRXEWriteDataBlock(ucTemp, 4);
  SRXEWriteCommand(0x2b); // set row address
  ucTemp[0] = 0; // start row high byte
  ucTemp[1] = y; // start row low byte
  ucTemp[2] = 0; // end row high byte
  ucTemp[3] = y+cy-1; // end row low byte
  SRXEWriteDataBlock(ucTemp, 4);
  SRXEWriteCommand(0x2c); // write RAM
} /* SRXESetPosition() */

//
// Draw an outline or filled rectangle
// Only draws on byte boundaries (3 pixels wide)
// (display is treated as 128x136)
//
void SRXERectangle(int x, int y, int cx, int cy, byte color, byte bFilled)
{
byte bTemp[128];

   if (x < 0 || x > 127 || y < 0 || y > 135) return;
   if (x+cx > 127 || y+cy > 135) return;
   if (bFilled)
   {
      SRXESetPosition(x*3, y, cx*3, cy);
      for (y=0; y<cy; y++)
      {
         memset(bTemp, bColorToByte[color], cx);
         SRXEWriteDataBlock(bTemp, cx);       
      }
   } // filled
   else // outline
   {
      // Draw top part
      SRXESetPosition(x*3, y, cx*3, 1);
      memset(bTemp, bColorToByte[color], cx);
      SRXEWriteDataBlock(bTemp, cx);
      // Bottom
      SRXESetPosition(x*3, y+cy-1, cx*3, 1);
      memset(bTemp, bColorToByte[color], cx);
      SRXEWriteDataBlock(bTemp, cx);
      // Left
      SRXESetPosition(x*3, y, 3, cy);
      memset(bTemp, bColorToByte[color], cy);
      SRXEWriteDataBlock(bTemp, cy);
      // Right
      SRXESetPosition((x+cx-1)*3, y, 3, cy);
      memset(bTemp, bColorToByte[color], cy);
      SRXEWriteDataBlock(bTemp, cy);
   }
} /* SRXERectangle() */
//
// Draw a string of normal (8x8), small (6x8) or large (16x24) characters
// At the given col+row
//
int SRXEWriteString(int x, int y, char *szMsg, int iSize, int iFGColor, int iBGColor)
{
int i, j, iLen;
unsigned char ucTemp[8], *s;
byte fgColor0, fgColor1, fgColor2, bgColor;

    if (iFGColor > 3) iFGColor = 3;
    if (iBGColor > 3) iBGColor = 3;
    if (iFGColor == 3)
    {
      fgColor0 = 0xe0; fgColor1 = 0x1c; fgColor2 = 0x3; 
    }
    else
    {
    fgColor0 = (byte)iFGColor << 6; // first pixel 3-bit version of the color
    fgColor1 = (byte)iFGColor << 3; // second pixel
    fgColor2 = (byte)iFGColor;      // 3rd pixel
    }
    bgColor = bColorToByte[iBGColor];
        
    iLen = strlen(szMsg);
    if (iSize == FONT_LARGE || iSize == FONT_MEDIUM) // draw 12x16 or 15x16 font
  {
  int iWidth, iDelta;
    iWidth = (iSize == FONT_LARGE) ? 15 : 12;
    iDelta = (iSize == FONT_LARGE) ? 5 : 4;
    if ((iWidth*iLen) + x > 384) iLen = (384 - x)/iWidth; // can't display it all
    if (iLen < 0)return -1;
    for (i=0; i<iLen; i++)
    {
      int tx, ty;
      byte bTemp[84], bMask, bOut, bOut2, *d;
      if (iSize == FONT_LARGE)
      {
         s = (unsigned char *)&ucFont[((unsigned char)szMsg[i]-32) * 8];
         memcpy_P(ucTemp, s, 8); // copy from FLASH memory
      }
      else
      {
         s = (unsigned char *)&ucSmallFont[((unsigned char)szMsg[i]-32) * 6];
         memcpy_P(ucTemp, s, 6);
      }
       // convert from 1-bpp to 2/3-bpp
      d = bTemp;
      s = ucTemp;
      bMask = 1;
      for (ty=0; ty<8; ty++)
      {
        for (tx=0; tx<iWidth-6; tx+=3) // 3 sets of 3 pixels
        {
           bOut = bOut2 = bgColor;
           if (s[tx] & bMask)
           {
              bOut &= 0x3; // clear top 6 bits
              bOut |= fgColor0 | fgColor1; // first 2 pixels (6 bits)
           }
           if (s[tx+1] & bMask)
           {
              bOut &= 0xfc; // clear bottom 2 bits
              bOut2 &= 0x1f; // clear top 3 bits
              bOut |= fgColor2; // third pixel (2 bits)
              bOut2 |= fgColor0;
           }
           if (s[tx+2] & bMask)
           {
              bOut2 &= 0xe0; // clear lower 5 bits
              bOut2 |= fgColor1 | fgColor2; // 2nd & 3rd pixel2 of second byte
           }
           d[0] = d[iDelta] = bOut;
           if (tx != 6)
                d[1] = d[iDelta+1] = bOut2;
           d += 2;
        } // for tx
        d += 4; // skip extra line (add 4 since we incremented by 6 already)
        bMask <<= 1;
      } // for ty
      SRXESetPosition(x, y, iWidth, 16);
      SRXEWriteDataBlock(bTemp, 16*iDelta); // write character pattern
      x += iWidth;
    } // for each character
  } // large+medium
  else if (iSize == FONT_NORMAL)// draw 8x8 font
   {
    if ((9*iLen) + x > 384) iLen = (384 - x)/9; // can't display it all
    if (iLen < 0)return -1;

    for (i=0; i<iLen; i++)
    {
      int tx, ty;
      byte bTemp[24], bMask, bOut, *d;
      s = (unsigned char *)&ucFont[((unsigned char)szMsg[i]-32) * 8];
      memcpy_P(ucTemp, s, 8); // copy from FLASH memory
       // convert from 1-bpp to 2/3-bpp
      d = bTemp;
      for (ty=0; ty<8; ty++)
      {
        bMask = 1<<ty;
        for (tx=0; tx<9; tx+=3) // 3 sets of 3 pixels
        {
           bOut = bgColor;
           if (ucTemp[tx] & bMask)
           {
              bOut &= 0x1f; // clear top 3 bits
              bOut |= fgColor0; // first pixel (3 bits)
           }
           if (ucTemp[tx+1] & bMask)
           {
              bOut &= 0xe3; // clear middle 3 bits
              bOut |= fgColor1; // second pixel (3 bits)
           }
           if (ucTemp[tx+2] & bMask && tx != 6)
           {
              bOut &= 0xfc; // clear lower 2 bits
              bOut |= fgColor2; // third pixel (2 bits)
           }
           *d++ = bOut;
        } // for tx
      } // for ty
      SRXESetPosition(x, y, 9, 8);
      x += 9;
      SRXEWriteDataBlock(bTemp, 24); // write character pattern
    } 
   } // normal
   else // 6x8
   {
    if ((6*iLen) + x > 384) iLen = (384 - x)/6; // can't display it all
    if (iLen < 0)return -1;

    for (i=0; i<iLen; i++)
    {
      int tx, ty;
      byte bTemp[16], bMask, bOut, *d;
      s = (unsigned char *)&ucSmallFont[((unsigned char)szMsg[i]-32) * 6];
      memcpy_P(ucTemp, s, 6); // copy from FLASH memory
       // convert from 1-bpp to 2/3-bpp
      d = bTemp;
      for (ty=0; ty<8; ty++)
      {
        bMask = 1<<ty;
        for (tx=0; tx<6; tx+=3) // 2 sets of 3 pixels
        {
           bOut = bgColor;
           if (ucTemp[tx] & bMask)
           {
              bOut &= 0x1f; // clear top 3 bits
              bOut |= fgColor0; // first pixel (3 bits)
           }
           if (ucTemp[tx+1] & bMask)
           {
              bOut &= 0xe3; // clear middle 3 bits
              bOut |= fgColor1; // second pixel (3 bits)
           }
           if (ucTemp[tx+2] & bMask)
           {
              bOut &= 0xfc; // clear lower 2 bits
              bOut |= fgColor2; // third pixel (2 bits)
           }
           *d++ = bOut;
        } // for tx
      } // for ty
      SRXESetPosition(x, y, 6, 8);
      x += 6;
      SRXEWriteDataBlock(bTemp, 16); // write character pattern
    } 
   } // small
  return 0;
} /* SRXEWriteString() */

// Fill the frame buffer with a byte pattern
// e.g. all off (0x00) or all on (0xff)
void SRXEFill(byte ucData)
{
int y;
byte temp[128];

     SRXESetPosition(0, 0, 384, 136);
     for (y=0; y<136; y++)
     {
          memset(temp, ucData, 128); // have to do this because the bytes get overwritten
          SRXEWriteDataBlock(temp, 128); // fill with data byte
     }
} /* SRXEFill() */

//
// Scan the rows and columns and store the results in the key map
//
void SRXEScanKeyboard(void)
{
byte r, c;

  for (r=0; r<ROWS; r++)
  {
    mypinMode(rowPins[r], INPUT_PULLUP);
  }
  // save current keymap to compare for pressed/released keys
  memcpy(bOldKeyMap, bKeyMap, sizeof(bKeyMap));
  
  for (c=0; c<COLS; c++)
  {
     bKeyMap[c] = 0;
     mypinMode(colPins[c], OUTPUT);
     mydigitalWrite(colPins[c], LOW); // test this column
     for (r=0; r<ROWS; r++)
     {
        if (mydigitalRead(rowPins[r]) == LOW)
           bKeyMap[c] |= (1 << r); // set a bit for each pressed key
     } // for r
     mydigitalWrite(colPins[c], HIGH); // leave pin in high impedance state
     mypinMode(colPins[c], INPUT);
  } // for c
} /* SRXEScanKeyboard() */
//
// Return a pointer to the internal key map
// (10 bytes with 6 bits each)
//
byte *SRXEGetKeyMap(void)
{
   return bKeyMap;
}
//
// Return the current key pressed
// includes code to provide shift + sym adjusted output
//
byte SRXEGetKey(void)
{
byte bShift, bSym, *pKeys;
byte iCol, iRow;
byte bMask;
byte bKey = 0;

   SRXEScanKeyboard();
   bShift =  bKeyMap[0] & 0x08;
   bSym = bKeyMap[0] & 0x10;
   for (iCol = 0; iCol < COLS; iCol++)
   {
       bMask = 1;
       for (iRow=0; iRow < ROWS; iRow++, bMask <<= 1)
       {
          if ((bKeyMap[iCol] & bMask) == bMask && (bOldKeyMap[iCol] & bMask) == 0)
          {
             // make sure it's not shift/sym
             if (iCol == 0 && (iRow == 3 || iRow == 4)) // shift/sym, ignore
                continue;
             // valid key, adjust it and return
             pKeys = OriginalKeys;
             if (bShift) pKeys = ShiftedKeys;
             else if (bSym) pKeys = SymKeys;
             bKey = pKeys[(iRow*COLS)+iCol];
          }
       } // for iRow
   } // for iCol
   return bKey; // 0 if no keys pressed
} /* SRXEGetKey() */
