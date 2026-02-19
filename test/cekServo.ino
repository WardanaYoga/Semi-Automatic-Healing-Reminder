#include <ESP32Servo.h>

Servo servo;

void setup() {
  servo.attach(27);   // ganti sesuai servo yang diuji    // posisi tengah
  servo.write(0);
  servo.attach(26);   // ganti sesuai servo yang diuji    // posisi tengah
  servo.write(0);
  servo.attach(25);   // ganti sesuai servo yang diuji    // posisi tengah
  servo.write(0);
  servo.attach(33);   // ganti sesuai servo yang diuji    // posisi tengah
  servo.write(0);
  servo.attach(32);   // ganti sesuai servo yang diuji    // posisi tengah
  servo.write(0);
}

void loop() {
}
