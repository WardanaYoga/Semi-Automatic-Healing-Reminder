#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include "time.h"

const char* ssid     = "AI-CENTER";
const char* password = "aicenter";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;   // WIB (UTC+7)
const int   daylightOffset_sec = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
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

  char timeString[16];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);
  lcd.clear();
  lcd.setCursor(5,0);
  lcd.print("Waktu:");
  lcd.setCursor(4,1);
  lcd.print(timeString);

  delay(1000);
}
