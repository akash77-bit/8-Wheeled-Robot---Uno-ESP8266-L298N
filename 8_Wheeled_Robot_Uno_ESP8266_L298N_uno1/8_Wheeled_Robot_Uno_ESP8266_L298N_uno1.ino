// 8-Wheeled Robot - Arduino Uno (same logic as robot_test.ino)
// Receives commands from the ESP8266 on Serial: F/B/L/R/S + speed, A<angle>, D
#include <Servo.h>

const int ENA = 5, IN1 = 7, IN2 = 8;    // left motor
const int ENB = 6, IN3 = 12, IN4 = 13;  // right motor
const int SERVO_PIN = 10, TRIG = A0, ECHO = A1;
const int STOP_DISTANCE_CM = 20;

Servo scanServo;
char currentDir = 'S';
int speedValue = 150;
String cmd = "";
unsigned long lastCharTime = 0, lastDistCheck = 0;

void setSide(int en, int inA, int inB, int dir, int spd) {
  digitalWrite(inA, dir == 1 ? HIGH : LOW);
  digitalWrite(inB, dir == -1 ? HIGH : LOW);
  analogWrite(en, dir == 0 ? 0 : spd);
}

void drive(char d) {
  switch (d) {
    case 'F': setSide(ENA, IN1, IN2, 1, speedValue);  setSide(ENB, IN3, IN4, 1, speedValue);  break;
    case 'B': setSide(ENA, IN1, IN2, -1, speedValue); setSide(ENB, IN3, IN4, -1, speedValue); break;
    case 'L': setSide(ENA, IN1, IN2, -1, speedValue); setSide(ENB, IN3, IN4, 1, speedValue);  break;
    case 'R': setSide(ENA, IN1, IN2, 1, speedValue);  setSide(ENB, IN3, IN4, -1, speedValue); break;
    default:  setSide(ENA, IN1, IN2, 0, 0); setSide(ENB, IN3, IN4, 0, 0); d = 'S'; break;
  }
  currentDir = d;
}

long readDistanceCm() {
  digitalWrite(TRIG, LOW); delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long us = pulseIn(ECHO, HIGH, 30000UL);
  if (us == 0) return 999;
  return us / 58;
}

void handleCommand(String c) {
  c.trim();
  if (c.length() == 0) return;
  char letter = toupper(c.charAt(0));
  int value = (c.length() > 1) ? c.substring(1).toInt() : -1;
  if (letter == 'A') {
    if (value < 0) value = 90;
    scanServo.write(constrain(value, 0, 180));
    return;
  }
  if (letter == 'D') {
    Serial.print(F("Distance: ")); Serial.println(readDistanceCm());
    return;
  }
  if (letter == 'F' || letter == 'B' || letter == 'L' || letter == 'R' || letter == 'S') {
    if (value >= 0) speedValue = constrain(value, 0, 255);
    drive(letter);
    Serial.print(F("Direction: ")); Serial.print(currentDir);
    Serial.print(F("  Speed: ")); Serial.println(speedValue);
  }
}

void setup() {
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  scanServo.attach(SERVO_PIN);
  scanServo.write(90);
  Serial.begin(9600);   // same baud as the ESP8266 bridge
  drive('S');
  Serial.println(F("Uno ready"));
}

void loop() {
  while (Serial.available()) {
    char ch = Serial.read();
    lastCharTime = millis();
    if (ch == '\n' || ch == '\r') { handleCommand(cmd); cmd = ""; }
    else cmd += ch;
  }
  if (cmd.length() > 0 && millis() - lastCharTime > 100) { handleCommand(cmd); cmd = ""; }

  if (millis() - lastDistCheck > 100) {
    lastDistCheck = millis();
    if (currentDir == 'F' && readDistanceCm() < STOP_DISTANCE_CM) {
      drive('S');
      Serial.println(F("Obstacle - stopped"));
    }
  }
}
