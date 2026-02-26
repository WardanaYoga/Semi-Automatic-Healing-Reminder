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
const char* ssid     = "Anung";
const char* password = "anung2728";

// ================= NTP =================
const char* ntpServer       = "pool.ntp.org";
const long  gmtOffset_sec   = 7 * 3600;
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

int sudutS1 = 0, sudutS2 = 0, sudutS3 = 0, sudutS4 = 0, sudutS5 = 0;

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

// ================= LAMPU LED =================
#define PIN_LAMPU_HIJAU  5    // Menyala 20 detik saat kloter dijalankan
#define PIN_LAMPU_MERAH  4    // Menyala 10 detik jika ada servo di 180°

bool lampuHijauAktif = false;
bool lampuMerahAktif = false;
unsigned long lampuHijauStart = 0;
unsigned long lampuMerahStart = 0;

const unsigned long DURASI_LAMPU_HIJAU = 20000UL;   // 20 detik
const unsigned long DURASI_LAMPU_MERAH = 10000UL;   // 10 detik

// =================================================
// ================= FUNGSI LAMPU ==================
// =================================================

void nyalakanLampuHijau() {
  lampuHijauAktif  = true;
  lampuHijauStart  = millis();
  digitalWrite(PIN_LAMPU_HIJAU, HIGH);
}

void nyalakanLampuMerah() {
  lampuMerahAktif = true;
  lampuMerahStart = millis();
  digitalWrite(PIN_LAMPU_MERAH, HIGH);
}

/**
 * Dipanggil setiap loop untuk mematikan lampu secara non-blocking
 * setelah durasi habis.
 */
void updateLampu() {
  if (lampuHijauAktif && millis() - lampuHijauStart >= DURASI_LAMPU_HIJAU) {
    lampuHijauAktif = false;
    digitalWrite(PIN_LAMPU_HIJAU, LOW);
  }
  if (lampuMerahAktif && millis() - lampuMerahStart >= DURASI_LAMPU_MERAH) {
    lampuMerahAktif = false;
    digitalWrite(PIN_LAMPU_MERAH, LOW);
  }
}

// =================================================
// ================= FUNGSI SERVO ==================
// =================================================

/**
 * Gerakkan servo satu langkah 30°.
 * Jika sudah 180°, cek apakah lampu merah perlu dinyalakan,
 * lalu reset ke 0 di langkah berikutnya (sudut > 180 reset).
 */
void gerakServoStep(Servo &servo, int &sudut, const char* key) {
  if (sudut >= 180) {
    sudut = 0;
  }
  sudut += 30;
  servo.write(sudut);
  prefs.putInt(key, sudut);

  // Nyalakan lampu merah jika servo mencapai tepat 180°
  if (sudut == 180) {
    nyalakanLampuMerah();
  }
}

/**
 * Cek kondisi semua servo saat ini; jika ada yang 180°, nyalakan lampu merah.
 * Berguna setelah reset atau load dari flash.
 */
void cekLampuMerahDariServo() {
  if (sudutS1 == 180 || sudutS2 == 180 || sudutS3 == 180 ||
      sudutS4 == 180 || sudutS5 == 180) {
    nyalakanLampuMerah();
  }
}

// =================================================
// ================= FUNGSI LAIN ===================
// =================================================

void playLagu3x(uint8_t track, int durasi) {
  for (int i = 0; i < 3; i++) {
    mp3.play(track);
    delay(durasi);
  }
}

void tampilPesan() {
  if (currentMode != MODE_ALARM) {
    lcd.clear();
    currentMode = MODE_ALARM;
    alarmStart  = millis();
  }
  lcd.setCursor(0, 0);
  lcd.print("    Saatnya    ");
  lcd.setCursor(0, 1);
  lcd.print("  minum obat  ");
}

