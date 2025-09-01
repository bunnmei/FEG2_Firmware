#include "persistance.h"

static Persistance persistance = {
    .brightness_lvl = 4, // Default brightness level
    .temp_f_caribration = 0, // Default front temperature calibration
    .temp_s_caribration = 0  // Default side temperature calibration
};

Persistance* get_persistance() {
  return &persistance;
};

void persistance_init(){
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      // パーティションの再初期化
      ESP_ERROR_CHECK(nvs_flash_erase());
      ESP_ERROR_CHECK(nvs_flash_init());
  }

  int8_t init_flag = 0; 
  nvs_handle_t my_handle;
  // 名前空間を開く
  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  ESP_ERROR_CHECK(err);

  if (nvs_get_i8(my_handle, "init_flag", &init_flag) == ESP_ERR_NVS_NOT_FOUND) {
      // 初期化フラグが見つからない場合、初期化処理を行う
      printf("First run, initializing...\n");
      nvs_set_i8(my_handle, "tm1640_brightness", 4);
      nvs_set_i8(my_handle, "temp_f_calib", 0);
      nvs_set_i8(my_handle, "temp_s_calib", 0);
      init_flag = 1; // 初期化フラグを設定
      nvs_set_i8(my_handle, "init_flag", init_flag);
      nvs_commit(my_handle); // 必ずコミット！

      persistance.brightness_lvl = 4; // 初期値を設定
      persistance.temp_f_caribration = 0; // 初期値を設定
      persistance.temp_s_caribration = 0; // 初期値を設定
  } else {
      printf("Not the first run, init_flag: %d\n", init_flag);
      // nvs_set_i8(my_handle, "tm1640_brightness", 4);
      // nvs_set_i8(my_handle, "temp_f_calib", 0);
      // nvs_set_i8(my_handle, "temp_s_calib", 0);
      // nvs_commit(my_handle); // 必ずコミット！
      // 初期化フラグが存在する場合、値を読み込む
      int8_t brightness_lvl;
      int8_t temp_f_calibration;
      int8_t temp_s_calibration;
      nvs_get_i8(my_handle, "tm1640_bright", &brightness_lvl);
      nvs_get_i8(my_handle, "temp_f_calib", &temp_f_calibration);
      nvs_get_i8(my_handle, "temp_s_calib", &temp_s_calibration);  
      persistance.brightness_lvl = brightness_lvl; // 読み込んだ値を設定
      persistance.temp_f_caribration = temp_f_calibration; // 読み込んだ値を設定
      persistance.temp_s_caribration = temp_s_calibration; // 読み込んだ値を設定
  
    }
    nvs_close(my_handle); 
}

int8_t check_new_val(int8_t new_value, persistance_key_t key) {
  switch (key)
  {
  case PERSISTANCE_BRIGHTNESS_LVL:
    if (new_value <= 0) {
      return 0; // OFF
    } else if (new_value >= 8) {
      return 8; // MAX brightness level
    } else {
      return new_value; // Return the new value as is
    }
    break;
  
  default:
    if (new_value <= -50) {
      return -50; // Minimum calibration value
    } else if (new_value > 50) {
      return 50; // Maximum calibration value
    } else {
      return new_value; // Return the new value as is
    }
    break;
  }
}

void persistance_save(int8_t new_value, persistance_key_t key) {
  esp_err_t err;
  nvs_handle_t my_handle;

  // 名前空間を開く
  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  ESP_ERROR_CHECK(err);
 
  int8_t new_value_checked = check_new_val(new_value, key); // 新しい値をチェック
  switch (key)
  {
    case PERSISTANCE_BRIGHTNESS_LVL:
      err = nvs_set_i8(my_handle, "tm1640_bright", new_value_checked);
      break;
    case PERSISTANCE_TEMP_F_CARIBRATION:
      err = nvs_set_i8(my_handle, "temp_f_calib", new_value_checked);
      break;
    case PERSISTANCE_TEMP_S_CARIBRATION:
      err = nvs_set_i8(my_handle, "temp_s_calib", new_value_checked);
      break;

    default:
      ESP_LOGE("Persistance", "Unknown key");
      nvs_close(my_handle);
      return;
    }

  if (err != ESP_OK) {
    ESP_LOGE("Persistance", "Failed to save value: %s", esp_err_to_name(err));
  } else {
    // 保存した値を構造体に反映
    switch (key) {
      case PERSISTANCE_BRIGHTNESS_LVL:
        persistance.brightness_lvl = new_value_checked;
        break;
      case PERSISTANCE_TEMP_F_CARIBRATION:
        persistance.temp_f_caribration = new_value_checked;
        break;
      case PERSISTANCE_TEMP_S_CARIBRATION:
        persistance.temp_s_caribration = new_value_checked;
        break;
      default:
        ESP_LOGE("Persistance", "Unknown key");
        break;
    }
    nvs_commit(my_handle); // 必ずコミット！
  }
  
  nvs_close(my_handle); 
}
