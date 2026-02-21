#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>
#include <WebServer.h>

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

// ================= SERVO =================
Servo s1, s2, s3, s4, s5;

// ================= JADWAL =================
int kloter1_hour = 9, kloter1_min = 0;
int kloter2_hour = 2, kloter2_min = 0;
int kloter3_hour = 18, kloter3_min = 0;

// ================= FLAG =================
bool done1 = false, done2 = false, done3 = false;
int lastMinute = -1;

// ================= MODE =================
enum Mode { MODE_JAM, MODE_ALARM };
Mode currentMode = MODE_JAM;
unsigned long alarmStart = 0;

// ================= FUNCTION =================
void gerakServo(Servo &servo) {
  servo.write(30);
  delay(700);
  servo.write(0);
}

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
    alarmStart = millis();
  }

  lcd.setCursor(0, 0);
  lcd.print("    Saatnya    ");
  lcd.setCursor(0, 1);
  lcd.print(" minum obat   ");
}

void tampilJam(int jam, int menit, int detik) {
  if (currentMode != MODE_JAM) {
    lcd.clear();
    currentMode = MODE_JAM;
  }

  char buf[17];
  lcd.setCursor(0, 0);
  lcd.print("     WAKTU     ");

  sprintf(buf, "   %02d:%02d:%02d   ", jam, menit, detik);
  lcd.setCursor(0, 1);
  lcd.print(buf);
}

// ================= KLOTER MANUAL =================
void jalankanKloter(int no) {
  manualOverride = true;
  manualTime = millis();

  tampilPesan();
  playLagu3x(2, 4500);

  if (no == 1) {
    gerakServo(s1);
    gerakServo(s2);
  } 
  else if (no == 2) {
    gerakServo(s3);
  } 
  else if (no == 3) {
    gerakServo(s4);
    gerakServo(s5);
  }
}

void handleKloter() {
  if (!server.hasArg("no")) {
    server.send(400, "text/plain", "Kloter tidak ada");
    return;
  }

  int no = server.arg("no").toInt();
  if (no < 1 || no > 3) {
    server.send(400, "text/plain", "Kloter invalid");
    return;
  }

  jalankanKloter(no);
  server.send(200, "text/plain", "OK");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.print("Connect WiFi");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  dfSerial.begin(9600, SERIAL_8N1, 16, 17);
  mp3.begin(dfSerial);
  mp3.volume(18);

  s1.attach(27);
  s2.attach(26);
  s3.attach(25);
  s4.attach(33);
  s5.attach(32);

  s1.write(0); s2.write(0); s3.write(0); s4.write(0); s5.write(0);

  server.on("/kloter", handleKloter);
  server.begin();

  lcd.clear();
  Serial.print("IP ESP32: ");
  Serial.println(WiFi.localIP());
}

// ================= LOOP =================
void loop() {
  server.handleClient();   // ⬅️ WAJIB

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  int jam = timeinfo.tm_hour;
  int menit = timeinfo.tm_min;
  int detik = timeinfo.tm_sec;

  // ===== RESET FLAG PER MENIT =====
  if (menit != lastMinute) {
    done1 = done2 = done3 = false;
    lastMinute = menit;
  }

  // ===== RESET MANUAL OVERRIDE (30 detik) =====
  if (manualOverride && millis() - manualTime > 30000) {
    manualOverride = false;
  }

  // ===== TAMPIL JAM =====
  if (currentMode == MODE_JAM) {
    tampilJam(jam, menit, detik);
  }

  // ===== JADWAL OTOMATIS (HANYA JIKA TIDAK MANUAL) =====
  if (!manualOverride) {

    if (jam == kloter1_hour && menit == kloter1_min && !done1) {
      tampilPesan();
      playLagu3x(2, 4500);
      gerakServo(s1);
      gerakServo(s2);
      done1 = true;
    }

    if (jam == kloter2_hour && menit == kloter2_min && !done2) {
      tampilPesan();
      playLagu3x(2, 4500);
      gerakServo(s3);
      done2 = true;
    }

    if (jam == kloter3_hour && menit == kloter3_min && !done3) {
      tampilPesan();
      playLagu3x(2, 4500);
      gerakServo(s4);
      gerakServo(s5);
      done3 = true;
    }
  }

  // ===== KEMBALI KE JAM =====
  if (currentMode == MODE_ALARM && millis() - alarmStart > 10000) {
    currentMode = MODE_JAM;
  }

  delay(500);
}
