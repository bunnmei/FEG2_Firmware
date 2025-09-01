#pragma once
#include <stdint.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

typedef struct
{
  uint8_t brightness_lvl;
  int8_t temp_f_caribration;
  int8_t temp_s_caribration;
} Persistance;

typedef enum {
  PERSISTANCE_BRIGHTNESS_LVL,
  PERSISTANCE_TEMP_F_CARIBRATION,
  PERSISTANCE_TEMP_S_CARIBRATION,
} persistance_key_t;

Persistance* get_persistance(void);
void persistance_init();
void persistance_save(int8_t new_value, persistance_key_t key);