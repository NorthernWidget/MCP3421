/******************************************************************************
MCP3421.cpp
Library for MCP3421 ADC
Bobby Schulz @ Northern Widget LLC & UMN
2/20/2018

MCP3421 is a 18 bit ADC, PGA, and various modes of resolution operation in order to facilitate various data rates

"Simplicity is prerquisite for reliability"
-Edsger W. Dijkstra

Distributed as-is; no warranty is given.
******************************************************************************/

#include <Arduino.h>
#include <MCP3421.h>
#include <Wire.h>


MCP3421::MCP3421(int _addr)
{
  addr = _addr;
  #if defined(ARDUINO) && ARDUINO >= 100
    Wire.begin();
  #elif defined(PARTICLE)
    if(!Wire.isEnabled()) Wire.begin(); //Only initialize I2C if not done already //INCLUDE FOR USE WITH PARTICLE
  #endif
}

MCP3421::MCP3421(void)
{
  addr = 0x6A; //Use 0x6A as address by default
  #if defined(ARDUINO) && ARDUINO >= 100
    Wire.begin();
  #elif defined(PARTICLE)
    if(!Wire.isEnabled()) Wire.begin(); //Only initialize I2C if not done already //INCLUDE FOR USE WITH PARTICLE
  #endif
}

int MCP3421::begin(int _addr) //Initialize the system in 1x gain, with 12 bit resolution, continuous conversions
{
  addr = _addr; //Allow for address to be set in begin statement as well
  return begin();
}

int MCP3421::begin(void) //Initialize the system in 1x gain, with 12 bit resolution, continuous conversions
{
  setGain(1);
  setResolution(12); // measurement duration defaults to that required for 12 bit
  setMode(CONTINUOUS);
  Wire.beginTransmission(addr);
  return Wire.endTransmission(); //Return I2C status
}

//Returns the bit value from the conversion - defaults to waiting for updated value
long MCP3421::getVoltageRaw(bool waitForVal) {
  int data[4];

  int config = getConfig(); //Get config register to test if single shot or continuous operation is being used
  if((config & 0x10) == SINGLE_SHOT) { //If single shot is being used, initiate a new conversion
    config = config | 0x80;
    Wire.beginTransmission(addr);
    Wire.write(config); //Set conversion bit
    Wire.endTransmission();
    delay(1);
  }

  unsigned long localTime = millis();
  while((config & 0x80) != 0 && waitForVal == true && (millis() - localTime) < 275) { //Wait for next conversion (for both single shot or continuous), only if told to wait for new value, or 275ms
    config = getConfig(); //Test register for new value to be read
  }

  Wire.requestFrom(addr, 4);

  if(Wire.available() == 4) //Get data bytes, 3 bytes of potential data and configuration register
  {
    data[0] = Wire.read();
    data[1] = Wire.read();
    data[2] = Wire.read();
    data[3] = Wire.read();
  }

  int numBits = 12 + ((data[3] & 0x0C) >> 2)*2; //Calculate the number of bits used for conversion from config register

  long rawADC = 0; //Use appropriate conversion math based on the number of bits used in the conversion
  if(numBits == 12) rawADC = ((data[0] & 0x0F) << 8) + data[1];
  if(numBits == 14) rawADC = ((data[0] & 0x3F) << 8) + data[1];
  if(numBits == 16) rawADC = ((data[0] & 0xFF) << 8) + data[1];
  if(numBits == 18) rawADC = ((long(data[0]) & 0x03) << 16) + (long(data[1]) << 8) + data[2];

  if(rawADC > pow(2, numBits)/2 - 1) // MSB set: negative in two's complement
    rawADC -= (long)pow(2, numBits);  // sign-extend to signed long

  config = getConfig();  //Get configuration register to do bit masking with
  Wire.beginTransmission(addr);
  Wire.write(config & 0xEF); //Clear conversion bit to ensure result is not reread
  Wire.endTransmission();

  return rawADC;  //return raw result
}

//Returns float voltage value in volts - defaults to waiting for updated value
float MCP3421::getVoltage(bool waitForVal) {
  long raw = getVoltageRaw(waitForVal); //Get the raw bits, pass on stale value flag
  int config = getConfig(); //Get configuration register

  int gain = round(pow(2, (config & 0x03))); //Calc gain from config register
  int numBits = 12 + ((config & 0x0C) >> 2)*2; //Calc number of bits from config register
  return (float(raw)*(4.096/pow(2, numBits)))/(float(gain));
}

//Returns the current gain setting read from the hardware config register (1, 2, 4, or 8)
int MCP3421::getGain() {
  return round(pow(2, (getConfig() & 0x03)));
}

//Returns the current resolution setting read from the hardware config register (12, 14, 16, or 18)
int MCP3421::getResolution() {
  return 12 + ((getConfig() & 0x0C) >> 2)*2;
}

//Sets the gain of the internal PGA, with either 1x, 2x, 4x, or 8x
//Takes as input the desired gain factor
//Returns result of I2C communication if gain value is valid, or returns -1 if gain value is not valid
int MCP3421::setGain(int gain) {
  boolean validGain = false;
  for(int i = 0; i < 4; i++){ //Test if gain value is valid
    if(round(pow(2, i)) == gain) validGain = true;
  }

  if(validGain){ //If gain value is valid, attempt to set new gain value
    int config = getConfig();
    config = (config & 0xFC) | int(log(gain)/log(2));
    Wire.beginTransmission(addr);
    Wire.write(config);
    return Wire.endTransmission(); //Return results of attempt
  }

  else return -1; //If gain value is not valid, return -1
}

//Sets the resolution of the converter, either 12 bit, 14 bit, 16 bit, or 18 bit
//Takes as input the desired resolution in number of bits
//Returns the result of the I2C communication if the resolution is valid, or returns -1 if resolution value is not valid
int MCP3421::setResolution(int resolution) {
  boolean validResolution = false;
  for(int i = 0; i < 4; i++){ //Test if resolution value is valid
    if(12 + 2*i == resolution) validResolution = true;
  }

  if(validResolution){ //If resolution value is valid, attempt to set new resolution
    int config = getConfig();
    config = (config & 0xF3) | ((resolution - 12)/2) << 2;
    Wire.beginTransmission(addr);
    Wire.write(config);
    return Wire.endTransmission();  //return result of attempt
  }

  else return -1; //If resolution value is not valid, return -1
}

//Returns the configuration register of the device, used for testing status of conversion, mode of operation, gain and resolution settings
int MCP3421::getConfig() {
  int data[4];

  Wire.requestFrom(addr, 4);
  if(Wire.available() == 4)  //Get 4 bytes from device
  {
    data[0] = Wire.read();
    data[1] = Wire.read();
    data[2] = Wire.read();
    data[3] = Wire.read();
  }

  return data[3]; //Return only last byte, configuration register
}

//Sets the mode of operation, either continuous or single shot operation
//Takes as input the desired mode, either CONTINUOUS or SINGLE_SHOT
//Returns either the result of the attempt if input is valid, or -1 if input is not valid (neither CONTINUOUS or SINGLE_SHOT)
int MCP3421::setMode(int mode) {
  int config = getConfig();  //Get current configuration register
  if(mode == CONTINUOUS || mode == SINGLE_SHOT) { //Value is either CONTINUOUS or SINGLE_SHOT and therefore valid, proceed
    config = (config & 0xEF) | (mode << 4); //Generate bit masked configuration register in order to set/clear mode bit
    Wire.beginTransmission(addr);  //Write new configuration register
    Wire.write(config);
    return Wire.endTransmission(); //Return result of attempt
  }
  else return -1; //If not valid input, return -1
}
