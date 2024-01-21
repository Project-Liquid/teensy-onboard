#include <QNEthernet.h>
#include <string.h>

using namespace qindesign::network;

// Set your static IP configuration
IPAddress teensyIP(169, 254, 44, 72);  // First three numbers should match laptop IP
IPAddress gateway(169, 254, 44, 1);  // First three numbers should match laptop IP
IPAddress subnet(255,255,0,0);  // Should map laptop subnet

bool foundLaptop = false; // check if packet received from laptop
IPAddress remoteIP(0, 0, 0, 0); // save laptop IP

constexpr uint16_t kPort = 5190;  // Chat port

EthernetUDP udp;

// Initialize command list with maximum 20 commands in it -- what's a reasonable number?
// Each command is max length 8 characters -- can we make use of that for memory?
String commandBuffer[20];
uint8_t numCommands = 0;

// Keep track of if we're waiting, when we started, and how long we should wait
bool waiting = false;
unsigned long startTime;
long duration;

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

  if(foundLaptop) {
    printf("Sending message to [%u.%u.%u.%u:%d]\n", remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3], kPort);
    udpSend("HSK");
  }

  // Execute the first command in the buffer
  // Unless the first command is PDW, SPK, etc and we are waiting
  executeCommands();

  delay(10); // TODO -- should we check for receiving packets less frequently than 100Hz?
}

void executeCommands()
{
  if (numCommands == 0) { 
    return;
  } 
  
  // Access and read the first command
  String command = commandBuffer[0];  
  String code = command.substring(0, 3);
  String data = command.substring(4);
  // compareTo returns 0 if strings match
  // These are all the commands we want to execute immediately, even if we're in a wait period
  if (code.compareTo("ABT") == 0) {
    // abort 
    // clear the buffer
    removeFirstCommand();
    return;
  } else if (code.compareTo("CLR") == 0) {
    // clear the buffer
    removeFirstCommand();
    return;
  } else if (code.compareTo("TMP") == 0) {
    // do temp stuff, i don't know
    removeFirstCommand();
    return;
  } else if (code.compareTo("SEN") == 0) {
    // do sensata stuff, i don't know
    removeFirstCommand();
    return;
  }

  // if bool waiting is true, check if waiting period is over
  if (waiting) {
    if (millis() - startTime >= duration) {
      waiting = false;
    } else {
      return;
    }
  }

  // These are all the commands we want to execute if we are not waiting
  if (code.compareTo("PDW") == 0) {
    // pin write
  } else if (code.compareTo("SPK") == 0) {
    // spark plug
  } else if (code.compareTo("IDL") == 0) {
    // idle
  } else if (code.compareTo("WAI") == 0) { // wait for x centiseconds (up to 100 sec)
    waitCentisec(data);
  } else {
      Serial.print("Command includes unrecognized command sequence:");
      Serial.println(code);
  }

  removeFirstCommand();
  return;
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
  const uint8_t *data = udp.data();
  remoteIP = udp.remoteIP();
  if (!foundLaptop) {
    udpSend("HSK");
    foundLaptop = true;
  }

  printf("[%u.%u.%u.%u][%d] ", remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3], size);

  // Hopefully this will work with strtok. If not -- const_cast<char*>(data_str.c_str()) or something? 
  const char* data_str(reinterpret_cast<const char*>(data));
  String newCommands[10];

  // Read and store each of the commands in sequence
  char* command = strtok(data_str, ";");
  size_t newCommandNum = 0;
  for (; command != NULL && newCommandNum < 10; newCommandNum++) {
    newCommands[newCommandNum] = command;
    command = strtok(NULL, ";");
  }

  // Shift existing commands back
  for (size_t i = newCommandNum; i < 20; i++) {
    commandBuffer[i + newCommandNum] = commandBuffer[i];
  }
  // Insert new commands into commandBuffer
  for (size_t i = 0; i < newCommandNum; i++) {
    commandBuffer[i] = newCommands[i];
  }

  numCommands += newCommandNum;
}

void removeFirstCommand() {
  if (numCommands > 0) {
    // Shift elements to remove the first command
    for (size_t i = 0; i < numCommands; i++) {
      commandBuffer[i] = commandBuffer[i + 1];
    }
    numCommands -= 1;
  }
}

void pinDigitalWrite(String data) {  // PDW
    if (data.length() < 3) {
          Serial.print("Command data too short for PDW:");
          Serial.println(data);
    }

    for (size_t pos = 0; pos < data.length(); pos += 3) {
        String pin_name = data.substring(pos, pos+1);
        int pin = pin_name.toInt();
        char dataVal = data.charAt(pos+2);

        // Serial.println(pin);
        if ((pin < 0) || (pin > 8 )) { //TODO: probably put these somewhere better
          Serial.print("Command includes out-of-bounds pin:");
          Serial.println(pin);
        }

        // int pinNum = valvePins[pin]; // if we store valve pins on the Teensy
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
            Serial.println(pin);
        }
    }
}


void waitCentisec(String data) {
  duration = data.toInt() * 10;
  startTime = millis();
  waiting = true;
}