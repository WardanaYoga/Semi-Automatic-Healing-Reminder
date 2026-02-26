#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>
#include <WebServer.h>
#include <Preferences.h>


// ================= WIFI =================
const char* ssid     = "AI-CENTER";
const char* password = "aicenter";

// ================= NTP =================
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;
const int   daylightOffset_sec = 0;

// ================= WEB SERVER =================
WebServer server(80);
bool manualOverride = false;
unsigned long manualTime = 0;

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= DFPLAYER =================
HardwareSerial dfSerial(2);
DFRobotDFPlayerMini mp3;

Preferences prefs;

// ================= SERVO =================
Servo s1, s2, s3, s4, s5;

// ================= SUDUT SERVO =================
int sudutS1 = 0;
int sudutS2 = 0;
int sudutS3 = 0;
int sudutS4 = 0;
int sudutS5 = 0;

// ================= JADWAL =================
int kloter1_hour, kloter1_min;
int kloter2_hour, kloter2_min;
int kloter3_hour, kloter3_min;

// ================= FLAG =================
bool done1 = false, done2 = false, done3 = false;
int lastMinute = -1;

// ================= MODE =================
enum Mode { MODE_JAM, MODE_ALARM };
Mode currentMode = MODE_JAM;
unsigned long alarmStart = 0;

// ================= LCD CUSTOM =================
String lcdAtasDefault  = "     WAKTU:     ";
String lcdBawahDefault = "    00:00:00    ";

String lcdAtasCustom;
String lcdBawahCustom;
bool lcdCustomAktif = false;

// ================= LED =================
#define LED_PUTIH 5
#define LED_MERAH 4

unsigned long ledPutihStart = 0;
bool ledPutihAktif = false;

unsigned long ledMerahStart = 0;
bool ledMerahAktif = false;

bool mp3Aktif = false;
int mp3Count = 0;
unsigned long mp3Start = 0;
unsigned long mp3Durasi = 0;
int mp3Track = 0;

// =================================================
// =================== FUNCTION =====================
// =================================================

void gerakServoStep(Servo &servo, int &sudut, const char* key) {

  int sudutLama = sudut;

  if (sudut >= 180) {
    sudut = 0;
  } else {
    sudut += 30;
  }

  servo.write(sudut);
  prefs.putInt(key, sudut);

  // LED MERAH jika baru mencapai 180
  if (sudut == 180 && sudutLama != 180) {
    digitalWrite(LED_MERAH, HIGH);
    ledMerahStart = millis();
    ledMerahAktif = true;
  }
}

void triggerLedPutih() {
  digitalWrite(LED_PUTIH, HIGH);
  ledPutihStart = millis();
  ledPutihAktif = true;
}

void mulaiMP3(int track, unsigned long durasi) {
  mp3Track = track;
  mp3Durasi = durasi;
  mp3Count = 0;
  mp3Aktif = true;

  mp3.play(track);
  mp3Start = millis();
}

void updateMP3() {
  if (mp3Aktif) {
    if (millis() - mp3Start >= mp3Durasi) {
      mp3Count++;
      if (mp3Count < 3) {
        mp3.play(mp3Track);
        mp3Start = millis();
      } else {
        mp3Aktif = false;
      }
    }
  }
}

void tampilPesan() {
  if (currentMode != MODE_ALARM) {
    lcd.clear();
    currentMode = MODE_ALARM;
    alarmStart = millis();
  }
  lcd.setCursor(0, 0);
  lcd.print("    Saatnya    ");
  lcd.setCursor(0, 1);
  lcd.print("  minum obat  ");
}

void tampilJam(int jam, int menit, int detik) {
  if (lcdCustomAktif) return;   // 

  if (currentMode != MODE_JAM) {
    lcd.clear();
    currentMode = MODE_JAM;
  }

  char buf[17];
  lcd.setCursor(0, 0);
  lcd.print("     WAKTU:     ");
  sprintf(buf, "    %02d:%02d:%02d   ", jam, menit, detik);
  lcd.setCursor(0, 1);
  lcd.print(buf);
}

