// libraries for sensata
#include "PTE7300_I2C.h"

#include <Arduino.h>
#include <cmath>
#include <Wire.h>

#define PCAADDR 0x70
#define DEV_I2C Wire

#undef round

// Create instances of the pressure sensor

unsigned long startTime;

std::vector<std::tuple<std::string, int>> sensatas = {
    std::make_tuple("P0", 0), std::make_tuple("P1", 1),
    std::make_tuple("P2", 2), std::make_tuple("P3", 3),
    std::make_tuple("P4", 4),
};

std::vector<float> sensataReadings(sensatas.size());
PTE7300_I2C sensataInterface;

void sensataSetup() {
    Wire.begin();
}

void pcaselect(uint8_t i) {
    if (i > 7) return;
    startTime = millis();
    Wire.beginTransmission(PCAADDR);
    Wire.write(1 << i);
    Wire.endTransmission();
    i2cTime = millis() - startTime;
    //Serial.print("pcaselect :");
    //Serial.println(i2cTime);
}

// Function to convert raw pressure to PSI
float convertToPSI(int16_t digitalPressure) {
    return ((float)(digitalPressure + 16000) / 32000) * 1450.38;
}

void readSensatas() {
    for (size_t i = 0; i < sensatas.size(); i++) {
        std::tuple<std::string, int> sensata = sensatas[i];

        int muxPort = std::get<1>(sensata);
        pcaselect(muxPort);
        //startTime = millis();
        int16_t rawPressure = sensataInterface.readDSP_S();
        //Serial.print("read DSP :");
        //Serial.println(millis() - startTime);
        float psi           = convertToPSI(rawPressure);
        sensataReadings[i]  = psi;
    }
}

void sensataToString(std::string &message) {
    for (size_t i = 0; i < sensataReadings.size(); i++) {
        std::tuple<std::string, int> sensata = sensatas[i];

        std::string name = std::get<0>(sensata);
        char buffer[20];
        sprintf(buffer, "%.2f", sensataReadings[i]);
        message += name;
        message += buffer;
    }
}