#include <string.h>

#include "driver/gpio.h"

#include "max31856.h"
void max31856_init(max31856_t *dev) {
  //cs_pinの初期化
  gpio_config_t io_conf = {
      .pull_down_en = 0,
      .intr_type = GPIO_INTR_DISABLE,
      .pin_bit_mask = (1ULL << dev->cs_pin),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = 0
  };
  gpio_config(&io_conf);
  gpio_set_level(dev->cs_pin, 1);

  max31856_write_register(dev, MAX31856_MASK_REG, 0x00);
  max31856_write_register(dev, MAX31856_CR0_REG, MAX31856_CR0_OCFAULT0);

  // set type K 
  uint8_t t = max31856_read_register(dev, MAX31856_CR1_REG);
  t &= 0xF0;  // Clear bits 0-3
  t |= (uint8_t)dev->type & 0x0F; // Set thermocouple type to K
  max31856_write_register(dev, MAX31856_CR1_REG, t);
}

void max31856_write_register(max31856_t *dev, uint8_t reg, uint8_t data) {

  spi_transaction_t t = {
      .length = 16,
      .tx_buffer = NULL,
      .flags = SPI_TRANS_USE_RXDATA
  };

  uint8_t tx_data[2];
  tx_data[0] = reg | 0x80;
  tx_data[1] = data;
  t.tx_buffer = tx_data;

  gpio_set_level(dev->cs_pin, 0);
  esp_err_t ret = spi_device_transmit(dev->spi, &t);
  gpio_set_level(dev->cs_pin, 1);

  // if(ret == ESP_OK) {
  //   printf("spi emit ok\n");
  // } else {
  //   printf("spi emit faild\n");
  // }
}

uint8_t max31856_read_register(max31856_t *dev, uint8_t reg) {
  uint8_t tx_data[2] = {reg & 0x7F, 0x00};  // MSB=0 で読み出し
  uint8_t rx_data[2] = {0};

  spi_transaction_t t = {
      .length = 16,              // 送信データ長（ビット単位）
      .tx_buffer = tx_data,
      .rx_buffer = rx_data
  };

  gpio_set_level(dev->cs_pin, 0);
  esp_err_t ret = spi_device_transmit(dev->spi, &t);
  gpio_set_level(dev->cs_pin, 1);
  assert(ret == ESP_OK);

  return rx_data[1];  // 2バイト目に読み出しデータが入る
}

void max31856_read_registers_N(max31856_t *dev, uint8_t reg, uint8_t *buf, size_t len) {
  uint8_t tx[len + 1];
  uint8_t rx[len + 1];

  tx[0] = reg & 0x7F;  // 読み出し命令
  memset(&tx[1], 0x00, len);  // ダミーバイト

  spi_transaction_t t = {
      .length = (len + 1) * 8,
      .tx_buffer = tx,
      .rx_buffer = rx
  };

  gpio_set_level(dev->cs_pin, 0);
  esp_err_t ret = spi_device_transmit(dev->spi, &t);
  gpio_set_level(dev->cs_pin, 1);
  assert(ret == ESP_OK);

  memcpy(buf, &rx[1], len);  // 最初のバイトはアドレス応答、2バイト目以降が実データ
}

float max31856_read_temperature(max31856_t *dev){
  // One shot trigger
  max31856_write_register(dev, MAX31856_CJTO_REG, 0x00); 
  uint8_t t = max31856_read_register(dev, MAX31856_CR0_REG);
  t &= ~MAX31856_CR0_AUTOCONVERT;            
  t |= MAX31856_CR0_1SHOT;                
  max31856_write_register(dev, MAX31856_CR0_REG, t);

  // Wait for conversion to complete
  while(max31856_read_register(dev, MAX31856_CR0_REG) & MAX31856_CR0_1SHOT) {
    vTaskDelay(pdMS_TO_TICKS(10));  // 10ms delay
  }

  uint8_t temp_buf[3];
  max31856_read_registers_N(dev, MAX31856_LTCBH_REG, temp_buf, 3);

  int32_t temp24 = (temp_buf[0] << 16) | (temp_buf[1] << 8) | temp_buf[2];
  if (temp24 & 0x800000) {
    temp24 |= 0xFF000000; // fix sign
  }
  temp24 >>= 5;

  dev->celsius_temp = temp24 * 0.0078125; // 0.0078125 = 1/128
  // printf("temp: %.4f\n", dev->celsius_temp); // 0.0078125 = 1/128
  return dev->celsius_temp; // 0.0078125 = 1/128
}