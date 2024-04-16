#include <stdint.h>
#include <stdbool.h>

// #define DEBUG

typedef enum {
  SUCCESS        = 0,
  INVALID_PARAMETERS,
  INVALID_STATE,
  DEVICE_BUSY,
  FREQUENCY_UPDATE_ERROR,
  DUTY_UPDATE_ERROR,
  INTERNEL_ERROR,
} stepper_err_t;

typedef struct {
  uint8_t instance_id; // for interval use only.
} stepper_t;

/**
 * @param idx_: integer from 0 to MAX_INSTANCE_NUMBER, i.e. 0-3 for ESP32C3
*/
#define STEPPER_INSTANCE(idx_) { .instance_id = idx_, }

typedef struct {
  int32_t  pin_dir;
  int32_t  pin_pulse;
  uint32_t subdivision;   // 细分，电机一圈的步数，一般步进电机为 200 步，也就是一步 1.8˚
  uint32_t pulse_us;      // 每个脉冲最短有效时长，默认 5us
  uint32_t rpm;           // 每分钟转速
  bool     direction :1;  // 转向
} stepper_config_t;

#define STEPPER_CONFIG(pin_dir_, pin_pulse_) {       \
  .pin_dir      = pin_dir_,                          \
  .pin_pulse    = pin_pulse_,                        \
  .pulse_us     = 6,                                 \
  .subdivision  = 3200,                              \
  .rpm          = 10,                                \
  .direction    = 0,                                 \
}

stepper_err_t stepper_init(stepper_t const * stepper, stepper_config_t const * config);

stepper_err_t stepper_uninit(stepper_t const * stepper);

stepper_err_t stepper_update_rpm(stepper_t const * stepper, uint32_t rpm);

stepper_err_t stepper_update_direction(stepper_t const * stepper, bool direction);

stepper_err_t stepper_start(stepper_t const * stepper);

stepper_err_t stepper_stop(stepper_t const * stepper);
