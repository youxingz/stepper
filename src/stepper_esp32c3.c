#include "stepper.h"

#define MAX_SUPPORT_STEPPER_NUMBER 4

typedef struct {
  stepper_config_t  config;
  volatile bool     running  : 1;
} state_t;

static volatile state_t states[MAX_SUPPORT_STEPPER_NUMBER];

stepper_err_t stepper_init(stepper_t const * stepper, stepper_config_t const * config)
{
  if (stepper->instance_id > MAX_SUPPORT_STEPPER_NUMBER) {
    return INVALID_PARAMETERS;
  }
  if (states[stepper->instance_id].running) {
    return INVALID_STATE; // Invalid State, Stepper is already inited & is working.
  }
  states[stepper->instance_id].config.pin_dir     = config->pin_dir;
  states[stepper->instance_id].config.pin_pulse   = config->pin_pulse;
  states[stepper->instance_id].config.cycle_step  = config->cycle_step;
  states[stepper->instance_id].config.rpm         = config->rpm;
  states[stepper->instance_id].config.direction   = config->direction;

  return SUCCESS;
}

stepper_err_t stepper_uninit(stepper_t const * stepper)
{
  return SUCCESS;
}

stepper_err_t stepper_update_rpm(stepper_t const * stepper, uint32_t rpm)
{
  return SUCCESS;
}

stepper_err_t stepper_update_direction(stepper_t const * stepper, bool direction)
{
  return SUCCESS;
}

stepper_err_t stepper_start(stepper_t const * stepper)
{
  return SUCCESS;
}

stepper_err_t stepper_stop(stepper_t const * stepper)
{
  return SUCCESS;
}
