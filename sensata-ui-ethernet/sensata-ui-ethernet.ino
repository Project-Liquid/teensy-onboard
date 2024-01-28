#include <QNEthernet.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace qindesign::network;

// Set your static IP configuration
IPAddress teensyIP(169, 254, 44, 72);  // First three numbers should match laptop IP
IPAddress gateway(169, 254, 44, 1);  // First three numbers should match laptop IP
IPAddress subnet(255,255,0,0);  // Should map laptop subnet

// bool foundLaptop = false; // check if packet received from laptop
IPAddress remoteIP(0, 0, 0, 0); // save laptop IP

constexpr uint16_t kPort = 5190;  // Chat port

EthernetUDP udp;

// Command name, time of command execution
std::vector<std::tuple<std::string, unsigned long>> commandSchedule;

// whether or not to transmit sensor data
bool sensataStream = false;
bool thermoStream = false;

/* J17: Ethane Vent : V0
J16: Upper Ethane : V1
J15: Lower Ethane : V2
J14: Nitrous Vent : V3
J13: Upper Nitrous : V4
J12: Lower Nitrous : V5 */
int sparkPlugPin = 20;
size_t numValves = 6;
//int valvePins[numValves] = {17, 16, 15, 14, 13, 12};
//int abortSeq[numValves] = {0, 0, 0, 1, 0, 0};
//int idleSeq[numValves] = {1, 1, 1, 0, 1, 1}; // this may not be right ... check with fluids

