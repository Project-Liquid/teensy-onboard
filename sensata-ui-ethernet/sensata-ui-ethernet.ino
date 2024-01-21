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
  // Check if new commands received
  // If they are, insert them at the beginning of the buffer to be executed immediately
  // Mainly useful for immediate abort
  receivePacket();   

  /*
  if(foundLaptop) {
    printf("Sending message to [%u.%u.%u.%u:%d]\n", remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3], kPort);
    udpSend("HSK");
  } */

  // if it's time to execute any commands , execute them
  executeScheduledCommands();

  // delay(10); // TODO -- should we check for receiving packets less frequently than 100Hz?
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
  // compareTo returns 0 if strings match
  // These are all the commands we want to execute immediately, even if we're in a wait period
  if (code == "ECH") {
    echo(data);
  } /* else if (code == "ABT") {
    // abort 
    // clear the buffer
  } else if (code.compareTo("CLR") == 0) {
    // clear the buffer
  } else if (code.compareTo("TMP") == 0) {
    // do temp stuff, i don't know
  } else if (code.compareTo("SEN") == 0) {
    // do sensata stuff, i don't know
  } */ else if (code == "PDW") {
    pinDigitalWrite(data);
  } /* else if (code.compareTo("SPK") == 0) {
    // spark plug
  } else if (code.compareTo("IDL") == 0) {
    // idle
  } */ else {
      Serial.print("Command includes unrecognized command sequence:");
      Serial.println(code.c_str());
  }
}

void echo(std::string data)
{
  udpSend(data.c_str());
}

void udpSend(const String &message) {
  udp.beginPacket(remoteIP, kPort);
  udp.print(message);
  udp.endPacket();
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


void pinDigitalWrite(std::string data) {  // PDW
    if (data.length() % 3 != 0 || data.length() == 0) {
          Serial.print("Command data incorrect length for PDW:");
          Serial.println(data.c_str());
          return;
    }

    for (size_t pos = 0; pos < data.length(); pos += 3) {
        std::string pin_name = data.substr(pos, pos+2);
        for(char ch : pin_name) {
            if (!std::isdigit(ch))
            {
              Serial.print("Command data incorrect type for PDW:");
              Serial.println(data.c_str());
              return;
            }
        }
        int pin = stoi(pin_name);
        char dataVal = data[pos+2];

        // Serial.println(pin);
        if (!( pin == 11 || (pin >= 15 && pin <= 20))) { // TODO change once we make list of valves-to-pins
          Serial.print("Command includes out-of-bounds pin:");
          Serial.println(pin);
          return;
        }

        if (dataVal == '0') {
            digitalWrite((uint8_t)pin, LOW);
            Serial.print("Set pin "); Serial.print((uint8_t)pin);
            Serial.println(" to LOW");
        } else if (dataVal == '1') {
            digitalWrite((uint8_t)pin, HIGH);
            Serial.print("Set pin "); Serial.print((uint8_t)pin);
            Serial.println(" to HIGH");
        } else {
            Serial.print("Command includes invalid write value:");
            Serial.println(dataVal);
        }
    }
}