void tampilJam(int jam, int menit, int detik) {
  if (lcdCustomAktif) return;

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
  manualTime     = millis();

  tampilPesan();
  playLagu3x(2, 4500);

  // Nyalakan lampu hijau 20 detik
  nyalakanLampuHijau();

  if (no == 1) {
    gerakServoStep(s1, sudutS1, "s1");
    gerakServoStep(s2, sudutS2, "s2");
    gerakServoStep(s3, sudutS3, "s3");
  } else if (no == 2) {
    gerakServoStep(s2, sudutS2, "s2");
    gerakServoStep(s3, sudutS3, "s3");
    gerakServoStep(s4, sudutS4, "s4");
  } else if (no == 3) {
    gerakServoStep(s3, sudutS3, "s3");
    gerakServoStep(s4, sudutS4, "s4");
    gerakServoStep(s5, sudutS5, "s5");
  }
}

void resetSemuaServo() {
  sudutS1 = sudutS2 = sudutS3 = sudutS4 = sudutS5 = 0;
  s1.write(0); s2.write(0); s3.write(0); s4.write(0); s5.write(0);
  prefs.putInt("s1", 0); prefs.putInt("s2", 0); prefs.putInt("s3", 0);
  prefs.putInt("s4", 0); prefs.putInt("s5", 0);

  // Pastikan lampu merah mati saat semua servo di-reset ke 0
  lampuMerahAktif = false;
  digitalWrite(PIN_LAMPU_MERAH, LOW);
}

// ================= HTTP HANDLERS =================
void handleKloter() {
  if (!server.hasArg("no")) { server.send(400, "text/plain", "No kloter?"); return; }
  int no = server.arg("no").toInt();
  if (no < 1 || no > 3) { server.send(400, "text/plain", "Invalid"); return; }
  jalankanKloter(no);
  server.send(200, "text/plain", "OK");
}

void handleReset() {
  manualOverride = true;
  manualTime     = millis();
  resetSemuaServo();

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("   SERVO RESET ");
  lcd.setCursor(0, 1); lcd.print("     BERHASIL  ");

  server.send(200, "text/plain", "RESET OK");
}

void handleSetJadwal() {
  if (!server.hasArg("kloter") || !server.hasArg("jam") || !server.hasArg("menit")) {
    server.send(400, "text/plain", "Parameter kurang"); return;
  }

  int k = server.arg("kloter").toInt();
  int h = server.arg("jam").toInt();
  int m = server.arg("menit").toInt();

  if (h < 0 || h > 23 || m < 0 || m > 59) {
    server.send(400, "text/plain", "Jam/menit tidak valid"); return;
  }

  switch (k) {
    case 1: kloter1_hour = h; kloter1_min = m; prefs.putInt("k1_h", h); prefs.putInt("k1_m", m); break;
    case 2: kloter2_hour = h; kloter2_min = m; prefs.putInt("k2_h", h); prefs.putInt("k2_m", m); break;
    case 3: kloter3_hour = h; kloter3_min = m; prefs.putInt("k3_h", h); prefs.putInt("k3_m", m); break;
    default: server.send(400, "text/plain", "Kloter tidak valid"); return;
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
    server.send(400, "text/plain", "Parameter kurang"); return;
  }
  lcdAtasCustom  = server.arg("atas").substring(0, 16);
  lcdBawahCustom = server.arg("bawah").substring(0, 16);

  prefs.putString("lcdAtas", lcdAtasCustom);
  prefs.putString("lcdBawah", lcdBawahCustom);
  prefs.putBool("lcdAktif", true);
  lcdCustomAktif = true;

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(lcdAtasCustom);
  lcd.setCursor(0, 1); lcd.print(lcdBawahCustom);

  server.send(200, "text/plain", "LCD UPDATED");
}

