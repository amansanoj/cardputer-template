// Wokwi Custom Chip: ST7789 240x135 IPS Display Driver
// SPDX-License-Identifier: MIT

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {
  MODE_COMMAND = 0,
  MODE_DATA    = 1,
} chip_mode_t;

typedef struct {
  pin_t      cs_pin;
  pin_t      dc_pin;
  pin_t      rst_pin;
  spi_dev_t  spi;
  uint8_t    spi_buffer[2048];

  /* Framebuffer state */
  buffer_t   framebuffer;
  uint32_t   width;
  uint32_t   height;

  /* Command state machine */
  chip_mode_t mode;
  bool        mode_changing;
  uint8_t     command_code;
  uint8_t     command_size;
  uint8_t     command_index;
  uint8_t     command_buf[16];
  bool        ram_write;

  /* Memory addressing */
  uint32_t   active_column;
  uint32_t   active_page;
  uint32_t   column_start;
  uint32_t   column_end;
  uint32_t   page_start;
  uint32_t   page_end;
  uint32_t   scanning_direction;
} chip_state_t;

/* ST7789 Commands */
#define CMD_NOP      (0x00)
#define CMD_SWRESET  (0x01)
#define CMD_SLPIN    (0x10)
#define CMD_SLPOUT   (0x11)
#define CMD_NORON    (0x13)
#define CMD_INVOFF   (0x20)
#define CMD_INVON    (0x21)
#define CMD_DISPOFF  (0x28)
#define CMD_DISPON   (0x29)
#define CMD_CASET    (0x2A)
#define CMD_PASET    (0x2B)
#define CMD_RAMWR    (0x2C)
#define CMD_MADCTL   (0x36)
#define CMD_COLMOD   (0x3A)

static void chip_pin_change(void *user_data, pin_t pin, uint32_t value);
static void chip_spi_done(void *user_data, uint8_t *buffer, uint32_t count);

void chip_reset(chip_state_t *chip) {
  chip->ram_write = false;
  chip->active_column = 0;
  chip->active_page = 0;
  chip->column_start = 0;
  chip->column_end = (chip->width > 0) ? (chip->width - 1) : 239;
  chip->page_start = 0;
  chip->page_end = (chip->height > 0) ? (chip->height - 1) : 134;
  chip->command_size = 0;
  chip->command_index = 0;
}

void chip_init(void) {
  chip_state_t *chip = (chip_state_t *)malloc(sizeof(chip_state_t));
  if (!chip) return;

  chip->mode_changing = false;
  chip->mode = MODE_COMMAND;

  const pin_watch_config_t watch_config = {
    .user_data = chip,
    .edge = BOTH,
    .pin_change = chip_pin_change,
  };

  chip->cs_pin = pin_init("CS", INPUT_PULLUP);
  pin_watch(chip->cs_pin, &watch_config);

  chip->dc_pin = pin_init("DC", INPUT);
  pin_watch(chip->dc_pin, &watch_config);

  chip->rst_pin = pin_init("RST", INPUT_PULLUP);
  pin_watch(chip->rst_pin, &watch_config);

  const spi_config_t spi_config = {
    .user_data = chip,
    .sck = pin_init("SCL", INPUT),
    .mosi = pin_init("SDA", INPUT),
    .miso = NO_PIN,
    .mode = 0,
    .done = chip_spi_done,
  };
  chip->spi = spi_init(&spi_config);

  chip->framebuffer = framebuffer_init(&chip->width, &chip->height);
  if (chip->width == 0) chip->width = 240;
  if (chip->height == 0) chip->height = 135;

  chip_reset(chip);

  // Initialize display canvas with pitch-black (#000000 / Alpha 0xFF)
  uint32_t black = 0xFF000000;
  for (uint32_t i = 0; i < chip->width * chip->height; i++) {
    buffer_write(chip->framebuffer, i * sizeof(black), &black, sizeof(black));
  }

  printf("[ST7789] Driver initialized: %ux%u\n", chip->width, chip->height);
}

static inline uint32_t rgb565_to_rgba(uint16_t value) {
  uint32_t r = ((value >> 11) & 0x1F) * 255 / 31;
  uint32_t g = ((value >> 5) & 0x3F) * 255 / 63;
  uint32_t b = (value & 0x1F) * 255 / 31;
  return 0xFF000000 | (b << 16) | (g << 8) | r;
}

