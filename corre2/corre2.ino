#include "rgb_lcd.h"
#include <LiquidCrystal.h>
#include <TH02_dev.h>
#include <OneShotTimer.h>

#define OPTION_SIZE 3 // too fucking lazy mate

// TODO: wtf
int index;
bool in_menu = false;

void display_freeram() {
  Serial.println(freeRam());
}


int freeRam() {
  extern int __heap_start,*__brkval;
  int v;

  return (int)&v - (__brkval == 0  

    ? (int)&__heap_start : (int) __brkval);  

}

byte celsius[8] = {B00000, B10000, B00110, B01000,
                   B01000, B00110, B00000, B00000};

byte check_mark[8] ={
    B00000,
    B00000,
    B00001,
    B00010,
    B10100,
    B01000,
    B00000,
    B00000
};

rgb_lcd lcd;
bool UP, DOWN;
typedef void (*handler_func)(bool, bool, int, int, int);

// TODO: better state management
int _ref_counter;
int _but_last_state;
int alarm[3] = {10, 10, 10};

/* // TODO: maybe ? */
/* template <typename Variable> */
/* const char* format_list(const char* format, Variable var) { */
/*     return format_string(format, var); */
/* } */
/*  */
/* // it is easy to see that the subset must contain the same type of elements */
/* // as its predicate */
/* template <typename Init, typename ...Subset> */
/* const char* format_list(const char* format, Init initial, Subset... subset) { */
/*     Serial.println(initial); */
/*     const char* f = format_string(format, initial); */
/*     return format_list(format_string("%s%s", f, format), subset...); */
/* } */

const char* format_time(int h, int m, int s) { // TODO: bruh
    // TODO: null byte terminator in format_string bruhh
    char* _h = (h < 10) ? format_string("0%d", h) : format_string("%d", h);
    char* _m = (m < 10) ? format_string("0%d", m) : format_string("%d", m);
    char* _s = (s < 10) ? format_string("0%d", s) : format_string("%d", s);

    return format_string("%s:%s:%s", _h, _m, _s);
}

// TODO: Separate menu and in-menu indices (a part of state management)
int _alarm_index = 0;
bool _alarm_increment = false;

// handles menu exit [cleans up LCD cursor and does stuff]
void exit() {
    Serial.println("exit() called");
    lcd.noCursor();
    lcd.noBlink();
    lcd.clear();
}

// TODO: menu exits early for some reason
void alarm_menu(bool up, bool down, int _jx, int _jy, bool pressed) {
  if (!pressed) {
      Serial.println("exit()");
    if (_alarm_index != 6) {
        _alarm_increment = true;
        return lcd.blink();
    }

    in_menu = false;
    delay(250);
    return exit();
  }

  // TODO: pure cancer
  if (_alarm_increment) {
      if (up) alarm[(int)floor(_alarm_index/2)]++;
      if (down) alarm[(int)floor(_alarm_index/2)]--;
  } else {
      lcd.cursor();

      if (up) _alarm_index++;
      if (down) _alarm_index--;
  }

  // this should logically be contained within the _alarm_increment if statement
  // but i'm too paranoid
  if (_alarm_index > 6 || _alarm_index < 0) _alarm_index = 0;
  
  if (!_but_last_state && digitalRead(3)) {
      _ref_counter++;
      _but_last_state = 0;
  }

  (int)digitalRead(3);
  const char* display = format_time(alarm[0], alarm[1], alarm[2]);

  lcd.clear();
  lcd.print("Alarm");
  lcd.setCursor(0, 1);
  lcd.print(format_time(alarm[0], alarm[1], alarm[2]));
  lcd.setCursor(strlen(display)+1, 1);
  lcd.write(1);
  lcd.setCursor(_alarm_index + floor(_alarm_index/2), 1);
}

void temperature_menu(bool up, bool down, int _jx, int _jy, bool pressed) {
  if (!pressed) {
    Serial.println("non-exit()");
    in_menu = false;
    delay(250);
    return lcd.clear();
  }

  lcd.clear();
  lcd.print("Temperature:");
  lcd.setCursor(0, 1);
  double temp = fetch_temperature();
  lcd.print(temp);
  /* lcd.setCursor(5, 1); */ // wtf
                             /* lcd.write(a); */
}

void settings_menu(bool up, bool down, int jx, int jy, bool pressed) {
  if (!pressed) {
    in_menu = false;
    delay(250);
    return lcd.clear();
  }

  lcd.clear();
  lcd.print("Settings");
}

