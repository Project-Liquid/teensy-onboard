/*
  PTE7300_I2C.h - Public library for PTE7300 I2C interfacing.
  Created by M.H.W. Stopel, 02 September 2019.
  Last update: 11 Dec 2019, initial public version  
*/

#include "Arduino.h"
#include "PTE7300_I2C.h"
#include "Wire.h"
#define MAXIMUM_TRIES 100

// default nodeaddress
#define DEFAULT_NODE_ADDRESS	0x6C

// command register address
#define RAM_ADDR_CMD       0x22
// serial register
#define RAM_ADDR_SERIAL    0x50
// result registers
#define RAM_ADDR_DSP_T     0x2E
#define RAM_ADDR_DSP_S     0x30
#define RAM_ADDR_STATUS	   0x36

PTE7300_I2C::PTE7300_I2C()
{
  _nodeAddress = DEFAULT_NODE_ADDRESS;
  _bUseCRC = true; // CRC flag
  //Wire.begin(); // initiate I2C connection  
}

bool PTE7300_I2C::isConnected()
{
	Wire.beginTransmission(_nodeAddress);
    if (Wire.endTransmission() == 0) {return true;}
	else { return false;}
}

void PTE7300_I2C::CRC(bool tf) {_bUseCRC = tf;}

unsigned int PTE7300_I2C::readRegister(uint8_t address, unsigned int number, uint16_t *buffer)
{
	unsigned int bytesRead=0;
	if(_bUseCRC)
	{
		bytesRead = this->readRegisterCRC(address, number, buffer);
		if (bytesRead != (number*2)+1)
		{
		  // "Error: Could not read from register!"
		  return 0;
		}
		return bytesRead;
	}
	else
	{
		bytesRead = this->readRegisterNoCRC(address, number, buffer);
		if (bytesRead != (number*2))
		{
		  // "Error: Could not read from register!"
		  return 0;
		}
		return bytesRead;
	}
}

unsigned int PTE7300_I2C::readRegisterNoCRC(uint8_t address, unsigned int number, uint16_t *buffer)
{
  
  unsigned int bytesRead = 0; // default return var

  Wire.beginTransmission(_nodeAddress);
  Wire.write(address); //Send register address
  Wire.endTransmission();
  Wire.requestFrom(_nodeAddress, number * 2); //Request register, note that register is 2 bytes wide

  bytesRead = Wire.available();
  if ( bytesRead >= number * 2 )
  {
    for (int i = 0; i < number; i++)
    {
      byte lowByte = Wire.read(); // read low byte
	  byte highByte = Wire.read(); // read high byte
	  buffer[i] = highByte << 8 | lowByte; // join two bytes into word (uint16)
    }
  }

  return bytesRead;
}

unsigned int PTE7300_I2C::readRegisterCRC(uint8_t address, unsigned int number, uint16_t *buffer)
{	
  
  unsigned int bytesRead=0;

  unsigned int i;
  unsigned char crc8_hold;
  unsigned char crc4;
  unsigned char crc8;
  unsigned char header[2];
  unsigned char all[3+number*2];
  unsigned char node;
  
  node = ((_nodeAddress << 1) & 0xFC) | 0x02; //CRC-Flag 1, Readflag 0
  header[0] = address;
  header[1] = ((number*2)-1) << 4;
  crc4 = this->calc_crc4(0x03,0x0F,header,2);
 
  
  all[0]=node;
  all[1]=address;
  all[2]=((number*2)-1) << 4| (crc4 & 0x0F);
  
  // Calculating new CRC(read-stub)
  crc8_hold = this->calc_crc8(0xD5,0xFF,all,3);
  // Serial.println("Info: New CRC8-stub is 0x" + String(crc8_hold, HEX));

  Wire.beginTransmission(_nodeAddress | 1); //indicate CRC-transmission by setting first address bit to 1
  Wire.write(address); //Send register address
  Wire.write((((number*2)-1) << 4) | (crc4 & 0x0F));
  Wire.endTransmission();
  Wire.requestFrom(_nodeAddress | 1,(number*2)+1); //Request registers, note that registers 2 bytes wide
  node = ((_nodeAddress << 1) & 0xFC) | 0x03; // CRC-Flag 1, Readflag 1
  bytesRead = Wire.available();
  // Serial.println("Bytes read: " + String(bytesRead, DEC));
  if(bytesRead >= (number*2)+1) 
  {
    for(int i=0;i<number;i++)
    {
       byte lowByte = Wire.read(); // read low byte
	   byte highByte = Wire.read(); // read high byte
	   buffer[i] = highByte << 8 | lowByte; // join two bytes into word (uint16)
    }
  }
  int crc8_received = Wire.read(); // read CRC byte, after reading the databuffer words
  // Serial.println("CRC8 received: 0x" + String(crc8_received,HEX));
 
  all[0]=node;
  crc8 = this->calc_crc8(0xD5, crc8_hold,all,1);
  crc8 = this->calc_crc8(0xD5, crc8,(unsigned char*)(buffer),number*2);
  // Serial.println("CRC8 calculated: 0x" + String(crc8,HEX));
  
  if(crc8!=crc8_received)
  {
	Serial.println("CRC ERROR!");
	for(int i=0;i<number;i++)
		{
			buffer[i]=0;
		}
	return 0; 
  }
  // Serial.println("No CRC error");  
  
  return bytesRead;
}

// SERIAL register
uint32_t PTE7300_I2C::readSERIAL()
{
  uint16_t serial[2];
  this->readRegister(RAM_ADDR_SERIAL, 2, serial);
  return (((uint32_t)(serial[1]) << 16) & 0xFFFF0000) | ((uint32_t)(serial[0]) & 0x0000FFFF); // join and type-cast to unsigned 32-bit integer
}

// result registers
int16_t PTE7300_I2C::readDSP_T()
{
  uint16_t DSP_T;
  this->readRegister(RAM_ADDR_DSP_T, 1, &DSP_T); 
  return (int16_t)(DSP_T); // type-cast to signed integer
}

int16_t PTE7300_I2C::readDSP_S()
{
  uint16_t DSP_S;
  this->readRegister(RAM_ADDR_DSP_S, 1, &DSP_S);  
  return (int16_t)(DSP_S); // type-cast to signed integer
}

uint16_t PTE7300_I2C::readSTATUS()
{
  uint16_t STATUS;
  this->readRegister(RAM_ADDR_STATUS, 1, &STATUS);  
  return STATUS; // unsigned 16-bit integer
}

char PTE7300_I2C::calc_crc4(unsigned char polynom, unsigned char init, unsigned char* data, unsigned int len)
{
  unsigned char shifter;
  int i,j;
  
  shifter = init;
  for(i=0;i<len;i++)
  {
    for(j=7;j>=0;j--)
    {
      if((i >= len - 1) && (j < 4)) break;
      if( ((shifter >> 3) & 0x01) != ((data[i] >> j)&0x01) ) shifter = (shifter << 1) ^ polynom;
      else shifter = shifter << 1;
      shifter = shifter & 0x0F;
    }
  }
  return shifter & 0x0F;
}

char PTE7300_I2C::calc_crc8(unsigned char polynom, unsigned char init, unsigned char* data, unsigned int len)
{
  unsigned char shifter;
  int i,j;
  
  shifter = init;
  for(i=0;i<len;i++)
  {
    for(j=7;j>=0;j--)
    {
      if( ((shifter >> 7) & 0x01) != ((data[i] >> j)&0x01) ) shifter = (shifter << 1) ^ polynom;
      else shifter = shifter << 1;
    }
  }
  return shifter & 0xFF;
}