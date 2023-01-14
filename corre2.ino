#include <LiquidCrystal.h>
#include <TH02_dev.h>
#include "rgb_lcd.h"

#define OPTION_SIZE 2 // too fucking lazy mate

rgb_lcd lcd;

int tempPin = A5, joystickX = 0, joystickY = 1, sw = 7;

// TODO: scroll overflow
const char* options[OPTION_SIZE] = {
    "Temperature",
    "Alarms     ",
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
    if (temp >= 25 || temp <= 20) log("temp", "temperature is no longer at optimum"); // TODO: buzzer
}

void setup() {
    Serial.begin(9600);

    // pin connections may vary depending on circuit
    pinMode(A5, INPUT);
    pinMode(A4, INPUT);
    pinMode(7, INPUT);
    pinMode(tempPin, INPUT);

    digitalWrite(7, HIGH);

    lcd.begin(16, 2);
    lcd.clear();
}

void loop() {
    // checkTemperature();

    int x =  (int)analogRead(A2);
    int y =  (int)analogRead(A3);
    int on = (int)digitalRead(7);
    // TODO: Use chinese temperature sensor instead
    Serial.println(format_string("%d", (double)TH02.ReadTemperature()));

    /* Serial.println((int)analogRead(A1), HEX); */

    int oscill = abs(sin(((double)millis()/1000)-floor((double)millis()/1000)*2*3.14));
    analogWrite(8, 255/2);

    int orientation = x + y - 1023;

    if (!(orientation >= -100 && orientation <= 100)) {
        if (x >= 750 || y >= 750)
            index++;

        if (x <= 350 || y <= 350)
            index--;
    }

    if (index > OPTION_SIZE-1) index = 0;

    lcd.setCursor(0,0);
    lcd.print(format_string("> %s", options[index]));
    lcd.setCursor(0,1);
    lcd.print(index);
    delay(250);
}
