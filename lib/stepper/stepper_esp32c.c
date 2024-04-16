#include "stepper.h"

#if defined(MCU_ESP32Cx) | defined(MCU_ESP32Sx)

#include "esp_err.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#ifdef DEBUG
#include "esp_log.h"
#endif

#define MAX_SUPPORT_STEPPER_NUMBER 4

#define FREQUENCY_THRESH           (360)
static volatile bool module_installed = false;
static volatile uint8_t duty_resolution = LEDC_TIMER_8_BIT;

typedef struct {
  stepper_config_t  config;
  volatile bool     running:  1;
  bool              inited:   1;
} state_t;

static volatile state_t states[MAX_SUPPORT_STEPPER_NUMBER];

#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CLK  LEDC_USE_APB_CLK
// LEDC_USE_APB_CLK LEDC_USE_XTAL_CLK
#define LEDC_DEF(pin_, timer_, channel_, freq_, duty_, duty_res_) {\
    ledc_timer_config_t ledc_timer = {                  \
        .speed_mode       = LEDC_MODE,                  \
        .duty_resolution  = (duty_res_),                \
        .timer_num        = (timer_),                   \
        .freq_hz          = (freq_),                    \
        .clk_cfg          = LEDC_CLK                    \
    };                                                  \
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));    \
    ledc_timer_pause(LEDC_MODE, timer_);                \
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

static inline uint32_t to_freq_hz(uint32_t subdivision, uint32_t rpm) {
  // 1RPS = 60RPM = subdivision pulse/s = 60 * subdivision pulse/s
  uint32_t pulse_per_second = (rpm * subdivision / 60);
  return pulse_per_second;
}
static inline uint32_t to_duty(uint32_t freq_hz, uint32_t mini_duty_us) {
// #define PULSE_WIDTH 6
#define PULSE_FRAC  ((1 << duty_resolution) * mini_duty_us)
  uint32_t period_us = PERIOD(freq_hz);
  if (period_us > PULSE_FRAC) return 2;

  uint32_t duty = PULSE_FRAC / period_us;
  if (duty > (1 << duty_resolution)) duty = (1 << duty_resolution) - 1;

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
      if (stepper->instance_id == i) continue;
      states[i].config.pin_dir    = -1;
      states[i].config.pin_pulse  = -1;
      states[i].config.subdivision = 3200;
      states[i].config.rpm        = 1;    // RPM
      states[i].config.direction  = false;
      states[i].config.pulse_us   = 6;    // us
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
  states[stepper->instance_id].config.subdivision = config->subdivision;
  states[stepper->instance_id].config.rpm         = config->rpm > 0 ? config->rpm : 1;
  states[stepper->instance_id].config.direction   = config->direction;
  states[stepper->instance_id].config.pulse_us    = config->pulse_us;

  int err = update_gpio_config();
  if (err) {
    return INVALID_PARAMETERS; // GPIO config fail.
  }

  uint32_t freq = to_freq_hz(states[stepper->instance_id].config.subdivision, states[stepper->instance_id].config.rpm);
  uint32_t duty = to_duty(freq, states[stepper->instance_id].config.pulse_us);

#ifdef DEBUG
  ESP_LOGI("[Stepper]", "Parameters: freq=%lu, duty=%lu", freq, duty);
#endif

  duty_resolution = (freq > FREQUENCY_THRESH) ? LEDC_TIMER_8_BIT : LEDC_TIMER_14_BIT;
  LEDC_DEF(
            config->pin_pulse,
            TIMER_IDX(stepper),
            CHANNEL_IDX(stepper),
            freq,
            duty,
            duty_resolution
          );

  // ledc_timer_pause(LEDC_MODE, TIMER_IDX(stepper)); // pause after initialized success.

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

  uint32_t freq = to_freq_hz(states[stepper->instance_id].config.subdivision, rpm);

  if (freq < 10) {
    return stepper_stop(stepper);
  }

  uint32_t duty = to_duty(freq, states[stepper->instance_id].config.pulse_us);

#ifdef DEBUG
  ESP_LOGI("[Stepper/stepper_update_rpm]", "Parameters: freq=%lu, duty=%lu", freq, duty);
#endif

  uint32_t freq_old = ledc_get_freq(LEDC_MODE, timer);
#ifdef DEBUG
  ESP_LOGI("[Stepper/stepper_update_rpm]", "Settings: old_freq=%lu, new_freq=%lu", freq_old, freq);
#endif
  if ((freq_old > FREQUENCY_THRESH) != (freq > FREQUENCY_THRESH)) {
    // update duty_resolution. (reconfig all)
    duty_resolution = (freq > FREQUENCY_THRESH) ? LEDC_TIMER_8_BIT : LEDC_TIMER_14_BIT;
    ledc_timer_pause(LEDC_MODE, timer);
    ledc_timer_config_t ledc_timer = {
      .speed_mode       = LEDC_MODE,
      .duty_resolution  = duty_resolution,
      .timer_num        = timer,
      .freq_hz          = freq,
      .clk_cfg          = LEDC_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));    
    ledc_timer_rst(LEDC_MODE, timer);
    ledc_timer_resume(LEDC_MODE, timer);
  } else {
    ledc_timer_pause(LEDC_MODE, timer);
    err = ledc_set_freq(LEDC_MODE, timer, freq);
    if (err != ESP_OK) {
      return FREQUENCY_UPDATE_ERROR;
    }

    err = ledc_set_duty(LEDC_MODE, channel, duty);
    if (err != ESP_OK) {
      return DUTY_UPDATE_ERROR;
    }

    err = ledc_update_duty(LEDC_MODE, channel);
    if (err != ESP_OK) {
      return DUTY_UPDATE_ERROR;
    }
    ledc_timer_resume(LEDC_MODE, timer);
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

#endif
