#pragma once

typedef enum {
  TEMP_ONE,
  TEMP_TWO
} temp_num_t;

float get_temp(temp_num_t temp_num);
void update_temp(float temp, temp_num_t temp_num);