#include <LiquidCrystal.h>
#include <TH02_dev.h>
#include "rgb_lcd.h"

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

const char* options[5] = {
    "Commit suicide",
    "Go kill yourself",
    "LMAO",
    "Watch e621",
    "Ok"
};

unsigned int index;

// some code i lifted from another project
template <typename... Args>
const char *format_string(const char *format, Args... args) {
  size_t nbytes = snprintf(NULL, 0, format, args...);

  char *buf = (char *)malloc(sizeof(char) * nbytes);
  snprintf(buf, nbytes + 1, format, args...);
  return buf;
}

void setup() {
    Serial.begin(9600);

    // pin connections may vary depending on circuit
    pinMode(A0, INPUT);
    pinMode(A1, INPUT);
    pinMode(7, INPUT);
    lcd.clear();
    lcd.begin(16, 2);
}

void loop() {
    int x =  (int)analogRead(A0);
    int y =  (int)analogRead(A1);
    int on = (int)digitalRead(7);
    int orientation = x + y - 1023;

    if (!(orientation >= -100 && orientation <= 100)) {
        if (x >= 750 || y >= 750)
            index++;

        if (x <= 350 || y <= 350)
            index--;
    }

    if (index > 4) index = 0;

    lcd.setCursor(0,0);
    lcd.print(format_string("> %s", options[index]));
    lcd.setCursor(0,1);
    lcd.print(index);
    delay(250);
}
