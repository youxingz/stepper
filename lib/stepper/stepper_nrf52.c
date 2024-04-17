#include "stepper.h"

#if defined(MCU_NORDIC_RF)

// #define NRFX_PWM_ENABLED  1
// #define NRFX_PWM0_ENABLED 1
// #define NRFX_PWM1_ENABLED 1
// #define NRFX_PWM2_ENABLED 1
// #define NRFX_PWM3_ENABLED 1

#include <nrfx_pwm.h>
#include <hal/nrf_gpio.h>

static nrfx_pwm_t const m_pwms[NRFX_PWM_ENABLED_COUNT] = {
#if NRFX_PWM0_ENABLED
  NRFX_PWM_INSTANCE(0),
#endif
#if NRFX_PWM1_ENABLED
  NRFX_PWM_INSTANCE(1),
#endif
#if NRFX_PWM2_ENABLED
  NRFX_PWM_INSTANCE(2),
#endif
#if NRFX_PWM3_ENABLED
  NRFX_PWM_INSTANCE(3),
#endif
};

#define MAX_SUPPORT_STEPPER_NUMBER  NRFX_PWM_ENABLED_COUNT
#define PWM_IRQ_PRIORITY            3

#define PWM_INSTANCE(stepper)       &(m_pwms[stepper->instance_id])

static volatile bool module_installed   = false;

typedef struct {
  // nrfx_pwm_t        pwm_ins;
  // nrfx_pwm_config_t pwm_config;
  stepper_config_t  config;
  volatile bool     running;
  bool              inited;
} state_t;

static volatile state_t states[MAX_SUPPORT_STEPPER_NUMBER];

static inline uint32_t to_period_us(uint32_t subdivision, float rpm) {
  return (uint32_t)(1000000L * 60 / (rpm * subdivision));
}

stepper_err_t stepper_init(stepper_t const * stepper, stepper_config_t const * config)
{
  if (!module_installed) {
    for (int i = 0; i < MAX_SUPPORT_STEPPER_NUMBER; i++) {
      if (stepper->instance_id == i) continue;
      for (int k = 0; i < 4; k++) {
        states[i].config.pin_dirs[k]    = -1;
        states[i].config.pin_pulses[k]  = -1;
      }
      states[i].config.subdivision = 3200;
      states[i].config.rpm         = 1;    // RPM
      states[i].config.direction   = false;
      states[i].config.pulse_us    = 6;    // us
    }
    module_installed = true;
  }

  if (stepper->instance_id > MAX_SUPPORT_STEPPER_NUMBER) {
    return INVALID_PARAMETERS;
  }
  if (states[stepper->instance_id].inited) {
    return INVALID_STATE;  // Stepper is already inited.
  }
  if (states[stepper->instance_id].running) {
    return INVALID_STATE;  // Invalid State, still working.
  }
  for (int i = 0; i < 4; i++) {
    states[stepper->instance_id].config.pin_dirs[i]     = config->pin_dirs[i];
    states[stepper->instance_id].config.pin_pulses[i]   = config->pin_pulses[i];
    // update gpio.
    if (config->pin_dirs[i] > 0) {
      nrf_gpio_cfg(config->pin_dirs[i], NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_H0H1, NRF_GPIO_PIN_NOSENSE);
    }
    if (config->pin_pulses[i] > 0) {
      nrf_gpio_cfg(config->pin_pulses[i], NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_H0H1, NRF_GPIO_PIN_NOSENSE);
    }
  }
  states[stepper->instance_id].config.subdivision = config->subdivision;
  states[stepper->instance_id].config.rpm         = config->rpm > 0 ? config->rpm : 1;
  states[stepper->instance_id].config.direction   = config->direction;
  states[stepper->instance_id].config.pulse_us    = config->pulse_us;

  stepper_err_t err = stepper_update_rpm(stepper, states[stepper->instance_id].config.rpm);
  if (err != SUCCESS) { return err; }

  err = stepper_update_direction(stepper, states[stepper->instance_id].config.direction);
  if (err != SUCCESS) { return err; }

  states[stepper->instance_id].inited = true;

  return SUCCESS;
}

stepper_err_t stepper_uninit(stepper_t const * stepper)
{
  nrfx_pwm_uninit(PWM_INSTANCE(stepper));
  return SUCCESS;
}

