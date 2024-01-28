#include <iostream>
#include <vector>




float voltToTemp(float v)
{
	float temperature =  ((6.71061387 * pow(0.1, 6) * pow(v, 4)) + (8.99614883 * pow(0.1, 4) * pow(v, 3)) +  (-7.62504049 * pow(0.1, 2) * pow(v, 2)) + (2.53919375 * pow(10, 1) * pow(v, 1)) + (-1.29329394));
	return temperature;
}

float analogToTemperature(int input)
{
	float voltage = ((0.003225) * input) / 47;
	float temperature = voltToTemp(voltage);
}

void thermocoupleToString(std::string& message)
{
  for(size_t i = 0; i < thermocoupleVoltage.size(); i++) {
    std::tuple<std::string, float> currentThermocoupleTuple = thermocoupleVoltage[i];
    std::string name = std::get<0>(currentThermocoupleTuple);
    float temperature = std::get<1>(currentThermocoupleTuple);
    message += (name + std::to_string(temperature));
  }
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
	std::vector<std::tuple<std::string, float>> thermocoupleVoltage;

	thermocoupleVoltage.push_emplace_back(std::tuple<std::string "T0", analogToTemperature((float) analogRead(A7)));
	thermocoupleVoltage.push_back(std::tuple<std::string "T1", analogToTemperature((float) analogRead(A8)));
	thermocoupleVoltage.push_back(std::tuple<std::string "T2", analogToTemperature((float) analogRead(A9)));
  
  std::string new_message;
  thermocoupleToString(new_message);
  Serial.println(new_message);
  thermocoupleVoltage.clear();
  new_message = "";

}
