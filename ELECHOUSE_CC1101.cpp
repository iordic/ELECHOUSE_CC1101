/*
  ELECHOUSE_CC1101.cpp - CC1101 module library
  Copyright (c) 2010 Michael.
    Author: Michael, <www.elechouse.com>
    Version: November 12, 2010

  This library is designed to use CC1101/CC1100 module on Arduino platform.
  CC1101/CC1100 module is an useful wireless module.Using the functions of the 
  library, you can easily send and receive data by the CC1101/CC1100 module. 
  Just have fun!
  For the details, please refer to the datasheet of CC1100/CC1101.
----------------------------------------------------------------------------------------------------------------
cc1101 Driver for RC Switch. Mod by Little Satan. With permission to modify and publish Wilson Shen (ELECHOUSE).
----------------------------------------------------------------------------------------------------------------
*/
#include <SPI.h>
#include "ELECHOUSE_CC1101.h"
#include <Arduino.h>

/****************************************************************/
#define   WRITE_BURST       0x40            //write burst
#define   READ_SINGLE       0x80            //read single
#define   READ_BURST        0xC0            //read burst
#define   BYTES_IN_RXFIFO   0x7F            //byte number in RXfifo
/****************************************************************/
uint8_t PA_TABLE_315[8] {0x12,0x0D,0x1C,0x34,0x51,0x85,0xCB,0xC2,};             //300 - 348
uint8_t PA_TABLE_433[8] {0x12,0x0E,0x1D,0x34,0x60,0x84,0xC8,0xC0,};             //387 - 464
//                        -30  -20  -15  -10  -6    0    5    7    10   12
uint8_t PA_TABLE_868[10] {0x03,0x17,0x1D,0x26,0x37,0x50,0x86,0xCD,0xC5,0xC0,};  //779 - 899.99
//                        -30  -20  -15  -10  -6    0    5    7    10   11
uint8_t PA_TABLE_915[10] {0x03,0x0E,0x1E,0x27,0x38,0x8E,0x84,0xCC,0xC3,0xC0,};  //900 - 928


ELECHOUSE_CC1101::ELECHOUSE_CC1101(byte gdo0, byte gdo2) {
	this->gdo0 = gdo0;
	this->gdo2 = gdo2;
	this->spi = &SPI;
	this->customSpiPins = false;
  this->sck = SCK_PIN;
	this->miso = MISO_PIN;
	this->mosi = MOSI_PIN;
	this->csn = SS_PIN;
}

ELECHOUSE_CC1101::ELECHOUSE_CC1101(byte gdo0, byte gdo2, int spi) {
	this->gdo0 = gdo0;
	this->gdo2 = gdo2;
	this->spi = new SPIClass(spi);
	this->customSpiPins = false;
  this->sck = SCK_PIN;
	this->miso = MISO_PIN;
	this->mosi = MOSI_PIN;
	this->csn = SS_PIN;
}

ELECHOUSE_CC1101::ELECHOUSE_CC1101(byte gdo0, byte gdo2, byte sck, byte miso, byte mosi, byte csn, int spi) {
	this->gdo0 = gdo0;
	this->gdo2 = gdo2;
	this->spi = new SPIClass(spi);
	this->sck = sck;
	this->miso = miso;
	this->mosi = mosi;
	this->csn = csn;
	this->customSpiPins = true;

  pinMode(sck, OUTPUT);
  pinMode(mosi, OUTPUT);
  pinMode(miso, INPUT);
  pinMode(csn, OUTPUT);  
}

void ELECHOUSE_CC1101::spiStart() {
	if (this->customSpiPins) {
		this->spi->begin(this->sck, this->miso, this->mosi, this->csn);
	} else {
		this->spi->begin();
	}
}

