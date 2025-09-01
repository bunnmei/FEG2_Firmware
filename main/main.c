#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "common.h"
#include "gap.h"
#include "gatt_svc.h"
#include "temp.h"
#include "max31856.h"
#include "tm1640.h"
#include "usb_send.h"
#include "persistance.h"

#define PIN_NUM_MISO 5
#define PIN_NUM_MOSI 6
#define PIN_NUM_CLK  4
#define PIN_NUM_CS_F   7
#define PIN_NUM_CS_S   8

#define DIN 20
#define SCL 21

typedef struct {
  max31856_t *f_temp;
  max31856_t *s_temp;
} AppStruct;

spi_device_handle_t
spi_init_cs_manual(void);

/* Library function declarations */
void ble_store_config_init(void);

/* Private function declarations */
static void on_stack_reset(int reason);
static void on_stack_sync(void);
static void nimble_host_config_init(void);
static void nimble_host_task(void *param);

static void on_stack_reset(int reason) {
  ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}

static void on_stack_sync(void) {
  adv_init();
}

static void nimble_host_config_init(void) {
  ble_hs_cfg.reset_cb = on_stack_reset;
  ble_hs_cfg.sync_cb = on_stack_sync;
  ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  ble_store_config_init();
}

static void nimble_host_task(void *param) {
  ESP_LOGI(TAG, "nimble host task has been started!");
  nimble_port_run();
  vTaskDelete(NULL);
}

static void temp_task(void *param) {
  AppStruct *app_struct = (AppStruct *)param;
  spi_device_handle_t spi = spi_init_cs_manual();
  Persistance* persistance = get_persistance();

  max31856_t dev_f = {
    .spi = spi,
    .type = MAX31856_TCTYPE_K, // Thermocouple type K
    .cs_pin = PIN_NUM_CS_F,
    .celsius_temp = 0.0f
  };
  max31856_t dev_s = {
    .spi = spi,
    .type = MAX31856_TCTYPE_K, // Thermocouple type K
    .cs_pin = PIN_NUM_CS_S,
    .celsius_temp = 0.0f
  };

  app_struct->f_temp = &dev_f;
  app_struct->s_temp = &dev_s;

  max31856_init(&dev_f);
  max31856_init(&dev_s);    
  
  vTaskDelay(pdMS_TO_TICKS(100)); // 電源安定待ち 
  TM1640 tm1640;
  TM1640_init(&tm1640, DIN, SCL);
  // int nnnn = 1005;
  while (1)
  {
    float temp_f = max31856_read_temperature(&dev_f);
    float temp_s = max31856_read_temperature(&dev_s);
    float temp_f_carib = (temp_f + persistance->temp_f_caribration/10.0f);
    float temp_s_carib = (temp_s + persistance->temp_s_caribration/10.0f);
    // printf("Front Temp: %.3f C\n", persistance->temp_f_caribration/10.0f);
    // printf("Front Temp: %.2f C, Side Temp: %.2f C\n", temp_f_carib, temp_s_carib);
    TM1640_send_auto_two_num(
      &tm1640, 
       temp_f_carib ,
       temp_s_carib ,
      persistance->brightness_lvl
    );
    update_temp( temp_f_carib , TEMP_ONE);
    update_temp(temp_s_carib, TEMP_TWO);
    send_temp_indication(); // Send indication to notify temperature change
    // nnnn -= 1; // Increment for testing purpose
    vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second delay
  }
}

void app_main(void){

  persistance_init(); // Bring persistence initialization

  int rc;
  esp_err_t ret;
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
  }
  if (ret != ESP_OK) {
      ESP_LOGE(TAG, "failed to initialize nvs flash, error code: %d ", ret);
      return;
  }

  ret = nimble_port_init();
  if (ret != ESP_OK) {
      ESP_LOGE(TAG, "failed to initialize nimble stack, error code: %d ",
                ret);
      return;
  }

  
    /* GAP service initialization */
    rc = gap_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to initialize GAP service, error code: %d", rc);
        return;
    }

    /* GATT server initialization */
    rc = gatt_svc_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to initialize GATT server, error code: %d", rc);
        return;
    }

    nimble_host_config_init();

    

    AppStruct app_struct = {
      .f_temp = NULL,
      .s_temp = NULL,
    };

    xTaskCreate(nimble_host_task, "NimBLE Host", 4*1024, NULL, 5, NULL);
    xTaskCreate(temp_task, "Max31856", 4*1024, &app_struct, 5, NULL);

    char buf[MESSAGE_BUF_SIZE] = {0};
    // vTaskDelay(pdMS_TO_TICKS(1000));
    while(1) {
        if (fgets(buf, sizeof(buf), stdin) != NULL) {
          if (app_struct.f_temp != NULL && app_struct.s_temp != NULL)
          { 
            send_to_artisan(buf, 
              app_struct.f_temp->celsius_temp,
              app_struct.s_temp->celsius_temp);
          }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

spi_device_handle_t spi_init_cs_manual(void){

  spi_device_handle_t spi;

  spi_bus_config_t buscfg = {
    .miso_io_num = PIN_NUM_MISO,
    .mosi_io_num = PIN_NUM_MOSI,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 0};

  spi_device_interface_config_t devcfg = {
  .clock_speed_hz = (APB_CLK_FREQ / 10), // 8 Mhz
  .dummy_bits = 0,
  .mode = 1,
  .flags = 0,
  .spics_io_num = -1, // Manually Control CS
  .queue_size = 1,
  };

  spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED);
  spi_bus_add_device(SPI2_HOST, &devcfg, &spi);

 
  return spi;
}