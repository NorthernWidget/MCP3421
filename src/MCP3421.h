/******************************************************************************
MCP3421.h
Library for MCP3421 ADC
Bobby Schulz @ Northern Widget LLC & UMN
2/20/2018

MCP3421 is a 18 bit ADC, PGA, and various modes of resolution operation in order to facilitate various data rates

"Simplicity is prerquisite for reliability"
-Edsger W. Dijkstra

Distributed as-is; no warranty is given.
******************************************************************************/

#ifndef MCP3421_h
#define MCP3421_h

#include "Arduino.h"

#define CONTINUOUS 1
#define SINGLE_SHOT 0

class MCP3421
{
  public:

    MCP3421(int _addr);
    MCP3421(void);

    int begin(int _addr);

    int begin(void);

    long getVoltageRaw(bool waitForVal = true);

    float getVoltage(bool waitForVal = true);

    int setGain(int gain);

    int getGain();

    int setResolution(int resolution);

    int getResolution();

    int setMode(int mode);

  private:
    int addr;
    int getConfig();

};

#endif