void ELECHOUSE_CC1101::spiEnd() {
  this->spi->endTransaction();
	this->spi->end();
}
/****************************************************************
*FUNCTION NAME:Reset
*FUNCTION     :CC1101 reset //details refer datasheet of CC1101/CC1100//
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::reset() {
	digitalWrite(this->csn, LOW);
	delay(1);
	digitalWrite(this->csn, HIGH);
	delay(1);
	digitalWrite(this->csn, LOW);
	while(digitalRead(this->miso));
  this->spi->transfer(CC1101_SRES);
  while(digitalRead(this->miso));
	digitalWrite(this->csn, HIGH);
}
/****************************************************************
*FUNCTION NAME:Init
*FUNCTION     :CC1101 initialization
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::init() {
  spiStart();
  digitalWrite(this->csn, HIGH);
  digitalWrite(this->sck, HIGH);
  digitalWrite(this->mosi, LOW);
  reset();                        //CC1101 reset
  regConfigSettings();            //CC1101 register config
  spiEnd();
}
/****************************************************************
*FUNCTION NAME:spiWriteReg
*FUNCTION     :CC1101 write data to register
*INPUT        :addr: register address; value: register value
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::spiWriteReg(byte addr, byte value) {
  spiStart();
  digitalWrite(this->csn, LOW);
  while(digitalRead(this->miso));
  this->spi->transfer(addr);
  this->spi->transfer(value); 
  digitalWrite(this->csn, HIGH);
  spiEnd();
}
/****************************************************************
*FUNCTION NAME:spiWriteBurstReg
*FUNCTION     :CC1101 write burst data to register
*INPUT        :addr: register address; buffer:register value array; num:number to write
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::spiWriteBurstReg(byte addr, byte *buffer, byte num) {
  byte i, temp;
  spiStart();
  temp = addr | WRITE_BURST;
  digitalWrite(this->csn, LOW);
  while(digitalRead(this->miso));
  this->spi->transfer(temp);
  for (i = 0; i < num; i++) {
    this->spi->transfer(buffer[i]);
  }
  digitalWrite(this->csn, HIGH);
  spiEnd();
}
/****************************************************************
*FUNCTION NAME:spiStrobe
*FUNCTION     :CC1101 Strobe
*INPUT        :strobe: command; //refer define in CC1101.h//
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::spiStrobe(byte strobe) {
  spiStart();
  digitalWrite(this->csn, LOW);
  while(digitalRead(this->miso));
  this->spi->transfer(strobe);
  digitalWrite(this->csn, HIGH);
  spiEnd();
}
/****************************************************************
*FUNCTION NAME:spiReadReg
*FUNCTION     :CC1101 read data from register
*INPUT        :addr: register address
*OUTPUT       :register value
****************************************************************/
byte ELECHOUSE_CC1101::spiReadReg(byte addr) {
  byte temp, value;
  spiStart();
  temp = addr | READ_SINGLE;
  digitalWrite(this->csn, LOW);
  while(digitalRead(this->miso));
  this->spi->transfer(temp);
  value = this->spi->transfer(0);
  digitalWrite(this->csn, HIGH);
  spiEnd();
  return value;
}

