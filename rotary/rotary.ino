uint16_t counter = 0;
int in_revolution = false;

void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
}

void loop() {
  int x = analogRead(A0);
  int y = analogRead(A1);
  if (!in_revolution) {
    if (y == 0 && x != 0) {
      in_revolution = true;
      counter++; // clockwise
    }
    if (x == 0 && y != 0) {
      in_revolution = true;
      counter--; // clockwise
    }
    in_revolution = false;
  }
  Serial.print(x);
  Serial.print(" ");
  Serial.print(y);
  Serial.print(" ");
  Serial.println(counter);
  delay(50);
}
