#include <LiquidCrystal.h>
#include <TH02_dev.h>
#include "rgb_lcd.h"

#define OPTION_SIZE 3 // too fucking lazy mate

rgb_lcd lcd;
bool UP, DOWN;
typedef void (*handler_func)(bool, bool, int, int, int);

int tempPin = A0, joystickX = 0, joystickY = 1, sw = 7, buzzerPin = 4;

void alarm_menu(bool up, bool down, int _jx, int _jy, bool pressed) {
    lcd.clear();
    lcd.print("Alarm");
}

void temperature_menu(bool up, bool down, int _jx, int _jy, bool pressed) {
    lcd.clear();
    lcd.print("Temperature");
}

void settings_menu(bool up, bool down, int jx, int jy, bool pressed) {
    lcd.clear();
    lcd.print("Settings");
}

// TODO: scroll overflow & translation
const char* options[OPTION_SIZE] = {
    "Temperature",
    "Alarms     ",
    "Settings   ",
};

handler_func handlers[OPTION_SIZE] = {
    temperature_menu,
    alarm_menu,
    settings_menu
};

int index;
bool in_menu = false;

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
    if (temp >= 25 || temp <= 20) digitalWrite(buzzerPin, HIGH);
    else digitalWrite(buzzerPin, LOW);
}

void main_menu(bool up, bool down, int _jx, int _jy, int pressed) {
    if (!pressed) {
        Serial.println(format_string("%d %d", pressed, in_menu));
        in_menu = true;
    }
    if (up) index--;
    if (up && index+1 == 0) index = OPTION_SIZE-1;
    if (down) index++;
    if (index > OPTION_SIZE-1) index = 0;

    lcd.setCursor(0,0);
    lcd.print(format_string("> %s", options[index]));
    lcd.setCursor(0,1);
    lcd.print(index);
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
    checkTemperature();

    int x =  (int)analogRead(A2);
    int y =  (int)analogRead(A3);
    int on = (int)digitalRead(7);
    /* Serial.println(format_string("%d", (double)TH02.ReadTemperature())); */

    /* Serial.println((int)analogRead(A1), HEX); */

    int oscill = abs(sin(((double)millis()/1000)-floor((double)millis()/1000)*2*3.14));
    analogWrite(8, 255/2);

    int orientation = x + y - 1023;

    if (!(orientation >= -100 && orientation <= 100)) {
        if (x >= 750 || y >= 750)
            UP = true;

        if (x <= 350 || y <= 350)
            DOWN = true;
    }

    if (in_menu) handlers[index](UP, DOWN, x, y, on);
    else main_menu(UP, DOWN, x, y, on);

    UP = false;
    DOWN =false;

    delay(250);
}
