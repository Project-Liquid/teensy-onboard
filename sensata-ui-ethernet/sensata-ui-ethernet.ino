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

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000) {
    // Wait for Serial
  }
  printf("Starting...\r\n");

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
  receivePacket();

  if(foundLaptop) {
    printf("Sending message to [%u.%u.%u.%u:%d]\n", remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3], kPort);
    udpSend("HSK");
  }

  delay(1000);
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

  // Print each character
  for (int i = 0; i < size; i++) {
    uint8_t b = data[i];
    if (b < 0x20) {
      printf("<%s>", kCtrlNames[b].c_str());
    } else if (b < 0x7f) {
      putchar(data[i]);
    } else {
      printf("<%02xh>", data[i]);
    }
  }
  printf("\r\n");
}