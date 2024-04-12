#include <stdint.h>
#include <stdbool.h>

typedef enum {
  SUCCESS        = 0,
  INVALID_PARAMETERS,
  INVALID_STATE,
  DEVICE_BUSY,
  INTERNEL_ERROR,
} stepper_err_t;

typedef struct {
  uint8_t instance_id; // for interval use only.
} stepper_t;

#define STEPPER_INSTANCE(idx_) { .instance_id = idx_, }

typedef struct {
  uint32_t pin_dir;
  uint32_t pin_pulse;
  uint32_t cycle_step;    // 电机一圈的步数，一般步进电机为 200 步，也就是一步 1.8˚
  uint32_t rpm;           // 每分钟转速
  bool     direction :1;  // 转向
} stepper_config_t;

#define STEPPER_CONFIG(pin_dir_, pin_pulse_) {    \
  .pin_dir = pin_dir_,                            \
  .pin_pulse = pin_pulse_,                        \
  .cycle_step = 200,                              \
  .rpm = 0,                                       \
  .direction = 0,                                 \
}


stepper_err_t stepper_init(stepper_t const * stepper, stepper_config_t const * config);

stepper_err_t stepper_uninit(stepper_t const * stepper);

stepper_err_t stepper_update_rpm(stepper_t const * stepper, uint32_t rpm);

stepper_err_t stepper_update_direction(stepper_t const * stepper, bool direction);

stepper_err_t stepper_start(stepper_t const * stepper);

stepper_err_t stepper_stop(stepper_t const * stepper);
