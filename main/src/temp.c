#include "temp.h"
static float temp_value_one = 0.0f;
static float temp_value_two = 0.0f;


float get_temp(temp_num_t temp_num) {
  if (temp_num == TEMP_ONE) { 
    return temp_value_one;
  } else if (temp_num == TEMP_TWO) {
    return temp_value_two;
  }
  return 0.0f; // Default return value if temp_num is invalid
}

void update_temp(float temp, temp_num_t temp_num) {
  if (temp_num == TEMP_TWO) {
    temp_value_two = temp;
    return;
  } else if (temp_num == TEMP_ONE) {
    temp_value_one = temp;
    return;
  }
  return; // Do nothing if temp_num is invalid
}