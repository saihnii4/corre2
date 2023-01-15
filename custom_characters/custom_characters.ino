#include <rgb_lcd.h>

rgb_lcd lcd;

byte _cat_tl[8] = {
    B00000,
    B00000,
    B00010,
    B00101,
    B01000,
    B01000,
    B01010,
    B01000
};

byte _cat_tr[8] = {
    B00000,
    B00000,
    B01000,
    B10100,
    B00010,
    B00010,
    B01010,
    B00010
};

byte _cat_bl[8] = {
    B01010,
    B01001,
    B01000,
    B01101,
    B01010,
    B00000,
    B00000,
    B00000
};

byte _cat_br[8] = {
    B10010,
    B01010,
    B00010,
    B10110,
    B01010,
    B00000,
    B00000,
    B00000
};

uint8_t cat_tl = 0;
uint8_t cat_tr = 1;
uint8_t cat_bl = 2;
uint8_t cat_br = 3;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.createChar(cat_tl, _cat_tl);
  lcd.createChar(cat_tr, _cat_tr);
  lcd.createChar(cat_bl, _cat_bl);
  lcd.createChar(cat_br, _cat_br);
  lcd.clear();
    lcd.write(cat_tl);
    lcd.write(cat_tr);
    lcd.setCursor(0,1);
    lcd.write(cat_bl);
    lcd.write(cat_br);
}

void loop() {
}
