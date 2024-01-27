void udpSend(const std::string &message) {
  udp.beginPacket(remoteIP, kPort);
  udp.print(message.c_str());
  udp.endPacket();
}

void sendSensorValues(){
  std::string message = "";
  if (sensataStream) {sensataToString(message); }
  // todo thermocoupleToString(message); 
  udpSend(message);
}