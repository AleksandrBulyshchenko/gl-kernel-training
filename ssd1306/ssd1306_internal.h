#pragma once

#define SSD1306_DATA                                          0x40
#define SSD1306_WRITE_CMD                                     0x00

#define SSD1306_CMD_CHARGE_PUMP_SETTING                       0x8D
#define SSD1306_CMD_CHARGE_ENABLE                             0x14

#define SSD1306_CMD_DISPLAY_ON                                0xAF
#define SSD1306_CMD_DISPLAY_OFF                               0xAE
#define SSD1306_CMD_SCAN_DEC                                  0xC8
#define SSD1306_CMD_SET_CONTRAST                              0x81
#define SSD1306_CMD_SET_NORMAL_DISPLAY                        0xA6
#define SSD1306_CMD_SET_DISPLAY_OFFSET                        0xD3
#define SSD1306_CMD_SET_DISPLAY_CLOCK_DIV                     0xD5
#define SSD1306_CMD_SET_PRECHARGE                             0xD9
#define SSD1306_CMD_SET_COM_PINS                              0xDA
#define SSD1306_CMD_SET_VCOM_DETECT                           0xDB

#define SSD1306_DISPLAY_WIDTH                                 128
#define SSD1306_DISPLAY_HEIGHT                                64

#define SSD1306_DISPLAY_MEMORY_MODE                           0x20
#define SSD1306_DISPLAY_MEMORY_MODE_VAL_HORIZONAL             0
#define SSD1306_DISPLAY_MEMORY_MODE_VAL_VERTICAL              1
#define SSD1306_DISPLAY_MEMORY_MODE_VAL_PAGE                  2

#define SSD1306_DISPLAY_COLUMN_ADDR                           0x21
#define SSD1306_DISPLAY_PAGE_ADDR                             0x22
