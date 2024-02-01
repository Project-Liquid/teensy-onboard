#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

// Command name, time of command execution
std::vector<std::tuple<std::string, unsigned long>> commandSchedule;

// whether or not to transmit sensor data
bool sensataStream = false;
bool thermoStream  = false;

// TODO: maybe only send sensor values on intervals

/* J17: Ethane Vent : V0
J16: Upper Ethane : V1
J15: Lower Ethane : V2
J14: Nitrous Vent : V3
J13: Upper Nitrous : V4
J12: Lower Nitrous : V5 */
int sparkPlugPin         = 20;
const uint8_t numValves  = 6;
int valvePins[numValves] = {0, 1, 2, 17, 16, 15};
int abortSeq[numValves]  = {0, 0, 0, 1, 0, 0};  // todo confirm
int idleSeq[numValves]   = {1, 1, 1, 0,
                            1, 1};  // this may not be right ... check with fluids

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 4000) {
        // Wait for Serial
    }

    // TODO put networking in a function
    udpSetup();

    sensataSetup();

    for (size_t i = 0; i < numValves; i++) { pinMode(valvePins[i], OUTPUT); }
}

void loop() {
    // Check if new commands received and insert into buffer
    // Execute immediately if necessary
    receivePacket();

    // if it's time to execute any commands , execute them
    executeScheduledCommands();

    readSensatas();

    sendSensorValues();
}

void executeScheduledCommands() {
    commandSchedule.erase(
        std::remove_if(
            commandSchedule.begin(), commandSchedule.end(),
            [](std::tuple<std::string, unsigned long> tuple) {
                unsigned long time = std::get<1>(tuple);
                if (time <= millis()) {
                    std::string command = std::get<0>(tuple);
                    parseCommand(command);
                    return 1;
                } else {
                    return 0;
                }
            }
        ),
        commandSchedule.end()
    );
}

void parseCommand(std::string command) {
    // Let laptop know that we're executing a command
    udpSend(command);
    // Access and read the first command
    std::string code = command.substr(0, 3);
    std::string data = command.substr(3);
    // These are all the commands we want to execute immediately, even if we're
    // in a wait period
    if (code == "ECH") {
        udpSend(data);
    } else if (code == "ABT") {
        abort();
        commandSchedule.clear();
    } else if (code == "CLR") {
        commandSchedule.clear();
    } else if (code == "TMP") {
        int start;
        if (!stringToInt(data, 0, 1, start) || (start != 0 && start != 1)) {
            error("TMP stream command not valid: " + data);
        } else {
            thermoStream = start;
        }
    } else if (code == "SEN") {
        int start;
        if (!stringToInt(data, 0, 1, start) || (start != 0 && start != 1)) {
            error("SEN stream command not valid: " + data);
        } else {
            sensataStream = start;
        }
    } else if (code == "PDW") {
        pinDigitalWrite(data);
    } else if (code == "VDW") {
        valveDigitalWrite(data);
    } else if (code == "SPK") {
        int val;
        if (!stringToInt(data, 0, 1, val) || (val != 0 && val != 1)) {
            error("SPK on/off command not valid: " + data);
        } else {
            digitalWrite(sparkPlugPin, val);
        }
    } else if (code == "IDL") {
        idle();
        commandSchedule.clear();
    } else {
        error("Command includes unrecognized command sequence: " + code);
    }
}

// Receives and prints chat packets
static void receivePacket() {
    int size = udp.parsePacket();
    if (size < 0) { return; }

    // Get the packet data and set remote address
    const uint8_t *data = udp.data();
    remoteIP            = udp.remoteIP();

    // Converting to stringstream
    const char *data_char_array = reinterpret_cast<const char *>(data);
    std::string data_str(
        data_char_array,
        std::min(strlen(data_char_array), static_cast<size_t>(size))
    );
    std::stringstream ss(data_str);
    std::string command;
    unsigned long executeTime = 0;
    while (std::getline(ss, command, ';')) {
        // if command = wait, increase executeTime
        std::string code = command.substr(0, 3);
        if (code == "WAI") {
            std::string message    = command.substr(3);
            unsigned long duration = std::stoul(message);
            executeTime += duration * 10;
        } else {
            if (executeTime == 0) {
                parseCommand(command);
            } else {
                std::tuple<std::string, unsigned long> commandTime(
                    command, executeTime + millis()
                );
                commandSchedule.push_back(commandTime);
            }
        }
    }
}

bool stringToInt(const std::string &data, size_t start, size_t end, int &out) {
    for (size_t i = start; i < end; i++) {
        if (!std::isdigit(data[i])) { return false; }
    }
    out = stoi(data.substr(start, end));
    return true;
}

void pinDigitalWrite(const std::string &data) {  // PDW
    if (data.length() % 3 != 0 || data.length() == 0) {
        error("Command data incorrect length for PDW: " + data);
        return;
    }

    for (size_t pos = 0; pos < data.length(); pos += 3) {
        int pin;
        int dataVal;
        if (!stringToInt(data, pos, pos + 2, pin)) {
            error("Invalid pin for PDW: " + data);
            return;
        }
        if (!stringToInt(data, pos + 2, pos + 3, dataVal) ||
            (dataVal != 0 && dataVal != 1)) {
            error("Invalid pin high/low for PDW: " + data);
            return;
        }

        // Serial.println(pin);
        if (!inArray(pin, valvePins, numValves)) {
            error("Command includes out-of-bounds pin: " + std::to_string(pin));
            return;
        }

        digitalWrite((uint8_t)pin, dataVal);
        Serial.print("Set pin ");
        Serial.print((uint8_t)pin);
        Serial.print(" to ");
        Serial.println(dataVal);
    }
}

void valveDigitalWrite(const std::string &data) {  // VDW
    if (data.length() % 2 != 0 || data.length() == 0) {
        error("Command data incorrect length for VDW: " + data);
        return;
    }

    for (size_t pos = 0; pos < data.length(); pos += 2) {
        int valve;
        int dataVal;
        if (!stringToInt(data, pos, pos + 1, valve)) {
            error("Invalid valve for VDW: " + data);
            return;
        }
        if (!stringToInt(data, pos + 1, pos + 2, dataVal) ||
            (dataVal != 0 && dataVal != 1)) {
            error("Invalid valve high/low for PDW: " + data);
            return;
        }

        if (!(valve >= 0 && valve <= 5)) {  //
            error(
                "Command includes out-of-bounds valve: " + std::to_string(valve)
            );
            return;
        }

        digitalWrite((uint8_t)valvePins[valve], dataVal);
        Serial.print("Set valve ");
        Serial.print((uint8_t)valve);
        Serial.print(" to ");
        Serial.println(dataVal);
    }
}

void abort() {
    for (size_t valve = 0; valve < numValves; valve++) {
        digitalWrite((uint8_t)valvePins[valve], abortSeq[valve]);
    }
}

void idle() {
    for (size_t valve = 0; valve < numValves; valve++) {
        digitalWrite((uint8_t)valvePins[valve], idleSeq[valve]);
    }
}

bool inArray(int element, const int *array, size_t arraySize) {
    for (size_t i = 0; i < arraySize; i++) {
        if (element == array[i]) { return true; }
    }
    return false;
}

void error(std::string message) {
    Serial.println(message.c_str());
    udpSend("ERR: " + message);
}
