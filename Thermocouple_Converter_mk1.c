#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

double voltToTemp(double v)
{
	double temperature =  ((6.71061387 * pow(0.1, 6) * pow(v, 4)) + (8.99614883 * pow(0.1, 4) * pow(v, 3)) +  (-7.62504049 * pow(0.1, 2) * pow(v, 2)) + (2.53919375 * pow(10, 1) * pow(v, 1)) + (-1.29329394));
	return temperature;
}

double tempToVolt(double t)
{
	double voltage =  ((3.24957283 * pow(0.1, 13) * pow(t, 4)) + (-4.91257347 * pow(0.1, 9) * pow(t, 3)) +  (6.79325573 * pow(0.1, 6) * pow(t, 2)) + (3.90235762 * pow(0.1, 2) * pow(t, 1)) + (5.78459610 * pow(0.1, 2)));
	return voltage; //Note that this voltage is in microvolts
}

int main(int argc, char* argv[])
{
	double input = abs(atof(argv[1]));
	bool second_command = false;
	char* command = "test";
	if (argc > 2)
	{
		command = argv[2];
		second_command = true;
	}
	char* from_temp_to_voltage = "ttv"; //Input ttv (temperature to voltage) in order to convert from temperature to voltage
	double result = 17;
	if ((second_command) && (strcmp(command, from_temp_to_voltage) == 0))
	{
		result = tempToVolt(input);
	}
	else
	{
		result = voltToTemp(input);
	}
	double result = voltToTemp(input);
	printf("%f \n", result);
	return 0;
}