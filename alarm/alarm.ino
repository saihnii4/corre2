#include <PollingTimer.h>
#include "rgb_lcd.h"

rgb_lcd lcd;

PollingTimer timer;

void setup() {
    Serial.begin(115200);
    timer.start();
    lcd.begin(16, 2);
    lcd.clear();
}

void loop() {
    if (timer.isRunning()) {
        lcd.print(timer.msec());
    }
    delay(500);
    lcd.clear();
}
