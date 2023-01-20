// TODO: Refactor this hunk of trash

#include "rgb_lcd.h"
#include <LiquidCrystal.h>
#include <TH02_dev.h>
#include <IRremote.hpp>
#include <IRProtocol.h>
#include <OneShotTimer.h>

#define OPTION_SIZE 3

#define CFG_METRIC      0
#define CFG_IGNORE_TEMP 1

#define IR_ZERO 0xE916FF00
#define IR_ONE 0xF30CFF00
#define IR_TWO 0xE718FF00
#define IR_THREE 0xA15EFF00
#define IR_FOUR 0xF708FF00
#define IR_FIVE 0xE31CFF00
#define IR_SIX 0xA55AFF00
#define IR_SEVEN 0xBD42FF00
#define IR_EIGHT 0xAD52FF00
#define IR_NINE 0xB54AFF00
#define IR_PLUS 0xEA15FF00
#define IR_MINUS 0xF807FF00

int index;
bool in_menu = false;
int lcd_color;

void display_freeram() {
  Serial.println(freeRam());
}

int freeRam() {
  extern int __heap_start,*__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int) __brkval);  
}

byte celsius[8] = {
    B00000,
    B10000,
    B00111,
    B01000,
    B01000,
    B01000,
    B00111,
    B00000
};

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

byte faranheit[8] = {
    B00000,
    B10000,
    B01111,
    B01000,
    B01110,
    B01000,
    B01000,
    B00000,
};

rgb_lcd lcd;
bool UP, DOWN;
typedef void (*handler_func)(bool, bool, int, int, int, IRData*);

// TODO: better state management
int _ref_counter;
int _but_last_state;
int alarm[3] = {0, 0, 5};
int settings[2] = {0, 1};
OneShotTimer* alarm_instance; 

const char* print_time(rgb_lcd lcd, int h, int m, int s) { // TODO: bruh
    // TODO: null byte terminator in format_string bruhh
    char* _h = (h < 10) ? format_string("0%d", h) : format_string("%d", h);
    char* _m = (m < 10) ? format_string("0%d", m) : format_string("%d", m);
    char* _s = (s < 10) ? format_string("0%d", s) : format_string("%d", s);

    const char* time = format_string("%s:%s:%s", _h, _m, _s);

    free(_h);
    free(_m);
    free(_s);

    lcd.print(time);
    lcd.setCursor(strlen(time)+1, 1);
    free(time);
    return 0;
}

// TODO: Separate menu and in-menu indices (a part of state management)
int in_menu_index = 0;
bool _alarm_increment = false;

// handles menu exit [cleans up LCD cursor and does stuff]
void exit() {
    Serial.println("exit() called");
    lcd.noCursor();
    lcd.noBlink();
    lcd.clear();
    in_menu_index = 0;
}

int to_seconds(int h, int m, int s) {
    return h*60*60 + m*60 + s;
}

// TODO: menu exits early for some reason
void alarm_menu(bool up, bool down, int _jx, int _jy, bool pressed, IRData* ir) {
  if (!pressed) {
    if (_alarm_increment) {
        _alarm_increment = false;
        lcd.noBlink();
        return lcd.cursor();
    }

    if (in_menu_index != 6) {
        _alarm_increment = true;
        return lcd.blink();
    }

    alarm_instance = new OneShotTimer(to_seconds(alarm[0], alarm[1], alarm[2]));

    alarm_instance->onUpdate([&]() {
        digitalWrite(4, HIGH);
        lcd_color = 2;
    });

    alarm_instance->start();

    in_menu = false;
    delay(250);
    return exit();
  }

  // TODO: pure cancer
  if (_alarm_increment) {
      if (up) alarm[(int)floor(in_menu_index/2)]++;
      if (down) alarm[(int)floor(in_menu_index/2)]--;
      if (alarm[(int)floor(in_menu_index/2)] < 0) alarm[(int)floor(in_menu_index/2)] = 0;
  } else {
      lcd.cursor();

      if (up) in_menu_index++;
      if (down) in_menu_index--;
  }

  // this should logically be contained within the _alarm_increment if statement
  // but i'm too paranoid
  if (in_menu_index > 6 || in_menu_index < 0) in_menu_index = 0;
  
  if (!_but_last_state && digitalRead(3)) {
      _ref_counter++;
      _but_last_state = 0;
  }

  lcd.clear();
  lcd.print("Alarm");
  lcd.setCursor(0, 1);
  print_time(lcd, alarm[0], alarm[1], alarm[2]);
  lcd.write(1);
  lcd.setCursor(in_menu_index + floor(in_menu_index/2), 1);
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
  double temp = fetch_temperature(settings[CFG_METRIC]);
  lcd.print(temp);
  lcd.setCursor(5, 1);
  lcd.write((uint8_t)(settings[CFG_METRIC] ? 2 : 0));
}

