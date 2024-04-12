#include "stepper.h"

static const stepper_t stepper0 = STEPPER_INSTANCE(0);

void app_main() {
  const stepper_config_t config0 = STEPPER_CONFIG(1, 2);
  int err = stepper_init(&stepper0, &config0);
  if (err) {
    // log it.
  }
}
