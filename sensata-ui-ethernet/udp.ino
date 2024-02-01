void udpSend(const std::string &message) {
    udp.beginPacket(remoteIP, kPort);
    udp.print(message.c_str());
    udp.endPacket();
}

void sendSensorValues() {
    std::string message = "";
    if (sensataStream) { sensataToString(message); }
    if (thermoStream) { thermocoupleToString(message); }
    if (sensataStream || thermoStream) { udpSend(message); }
}
