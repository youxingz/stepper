## Stepper Controller

[![Licence](https://img.shields.io/github/license/Ileriayo/markdown-badges?style=for-the-badge)](./LICENSE)

### Supports

- [x] hardware layer timers replace software `delay` functions
- [x] configurable `subdivision` for different motor driver boards
- [x] custom `RPM` and `direction`, `rpm` range from `0` to `30000`

Multiple platforms:

- [x] ESP32Cx, ESP32Sx Series
- [ ] STM32
- [x] Nordic RF52 Series

### Install

Download this repository, and copy `lib` directory into your own project path, that's all.

Example Code:
```c
#include "stepper.h"

#define PULSE_PIN 1
#define DIR_PIN   2
static const stepper_t stepper0 = STEPPER_INSTANCE(0);

int main() {
  const stepper_config_t config0 = STEPPER_CONFIG(DIR_PIN, PULSE_PIN);
  int err = stepper_init(&stepper0, &config0);
  if (err) { return err; }

  // you can define more stepper devices:
  // static const stepper_t stepper1 = STEPPER_INSTANCE(1);
  // const stepper_config_t config1  = STEPPER_CONFIG(DIR_PIN1, PULSE_PIN1);
  // stepper_init(&stepper1, &config1);
  

  stepper_start(&stepper0);

  uint32_t rpm  = 0;
  bool dir      = false;
  for (int i = 0 ;; i++) { // rpm from 0 to 5000, direction toggled.
    if (i > 100) { i = 0; }
    delay_ms(10);
    rpm = (i * 50);
    dir = !dir;
    stepper_update_rpm(&stepper0, rpm);
    stepper_update_direction(&stepper0, dir);
  }
}
```
