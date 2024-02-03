#ifndef UDP_H
#define UDP_H

#include <QNEthernet.h>
using namespace qindesign::network;

// Set your static IP configuration
IPAddress
    teensyIP(169, 254, 44, 72); // First three numbers should match laptop IP
IPAddress
    gateway(169, 254, 44, 1);     // First three numbers should match laptop IP
IPAddress subnet(255, 255, 0, 0); // Should map laptop subnet

// Set to the latest IP address that the Teensy receives a message from
IPAddress remoteIP(0, 0, 0, 0);

constexpr uint16_t kPort = 5190; // Chat port

EthernetUDP udp;

void udpSetup();

void udpSend(const std::string &message);

void sendSensorValues();

#endif