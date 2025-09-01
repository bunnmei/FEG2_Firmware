#pragma once
#include "driver/spi_master.h"

#define MAX31856_CR0_REG 0x00         ///< Config 0 register
#define MAX31856_CR0_AUTOCONVERT 0x80 ///< Config 0 Auto convert flag
#define MAX31856_CR0_1SHOT 0x40       ///< Config 0 one shot convert flag
#define MAX31856_CR0_OCFAULT1 0x20    ///< Config 0 open circuit fault 1 flag
#define MAX31856_CR0_OCFAULT0 0x10    ///< Config 0 open circuit fault 0 flag
#define MAX31856_CR0_CJ 0x08          ///< Config 0 cold junction disable flag
#define MAX31856_CR0_FAULT 0x04       ///< Config 0 fault mode flag
#define MAX31856_CR0_FAULTCLR 0x02    ///< Config 0 fault clear flag

#define MAX31856_CR1_REG 0x01  ///< Config 1 register
#define MAX31856_MASK_REG 0x02 ///< Fault Mask register
#define MAX31856_CJHF_REG 0x03 ///< Cold junction High temp fault register
#define MAX31856_CJLF_REG 0x04 ///< Cold junction Low temp fault register
#define MAX31856_LTHFTH_REG                                                    \
  0x05 ///< Linearized Temperature High Fault Threshold Register, MSB
#define MAX31856_LTHFTL_REG                                                    \
  0x06 ///< Linearized Temperature High Fault Threshold Register, LSB
#define MAX31856_LTLFTH_REG                                                    \
  0x07 ///< Linearized Temperature Low Fault Threshold Register, MSB
#define MAX31856_LTLFTL_REG                                                    \
  0x08 ///< Linearized Temperature Low Fault Threshold Register, LSB
#define MAX31856_CJTO_REG 0x09  ///< Cold-Junction Temperature Offset Register
#define MAX31856_CJTH_REG 0x0A  ///< Cold-Junction Temperature Register, MSB
#define MAX31856_CJTL_REG 0x0B  ///< Cold-Junction Temperature Register, LSB
#define MAX31856_LTCBH_REG 0x0C ///< Linearized TC Temperature, Byte 2
#define MAX31856_LTCBM_REG 0x0D ///< Linearized TC Temperature, Byte 1
#define MAX31856_LTCBL_REG 0x0E ///< Linearized TC Temperature, Byte 0
#define MAX31856_SR_REG 0x0F    ///< Fault Status Register

#define MAX31856_FAULT_CJRANGE                                                 \
  0x80 ///< Fault status Cold Junction Out-of-Range flag
#define MAX31856_FAULT_TCRANGE                                                 \
  0x40 ///< Fault status Thermocouple Out-of-Range flag
#define MAX31856_FAULT_CJHIGH                                                  \
  0x20 ///< Fault status Cold-Junction High Fault flag
#define MAX31856_FAULT_CJLOW 0x10 ///< Fault status Cold-Junction Low Fault flag
#define MAX31856_FAULT_TCHIGH                                                  \
  0x08 ///< Fault status Thermocouple Temperature High Fault flag
#define MAX31856_FAULT_TCLOW                                                   \
  0x04 ///< Fault status Thermocouple Temperature Low Fault flag
#define MAX31856_FAULT_OVUV                                                    \
  0x02 ///< Fault status Overvoltage or Undervoltage Input Fault flag
#define MAX31856_FAULT_OPEN                                                    \
  0x01 ///< Fault status Thermocouple Open-Circuit Fault flag

/** Noise filtering options enum. Use with setNoiseFilter() */
typedef enum {
  MAX31856_NOISE_FILTER_50HZ,
  MAX31856_NOISE_FILTER_60HZ
} max31856_noise_filter_t;

/** Multiple types of thermocouples supported */
typedef enum {
  MAX31856_TCTYPE_B = 0b0000,
  MAX31856_TCTYPE_E = 0b0001,
  MAX31856_TCTYPE_J = 0b0010,
  MAX31856_TCTYPE_K = 0b0011,
  MAX31856_TCTYPE_N = 0b0100,
  MAX31856_TCTYPE_R = 0b0101,
  MAX31856_TCTYPE_S = 0b0110,
  MAX31856_TCTYPE_T = 0b0111,
  MAX31856_VMODE_G8 = 0b1000,
  MAX31856_VMODE_G32 = 0b1100,
} max31856_thermocoupletype_t;

/** Temperature conversion mode */
typedef enum {
  MAX31856_ONESHOT,
  MAX31856_ONESHOT_NOWAIT,
  MAX31856_CONTINUOUS
} max31856_conversion_mode_t;

// --- add ---

typedef struct {
  spi_device_handle_t spi; ///< SPI device handle
  max31856_thermocoupletype_t type; ///< Thermocouple type
  uint8_t cs_pin; ///< Chip select pin number
  float celsius_temp;
} max31856_t;

void max31856_init(max31856_t *dev);
void max31856_write_register(max31856_t *dev, uint8_t reg, uint8_t data);
uint8_t max31856_read_register(max31856_t *dev, uint8_t reg);
void max31856_read_registers_N(max31856_t *dev, uint8_t reg, uint8_t *buf, size_t len);
float max31856_read_temperature(max31856_t *dev);