void handleLCDDefault() {
  prefs.remove("lcdAtas");
  prefs.remove("lcdBawah");
  prefs.putBool("lcdAktif", false);
  lcdCustomAktif = false;

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(lcdAtasDefault);
  lcd.setCursor(0, 1); lcd.print(lcdBawahDefault);

  server.send(200, "text/plain", "LCD DEFAULT");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // Init pin lampu
  pinMode(PIN_LAMPU_HIJAU, OUTPUT);
  pinMode(PIN_LAMPU_MERAH, OUTPUT);
  digitalWrite(PIN_LAMPU_HIJAU, LOW);
  digitalWrite(PIN_LAMPU_MERAH, LOW);

  prefs.begin("servo", false);

  // Load jadwal
  kloter1_hour = prefs.getInt("k1_h", 19); kloter1_min = prefs.getInt("k1_m", 30);
  kloter2_hour = prefs.getInt("k2_h", 19); kloter2_min = prefs.getInt("k2_m", 32);
  kloter3_hour = prefs.getInt("k3_h", 19); kloter3_min = prefs.getInt("k3_m", 34);

  // Load posisi servo
  sudutS1 = prefs.getInt("s1", 0); sudutS2 = prefs.getInt("s2", 0);
  sudutS3 = prefs.getInt("s3", 0); sudutS4 = prefs.getInt("s4", 0);
  sudutS5 = prefs.getInt("s5", 0);

  // Load LCD custom
  lcdCustomAktif = prefs.getBool("lcdAktif", false);
  if (lcdCustomAktif) {
    lcdAtasCustom  = prefs.getString("lcdAtas", lcdAtasDefault);
    lcdBawahCustom = prefs.getString("lcdBawah", lcdBawahDefault);
  }

  lcd.init();
  lcd.backlight();
  lcd.print("Connect WiFi");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  dfSerial.begin(9600, SERIAL_8N1, 16, 17);
  mp3.begin(dfSerial);
  mp3.volume(25);

  s1.attach(27); s2.attach(26); s3.attach(25); s4.attach(33); s5.attach(32);
  s1.write(sudutS1); s2.write(sudutS2); s3.write(sudutS3);
  s4.write(sudutS4); s5.write(sudutS5);

  // Cek apakah ada servo yang sudah di 180° saat boot
  cekLampuMerahDariServo();

  server.on("/kloter",     handleKloter);
  server.on("/setJadwal",  handleSetJadwal);
  server.on("/jadwal",     handleGetJadwal);
  server.on("/reset",      handleReset);
  server.on("/lcd",        handleSetLCD);
  server.on("/lcdDefault", handleLCDDefault);
  server.begin();

  lcd.clear();
  Serial.print("IP ESP32: ");
  Serial.println(WiFi.localIP());

  if (lcdCustomAktif) {
    lcd.setCursor(0, 0); lcd.print(lcdAtasCustom);
    lcd.setCursor(0, 1); lcd.print(lcdBawahCustom);
  }
}

// ================= LOOP =================
void loop() {
  server.handleClient();

  // Update timer lampu (non-blocking)
  updateLampu();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  int jam   = timeinfo.tm_hour;
  int menit = timeinfo.tm_min;
  int detik = timeinfo.tm_sec;

  // Reset flag tiap menit
  if (menit != lastMinute) {
    done1 = done2 = done3 = false;
    lastMinute = menit;
  }

  // Manual override timeout 30 detik
  if (manualOverride && millis() - manualTime > 30000) {
    manualOverride = false;
  }

  if (currentMode == MODE_JAM) {
    tampilJam(jam, menit, detik);
  }

  // ===== JADWAL OTOMATIS =====
  if (!manualOverride) {
    if (jam == kloter1_hour && menit == kloter1_min && !done1) {
      jalankanKloter(1); done1 = true;
    }
    if (jam == kloter2_hour && menit == kloter2_min && !done2) {
      jalankanKloter(2); done2 = true;
    }
    if (jam == kloter3_hour && menit == kloter3_min && !done3) {
      jalankanKloter(3); done3 = true;
    }
  }

  // Kembali ke tampilan jam setelah 10 detik alarm
  if (currentMode == MODE_ALARM && millis() - alarmStart > 10000) {
    currentMode = MODE_JAM;
  }

  delay(500);
}