// TODO error command 

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000) {
    // Wait for Serial
  }
  printf("Starting...\r\n");

  // TODO put networking in a function
  // Get Teensy mac address
  uint8_t mac[6];
  Ethernet.macAddress(mac);  // This is informative; it retrieves, not sets
  printf("MAC = %02x:%02x:%02x:%02x:%02x:%02x\r\n",
         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // Listen for link changes
  Ethernet.onLinkState([](bool state) {
    if (state) {
      printf("[Ethernet] Link ON: %d Mbps, %s duplex, %s crossover\r\n",
             Ethernet.linkSpeed(),
             Ethernet.linkIsFullDuplex() ? "full" : "half",
             Ethernet.linkIsCrossover() ? "is" : "not");
    } else {
      printf("[Ethernet] Link OFF\r\n");
    }
  });

  // Begin Ethernet connection with static IP address
  Ethernet.setDHCPEnabled(false);
  Ethernet.begin(teensyIP, subnet, gateway);

  // Print Teensy IP
  IPAddress ip = Ethernet.localIP();
  printf("    Local IP     = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);

  if (!Ethernet.waitForLink(15000)) {
    printf("Failed to get link \r\n");
    return;
  }
  else{
    printf("link established \r\n");
  }

  // Start UDP server
  udp.begin(kPort);
}

void loop() {
  // Check if new commands received and insert into buffer
  // Execute immediately if necessary
  receivePacket();   

  // if it's time to execute any commands , execute them
  executeScheduledCommands();

  readSensatas();

  sendSensorValues();

  Serial.println("Running loop");

  delay(100);
}

void executeScheduledCommands()
{
  commandSchedule.erase(std::remove_if(commandSchedule.begin(), commandSchedule.end(), [](std::tuple<std::string, unsigned long> tuple) {
    unsigned long time = std::get<1>(tuple);
    if(time <= millis()){
      std::string command = std::get<0>(tuple);
      parseCommand(command);
      return 1;
    } else {
      return 0;
    }
  }), commandSchedule.end());  
}

void parseCommand(std::string command) { 
  // Access and read the first command
  std::string code = command.substr(0, 3);
  std::string data = command.substr(3);
  // These are all the commands we want to execute immediately, even if we're in a wait period
  if (code == "ECH") {
    udpSend(data.c_str());
  } else if (code == "ABT") {
    //abort();
    commandSchedule.clear();
  } else if (code == "CLR") {
    commandSchedule.clear();
  } else if (code == "TMP") {
    int start;
    if(!stringToInt(data, 0, 1, start) || (start != 0 || start != 1))
    {
      Serial.print("TMP stream command not valid: ");
      Serial.println(data.c_str());
    } else {
      thermoStream = start;
    }
  } else if (code == "SEN") {
    int start;
    if(!stringToInt(data, 0, 1, start) || (start != 0 || start != 1))
    {
      Serial.print("SEN stream command not valid: ");
      Serial.println(data.c_str());
    } else {
      sensataStream = start;
    }
  } else if (code == "PDW") {
    pinDigitalWrite(data);
  } else if (code == "SPK") {
    int val;
    if(!stringToInt(data, 0, 1, val) || (val != 0 || val != 1))
    {
      Serial.print("SPK on/off command not valid: ");
      Serial.println(data.c_str());
    } else {
      digitalWrite(sparkPlugPin, val);    
    }
  } else if (code == "IDL") {
    //idle();
  } else {
      Serial.print("Command includes unrecognized command sequence:");
      Serial.println(code.c_str());
  }
}

// Control character names.
static const String kCtrlNames[]{
  "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
  "BS",  "HT",  "LF",  "VT",  "FF",  "CR",  "SO",  "SI",
  "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
  "CAN", "EM",  "SUB", "ESC", "FS",  "GS",  "RS",  "US",
};


// Receives and prints chat packets
static void receivePacket() {
  int size = udp.parsePacket();
  if (size < 0) {
    return;
  }

  // Get the packet data and set remote address
  const uint8_t *data = udp.data(); // TODO type
  remoteIP = udp.remoteIP();

  // Converting to stringstream
  const char* data_char_array = reinterpret_cast<const char*>(data);
  std::string data_str(data_char_array, std::min(strlen(data_char_array), static_cast<size_t>(size))); 
  std::stringstream ss(data_str);
  std::string command;
  unsigned long executeTime = 0; 
  while(std::getline(ss, command, ';')) {
    // if command = wait, increase executeTime
    std::string code = command.substr(0, 3);
    if (code == "WAI") {
      std::string message = command.substr(3);
      unsigned long duration = std::stoul(message);
      executeTime += duration * 10;
    } else {
      if(executeTime == 0) {
        parseCommand(command);
      } else {
        std::tuple<std::string, unsigned long> commandTime(command, executeTime + millis());
        commandSchedule.push_back(commandTime);
      }
    }
  }
}

bool stringToInt(const std::string& data, size_t start, size_t end, int& out){
    for(size_t i = start; i < end; i++) {
        if (!std::isdigit(data[i]))
        {
          return false;
        }
    }
    out = stoi(data.substr(start, end));
    return true;
}

void pinDigitalWrite(const std::string& data) {  // PDW
    if (data.length() % 3 != 0 || data.length() == 0) {
          Serial.print("Command data incorrect length for PDW:");
          Serial.println(data.c_str());
          return;
    }

    for (size_t pos = 0; pos < data.length(); pos += 3) {
        int pin;
        int dataVal;
        if (!stringToInt(data, pos, pos+2, pin)){
          Serial.print("Invalid pin for PDW: ");
          Serial.println(data.c_str());
          return;
        }
        if (!stringToInt(data, pos+2, pos+3, dataVal) || (dataVal != 0 || dataVal != 1)) {
          Serial.print("Invalid pin high/low for PDW: ");
          Serial.println(data.c_str());
          return;
        }

        // Serial.println(pin);
        if (!( pin == 11 || (pin >= 15 && pin <= 20))) { // TODO change once we make list of valves-to-pins
          Serial.print("Command includes out-of-bounds pin:");
          Serial.println(pin);
          return;
        }

        digitalWrite((uint8_t)pin, dataVal);
        Serial.print("Set pin "); Serial.print((uint8_t)pin);
        Serial.print(" to "); Serial.println(dataVal);
    }
}

/*
void valveDigitalWrite(const std::string& data) {  // VDW
    if (data.length() % 2 != 0 || data.length() == 0) {
          Serial.print("Command data incorrect length for VDW:");
          Serial.println(data.c_str());
          return;
    }

    for (size_t pos = 0; pos < data.length(); pos += 2) {
        int valve;
        int dataVal;
        if (!stringToInt(data, pos, pos+1, valve)){
          Serial.print("Invalid valve for VDW: ");
          Serial.println(data.c_str());
          return;
        }
        if (!stringToInt(data, pos+1, pos+2, dataVal) || (dataVal != 0 || dataVal != 1)) {
          Serial.print("Invalid valve high/low for PDW: ");
          Serial.println(data.c_str());
          return;
        }

        if (!(valve >= 0 && valve <=5)) { // 
          Serial.print("Command includes out-of-bounds valve:");
          Serial.println(valve);
          return;
        }

        digitalWrite((uint8_t)valvePins[valve], dataVal);
        Serial.print("Set valve "); Serial.print((uint8_t)valve);
        Serial.print(" to "); Serial.println(dataVal);
    }
}

void abort()
{
  for(size_t valve =0; valve < numValves; valve++) {
    digitalWrite((uint8_t)valvePins[valve], abortSeq[valve]);
  }
}

void idle()
{
  for(size_t valve =0; valve < numValves; valve++) {
    digitalWrite((uint8_t)valvePins[valve], idleSeq[valve]);
  }
} */