// ================= KLOTER =================
void jalankanKloter(int no) {
  manualOverride = true;
  manualTime = millis();

  triggerLedPutih();

  tampilPesan();
  mulaiMP3(2, 4500);

  if (no == 1) {          // s1 s2 s3
    gerakServoStep(s1, sudutS1, "s1");
    gerakServoStep(s2, sudutS2, "s2");
    gerakServoStep(s3, sudutS3, "s3");
  }
  else if (no == 2) {     // s2 s3 s4
    gerakServoStep(s2, sudutS2, "s2");
    gerakServoStep(s3, sudutS3, "s3");
    gerakServoStep(s4, sudutS4, "s4");
  }
  else if (no == 3) {     // s3 s4 s5
    gerakServoStep(s3, sudutS3, "s3");
    gerakServoStep(s4, sudutS4, "s4");
    gerakServoStep(s5, sudutS5, "s5");
  }
}

void resetServo(const char* key, int &sudut, Servo &servo) {
  sudut = 0;
  servo.write(0);
  prefs.putInt(key, 0);
}

void resetSemuaServo() {
  sudutS1 = sudutS2 = sudutS3 = sudutS4 = sudutS5 = 0;

  s1.write(0);
  s2.write(0);
  s3.write(0);
  s4.write(0);
  s5.write(0);

  prefs.putInt("s1", 0);
  prefs.putInt("s2", 0);
  prefs.putInt("s3", 0);
  prefs.putInt("s4", 0);
  prefs.putInt("s5", 0);
}

// ================= HTTP =================
void handleKloter() {
  if (!server.hasArg("no")) {
    server.send(400, "text/plain", "No kloter?");
    return;
  }
  int no = server.arg("no").toInt();
  if (no < 1 || no > 3) {
    server.send(400, "text/plain", "Invalid");
    return;
  }
  jalankanKloter(no);
  server.send(200, "text/plain", "OK");
}

void handleReset() {
  manualOverride = true;
  manualTime = millis();

  resetSemuaServo();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   SERVO RESET ");
  lcd.setCursor(0, 1);
  lcd.print("     BERHASIL  ");

  server.send(200, "text/plain", "RESET OK");
}

