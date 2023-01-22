/// TODO: Refactor this hunk of trash

#include "rgb_lcd.h"
#include <LiquidCrystal.h>
#include <TH02_dev.h>
#include <IRremote.hpp>
#include <IRProtocol.h>
#include <OneShotTimer.h>

#define OPTION_SIZE 2

#define CFG_METRIC 0
#define CFG_SILENT 1
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
#define IR_PLAY 0xBC43FF00

#define PIN_BUZZER 4
#define PIN_TEMP   A0
#define PIN_JOYSW  7
#define PIN_JOY1   A2
#define PIN_JOY2   A3
#define PIN_IR     5

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

int alarm[3] = {0, 0, 5};
int settings[2] = {0, 0};
OneShotTimer* alarm_instance; 

const char* print_time(rgb_lcd lcd, int h, int m, int s) {
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

int manip(int index, int entry, int digit) {
  if (digit > 9) return -1;
  if (index % 2 == 0) return (entry % 10) + 10*digit;
  else return (entry - (entry % 10)) + digit;
} 

// i mean, you don't really need to validate it
/* void validate(int orig[3]) { */
/*     if (orig[2] > 59) { */
/*         orig[1] += floor(orig[2] / 60) */
/*     } */
/*  */
/*     if (orig[1] > 59) { */
/*         orig[0] += floor(orig[1]) */
/*     } */
/* } */

void alarm_menu(bool up, bool down, int _jx, int _jy, bool pressed, IRData* ir) {
    bool ir_safe = ir != NULL && ir->protocol == decode_type_t::NEC;

  if (!pressed || (ir_safe && ir->decodedRawData == IR_PLAY)) {
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
        digitalWrite(PIN_BUZZER, HIGH);
        lcd_color = 2;
    });

    alarm_instance->start();

    in_menu = false;
    delay(250);
    return exit();
  }

  // TODO: pure cancer
  if (_alarm_increment) {
      if (ir_safe) {
          int _digit = -1;
          switch(ir->decodedRawData) {
              case IR_PLUS:
                alarm[(int)floor(in_menu_index/2)]++;
                break;
              case IR_MINUS:
                alarm[(int)floor(in_menu_index/2)]--;
                break;
              case IR_ZERO:
                _digit = 0;
                break;
              case IR_ONE:
                _digit = 1;
                break;
              case IR_TWO:
                _digit = 2;
                break;
              case IR_THREE:
                _digit = 3;
                break;
              case IR_FOUR:
                _digit = 4;
                break;
              case IR_FIVE:
                _digit = 5;
                break;
              case IR_SIX:
                _digit = 6;
                break;
              case IR_SEVEN:
                _digit = 7;
                break;
              case IR_EIGHT:
                _digit = 8;
                break;
              case IR_NINE:
                _digit = 8;
                break;
              default:
                break;
          }

        if (_digit >= 0) alarm[(int)floor(in_menu_index/2)] = manip(in_menu_index, alarm[(int)floor(in_menu_index/2)], _digit);
      }

      if (up) alarm[(int)floor(in_menu_index/2)]++;
      if (down) alarm[(int)floor(in_menu_index/2)]--;
      if (alarm[(int)floor(in_menu_index/2)] < 0) alarm[(int)floor(in_menu_index/2)] = 0;
  } else {
      lcd.cursor();

      if (ir_safe) {
          switch(ir->decodedRawData) {
              case IR_PLUS:
                in_menu_index++;
                return;
              case IR_MINUS:
                in_menu_index--;
                return;
          }
      }

      if (up) in_menu_index++;
      if (down) in_menu_index--;
  }

  // this should logically be contained within the _alarm_increment if statement
  // but i'm too paranoid
  if (in_menu_index > 6 || in_menu_index < 0) in_menu_index = 0;

  lcd.clear();
  lcd.print("Alarm");
  lcd.setCursor(0, 1);
  print_time(lcd, alarm[0], alarm[1], alarm[2]);
  lcd.write(1);
  lcd.setCursor(in_menu_index + floor(in_menu_index/2), 1);
}

void temperature_menu(bool up, bool down, int _jx, int _jy, bool pressed, IRData* ir) {
  if (!pressed || (ir != NULL && ir->protocol == decode_type_t::NEC && ir->decodedRawData == IR_PLAY)) {
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

const char *options[OPTION_SIZE] = {
    "Temperature",
    "Alarms     ",
};

handler_func handlers[OPTION_SIZE] = {temperature_menu, alarm_menu};

double fetch_temperature(bool faranheit) {
  int raw_adc = analogRead(PIN_TEMP);
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

  if (ir != NULL && ir->protocol == decode_type_t::NEC) {
      switch (ir->decodedRawData) {
          case IR_PLUS:
              index++;
              return;
          case IR_MINUS:
              index--;
              return;
          case IR_PLAY:
              in_menu = true;
              return;
          default:
              return;
      }
  }

  if (up)
    index--;
  if (index + 1 == 0)
    index = OPTION_SIZE - 1;
  if (down)
    index++;
  if (index > OPTION_SIZE - 1)
    index = 0;

  lcd.setCursor(0, 0);
  lcd.print("Sapling");
  lcd.setCursor(0, 1);
  format_print(lcd, "> %s", options[index]);
}

void setup() {
  Serial.begin(115200);
  // pin connections may vary depending on circuit
  // ?
  pinMode(A5, INPUT);
  pinMode(A4, INPUT);
  pinMode(PIN_JOYSW, INPUT);
  pinMode(PIN_TEMP, INPUT);

  IrReceiver.begin(PIN_IR, true);
  digitalWrite(PIN_JOYSW, HIGH);

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
  if (!CFG_IGNORE_TEMP) checkTemperature();

  if (alarm_instance != NULL) alarm_instance->update();
  if (IrReceiver.decode()) {
      _ir_data = IrReceiver.decodedIRData;
      _ir_updated = true;
      IrReceiver.resume();
  }

  int x = (int)analogRead(PIN_JOY1);
  int y = (int)analogRead(PIN_JOY2);
  int on = (int)digitalRead(PIN_JOYSW);

  int orientation = x + y - 1023;

  if (lcd_color && !settings[CFG_SILENT]) digitalWrite(PIN_BUZZER, HIGH);
  else digitalWrite(PIN_BUZZER, LOW);

  if (!(orientation >= -100 && orientation <= 100)) {
    if (x >= 750 || y >= 750)
      UP = true;

    if (x <= 350 || y <= 350)
      DOWN = true;
  }

  if (lcd_color == 2 && (UP || DOWN || !on || _ir_updated)) {
      free(alarm_instance); // TODO: mem leak
      lcd_color = 0;
  }

  lcd.setColor(lcd_color);

  if (in_menu)
    handlers[index](UP, DOWN, x, y, on, _ir_updated ? &_ir_data : NULL);
  else
    main_menu(UP, DOWN, x, y, on, _ir_updated ? &_ir_data : NULL);

  UP = false;
  DOWN = false;
  _ir_updated = false;

  delay(100);
}

