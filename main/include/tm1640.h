#pragma once
#include <stdint.h>

typedef struct
{
  uint8_t DIN;
  uint8_t SCL;
  uint8_t send_num[8];
  uint8_t font_7seg[16];
} TM1640;


void TM1640_init(TM1640 *stru, uint8_t din, uint8_t scl);
void TM1640_start(TM1640 *stru);
void TM1640_end(TM1640 *stru);
void send_data(TM1640 *stru, uint8_t data);
void TM1640_send_auto(TM1640 *stru, float num);
void TM1640_send_auto_two_num(TM1640 *stru, float num, float num2, uint8_t brightness_lvl);
void slice_num(TM1640 *stru, float num, uint8_t temp_num);

