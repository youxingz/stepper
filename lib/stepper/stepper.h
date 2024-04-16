#include <stdint.h>
#include <stdbool.h>

// #define DEBUG
#define MCU_ESP32Cx
// #define MCU_ESP32Sx

typedef enum {
  SUCCESS        = 0,
  INVALID_PARAMETERS,
  INVALID_STATE,
  DEVICE_BUSY,
  FREQUENCY_UPDATE_ERROR,
  DUTY_UPDATE_ERROR,
  INTERNAL_ERROR,
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
  .pulse_us     = 3,                                 \
  .subdivision  = 3200,                              \
  .rpm          = 10,                                \
  .direction    = 0,                                 \
}

/**
 * @brief initialize stepper device and config it.
 * 
 * @param stepper the instance of device
 * @param config  configuration
 * 
 * @return `stepper_err_t`
 *    - SUCCESS             initialize successfully.
 *    - INVALID_PARAMETERS  make sure your instance id is valid, and GPIO pin is avaliable.
 *    - INVALID_STATE       this instance was already initialized, or it is running.
 *    - INTERNAL_ERROR      mcu internal error.
*/
stepper_err_t stepper_init(stepper_t const * stepper, stepper_config_t const * config);

/**
 * @brief uninitalize stepper device.
 * 
 * @param stepper the instance of device
 * 
 * @return
 *    - SUCCESS             uninitialize successfully.
 *    - INTERNAL_ERROR      mcu internal error.
*/
stepper_err_t stepper_uninit(stepper_t const * stepper);

/**
 * @brief update the RPM, i.e., current speed.
 * 
 * @param stepper the instance of device
 * @param rpm     the speed value, round per minute.
 * 
 * @return
 *    - SUCCESS                 update successfully.
 *    - FREQUENCY_UPDATE_ERROR  frequency error, you can check your speed if it is a valid number and in a valid range.
 *    - DUTY_UPDATE_ERROR       duty error, please check `config.subdivision`, the value of `config.subdivision` usually valid from 3 to 10 micromills.
 *    - INTERNAL_ERROR          mcu internal error.
*/
stepper_err_t stepper_update_rpm(stepper_t const * stepper, uint32_t rpm);

/**
 * @brief update the direction of motor.
 * 
 * @param stepper   the instance of device
 * @param direction `true` means positive, `false` means negative. Notice that this value may not corresponses
 *                  with your motor rotation direction, which means the positive value may be corresponsed with
 *                  the counterclockwise or clockwise, but it must be fixed in a known board.
 * 
 * @return
 *    - SUCCESS         update successfully.
 *    - INTERNAL_ERROR  mcu internal error.
 * 
*/
stepper_err_t stepper_update_direction(stepper_t const * stepper, bool direction);

/**
 * @brief start (resume) motor rotation.
 * 
 * @param stepper   the instance of device
 * 
 * @return
 *    - SUCCESS         start successfully.
 *    - INTERNAL_ERROR  mcu internal error.
*/
stepper_err_t stepper_start(stepper_t const * stepper);

/**
 * @brief stop (pause) motor rotation.
 * 
 * @param stepper   the instance of device
 * 
 * @return
 *    - SUCCESS         stop successfully.
 *    - INTERNAL_ERROR  mcu internal error.
*/
stepper_err_t stepper_stop(stepper_t const * stepper);
