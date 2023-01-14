#include <LiquidCrystal.h>
// #include <TH02_dev.h>
// #include "rgb_lcd.h"

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

int tempPin = A5, joystickX = 0, joystickY = 1, sw = 7;

// TODO: scroll overflow
const char* options[5] = {
    "Take the blue pill",
    "Take the red pill",
    "Take the green pill",
    "idk what to put here",
    "lol"
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

template <typename... Args>
void log(const char* module, const char* message) {
    Serial.println(format_string("[%s] %s", module, message));
}

void checkTemperature() {
    int raw_adc = analogRead(tempPin);
    double temp = log(10000*((1024.0/raw_adc-1)));
    temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * temp * temp ))* temp );
    temp = temp - 273.15;            // apparently data is sent in kelvins
    if (temp >= 25 || temp <= 20) log("temp", "temperature is out of recommended");
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
    checkTemperature();

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
