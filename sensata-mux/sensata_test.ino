#include <Arduino.h>
#include <Wire.h>

#define ADDR 0x6C
#define REG_PRESSURE 0x30

#define MUXADDR 0x70

void setup() {
  Serial.begin(9600);
  Serial.println("Starting");
  Wire.begin();
  //muxselect(0);
}

void loop() {
  muxselect(0);
  Serial.print("Sensor 0: ");
  if (isConnected()) {
    Serial.println(pressureCalc(readPressure()));
  } else {
    Serial.println("disconnected");
  }
  muxselect(1);
  Serial.print("Sensor 1: ");
  if (isConnected()) {
    Serial.println(pressureCalc(readPressure()));
  } else {
    Serial.println("disconnected");
  }
}

void muxselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(MUXADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

uint8_t readRegisterNoCRC(uint8_t register_addr, uint8_t read_len, uint16_t *dest) {
  Wire.beginTransmission(ADDR);
  Wire.write(register_addr);
  Wire.endTransmission();
  Wire.requestFrom(ADDR, read_len * 2); // register is two bytes wide

  uint8_t num_bytes_read = Wire.available();
  if (num_bytes_read >= read_len * 2) {
    for (int i = 0; i < read_len; ++i) {
      char lowByte = Wire.read();
      char highByte = Wire.read();
      dest[i] = highByte << 8 | lowByte;
    }
  }

  return num_bytes_read;
}

bool isConnected() {
  Wire.beginTransmission(ADDR);
  if (Wire.endTransmission() == 0) return true;
  else return false;
}

int16_t readPressure() {
  uint16_t pressureOut;
  readRegisterNoCRC(REG_PRESSURE, 1, &pressureOut);
  return (int16_t)pressureOut;
}

long pressureCalc(int16_t digitalPressure)
{
  return digitalPressure / 32768.0;
}

long oldPressureCalc(int16_t digitalPressure)
{
  return (355 + (-7.16E-03 * digitalPressure) + (-1.84E-06 * (digitalPressure * digitalPressure)));
}

