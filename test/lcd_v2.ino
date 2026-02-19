#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include "time.h"

const char* ssid     = "AI-CENTER";
const char* password = "aicenter";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;   // WIB
const int   daylightOffset_sec = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool sudahTampil = false;   // agar tidak berulang

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  lcd.clear();
  lcd.print("WiFi Connected");
  delay(1000);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    lcd.setCursor(0,0);
    lcd.print("NTP Error      ");
    return;
  }

  int jam = timeinfo.tm_hour;
  int menit = timeinfo.tm_min;
  int detik = timeinfo.tm_sec;

  // === KONDISI ALARM 17:07 ===
  if (jam == 17 && menit == 20 && !sudahTampil) {
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Saatnya Minum");
    lcd.setCursor(0,1);
    lcd.print("Obat...");

    delay(10000);   // tampil 10 detik

    sudahTampil = true;
    lcd.clear();
  }

  // reset flag setelah lewat menit 7
  if (menit != 7) {
    sudahTampil = false;
  }

  // === TAMPIL JAM NORMAL ===
  char timeString[16];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);

  lcd.clear();
  lcd.setCursor(5,0);
  lcd.print("Waktu:");
  lcd.setCursor(4,1);
  lcd.print(timeString);

  delay(1000);
}