void handleSetJadwal() {
  if (!server.hasArg("kloter") || !server.hasArg("jam") || !server.hasArg("menit")) {
    server.send(400, "text/plain", "Parameter kurang");
    return;
  }

  int k = server.arg("kloter").toInt();
  int h = server.arg("jam").toInt();
  int m = server.arg("menit").toInt();

  if (h < 0 || h > 23 || m < 0 || m > 59) {
    server.send(400, "text/plain", "Jam/menit tidak valid");
    return;
  }

  switch (k) {
    case 1:
      kloter1_hour = h;
      kloter1_min  = m;
      prefs.putInt("k1_h", h);
      prefs.putInt("k1_m", m);
      break;

    case 2:
      kloter2_hour = h;
      kloter2_min  = m;
      prefs.putInt("k2_h", h);
      prefs.putInt("k2_m", m);
      break;

    case 3:
      kloter3_hour = h;
      kloter3_min  = m;
      prefs.putInt("k3_h", h);
      prefs.putInt("k3_m", m);
      break;

    default:
      server.send(400, "text/plain", "Kloter tidak valid");
      return;
  }

  server.send(200, "text/plain", "Jadwal tersimpan");
}
void handleGetJadwal() {
  String json = "{";
  json += "\"k1\":\"" + String(kloter1_hour) + ":" + String(kloter1_min) + "\",";
  json += "\"k2\":\"" + String(kloter2_hour) + ":" + String(kloter2_min) + "\",";
  json += "\"k3\":\"" + String(kloter3_hour) + ":" + String(kloter3_min) + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handleSetLCD() {
  if (!server.hasArg("atas") || !server.hasArg("bawah")) {
    server.send(400, "text/plain", "Parameter kurang");
    return;
  }

  lcdAtasCustom  = server.arg("atas");
  lcdBawahCustom = server.arg("bawah");

  // Batasi 16 karakter
  lcdAtasCustom  = lcdAtasCustom.substring(0, 16);
  lcdBawahCustom = lcdBawahCustom.substring(0, 16);

  prefs.putString("lcdAtas", lcdAtasCustom);
  prefs.putString("lcdBawah", lcdBawahCustom);
  prefs.putBool("lcdAktif", true);

  lcdCustomAktif = true;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(lcdAtasCustom);
  lcd.setCursor(0, 1);
  lcd.print(lcdBawahCustom);

  server.send(200, "text/plain", "LCD UPDATED");
}

void handleLCDDefault() {
  prefs.remove("lcdAtas");
  prefs.remove("lcdBawah");
  prefs.putBool("lcdAktif", false);

  lcdCustomAktif = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(lcdAtasDefault);
  lcd.setCursor(0, 1);
  lcd.print(lcdBawahDefault);

  server.send(200, "text/plain", "LCD DEFAULT");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  prefs.begin("servo", false);

  lcdCustomAktif = prefs.getBool("lcdAktif", false);

  if (lcdCustomAktif) {
    lcdAtasCustom  = prefs.getString("lcdAtas", lcdAtasDefault);
    lcdBawahCustom = prefs.getString("lcdBawah", lcdBawahDefault);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(lcdAtasCustom);
    lcd.setCursor(0, 1);
    lcd.print(lcdBawahCustom);
  }

  kloter1_hour = prefs.getInt("k1_h", 19);
  kloter1_min  = prefs.getInt("k1_m", 30);

  kloter2_hour = prefs.getInt("k2_h", 19);
  kloter2_min  = prefs.getInt("k2_m", 32);

  kloter3_hour = prefs.getInt("k3_h", 19);
  kloter3_min  = prefs.getInt("k3_m", 34);

  sudutS1 = prefs.getInt("s1", 0);
  sudutS2 = prefs.getInt("s2", 0);
  sudutS3 = prefs.getInt("s3", 0);
  sudutS4 = prefs.getInt("s4", 0);
  sudutS5 = prefs.getInt("s5", 0);

  lcd.init();
  lcd.backlight();
  lcd.print("Connect WiFi");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  dfSerial.begin(9600, SERIAL_8N1, 16, 17);
  mp3.begin(dfSerial);
  mp3.volume(25);

  // ATTACH
  s1.attach(27);
  s2.attach(26);
  s3.attach(25);
  s4.attach(33);
  s5.attach(32);

  pinMode(LED_PUTIH, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);

  digitalWrite(LED_PUTIH, LOW);
  digitalWrite(LED_MERAH, LOW);

  // POSISI TERAKHIR
  s1.write(sudutS1);
  s2.write(sudutS2);
  s3.write(sudutS3);
  s4.write(sudutS4);
  s5.write(sudutS5);

  server.on("/kloter", handleKloter);
  server.on("/setJadwal", handleSetJadwal);
  server.on("/jadwal", handleGetJadwal);
  server.on("/reset", handleReset);
  server.on("/lcd", handleSetLCD);
  server.on("/lcdDefault", handleLCDDefault);

  server.begin();

  lcd.clear();
  Serial.print("IP ESP32: ");
  Serial.println(WiFi.localIP());
}

// ================= LOOP =================
void loop() {
  server.handleClient();
  updateMP3();

  struct tm timeinfo;
  bool timeOk = getLocalTime(&timeinfo);

  int jam = 0, menit = 0, detik = 0;

  if (timeOk) {
    jam   = timeinfo.tm_hour;
    menit = timeinfo.tm_min;
    detik = timeinfo.tm_sec;

    // Reset flag tiap menit
    if (menit != lastMinute) {
      done1 = done2 = done3 = false;
      lastMinute = menit;
    }

    if (!manualOverride) {
      if (jam == kloter1_hour && menit == kloter1_min && !done1) {
        jalankanKloter(1);
        done1 = true;
      }
      if (jam == kloter2_hour && menit == kloter2_min && !done2) {
        jalankanKloter(2);
        done2 = true;
      }
      if (jam == kloter3_hour && menit == kloter3_min && !done3) {
        jalankanKloter(3);
        done3 = true;
      }
    }
  }

  // ========================
  // LOGIKA MODE DIPISAHKAN
  // ========================

  if (currentMode == MODE_ALARM && millis() - alarmStart > 10000) {
    currentMode = MODE_JAM;
  }

  if (currentMode == MODE_JAM) {
    tampilJam(jam, menit, detik);
  }

  // ===== TIMER LED =====
  if (ledPutihAktif && millis() - ledPutihStart >= 20000) {
    digitalWrite(LED_PUTIH, LOW);
    ledPutihAktif = false;
  }

  if (ledMerahAktif && millis() - ledMerahStart >= 10000) {
    digitalWrite(LED_MERAH, LOW);
    ledMerahAktif = false;
  }
}
