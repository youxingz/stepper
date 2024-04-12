#include "stepper.h"

#include "esp_err.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#define DEBUG
#ifdef DEBUG
#include "esp_log.h"
#endif

#define MAX_SUPPORT_STEPPER_NUMBER 4

static volatile bool module_installed = false;

typedef struct {
  stepper_config_t  config;
  volatile bool     running:  1;
  bool              inited:   1;
} state_t;

static volatile state_t states[MAX_SUPPORT_STEPPER_NUMBER];

#define LEDC_MODE LEDC_LOW_SPEED_MODE
// LEDC_USE_APB_CLK LEDC_USE_XTAL_CLK
#define LEDC_DEF(pin_, timer_, channel_, freq_, duty_) {\
    ledc_timer_config_t ledc_timer = {                  \
        .speed_mode       = LEDC_MODE,                  \
        .duty_resolution  = LEDC_TIMER_8_BIT,           \
        .timer_num        = (timer_),                   \
        .freq_hz          = (freq_),                    \
        .clk_cfg          = LEDC_USE_APB_CLK            \
    };                                                  \
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));    \
    ledc_channel_config_t ledc_channel = {              \
        .gpio_num       = pin_,                         \
        .speed_mode     = LEDC_MODE,                    \
        .channel        = (channel_),                   \
        .intr_type      = LEDC_INTR_DISABLE,            \
        .timer_sel      = (timer_),                     \
        .duty           = (duty_),                      \
        .hpoint         = 0                             \
    };                                                  \
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));\
}

#define TIMER_IDX(instance)   (ledc_timer_t)(LEDC_TIMER_0 + (instance)->instance_id)
#define CHANNEL_IDX(instance) (ledc_channel_t)(LEDC_CHANNEL_0 + (instance)->instance_id)
#define PERIOD(freq)          (1000000 / freq)

static inline uint32_t to_freq_hz(uint32_t cycle_step, uint32_t rpm) {
  // 1RPS = 60RPM = cycle_step pulse/s = 60 * cycle_step pulse/s
  uint32_t pulse_per_second = (rpm * cycle_step / 60);
  return pulse_per_second;
}
static inline uint32_t to_duty(uint32_t freq_hz) { // 2^4=16 resolution.
  uint32_t period_us = PERIOD(freq_hz);
  if (period_us > 80) return 2;

  uint32_t duty = 80 / period_us;
  if (duty > 15) duty = 15;

  return duty;
}

static int update_gpio_config() {
    uint64_t pin_mask = 0;
    for (int i = 0; i < MAX_SUPPORT_STEPPER_NUMBER; i++) {
      if (states[i].config.pin_dir > -1) {
        pin_mask |= 1ULL << (states[i].config.pin_dir);
      }
      if (states[i].config.pin_pulse > -1) {
        pin_mask |= 1ULL << (states[i].config.pin_pulse);
      }
    }
    gpio_config_t io_conf = {
        .pin_bit_mask   = pin_mask,
        .mode           = GPIO_MODE_OUTPUT,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .intr_type      = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
      return -1;
    }

    for (int i = 0; i < MAX_SUPPORT_STEPPER_NUMBER; i++) {
      gpio_set_level(states[i].config.pin_dir,    0);
      gpio_set_level(states[i].config.pin_pulse,  0);
    }

    return 0;
}

stepper_err_t stepper_init(stepper_t const * stepper, stepper_config_t const * config)
{
  if (!module_installed) {
    for (int i = 0; i < MAX_SUPPORT_STEPPER_NUMBER; i++) {
      states[i].config.pin_dir    = -1;
      states[i].config.pin_pulse  = -1;
      states[i].config.cycle_step = 200;
      states[i].config.rpm        = 60; // RPM
      states[i].config.direction  = false;
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
  states[stepper->instance_id].config.pin_dir     = config->pin_dir;
  states[stepper->instance_id].config.pin_pulse   = config->pin_pulse;
  states[stepper->instance_id].config.cycle_step  = config->cycle_step;
  states[stepper->instance_id].config.rpm         = config->rpm > 0 ? config->rpm : 1;
  states[stepper->instance_id].config.direction   = config->direction;

  int err = update_gpio_config();
  if (err) {
    return INVALID_PARAMETERS; // GPIO config fail.
  }

  uint32_t freq = to_freq_hz(states[stepper->instance_id].config.cycle_step, states[stepper->instance_id].config.rpm);
  uint32_t duty = to_duty(freq);

#ifdef DEBUG
  ESP_LOGI("[Stepper]", "Parameters: freq=%lu, duty=%lu", freq, duty);
#endif

  LEDC_DEF(
            config->pin_pulse,
            TIMER_IDX(stepper),
            CHANNEL_IDX(stepper),
            freq,
            duty
          );
  
  states[stepper->instance_id].inited = true;

  return SUCCESS;
}

stepper_err_t stepper_uninit(stepper_t const * stepper)
{
  return SUCCESS;
}

stepper_err_t stepper_update_rpm(stepper_t const * stepper, uint32_t rpm)
{
  esp_err_t err = ESP_OK;

  ledc_timer_t   timer    = TIMER_IDX(stepper);
  ledc_channel_t channel  = CHANNEL_IDX(stepper);

  uint32_t freq = to_freq_hz(states[stepper->instance_id].config.cycle_step, rpm);

  if (freq == 0) {
    return stepper_stop(stepper);
  }

  err = ledc_set_freq(LEDC_MODE, timer, freq);
  if (err != ESP_OK) {
    return FREQUENCY_UPDATE_ERROR;
  }

  err = ledc_set_duty(LEDC_MODE, channel, to_duty(freq));
  if (err != ESP_OK) {
    return DUTY_UPDATE_ERROR;
  }

  err = ledc_update_duty(LEDC_MODE, channel);
  if (err != ESP_OK) {
    return DUTY_UPDATE_ERROR;
  }
  return SUCCESS;
}

stepper_err_t stepper_update_direction(stepper_t const * stepper, bool direction)
{
  esp_err_t err = gpio_set_level(states[stepper->instance_id].config.pin_dir, direction ? 1 : 0);
  if (err != ESP_OK) {
    return INTERNEL_ERROR;
  }
  return SUCCESS;
}

stepper_err_t stepper_start(stepper_t const * stepper)
{
  ledc_timer_t   timer    = TIMER_IDX(stepper);

  esp_err_t err = ledc_timer_resume(LEDC_MODE, timer);
  if (err != ESP_OK) {
    return INTERNEL_ERROR;
  }
  
  return SUCCESS;
}

stepper_err_t stepper_stop(stepper_t const * stepper)
{
  ledc_timer_t   timer    = TIMER_IDX(stepper);

  esp_err_t err = ledc_timer_pause(LEDC_MODE, timer);
  if (err != ESP_OK) {
    return INTERNEL_ERROR;
  }

  gpio_set_level(states[stepper->instance_id].config.pin_dir,   0);
  gpio_set_level(states[stepper->instance_id].config.pin_pulse, 0);

  return SUCCESS;
}
