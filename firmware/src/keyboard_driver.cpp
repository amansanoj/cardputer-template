#if __has_include("keyboard_driver.h")
#include "keyboard_driver.h"
#else
#include "../include/keyboard_driver.h"
#endif

#define TCA8418_I2C_ADDR    0x34
#define REG_CFG             0x01
#define REG_INT_STAT        0x02
#define REG_KEY_LCK_EC      0x03
#define REG_KEY_EVENT_A     0x04
#define REG_KP_GPIO1        0x1D // ROW0..ROW6 (0x7F)
#define REG_KP_GPIO2        0x1E // COL0..COL7 (0xFF)

// Cardputer physical layout mapped to 7x8 electrical matrix
static const char keyMap[7][8] = {
  { '`', '1', '2', '3', '4', '5', '6', '7' },
  { '8', '9', '0', '-', '=', '\b', '\t', 'q' },
  { 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o' },
  { 'p', '[', ']', '\\', ' ', 'a', 's', 'd' },
  { 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'' },
  { '\n', 'z', 'x', 'c', 'v', 'b', 'n', 'm' },
  { ',', '.', '/', ' ', ' ', ' ', ' ', ' ' }
};

KeyboardDriver::KeyboardDriver() : hardwarePresent(false) {}

uint8_t KeyboardDriver::readReg(uint8_t reg) {
  Wire.beginTransmission(TCA8418_I2C_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)TCA8418_I2C_ADDR, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0;
}

void KeyboardDriver::writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(TCA8418_I2C_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

bool KeyboardDriver::begin() {
#if defined(TARGET_WOKWI_SIMULATOR)
  // Wokwi Simulator uses USB Serial keyboard input; do NOT touch pins 8/9 (TFT RST/DC)
  hardwarePresent = false;
  Serial.println("[Keyboard] Wokwi Simulator active. Serial keyboard ready.");
  return false;
#else
  // Cardputer-Adv: TCA8418 keypad matrix on dedicated I2C pins (SDA=2, SCL=1)
  Wire.begin(2, 1, 400000);
  Wire.beginTransmission(TCA8418_I2C_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println("[Keyboard] TCA8418 not detected. Serial keyboard fallback active.");
    hardwarePresent = false;
    return false;
  }
#endif

  // Configure 7 rows and 8 columns for keypad scanning
  writeReg(REG_KP_GPIO1, 0x7F);
  writeReg(REG_KP_GPIO2, 0xFF);

  // Enable keypad events & FIFO auto-increment
  writeReg(REG_CFG, 0x01);

  // Clear pending interrupt flags
  writeReg(REG_INT_STAT, 0x01);

  hardwarePresent = true;
  Serial.println("[Keyboard] TCA8418 I2C keyboard initialized on Cardputer-Adv!");
  return true;
}

char KeyboardDriver::mapMatrixKey(uint8_t row, uint8_t col) {
  if (row < 7 && col < 8) {
    return keyMap[row][col];
  }
  return 0;
}

char KeyboardDriver::getKey() {
  // 1. Check physical TCA8418 keyboard (on Cardputer-Adv)
  if (hardwarePresent) {
    uint8_t count = readReg(REG_KEY_LCK_EC) & 0x0F;
    while (count > 0) {
      uint8_t event = readReg(REG_KEY_EVENT_A);
      writeReg(REG_INT_STAT, 0x01); // clear interrupt
      count--;

      bool isPress = (event & 0x80) != 0;
      if (isPress) {
        uint8_t keyCode = (event & 0x7F);
        if (keyCode >= 1) {
          uint8_t row = (keyCode - 1) / 10;
          uint8_t col = (keyCode - 1) % 10;
          char c = mapMatrixKey(row, col);
          if (c) {
            Serial.printf("[Key] Physical key pressed: '%c' (code %u)\n", c, keyCode);
            return c;
          }
        }
      }
    }
  }

  // 2. Check Serial input (for Wokwi simulation and USB console)
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') return 0;
    Serial.printf("[Key] Serial key received: '%c'\n", c);
    return c;
  }

  return 0;
}
