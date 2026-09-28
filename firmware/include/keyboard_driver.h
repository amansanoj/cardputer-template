#pragma once
#include <Arduino.h>
#include <Wire.h>

class KeyboardDriver {
public:
  KeyboardDriver();
  bool begin();
  char getKey();
  bool isHardwarePresent() const { return hardwarePresent; }

private:
  bool hardwarePresent;
  uint8_t readReg(uint8_t reg);
  void writeReg(uint8_t reg, uint8_t val);
  char mapMatrixKey(uint8_t row, uint8_t col);
};