void settings_menu(bool up, bool down, int jx, int jy, bool pressed, IRData* ir) {
  if (!pressed) {
    in_menu = false;
    delay(250);
    return lcd.clear();
  }

  const char* display;
  
  if (up) in_menu_index++;
  if (down) in_menu_index--;
  if (in_menu_index < 0 || in_menu_index > 1) in_menu_index = 0;

  switch (in_menu_index) {
      case 0:
          return display = format_string("> Metric: %s", CFG_METRIC);
      case 1:
          return display = format_string("> Quiet: %s", CFG_IGNORE_TEMP);
      default:
          return;
  }

  lcd.clear();
  lcd.print("Settings");
  lcd.setCursor(0, 1);
  lcd.print(display);
  free(display);
}

const char *options[OPTION_SIZE] = {
    "Temperature",
    "Alarms     ",
    "Settings   ",
};

handler_func handlers[OPTION_SIZE] = {temperature_menu, alarm_menu,
                                      settings_menu};

double fetch_temperature(bool faranheit) {
  int raw_adc = analogRead(A0);
  double temp = log(10000 * ((1024.0 / raw_adc - 1)));
  temp = 1 /
         (0.001129148 + (0.000234125 + (0.0000000876741 * temp * temp)) * temp);
  temp = temp - 273.15;

  return faranheit ? temp*9/5 + 32 : temp; // apparently data is sent in kelvins
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

template <typename... Args>
int *format_print(rgb_lcd lcd, const char *format, Args... args) {
  size_t nbytes = snprintf(NULL, 0, format, args...);

  // a null terminator is added (no bueno) if we allocate nbytes of memory
  char *buf = (char *)malloc(sizeof(char) * (nbytes));
  snprintf(buf, nbytes+1, format, args...);

  lcd.print(buf);

  free(buf);
  return 0;
}

// this is so braindead oialnm,awoijlknk
void checkTemperature() {
  if (lcd_color == 2) return;

  double temp = fetch_temperature(0);

  if (temp >= 25 || temp <= 20) {
    if (temp >= 25)
        lcd_color = 1;
    if (temp <= 20)
        lcd_color = 3;
  } else {
      lcd_color = 0;
  }
}



void main_menu(bool up, bool down, int _jx, int _jy, int pressed, IRData* ir) {
  if (!pressed) in_menu = true;

  /* if (ir != NULL) { */
  /*     Serial.println("making sure"); */
  /*     if (ir->protocol == decode_type_t::NEC && ir->decodedRawData == IR_PLUS) // https://github.com/Arduino-IRremote/Arduino-IRremote/blob/master/src/IRProtocol.h */
  /*         Serial.println("up"); */
  /* } */

  if (up)
    index--;
  if (up && index + 1 == 0)
    index = OPTION_SIZE - 1;
  if (down)
    index++;
  if (index > OPTION_SIZE - 1)
    index = 0;

  Serial.println("wow");

  lcd.setCursor(0, 0);
  Serial.println(index);
  display_freeram();
  format_print(lcd, "> %s", options[index]);
  lcd.setCursor(0, 1);
  lcd.print(index);
  Serial.print("hmm");
}

void setup() {
  Serial.begin(115200);
  // pin connections may vary depending on circuit
  pinMode(A5, INPUT);
  pinMode(A4, INPUT);
  pinMode(7, INPUT);
  pinMode(A0, INPUT);

  IrReceiver.begin(5, true);
  digitalWrite(7, HIGH);

  uint8_t b = 1; // TODO: WTFFFF!!!
  uint8_t a = 0; // TODO: WTFFFF!!!
  uint8_t c = 2;

  lcd.begin(16, 2);
  lcd.createChar(a, celsius);
  lcd.createChar(b, check_mark);
  lcd.createChar(c, faranheit);
  lcd.clear();
}

IRData _ir_data;
bool _ir_updated;

void loop() {
  /* display_freeram(); */
  checkTemperature();
  Serial.println("*");

  if (alarm_instance != NULL) alarm_instance->update();
  if (IrReceiver.decode()) {
      _ir_data = IrReceiver.decodedIRData;
      _ir_updated = true;
      IrReceiver.resume();
  }

  int x = (int)analogRead(A2);
  int y = (int)analogRead(A3);
  int on = (int)digitalRead(7);

  int orientation = x + y - 1023;

  if (lcd_color && !settings[CFG_IGNORE_TEMP]) digitalWrite(4, HIGH);
  else digitalWrite(4, LOW);

  if (!(orientation >= -100 && orientation <= 100)) {
    if (x >= 750 || y >= 750)
      UP = true;

    if (x <= 350 || y <= 350)
      DOWN = true;
  }

  if (lcd_color == 2 && (UP || DOWN || !on)) {
      free(alarm_instance);
      lcd_color = 0;
  }

  lcd.setColor(lcd_color);
  checkTemperature();

  if (in_menu)
    handlers[index](UP, DOWN, x, y, on, _ir_updated ? &_ir_data : NULL);
  else
    main_menu(UP, DOWN, x, y, on, _ir_updated ? &_ir_data : NULL);

  UP = false;
  DOWN = false;

  delay(150);
}

