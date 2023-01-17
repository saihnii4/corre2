#include <OneShotTimer.h>
#include "rgb_lcd.h"

OneShotTimer oneshot(10);
OneShotTimer twoshot(5);

rgb_lcd lcd;

void setup() {
    Serial.begin(115200);
    oneshot.onUpdate([&]() {
        Serial.println(millis());
    });
    twoshot.onUpdate([&]() {
        Serial.println(millis());
    });

    oneshot.start();
    twoshot.start();
}

void loop() {
    oneshot.update();
    twoshot.update();
}