void chip_pin_change(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *chip = (chip_state_t *)user_data;

  // Handle Chip Select
  if (pin == chip->cs_pin) {
    if (value == LOW) {
      spi_start(chip->spi, chip->spi_buffer, sizeof(chip->spi_buffer));
    } else {
      spi_stop(chip->spi);
    }
  }

  // Handle Data/Command line change
  if (pin == chip->dc_pin && chip->mode != (chip_mode_t)value) {
    if (pin_read(chip->cs_pin) == LOW) {
      // Flush current SPI buffer with the previous mode before switching
      chip->mode_changing = true;
      spi_stop(chip->spi);
      chip->mode = (chip_mode_t)value;
      chip->mode_changing = false;
      spi_start(chip->spi, chip->spi_buffer, sizeof(chip->spi_buffer));
    } else {
      chip->mode = (chip_mode_t)value;
    }
  }

  // Handle Hardware Reset
  if (pin == chip->rst_pin && value == LOW) {
    spi_stop(chip->spi);
    chip_reset(chip);
  }
}

int command_args_size(uint8_t command_code) {
  switch (command_code) {
    case CMD_MADCTL:
    case CMD_COLMOD:  return 1;
    case CMD_CASET:
    case CMD_PASET:   return 4;
    default:          return 0;
  }
}

void execute_command(chip_state_t *chip) {
  switch (chip->command_code) {
    case CMD_SWRESET:
      chip_reset(chip);
      break;

    case CMD_RAMWR:
      chip->ram_write = true;
      chip->active_column = chip->column_start;
      chip->active_page = chip->page_start;
      break;

    case CMD_MADCTL:
      chip->scanning_direction = chip->command_buf[0];
      break;

    case CMD_CASET: {
      uint16_t start = (chip->command_buf[0] << 8) | chip->command_buf[1];
      uint16_t end   = (chip->command_buf[2] << 8) | chip->command_buf[3];
      // Normalize Adafruit 135x240 landscape offset if present (colstart = 40)
      if (start >= 40 && start < 320) {
        start -= 40;
        end   -= 40;
      }
      chip->column_start  = start;
      chip->active_column = start;
      chip->column_end    = end;
      break;
    }

    case CMD_PASET: {
      uint16_t start = (chip->command_buf[0] << 8) | chip->command_buf[1];
      uint16_t end   = (chip->command_buf[2] << 8) | chip->command_buf[3];
      // Normalize Adafruit 135x240 landscape offset if present (rowstart = 52)
      if (start >= 52 && start < 320) {
        start -= 52;
        end   -= 52;
      }
      chip->page_start  = start;
      chip->active_page = start;
      chip->page_end    = end;
      break;
    }

    default:
      break;
  }
}

void process_command(chip_state_t *chip, uint8_t *buffer, uint32_t buffer_size) {
  chip->ram_write = false;
  for (uint32_t i = 0; i < buffer_size; i++) {
    chip->command_code  = buffer[i];
    chip->command_size  = command_args_size(chip->command_code);
    chip->command_index = 0;
    if (chip->command_size == 0) {
      execute_command(chip);
    }
  }
}

void process_command_args(chip_state_t *chip, uint8_t *buffer, uint32_t buffer_size) {
  for (uint32_t i = 0; i < buffer_size; i++) {
    if (chip->command_index < chip->command_size) {
      chip->command_buf[chip->command_index++] = buffer[i];
      if (chip->command_size == chip->command_index) {
        execute_command(chip);
      }
    }
  }
}

void process_data(chip_state_t *chip, const uint16_t *buffer16, uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    int32_t x = chip->active_column;
    int32_t y = chip->active_page;

    uint16_t raw_pixel = (buffer16[i] >> 8) | (buffer16[i] << 8);
    uint32_t color = rgb565_to_rgba(raw_pixel);

    if (x >= 0 && x < (int32_t)chip->width && y >= 0 && y < (int32_t)chip->height) {
      uint32_t pix_index = y * chip->width + x;
      buffer_write(chip->framebuffer, pix_index * sizeof(color), &color, sizeof(color));
    }

    chip->active_column++;
    if (chip->active_column > chip->column_end) {
      chip->active_column = chip->column_start;
      chip->active_page++;
      if (chip->active_page > chip->page_end) {
        chip->active_page = chip->page_start;
      }
    }
  }
}

void chip_spi_done(void *user_data, uint8_t *buffer, uint32_t count) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (count == 0) return;

  if (chip->mode == MODE_DATA) {
    if (chip->ram_write) {
      process_data(chip, (const uint16_t *)buffer, count / 2);
    } else {
      process_command_args(chip, buffer, count);
    }
  } else {
    process_command(chip, buffer, count);
  }

  // Only auto-restart SPI if we are NOT in the middle of a DC mode transition
  if (!chip->mode_changing && pin_read(chip->cs_pin) == LOW) {
    spi_start(chip->spi, chip->spi_buffer, sizeof(chip->spi_buffer));
  }
}
