void setup() {
    Serial.begin(9600);
    pinMode(A0, INPUT);
    pinMode(A1, INPUT);
}

void loop() {
    Serial.print((int)analogRead(A0));
    Serial.print(" ");
    Serial.println((int)analogRead(A1));
}
