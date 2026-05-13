#include <MCP3421.h>

MCP3421 adc; //Initialize MCP3421 with default address, 0x6A

void setup() {
  Serial.begin(9600); //Begin serial communication
  adc.begin(); //Initialize ADC
  Serial.print("Welcome to the Machine...\nBegin MCP3421 Demonstration\n\n"); //Token greeting to verify Serial
}

void loop() {
  adc.setResolution(12);
  printVoltage(adc.getVoltage(true), 12);  //Read at 12 bit resolution
  adc.setResolution(14);
  printVoltage(adc.getVoltage(true), 14);  //Read at 14 bit resolution
  adc.setResolution(16);
  printVoltage(adc.getVoltage(true), 16);  //Read at 16 bit resolution
  adc.setResolution(18);
  printVoltage(adc.getVoltage(true), 18);  //Read at 18 bit resolution
  Serial.print("\n\n");
  delay(500); //delay some period between outputs as to not hurt ones eyes with streaming text
}

void printVoltage(float voltage, int bits) {
  String bitLine = "Voltage, " + String(bits) + " bit = ";
  Serial.print(bitLine);
  Serial.print(voltage, bits - 9); //Print voltage, rounded to the significant digits
  Serial.print(" V\n");
}