/****************************************************************
*FUNCTION NAME:spiReadBurstReg
*FUNCTION     :CC1101 read burst data from register
*INPUT        :addr: register address; buffer:array to store register value; num: number to read
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::spiReadBurstReg(byte addr, byte *buffer, byte num) {
  byte i,temp;
  spiStart();
  temp = addr | READ_BURST;
  digitalWrite(this->csn, LOW);
  while(digitalRead(this->miso));
  this->spi->transfer(temp);
  for (i=0; i<num; i++) {
    buffer[i] = this->spi->transfer(0);
  }
  digitalWrite(this->csn, HIGH);
  spiEnd();
}

/****************************************************************
*FUNCTION NAME:spiReadStatus
*FUNCTION     :CC1101 read status register
*INPUT        :addr: register address
*OUTPUT       :status value
****************************************************************/
byte ELECHOUSE_CC1101::spiReadStatus(byte addr) {
  byte value,temp;
  spiStart();
  temp = addr | READ_BURST;
  digitalWrite(this->csn, LOW);
  while(digitalRead(this->miso));
  this->spi->transfer(temp);
  value = this->spi->transfer(0);
  digitalWrite(this->csn, HIGH);
  spiEnd();
  return value;
}
/****************************************************************
*FUNCTION NAME:CCMode
*FUNCTION     :Format of RX and TX data
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setCCMode(bool s) {
  this->ccMode = s;
  if (this->ccMode) {
    spiWriteReg(CC1101_IOCFG2,      0x0B);
    spiWriteReg(CC1101_IOCFG0,      0x06);
    spiWriteReg(CC1101_PKTCTRL0,    0x05);
    spiWriteReg(CC1101_MDMCFG3,     0xF8);
    spiWriteReg(CC1101_MDMCFG4,11 + this->m4RxBw);
  } else {
    spiWriteReg(CC1101_IOCFG2,      0x0D);
    spiWriteReg(CC1101_IOCFG0,      0x0D);
    spiWriteReg(CC1101_PKTCTRL0,    0x32);
    spiWriteReg(CC1101_MDMCFG3,     0x93);
    spiWriteReg(CC1101_MDMCFG4, 7 + this->m4RxBw);
  }
  setModulation(this->modulation);
}
/****************************************************************
*FUNCTION NAME:Modulation
*FUNCTION     :set CC1101 Modulation 
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setModulation(byte m) {
  if (m > 4) m = 4;
  this->modulation = m;
  splitMDMCFG2();
  switch (m) {
    case 0: this->m2MODFM = 0x00; this->frend0 = 0x10; break; // 2-FSK
    case 1: this->m2MODFM = 0x10; this->frend0 = 0x10; break; // GFSK
    case 2: this->m2MODFM = 0x30; this->frend0 = 0x11; break; // ASK
    case 3: this->m2MODFM = 0x40; this->frend0 = 0x10; break; // 4-FSK
    case 4: this->m2MODFM = 0x70; this->frend0 = 0x10; break; // MSK
  }
  spiWriteReg(CC1101_MDMCFG2, this->m2DCOFF + this->m2MODFM + this->m2MANCH + this->m2SYNCM);
  spiWriteReg(CC1101_FREND0, this->frend0);
  setPA(this->pa);
}
/****************************************************************
*FUNCTION NAME:PA Power
*FUNCTION     :set CC1101 PA Power 
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setPA(int p) {
  int a;
  this->pa = p;

  if (this->frequency >= 300 && this->frequency <= 348) {
  if (this->pa <= -30){a = PA_TABLE_315[0];}
    else if (this->pa > -30 && this->pa <= -20){a = PA_TABLE_315[1];}
    else if (this->pa > -20 && this->pa <= -15){a = PA_TABLE_315[2];}
    else if (this->pa > -15 && this->pa <= -10){a = PA_TABLE_315[3];}
    else if (this->pa > -10 && this->pa <= 0){a = PA_TABLE_315[4];}
    else if (this->pa > 0 && this->pa <= 5){a = PA_TABLE_315[5];}
    else if (this->pa > 5 && this->pa <= 7){a = PA_TABLE_315[6];}
    else if (this->pa > 7){a = PA_TABLE_315[7];}
    this->lastPa = 1;
  } else if (this->frequency >= 378 && this->frequency <= 464) {
    if (this->pa <= -30){a = PA_TABLE_433[0];}
    else if (this->pa > -30 && this->pa <= -20){a = PA_TABLE_433[1];}
    else if (this->pa > -20 && this->pa <= -15){a = PA_TABLE_433[2];}
    else if (this->pa > -15 && this->pa <= -10){a = PA_TABLE_433[3];}
    else if (this->pa > -10 && this->pa <= 0){a = PA_TABLE_433[4];}
    else if (this->pa > 0 && this->pa <= 5){a = PA_TABLE_433[5];}
    else if (this->pa > 5 && this->pa <= 7){a = PA_TABLE_433[6];}
    else if (this->pa > 7){a = PA_TABLE_433[7];}
    this->lastPa = 2;
  } else if (this->frequency >= 779 && this->frequency <= 899.99) {
    if (this->pa <= -30){a = PA_TABLE_868[0];}
    else if (this->pa > -30 && this->pa <= -20){a = PA_TABLE_868[1];}
    else if (this->pa > -20 && this->pa <= -15){a = PA_TABLE_868[2];}
    else if (this->pa > -15 && this->pa <= -10){a = PA_TABLE_868[3];}
    else if (this->pa > -10 && this->pa <= -6){a = PA_TABLE_868[4];}
    else if (this->pa > -6 && this->pa <= 0){a = PA_TABLE_868[5];}
    else if (this->pa > 0 && this->pa <= 5){a = PA_TABLE_868[6];}
    else if (this->pa > 5 && this->pa <= 7){a = PA_TABLE_868[7];}
    else if (this->pa > 7 && this->pa <= 10){a = PA_TABLE_868[8];}
    else if (this->pa > 10){a = PA_TABLE_868[9];}
    this->lastPa = 3;
  } else if (this->frequency >= 900 && this->frequency <= 928) {
    if (this->pa <= -30){a = PA_TABLE_915[0];}
    else if (this->pa > -30 && this->pa <= -20){a = PA_TABLE_915[1];}
    else if (this->pa > -20 && this->pa <= -15){a = PA_TABLE_915[2];}
    else if (this->pa > -15 && this->pa <= -10){a = PA_TABLE_915[3];}
    else if (this->pa > -10 && this->pa <= -6){a = PA_TABLE_915[4];}
    else if (this->pa > -6 && this->pa <= 0){a = PA_TABLE_915[5];}
    else if (this->pa > 0 && this->pa <= 5){a = PA_TABLE_915[6];}
    else if (this->pa > 5 && this->pa <= 7){a = PA_TABLE_915[7];}
    else if (this->pa > 7 && this->pa <= 10){a = PA_TABLE_915[8];}
    else if (this->pa > 10){a = PA_TABLE_915[9];}
    this->lastPa = 4;
  }
  if (modulation == 2) {
    this->paTable[0] = 0;  
    this->paTable[1] = a;
  } else {
    this->paTable[0] = a;  
    this->paTable[1] = 0; 
  }
  spiWriteBurstReg(CC1101_PATABLE, this->paTable, 8);
}
/****************************************************************
*FUNCTION NAME:Frequency Calculator
*FUNCTION     :Calculate the basic frequency.
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setFrequency(float mhz) {
  byte freq2 = 0;
  byte freq1 = 0;
  byte freq0 = 0;

  this->frequency = mhz;

  for (bool i = 0; i==0;) {
    if (mhz >= 26) {
      mhz-=26;
      freq2+=1;
    } else if (mhz >= 0.1015625) {
      mhz-=0.1015625;
      freq1 += 1;
    } else if (mhz >= 0.00039675) {
      mhz-=0.00039675;
      freq0+=1;
    } else {
      i=1;
    }
  }
  if (freq0 > 255) {freq1+=1; freq0-=256;}

  spiWriteReg(CC1101_FREQ2, freq2);
  spiWriteReg(CC1101_FREQ1, freq1);
  spiWriteReg(CC1101_FREQ0, freq0);

  calibrate();
}
/****************************************************************
*FUNCTION NAME:Calibrate
*FUNCTION     :Calibrate frequency
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::calibrate() {
  if (this->frequency >= 300 && this->frequency <= 348) {
    spiWriteReg(CC1101_FSCTRL0, map(this->frequency, 300, 348, this->clb1[0], this->clb1[1]));
    if (this->frequency < 322.88) { 
      spiWriteReg(CC1101_TEST0, 0x0B);
    } else {
      spiWriteReg(CC1101_TEST0, 0x09);
      int s = spiReadStatus(CC1101_FSCAL2);
      if (s < 32) spiWriteReg(CC1101_FSCAL2, s+32);
      if (this->lastPa != 1) setPA(this->pa);
    }
  } else if (this->frequency >= 378 && this->frequency <= 464) {
    spiWriteReg(CC1101_FSCTRL0, map(this->frequency, 378, 464, this->clb2[0], this->clb2[1]));
    if (this->frequency < 430.5) {
      spiWriteReg(CC1101_TEST0,0x0B);
    } else {
      spiWriteReg(CC1101_TEST0,0x09);
      int s = spiReadStatus(CC1101_FSCAL2);
      if (s < 32) spiWriteReg(CC1101_FSCAL2, s+32);
      if (this->lastPa != 2) setPA(this->pa);
    }
  } else if (this->frequency >= 779 && this->frequency <= 899.99) {
    spiWriteReg(CC1101_FSCTRL0, map(this->frequency, 779, 899, this->clb3[0], this->clb3[1]));
    if (this->frequency < 861) {
      spiWriteReg(CC1101_TEST0,0x0B);
    } else {
      spiWriteReg(CC1101_TEST0,0x09);
      int s = spiReadStatus(CC1101_FSCAL2);
      if (s < 32) spiWriteReg(CC1101_FSCAL2, s+32);
      if (this->lastPa != 3) setPA(this->pa);
    }
  } else if (this->frequency >= 900 && this->frequency <= 928) {
    spiWriteReg(CC1101_FSCTRL0, map(this->frequency, 900, 928, this->clb4[0], this->clb4[1]));
    spiWriteReg(CC1101_TEST0,0x09);
    int s = spiReadStatus(CC1101_FSCAL2);
    if (s < 32) spiWriteReg(CC1101_FSCAL2, s+32);
    if (this->lastPa != 4) setPA(this->pa);
  }
}
/****************************************************************
*FUNCTION NAME:Calibration offset
*FUNCTION     :Set calibration offset
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setClb(byte b, byte s, byte e) {
  if (b == 1) {
    this->clb1[0] = s;
    this->clb1[1] = e;  
  } else if (b == 2) {
    this->clb2[0] = s;
    this->clb2[1] = e;  
  } else if (b == 3) {
    this->clb3[0] = s;
    this->clb3[1] = e;  
  } else if (b == 4) {
    this->clb4[0] = s;
    this->clb4[1] = e;  
  }
}
/****************************************************************
*FUNCTION NAME:getCC1101
*FUNCTION     :Test Spi connection and return 1 when true.
*INPUT        :none
*OUTPUT       :none
****************************************************************/
bool ELECHOUSE_CC1101::getCC1101() {
  //setSpi();
  if (spiReadStatus(0x31) > 0) {
    return 1;
  } else {
    return 0;
  }
}
/****************************************************************
*FUNCTION NAME:getFrequency
*FUNCTION     :Return the frequency.
*INPUT        :none
*OUTPUT       :none
****************************************************************/
float ELECHOUSE_CC1101::getFrequency() {
  return this->frequency;
}
/****************************************************************
*FUNCTION NAME:getMode
*FUNCTION     :Return the Mode. Sidle = 0, TX = 1, Rx = 2.
*INPUT        :none
*OUTPUT       :none
****************************************************************/
byte ELECHOUSE_CC1101::getMode() {
  return this->trxstate;
}
/****************************************************************
*FUNCTION NAME:getModulation
*FUNCTION     :Return the configured modulation. 2-FSK = 0, GFSK = 1, ASK = 2, 4-FSK = 3, MSK = 4.
*INPUT        :none
*OUTPUT       :none
****************************************************************/
byte ELECHOUSE_CC1101::getModulation() {
  return this->modulation;
}
/****************************************************************
*FUNCTION NAME:Set Sync_Word
*FUNCTION     :Sync Word
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setSyncWord(byte sh, byte sl) {
  spiWriteReg(CC1101_SYNC1, sh);
  spiWriteReg(CC1101_SYNC0, sl);
  }
/****************************************************************
*FUNCTION NAME:Set ADDR
*FUNCTION     :Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setAddr(byte v) {
  spiWriteReg(CC1101_ADDR, v);
}
/****************************************************************
*FUNCTION NAME:Set PQT
*FUNCTION     :Preamble quality estimator threshold
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setPQT(byte v) {
  splitPKTCTRL1();
  this->pc1PQT = 0;
  if (v > 7) v = 7;
  this->pc1PQT = v * 32;
  spiWriteReg(CC1101_PKTCTRL1, this->pc1PQT + this->pc1CRC_AF + this->pc1APP_ST + this->pc1ADRCHK);
}
/****************************************************************
*FUNCTION NAME:Set CRC_AUTOFLUSH
*FUNCTION     :Enable automatic flush of RX FIFO when CRC is not OK
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setCRC_AF(bool v) {
  splitPKTCTRL1();
  this->pc1CRC_AF = 0;
  if (v == 1) this->pc1CRC_AF = 8;
  spiWriteReg(CC1101_PKTCTRL1, this->pc1PQT + this->pc1CRC_AF + this->pc1APP_ST + this->pc1ADRCHK);
}
/****************************************************************
*FUNCTION NAME:Set APPEND_STATUS
*FUNCTION     :When enabled, two status bytes will be appended to the payload of the packet
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setAppendStatus(bool v) {
  splitPKTCTRL1();
  this->pc1APP_ST = 0;
  if (v == 1) this->pc1APP_ST = 4;
  spiWriteReg(CC1101_PKTCTRL1, this->pc1PQT + this->pc1CRC_AF + this->pc1APP_ST + this->pc1ADRCHK);
}
/****************************************************************
*FUNCTION NAME:Set ADR_CHK
*FUNCTION     :Controls address check configuration of received packages
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setAdrChk(byte v) {
  splitPKTCTRL1();
  pc1ADRCHK = 0;
  if (v > 3) v = 3;
  this->pc1ADRCHK = v;
  spiWriteReg(CC1101_PKTCTRL1, this->pc1PQT + this->pc1CRC_AF + this->pc1APP_ST + this->pc1ADRCHK);
}
/****************************************************************
*FUNCTION NAME:Set WHITE_DATA
*FUNCTION     :Turn data whitening on / off.
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setWhiteData(bool v) {
  splitPKTCTRL0();
  this->pc0WDATA = 0;
  if (v == 1) this->pc0WDATA = 64;
  spiWriteReg(CC1101_PKTCTRL0, this->pc0WDATA + this->pc0PktForm + this->pc0CRC_EN + this->pc0LenConf);
}
/****************************************************************
*FUNCTION NAME:Set PKT_FORMAT
*FUNCTION     :Format of RX and TX data
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setPktFormat(byte v) {
  splitPKTCTRL0();
  this->pc0PktForm = 0;
  if (v > 3) v = 3;
  this->pc0PktForm = v * 16;
  spiWriteReg(CC1101_PKTCTRL0, this->pc0WDATA + this->pc0PktForm + this->pc0CRC_EN + this->pc0LenConf);
}
/****************************************************************
*FUNCTION NAME:Set CRC
*FUNCTION     :CRC calculation in TX and CRC check in RX
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setCrc(bool v) {
  splitPKTCTRL0();
  this->pc0CRC_EN = 0;
  if (v==1) this->pc0CRC_EN=4;
  spiWriteReg(CC1101_PKTCTRL0, this->pc0WDATA + this->pc0PktForm + this->pc0CRC_EN + this->pc0LenConf);
}
/****************************************************************
*FUNCTION NAME:Set LENGTH_CONFIG
*FUNCTION     :Configure the packet length
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setLengthConfig(byte v) {
  splitPKTCTRL0();
  this->pc0LenConf = 0;
  if (v > 3) v = 3;
  this->pc0LenConf = v;
  spiWriteReg(CC1101_PKTCTRL0, this->pc0WDATA + this->pc0PktForm + this->pc0CRC_EN + this->pc0LenConf);
}
/****************************************************************
*FUNCTION NAME:Set PACKET_LENGTH
*FUNCTION     :Indicates the packet length
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setPacketLength(byte v) {
  spiWriteReg(CC1101_PKTLEN, v);
}
/****************************************************************
*FUNCTION NAME:Set DCFILT_OFF
*FUNCTION     :Disable digital DC blocking filter before demodulator
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setDcFilterOff(bool v) {
  splitMDMCFG2();
  this->m2DCOFF = 0;
  if (v == 1) this->m2DCOFF = 128;
  spiWriteReg(CC1101_MDMCFG2, this->m2DCOFF + this->m2MODFM + this->m2MANCH + this->m2SYNCM);
}
/****************************************************************
*FUNCTION NAME:Set MANCHESTER
*FUNCTION     :Enables Manchester encoding/decoding
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setManchester(bool v) {
  splitMDMCFG2();
  this->m2MANCH = 0;
  if (v == 1) this->m2MANCH = 8;
  spiWriteReg(CC1101_MDMCFG2, this->m2DCOFF + this->m2MODFM + this->m2MANCH + this->m2SYNCM);
}
/****************************************************************
*FUNCTION NAME:Set SYNC_MODE
*FUNCTION     :Combined sync-word qualifier mode
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setSyncMode(byte v) {
  splitMDMCFG2();
  this->m2SYNCM = 0;
  if (v > 7) v = 7;
  this->m2SYNCM = v;
  spiWriteReg(CC1101_MDMCFG2, this->m2DCOFF + this->m2MODFM + this->m2MANCH + this->m2SYNCM);
}
/****************************************************************
*FUNCTION NAME:Set FEC
*FUNCTION     :Enable Forward Error Correction (FEC)
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setFEC(bool v) {
  splitMDMCFG1();
  this->m1FEC = 0;
  if (v == 1) this->m1FEC = 128;
  spiWriteReg(CC1101_MDMCFG1, this->m1FEC + this->m1PRE + this->m1CHSP);
}
/****************************************************************
*FUNCTION NAME:Set PRE
*FUNCTION     :Sets the minimum number of preamble bytes to be transmitted.
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setPRE(byte v) {
  splitMDMCFG1();
  this->m1PRE = 0;
  if (v > 7) v = 7;
  this->m1PRE = v * 16;
  spiWriteReg(CC1101_MDMCFG1, this->m1FEC + this->m1PRE + this->m1CHSP);
}
/****************************************************************
*FUNCTION NAME:Set Channel
*FUNCTION     :none
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setChannel(byte ch) {
  this->channel = ch;
  spiWriteReg(CC1101_CHANNR,   this->channel);
  }
/****************************************************************
*FUNCTION NAME:Set Channel spacing
*FUNCTION     :none
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setChsp(float f) {
  splitMDMCFG1();
  byte MDMCFG0 = 0;
  this->m1CHSP = 0;
  if (f > 405.456543) f = 405.456543;
  if (f < 25.390625) f = 25.390625;
  for (int i = 0; i < 5; i++) {
    if (f <= 50.682068) {
      f -= 25.390625;
      f /= 0.0991825;
      MDMCFG0 = f;
      float s1 = (f - MDMCFG0) * 10;
      if (s1 >= 5) MDMCFG0++;
      i = 5;
    } else {
      this->m1CHSP++;
      f/=2;
    }
  }
  spiWriteReg(19, this->m1CHSP + this->m1FEC + this->m1PRE);
  spiWriteReg(20, MDMCFG0);
}
/****************************************************************
*FUNCTION NAME:Set Receive bandwidth
*FUNCTION     :none
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setRxBW(float f) {
  splitMDMCFG4();
  int s1 = 3;
  int s2 = 3;
  for (int i = 0; i < 3; i++) {
    if (f > 101.5625) {
      f /= 2;
      s1--;
    } else {
      i = 3;
    }
  }
  for (int i = 0; i < 3; i++) {
    if (f > 58.1) {
      f /= 1.25;
      s2--;
    } else {
      i=3;
    }
  }
  s1 *= 64;
  s2 *= 16;
  this->m4RxBw = s1 + s2;
  spiWriteReg(16, this->m4RxBw + this->m4DaRa);
}
/****************************************************************
*FUNCTION NAME:Set Data Rate
*FUNCTION     :none
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setDRate(float d) {
  float c = d;
  byte MDMCFG3 = 0;  
  splitMDMCFG4();
  if (c > 1621.83) c = 1621.83;
  if (c < 0.0247955) c = 0.0247955;
  this->m4DaRa = 0;
  for (int i = 0; i < 20; i++) {
    if (c <= 0.0494942) {
      c = c - 0.0247955;
      c = c / 0.00009685;
      MDMCFG3 = c;
      float s1 = (c - MDMCFG3) * 10;
      if (s1 >= 5) MDMCFG3++;
      i = 20;
    } else {
      this->m4DaRa++;
      c = c/2;
    }
  }
  spiWriteReg(16,  this->m4RxBw + this->m4DaRa);
  spiWriteReg(17,  MDMCFG3);
}
/****************************************************************
*FUNCTION NAME:Set Devitation
*FUNCTION     :none
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setDeviation(float d) {
  float f = 1.586914;
  float v = 0.19836425;
  int c = 0;
  if (d > 380.859375) d = 380.859375;
  if (d < 1.586914) d = 1.586914;
  for (int i = 0; i < 255; i++) {
    f+=v;
    if (c == 7){
      v *= 2;
      c = -1;
      i += 8;
    }
    if (f >= d) {
      c = i;
      i = 255;
    }
    c++;
  }
  spiWriteReg(21, c);
}
/****************************************************************
*FUNCTION NAME:Split PKTCTRL0
*FUNCTION     :none
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::splitPKTCTRL1() {
  int calc = spiReadStatus(7);
  this->pc1PQT = 0;
  this->pc1CRC_AF = 0;
  this->pc1APP_ST = 0;
  this->pc1ADRCHK = 0;
  for (bool i = 0; i == 0;) {
    if (calc >= 32) {
      calc -= 32;
      this->pc1PQT += 32;
    } else if (calc >= 8) {
      calc -= 8;
      this->pc1CRC_AF += 8;
    } else if (calc >= 4) {
      calc -= 4;
      this->pc1APP_ST += 4;
    } else {
      this->pc1ADRCHK = calc;
      i = 1;
    }
  }
}
/****************************************************************
*FUNCTION NAME:Split PKTCTRL0
*FUNCTION     :none
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::splitPKTCTRL0() {
  int calc = spiReadStatus(8);
  this->pc0WDATA = 0;
  this->pc0PktForm = 0;
  this->pc0CRC_EN = 0;
  this->pc0LenConf = 0;
  for (bool i = 0; i == 0;) {
    if (calc >= 64){
      calc -= 64;
      this->pc0WDATA += 64;
    } else if (calc >= 16) {
      calc -= 16;
      this->pc0PktForm += 16;
    } else if (calc >= 4) {
      calc -= 4;
      this->pc0CRC_EN += 4;
    } else {
      this->pc0LenConf = calc;
      i = 1;
    }
  }
}
/****************************************************************
*FUNCTION NAME:Split MDMCFG1
*FUNCTION     :none
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::splitMDMCFG1() {
  int calc = spiReadStatus(19);
  int s2 = 0;  
  this->m1FEC = 0;
  this->m1PRE = 0;
  this->m1CHSP = 0;
  for (bool i = 0; i == 0;) {
    if (calc >= 128) {
      calc -= 128;
      this->m1FEC += 128;
    } else if (calc >= 16) {
      calc -= 16;
      this->m1PRE += 16;
    } else {
      this->m1CHSP = calc;
      i=1;
    }
  }
}
/****************************************************************
*FUNCTION NAME:Split MDMCFG2
*FUNCTION     :none
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::splitMDMCFG2() {
  int calc = spiReadStatus(18);
  this->m2DCOFF = 0;
  this->m2MODFM = 0;
  this->m2MANCH = 0;
  this->m2SYNCM = 0;
  for (bool i = 0; i == 0;) {
    if (calc >= 128) {
      calc -= 128;
      this->m2DCOFF += 128;
    } else if (calc >= 16) {
      calc -= 16;
      this->m2MODFM += 16;
    } else if (calc >= 8) {
      calc -= 8;
      this->m2MANCH += 8;
    } else {
      this->m2SYNCM = calc;
      i = 1;
    }
  }
}
/****************************************************************
*FUNCTION NAME:Split MDMCFG4
*FUNCTION     :none
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::splitMDMCFG4() {
  int calc = spiReadStatus(16);
  this->m4RxBw = 0;
  this->m4DaRa = 0;
  for (bool i = 0; i == 0;) {
    if (calc >= 64) {
      calc -= 64;
      this->m4RxBw += 64;
    } else if (calc >= 16) {
      calc -= 16;
      this->m4RxBw += 16;
    } else {
      this->m4DaRa = calc;
      i = 1;
    }
  }
}
/****************************************************************
*FUNCTION NAME:regConfigSettings
*FUNCTION     :CC1101 register config //details refer datasheet of CC1101/CC1100//
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::regConfigSettings() {   
  spiWriteReg(CC1101_FSCTRL1,  0x06);

  setCCMode(this->ccMode);
  setFrequency(this->frequency);

  spiWriteReg(CC1101_MDMCFG1,  0x02);
  spiWriteReg(CC1101_MDMCFG0,  0xF8);
  spiWriteReg(CC1101_CHANNR,   this->channel);
  spiWriteReg(CC1101_DEVIATN,  0x47);
  spiWriteReg(CC1101_FREND1,   0x56);
  spiWriteReg(CC1101_MCSM0 ,   0x18);
  spiWriteReg(CC1101_FOCCFG,   0x16);
  spiWriteReg(CC1101_BSCFG,    0x1C);
  spiWriteReg(CC1101_AGCCTRL2, 0xC7);
  spiWriteReg(CC1101_AGCCTRL1, 0x00);
  spiWriteReg(CC1101_AGCCTRL0, 0xB2);
  spiWriteReg(CC1101_FSCAL3,   0xE9);
  spiWriteReg(CC1101_FSCAL2,   0x2A);
  spiWriteReg(CC1101_FSCAL1,   0x00);
  spiWriteReg(CC1101_FSCAL0,   0x1F);
  spiWriteReg(CC1101_FSTEST,   0x59);
  spiWriteReg(CC1101_TEST2,    0x81);
  spiWriteReg(CC1101_TEST1,    0x35);
  spiWriteReg(CC1101_TEST0,    0x09);
  spiWriteReg(CC1101_PKTCTRL1, 0x04);
  spiWriteReg(CC1101_ADDR,     0x00);
  spiWriteReg(CC1101_PKTLEN,   0x00);
}
/****************************************************************
*FUNCTION NAME:setTx
*FUNCTION     :set CC1101 send data
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setTx() {
  spiStrobe(CC1101_SIDLE);
  spiStrobe(CC1101_STX);        //start send
  this->trxstate = 1;
}
/****************************************************************
*FUNCTION NAME:setRx
*FUNCTION     :set CC1101 to receive state
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setRx() {
  spiStrobe(CC1101_SIDLE);
  spiStrobe(CC1101_SRX);        //start receive
  this->trxstate = 2;
}
/****************************************************************
*FUNCTION NAME:setTx
*FUNCTION     :set CC1101 send data and change frequency
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setTx(float mhz) {
  spiStrobe(CC1101_SIDLE);
  setFrequency(mhz);
  spiStrobe(CC1101_STX);        //start send
  this->trxstate = 1;
}
/****************************************************************
*FUNCTION NAME:setRx
*FUNCTION     :set CC1101 to receive state and change frequency
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setRx(float mhz) {
  spiStrobe(CC1101_SIDLE);
  setFrequency(mhz);
  spiStrobe(CC1101_SRX);        //start receive
  this->trxstate = 2;
}
/****************************************************************
*FUNCTION NAME:RSSI Level
*FUNCTION     :Calculating the RSSI Level
*INPUT        :none
*OUTPUT       :none
****************************************************************/
int ELECHOUSE_CC1101::getRssi() {
  int rssi;
  rssi = spiReadStatus(CC1101_RSSI);
  if (rssi >= 128) rssi = (rssi - 256) / 2 - 74;
  else rssi = (rssi / 2) - 74;
  return rssi;
}
/****************************************************************
*FUNCTION NAME:LQI Level
*FUNCTION     :get Lqi state
*INPUT        :none
*OUTPUT       :none
****************************************************************/
byte ELECHOUSE_CC1101::getLqi() {
  byte lqi;
  lqi = spiReadStatus(CC1101_LQI);
  return lqi;
}
/****************************************************************
*FUNCTION NAME:SetSres
*FUNCTION     :Reset CC1101
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setSres() {
  spiStrobe(CC1101_SRES);
  this->trxstate = 0;
}
/****************************************************************
*FUNCTION NAME:setSidle
*FUNCTION     :set Rx / TX Off
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::setSidle() {
  spiStrobe(CC1101_SIDLE);
  this->trxstate = 0;
}
/****************************************************************
*FUNCTION NAME:goSleep
*FUNCTION     :set cc1101 Sleep on
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::goSleep() {
  this->trxstate = 0;
  spiStrobe(0x36);//Exit RX / TX, turn off frequency synthesizer and exit
  spiStrobe(0x39);//Enter power down mode when CSn goes high.
}
/****************************************************************
*FUNCTION NAME:Char direct sendData
*FUNCTION     :use CC1101 send data
*INPUT        :txBuffer: data array to send; size: number of data to send, no more than 61
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::sendData(char *txchar) {
  int len = strlen(txchar);
  byte chartobyte[len];
  for (int i = 0; i < len; i++) chartobyte[i] = txchar[i];
  sendData(chartobyte, len);
}
/****************************************************************
*FUNCTION NAME:sendData
*FUNCTION     :use CC1101 send data
*INPUT        :txBuffer: data array to send; size: number of data to send, no more than 61
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::sendData(byte *txBuffer, byte size) {
  spiWriteReg(CC1101_TXFIFO, size);
  spiWriteBurstReg(CC1101_TXFIFO, txBuffer, size);  // write data to send
  spiStrobe(CC1101_SIDLE);
  spiStrobe(CC1101_STX);                            // start send
  while (!digitalRead(this->gdo0));                 // Wait for GDO0 to be set -> sync transmitted  
  while (digitalRead(this->gdo0));                  // Wait for GDO0 to be cleared -> end of packet
  spiStrobe(CC1101_SFTX);                           // flush TXfifo
  this->trxstate = 1;
}
/****************************************************************
*FUNCTION NAME:Char direct sendData
*FUNCTION     :use CC1101 send data without GDO
*INPUT        :txBuffer: data array to send; size: number of data to send, no more than 61
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::sendData(char *txchar, int t) {
  int len = strlen(txchar);
  byte chartobyte[len];
  for (int i = 0; i<len; i++) chartobyte[i] = txchar[i];
  sendData(chartobyte, len, t);
}
/****************************************************************
*FUNCTION NAME:sendData
*FUNCTION     :use CC1101 send data without GDO
*INPUT        :txBuffer: data array to send; size: number of data to send, no more than 61
*OUTPUT       :none
****************************************************************/
void ELECHOUSE_CC1101::sendData(byte *txBuffer, byte size, int t) {
  spiWriteReg(CC1101_TXFIFO, size);
  spiWriteBurstReg(CC1101_TXFIFO, txBuffer, size);  // write data to send
  spiStrobe(CC1101_SIDLE);
  spiStrobe(CC1101_STX);                            // start send
  delay(t);
  spiStrobe(CC1101_SFTX);                           // flush TXfifo
  this->trxstate = 1;
}
/****************************************************************
*FUNCTION NAME:Check CRC
*FUNCTION     :none
*INPUT        :none
*OUTPUT       :none
****************************************************************/
bool ELECHOUSE_CC1101::checkCRC() {
  byte lqi = spiReadStatus(CC1101_LQI);
  bool crc_ok = bitRead(lqi, 7);
  if (crc_ok == 1) {
    return 1;
  } else {
    spiStrobe(CC1101_SFRX);
    spiStrobe(CC1101_SRX);
    return 0;
  }
}
/****************************************************************
*FUNCTION NAME:checkRxFifo
*FUNCTION     :check receive data or not
*INPUT        :none
*OUTPUT       :flag: 0 no data; 1 receive data 
****************************************************************/
bool ELECHOUSE_CC1101::checkRxFifo(int t) {
  if(this->trxstate != 2) setRx();
  if(spiReadStatus(CC1101_RXBYTES) & BYTES_IN_RXFIFO) {
    delay(t);
    return 1;
  } else {
    return 0;
  }
}
/****************************************************************
*FUNCTION NAME:checkReceiveFlag
*FUNCTION     :check receive data or not
*INPUT        :none
*OUTPUT       :flag: 0 no data; 1 receive data 
****************************************************************/
byte ELECHOUSE_CC1101::checkReceiveFlag() {
  if(this->trxstate != 2) setRx();
	if(digitalRead(this->gdo0)) {
		while (digitalRead(this->gdo0));
		return 1;
	}	else {
		return 0;
	}
}
/****************************************************************
*FUNCTION NAME:receiveData
*FUNCTION     :read data received from RXfifo
*INPUT        :rxBuffer: buffer to store data
*OUTPUT       :size of data received
****************************************************************/
byte ELECHOUSE_CC1101::receiveData(byte *rxBuffer) {
	byte size;
	byte status[2];

	if(spiReadStatus(CC1101_RXBYTES) & BYTES_IN_RXFIFO) {
		size = spiReadReg(CC1101_RXFIFO);
		spiReadBurstReg(CC1101_RXFIFO, rxBuffer, size);
		spiReadBurstReg(CC1101_RXFIFO, status, 2);
		spiStrobe(CC1101_SFRX);
    spiStrobe(CC1101_SRX);
		return size;
	}	else {
		spiStrobe(CC1101_SFRX);
    spiStrobe(CC1101_SRX);
 		return 0;
	}
}
