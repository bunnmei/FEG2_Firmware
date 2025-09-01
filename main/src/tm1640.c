#include <math.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tm1640.h"

#define not_a_num 0b00000000
#define dot_mask 0b10000000
#define minus 0b01000000

// 0b DP G F E D C B A
//                   
//       A            
//     ーーー         
//  F |     | B       
//    |  G  |        
//     ーーー        
//    |     |       
//  E |     | C     
//     ーーー    ○ DP        
//       D            

static const int font_7seg_msg[] = {
    0b01011100, // o = over
    0b01111001, // e = error
};

static const int font_7seg[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00100111, // 7
    0b01111111, // 8
    0b01101111, // 9
};

static const uint8_t brightness_lvl_list[] = {
    0b10000000, // OFF 0
    0b10001000, // 1/16 Duty 1
    0b10001001, // 2/16 Duty 2
    0b10001010, // 4/16 Duty 3
    0b10001011, // 10/16 Duty 4
    0b10001100, // 11/16 Duty 5
    0b10001101, // 12/16 Duty 6
    0b10001110, // 13/16 Duty 7
    0b10001111, // 14/16 Duty 8
};

void TM1640_init(TM1640 *stru, uint8_t din, uint8_t scl) {
  // TM1640初期化
  stru->DIN = din;
  stru->SCL = scl;

  // DIN,SCL初期化
  gpio_reset_pin(din);
  gpio_set_direction(din, GPIO_MODE_OUTPUT);
  gpio_set_level(din, 1);

  gpio_reset_pin(scl);
  gpio_set_direction(scl, GPIO_MODE_OUTPUT);
  gpio_set_level(scl, 1);
}

void TM1640_start(TM1640 *stru) {
  gpio_set_level(stru->DIN, 0);
  vTaskDelay(pdMS_TO_TICKS(1)); 
}

void TM1640_end(TM1640 *stru) {
  vTaskDelay(pdMS_TO_TICKS(1)); 
  gpio_set_level(stru->SCL, 0);
  gpio_set_level(stru->DIN, 0);
  gpio_set_level(stru->SCL, 1);
  gpio_set_level(stru->DIN, 1);
}

void send_data(TM1640 *stru, uint8_t data) {
  for (int i = 0; i < 8; i++){
    gpio_set_level(stru->SCL, 0);
    vTaskDelay(pdMS_TO_TICKS(1)); 
    gpio_set_level(stru->DIN, (data>>i)&0x01);
    gpio_set_level(stru->SCL, 1);
  }
};

void slice_num(TM1640 *stru, float num, uint8_t temp_num) {

  uint8_t minus_flag = 0;
  if(num > 1000) {
    if (temp_num == 1) {
      stru->send_num[0] = font_7seg_msg[0]; // over
      stru->send_num[1] = font_7seg[9];
      stru->send_num[2] = font_7seg[9];
      stru->send_num[3] = font_7seg[9];
    } else if (temp_num == 2) {
      stru->send_num[4] = font_7seg_msg[0]; // over
      stru->send_num[5] = font_7seg[9];
      stru->send_num[6] = font_7seg[9];
      stru->send_num[7] = font_7seg[9];
    }
      return;
  }
  if(num <= -100) {
    if (temp_num == 1) {
      stru->send_num[0] = font_7seg_msg[0]; // over
      stru->send_num[1] = minus;
      stru->send_num[2] = font_7seg[9];
      stru->send_num[3] = font_7seg[9];
    } else if (temp_num == 2) {
      stru->send_num[4] = font_7seg_msg[0]; // over
      stru->send_num[5] = minus;
      stru->send_num[6] = font_7seg[9];
      stru->send_num[7] = font_7seg[9];
    }
    return;
  }

  float display_num = num;
  if (num < 0) {
    display_num = num * -1; // 負の数を正の数に変換
    minus_flag = 1; // マイナスフラグを立てる
    stru->send_num[0] = minus; // マイナス記号を設定
  } 
  
  float num_r = roundf(display_num * 10.0f) / 10.0f; // 少数下二桁を四捨五入
  uint8_t num_h = num_r / 100; // 百の位
  uint8_t num_t = (((int)(num_r) % 100) / 10); // 十の位
  uint8_t num_o = (int)(num_r) % 10; // 一の位
  uint8_t num_f = (int)(num_r * 10) % 10; // 少数の第一位

  if (temp_num == 1) {
    // 1桁目の少数点を表示
    if (minus_flag) {
      stru->send_num[0] = minus; // マイナス記号を設定
    } else {
      stru->send_num[0] = font_7seg[num_h];
    }
    stru->send_num[1] = font_7seg[num_t];
    stru->send_num[2] = font_7seg[num_o] | dot_mask;
    stru->send_num[3] = font_7seg[num_f];

  } else if (temp_num == 2) {
    // 2桁目の少数点を表示
    if (minus_flag) {
      stru->send_num[4] = minus; // マイナス記号を設定
    } else {
      stru->send_num[4] = font_7seg[num_h];
    }
    stru->send_num[5] = font_7seg[num_t];
    stru->send_num[6] = font_7seg[num_o] | dot_mask; // 少数点を表示
      stru->send_num[7] = font_7seg[num_f];
  } else {
    stru->send_num[0] = font_7seg[num_h];
    stru->send_num[1] = font_7seg[num_t];
    stru->send_num[2] = font_7seg[num_o];
    stru->send_num[3] = font_7seg[num_f];
  }
};

void TM1640_send_auto(TM1640 *stru, float num) {

  slice_num(stru, num, 1);
  // set_num_stru(stru);
  vTaskDelay(pdMS_TO_TICKS(1)); 
  TM1640_start(stru);
  send_data(stru, 0x40); //mode 0x40 or 0x44
  TM1640_end(stru);

  vTaskDelay(pdMS_TO_TICKS(1)); 
  TM1640_start(stru);
  send_data(stru, 0b11000000); //addr
  send_data(stru, stru->send_num[0]);
  send_data(stru, stru->send_num[1]);
  send_data(stru, stru->send_num[2]);
  send_data(stru, stru->send_num[3]);
  TM1640_end(stru);

  vTaskDelay(pdMS_TO_TICKS(1)); 
  TM1640_start(stru);
  send_data(stru, 0b10001100); //brightness
  TM1640_end(stru);
  vTaskDelay(pdMS_TO_TICKS(1)); 
  
};

void TM1640_send_auto_two_num(TM1640 *stru, float num, float num2, uint8_t brightness_lvl) {
  slice_num(stru, num, 1);
  slice_num(stru, num2, 2);
  // set_num_stru(stru);
  vTaskDelay(pdMS_TO_TICKS(1)); 
  TM1640_start(stru);
  send_data(stru, 0x40); //mode 0x40 or 0x44
  TM1640_end(stru);

  vTaskDelay(pdMS_TO_TICKS(1)); 
  TM1640_start(stru);
  send_data(stru, 0b11000000); //addr
  send_data(stru, stru->send_num[0]);
  send_data(stru, stru->send_num[1]);
  send_data(stru, stru->send_num[2]);
  send_data(stru, stru->send_num[3]);
  send_data(stru, stru->send_num[4]);
  send_data(stru, stru->send_num[5]);
  send_data(stru, stru->send_num[6]);
  send_data(stru, stru->send_num[7]);
  TM1640_end(stru);

  vTaskDelay(pdMS_TO_TICKS(1)); 
  TM1640_start(stru);
  send_data(stru, brightness_lvl_list[brightness_lvl]); //brightness
  TM1640_end(stru);
  vTaskDelay(pdMS_TO_TICKS(1)); 
}