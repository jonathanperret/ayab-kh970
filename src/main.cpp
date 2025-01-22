#include <Arduino.h>

#include "kh970client.h"

KH970Client khClient;

void setup() {
  Serial.begin(115200);
  khClient.begin();
}

uint8_t default_pattern[] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                           0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                           0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};

void loop() {
#ifdef TXLED0
  if (millis() % 1000 < 500)
    TXLED1;
  else
    TXLED0;
#endif
  khClient.set_pattern(default_pattern);

  khClient.update();
}
