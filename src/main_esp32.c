#include <stepper.h>

#if defined(MCU_ESP32)

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#ifdef DEBUG
#include "esp_log.h"
#endif

static const stepper_t stepper0 = STEPPER_INSTANCE(0);

void app_main() {
  const stepper_config_t config0 = STEPPER_CONFIG(1, 2);
  int err = stepper_init(&stepper0, &config0);
  if (err) {
    // log it.
  }
  // stepper_start(&stepper0);
  uint32_t rpm;
  bool dir = false;
  for (int i = 0 ;; i++) {
    vTaskDelay(10);
    rpm = (i * 50);
    dir = !dir;
    if (i > 100) { i = 0; }
#ifdef DEBUG
  ESP_LOGI("[Motor]", "Parameters: rpm=%lu, dir=%s", rpm, dir ? "po" : "ne");
#endif
    stepper_update_rpm(&stepper0, rpm);
    stepper_update_direction(&stepper0, dir);
  }
}

#endif
