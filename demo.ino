#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= DFPLAYER =================
HardwareSerial dfSerial(2);   // UART2
DFRobotDFPlayerMini mp3;

// ================= SERVO =================
Servo servo1, servo2, servo3, servo4, servo5;

void setup() {
  Serial.begin(115200);
  delay(500);

  // ================= DFPLAYER INIT =================
  dfSerial.begin(9600, SERIAL_8N1, 16, 17);

  if (!mp3.begin(dfSerial)) {
    Serial.println("DFPlayer gagal!");
    while (1);
  }

  mp3.volume(25);      // 0–30
  delay(500);

  // ================= LCD INIT =================
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Siap");
  lcd.setCursor(0, 1);
  lcd.print("DFPlayer OK");

  delay(1000);

  // ================= SERVO INIT =================
  servo1.attach(27);
  servo2.attach(26);
  servo3.attach(25);
  servo4.attach(33);
  servo5.attach(32);

  // Posisi awal (aman)
  servo1.write(0);
  servo2.write(0);
  servo3.write(0);
  servo4.write(0);
  servo5.write(0);

  delay(500);

  // ================= PLAY AUDIO =================
  mp3.play(1);   // mainkan 0001.mp3
}

void loop() {
  // Contoh gerakan servo bergantian (AMAN DARI DROP TEGANGAN)
  servo1.write(90);
  delay(500);

  servo2.write(90);
  delay(500);

  servo3.write(90);
  delay(500);

  servo4.write(90);
  delay(500);

  servo5.write(90);
  delay(1000);

  // Kembali ke posisi awal
  servo1.write(0);
  servo2.write(0);
  servo3.write(0);
  servo4.write(0);
  servo5.write(0);

  delay(2000);

  // Putar suara kedua
  mp3.play(2);   // 0002.mp3
  delay(5000);
}
