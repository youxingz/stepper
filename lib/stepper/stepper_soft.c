#include "stepper.h"

#if !defined(MCU_ESP32) & !defined(MCU_NORDIC_RF) & !defined(MCU_STM32) & !defined(MCU_STM8)

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
stepper_err_t stepper_uninit(stepper_t const * stepper)
{
  return SUCCESS;
}

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
stepper_err_t stepper_update_rpm(stepper_t const * stepper, uint32_t rpm)
{
  return SUCCESS;
}

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
stepper_err_t stepper_update_direction(stepper_t const * stepper, bool direction)
{
  return SUCCESS;
}

/**
 * @brief start (resume) motor rotation.
 * 
 * @param stepper   the instance of device
 * 
 * @return
 *    - SUCCESS         start successfully.
 *    - INTERNAL_ERROR  mcu internal error.
*/
stepper_err_t stepper_start(stepper_t const * stepper)
{
  return SUCCESS;
}

/**
 * @brief stop (pause) motor rotation.
 * 
 * @param stepper   the instance of device
 * 
 * @return
 *    - SUCCESS         stop successfully.
 *    - INTERNAL_ERROR  mcu internal error.
*/
stepper_err_t stepper_stop(stepper_t const * stepper)
{
  return SUCCESS;
}

#endif