stepper_err_t stepper_update_rpm(stepper_t const * stepper, float rpm)
{
  const nrfx_pwm_t * instance = PWM_INSTANCE(stepper);

  uint32_t period_us = to_period_us(states[stepper->instance_id].config.subdivision, states[stepper->instance_id].config.rpm);
  uint32_t duty_us   = states[stepper->instance_id].config.pulse_us;

  if (period_us > 262140 || period_us < 5) { // ~4Hz, 200kHz
    // stop pwm, since pwm does not support such pulse width (too wide).
    nrfx_pwm_stop(instance, true);
    return FREQUENCY_UPDATE_ERROR;
  }

  nrf_pwm_clk_t pwm_clock = NRF_PWM_CLK_16MHz;
  // find suitable clock source:
  if      (period_us > 32767) {  pwm_clock = NRF_PWM_CLK_125kHz;  period_us /= 8;   duty_us /= 8;   }
  else if (period_us >  1000) {  pwm_clock = NRF_PWM_CLK_1MHz;    period_us *= 1;   duty_us *= 1;   }
  else                        {  pwm_clock = NRF_PWM_CLK_16MHz;   period_us *= 16;  duty_us *= 16;  }

  const nrfx_pwm_config_t pwm_config = {
    .output_pins =
    {
      states[stepper->instance_id].config.pin_dirs[0] > 0 ? states[stepper->instance_id].config.pin_dirs[0] : NRF_PWM_PIN_NOT_CONNECTED, // channel 0 
      states[stepper->instance_id].config.pin_dirs[1] > 0 ? states[stepper->instance_id].config.pin_dirs[1] : NRF_PWM_PIN_NOT_CONNECTED, // channel 1
      states[stepper->instance_id].config.pin_dirs[2] > 0 ? states[stepper->instance_id].config.pin_dirs[2] : NRF_PWM_PIN_NOT_CONNECTED, // channel 2
      states[stepper->instance_id].config.pin_dirs[3] > 0 ? states[stepper->instance_id].config.pin_dirs[3] : NRF_PWM_PIN_NOT_CONNECTED, // channel 3
    },
    .irq_priority = PWM_IRQ_PRIORITY,
    .base_clock   = pwm_clock,            // initial arguments.
    .count_mode   = NRF_PWM_MODE_UP,
    .top_value    = period_us,
    .load_mode    = NRF_PWM_LOAD_COMMON,  // 波形模式
    .step_mode    = NRF_PWM_STEP_AUTO     // 自动，重复次数后刷新
  };

  // states[stepper->instance_id].pwm_config = pwm_config;

  nrfx_err_t err_code = nrfx_pwm_init(instance, &pwm_config, NULL, NULL);
  if (err_code == NRFX_ERROR_ALREADY_INITIALIZED) { // re-init.
    nrfx_pwm_uninit(instance);
    err_code = nrfx_pwm_init(instance, &pwm_config, NULL, NULL);
  }
  if (err_code != NRFX_SUCCESS) {
    return INTERNAL_ERROR;
  }

  nrf_pwm_values_common_t duty_value = (nrf_pwm_values_common_t) duty_us;
  nrf_pwm_values_t values = {
    .p_common = duty_value,
  };
  nrf_pwm_sequence_t sequence = {
    .values     = values,
    .length     = NRF_PWM_VALUES_LENGTH(duty_value),
    .repeats    = 0,
    .end_delay  = 0,
  };

  nrfx_pwm_stop(instance, true);
  // uint32_t task_address =
  nrfx_pwm_simple_playback(instance, &sequence, 1, NRFX_PWM_FLAG_LOOP); // loop mode.

  return SUCCESS;
}

stepper_err_t stepper_update_direction(stepper_t const * stepper, bool direction)
{
  for (int i = 0; i < 4; i++) {
    int32_t pin = states[stepper->instance_id].config.pin_dirs[i];
    if (pin > 0) {
      if (direction) { nrf_gpio_pin_set(pin); } else { nrf_gpio_pin_clear(pin); }
    }
  }
  return SUCCESS;
}

stepper_err_t stepper_start(stepper_t const * stepper)
{
  return stepper_update_rpm(stepper, states[stepper->instance_id].config.rpm);
}

stepper_err_t stepper_stop(stepper_t const * stepper)
{
  nrfx_pwm_stop(PWM_INSTANCE(stepper), true);
  // stepper_update_direction(stepper, flase);
  return SUCCESS;
}

#endif