// TODO: scroll overflow & translation
const char *options[OPTION_SIZE] = {
    "Temperature",
    "Alarms     ",
    "Settings   ",
};

handler_func handlers[OPTION_SIZE] = {temperature_menu, alarm_menu,
                                      settings_menu};

double fetch_temperature() {
  int raw_adc = analogRead(A0);
  double temp = log(10000 * ((1024.0 / raw_adc - 1)));
  temp = 1 /
         (0.001129148 + (0.000234125 + (0.0000000876741 * temp * temp)) * temp);

  return temp - 273.15; // apparently data is sent in kelvins
}

// some code i lifted from another project
template <typename... Args>
const char *format_string(const char *format, Args... args) {
  size_t nbytes = snprintf(NULL, 0, format, args...);

  // a null terminator is added (no bueno) if we allocate nbytes of memory
  char *buf = (char *)malloc(sizeof(char) * (nbytes+1));
  snprintf(buf, nbytes+1, format, args...);
  return buf;
}

// TODO: make clear that 9 is not a magic constant
template <typename... Args>
const char *format_date(const char *format, struct tm *_time) {
  char *buf = (char *)malloc(sizeof(char) * 9);
  strftime(buf, 9, format, _time);
  return buf;
}

template <typename... Args> void log(const char *module, const char *message) {
  Serial.println(format_string("[%s] %s", module, message));
}

// this is so braindead oialnm,awoijlknk
void checkTemperature() {
  double temp = fetch_temperature();
  if (temp >= 25 || temp <= 20) {
    digitalWrite(4, HIGH); // buzzer
    if (temp >= 25)
      lcd.setColor(1);
    if (temp <= 20)
      lcd.setColor(3);
  } else {
    digitalWrite(4, LOW);
    lcd.setColor(0);
  }
}


int last_state;

void main_menu(bool up, bool down, int _jx, int _jy, int pressed) {
  if (!pressed)
    in_menu = true;

  if (up)
    index--;
  if (up && index + 1 == 0)
    index = OPTION_SIZE - 1;
  if (down)
    index++;
  if (index > OPTION_SIZE - 1)
    index = 0;

  // don't re-render
  if (last_state == index) return;

  last_state = index;

  lcd.setCursor(0, 0);
  lcd.print(format_string("> %s", options[index]));
  lcd.setCursor(0, 1);
  lcd.print(index);
}

void setup() {
  Serial.begin(115200);
  // pin connections may vary depending on circuit
  pinMode(A5, INPUT);
  pinMode(A4, INPUT);
  pinMode(7, INPUT);
  pinMode(A0, INPUT);

  digitalWrite(7, HIGH);

  uint8_t b = 1; // TODO: WTFFFF!!!
  uint8_t a = 0; // TODO: WTFFFF!!!

  lcd.begin(16, 2);
  lcd.createChar(a, celsius);
  lcd.createChar(b, check_mark);
  Serial.println("testing");
  lcd.clear();
}

void loop() {
  display_freeram();
  checkTemperature();

  int x = (int)analogRead(A2);
  int y = (int)analogRead(A3);
  int on = (int)digitalRead(7);
  // /* Serial.println(format_string("%d", (double)TH02.ReadTemperature())); */

  // /* Serial.println((int)analogRead(A1), HEX); */
  /* analogWrite(8, 255 / 2); */

  int orientation = x + y - 1023;

  if (!(orientation >= -100 && orientation <= 100)) {
    if (x >= 750 || y >= 750)
      UP = true;

    if (x <= 350 || y <= 350)
      DOWN = true;
  }

  // if (in_menu) {
  //     /* switch (index) { */
  //     /*     case 0: */
  //     /*         return temperature_menu(UP, DOWN, x, y, on); */
  //     /*     case 1: */
  //     /*         return alarm_menu(UP, DOWN, x, y, on); */
  //     /*     case 2: */
  //     /*         return settings_menu(UP, DOWN, x, y, on); */
  //     /*     default: */
  //     /*         return; // unreachable */
  //     /* } */
  // } else main_menu(UP, DOWN, x, y, on);

  if (false)
    handlers[index](UP, DOWN, x, y, on);
  else
    main_menu(UP, DOWN, x, y, on);

  UP = false;
  DOWN = false;

  delay(250);
}

