#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <time.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>
#include <Preferences.h>

// ================= WIFI =================
const char* ssid = "AI-CENTER";
const char* password = "aicenter";
const char* mdnsName = "obat";

// ================= NTP =================
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;

// ================= OBJECT =================
WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);
HardwareSerial dfSerial(2);
DFRobotDFPlayerMini mp3;
Preferences prefs;

// ================= SERVO =================
Servo s1, s2, s3, s4, s5;
int sudutS1, sudutS2, sudutS3, sudutS4, sudutS5;

// ================= JADWAL =================
int k1_h, k1_m, k2_h, k2_m, k3_h, k3_m;

// ================= MODE =================
enum Mode { MODE_JAM, MODE_ALARM };
Mode currentMode = MODE_JAM;
unsigned long alarmStart = 0;

// ================= FLAG =================
bool done1=false, done2=false, done3=false;
int lastMinute = -1;
bool manualOverride = false;
unsigned long manualTime = 0;

// ================= LCD CUSTOM =================
String lcdAtasDefault  = "     WAKTU:     ";
String lcdBawahDefault = "    00:00:00    ";
String lcdAtasCustom, lcdBawahCustom;
bool lcdCustomAktif = false;

// =================================================
// ================== FUNCTION =====================
// =================================================

void gerakServoStep(Servo &servo, int &sudut, const char* key) {
  sudut += 30;
  if (sudut > 180) sudut = 0;
  servo.write(sudut);
  prefs.putInt(key, sudut);
}

void playLagu3x(uint8_t track, int delayMs) {
  for (int i=0;i<3;i++) {
    mp3.play(track);
    delay(delayMs);
  }
}

void tampilPesan() {
  if (currentMode != MODE_ALARM) {
    lcd.clear();
    currentMode = MODE_ALARM;
    alarmStart = millis();
  }
  lcd.setCursor(0,0);
  lcd.print("    Saatnya    ");
  lcd.setCursor(0,1);
  lcd.print("  minum obat  ");
}

void tampilJam(int h,int m,int s) {
  if (lcdCustomAktif) return;

  if (currentMode != MODE_JAM) {
    lcd.clear();
    currentMode = MODE_JAM;
  }

  char buf[17];
  lcd.setCursor(0,0);
  lcd.print("     WAKTU:     ");
  sprintf(buf,"    %02d:%02d:%02d   ",h,m,s);
  lcd.setCursor(0,1);
  lcd.print(buf);
}

// ================= KLOTER =================
void jalankanKloter(int no) {
  manualOverride = true;
  manualTime = millis();

  tampilPesan();
  playLagu3x(2, 4500);

  if (no == 1) {
    gerakServoStep(s1, sudutS1, "s1");
    gerakServoStep(s2, sudutS2, "s2");
    gerakServoStep(s3, sudutS3, "s3");
  }
  if (no == 2) {
    gerakServoStep(s2, sudutS2, "s2");
    gerakServoStep(s3, sudutS3, "s3");
    gerakServoStep(s4, sudutS4, "s4");
  }
  if (no == 3) {
    gerakServoStep(s3, sudutS3, "s3");
    gerakServoStep(s4, sudutS4, "s4");
    gerakServoStep(s5, sudutS5, "s5");
  }
}

void resetSemuaServo() {
  sudutS1=sudutS2=sudutS3=sudutS4=sudutS5=0;
  s1.write(0); s2.write(0); s3.write(0); s4.write(0); s5.write(0);
  prefs.putInt("s1",0); prefs.putInt("s2",0); prefs.putInt("s3",0);
  prefs.putInt("s4",0); prefs.putInt("s5",0);
}
void handleRoot() {
  server.send(200, "text/plain",
    "ESP32 OBAT AKTIF\n"
    "Gunakan endpoint:\n"
    "/kloter?no=1\n"
    "/reset\n"
    "/setJadwal\n"
    "/lcd\n"
  );
}

// ================= HTTP =================
void handleKloter(){
  int no = server.arg("no").toInt();
  jalankanKloter(no);
  server.send(200,"text/plain","OK");
}

void handleReset(){
  resetSemuaServo();
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("   SERVO RESET ");
  lcd.setCursor(0,1); lcd.print("     BERHASIL  ");
  server.send(200,"text/plain","RESET OK");
}

void handleSetJadwal(){
  int k=server.arg("kloter").toInt();
  int h=server.arg("jam").toInt();
  int m=server.arg("menit").toInt();

  if(h<0||h>23||m<0||m>59){
    server.send(400,"text/plain","Invalid");
    return;
  }

  if(k==1){k1_h=h;k1_m=m;prefs.putInt("k1_h",h);prefs.putInt("k1_m",m);}
  if(k==2){k2_h=h;k2_m=m;prefs.putInt("k2_h",h);prefs.putInt("k2_m",m);}
  if(k==3){k3_h=h;k3_m=m;prefs.putInt("k3_h",h);prefs.putInt("k3_m",m);}

  server.send(200,"text/plain","OK");
}

void handleSetLCD(){
  lcdAtasCustom  = server.arg("atas").substring(0,16);
  lcdBawahCustom = server.arg("bawah").substring(0,16);
  prefs.putString("lcdAtas",lcdAtasCustom);
  prefs.putString("lcdBawah",lcdBawahCustom);
  prefs.putBool("lcdAktif",true);
  lcdCustomAktif=true;

  lcd.clear();
  lcd.setCursor(0,0); lcd.print(lcdAtasCustom);
  lcd.setCursor(0,1); lcd.print(lcdBawahCustom);

  server.send(200,"text/plain","LCD OK");
}

void handleLCDDefault(){
  prefs.putBool("lcdAktif",false);
  lcdCustomAktif=false;
  currentMode = MODE_JAM;
  server.send(200,"text/plain","DEFAULT");
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);
  prefs.begin("servo",false);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcdCustomAktif=prefs.getBool("lcdAktif",false);
  lcdAtasCustom=prefs.getString("lcdAtas",lcdAtasDefault);
  lcdBawahCustom=prefs.getString("lcdBawah",lcdBawahDefault);

  k1_h=prefs.getInt("k1_h",19); k1_m=prefs.getInt("k1_m",30);
  k2_h=prefs.getInt("k2_h",19); k2_m=prefs.getInt("k2_m",32);
  k3_h=prefs.getInt("k3_h",19); k3_m=prefs.getInt("k3_m",34);

  sudutS1=prefs.getInt("s1",0); sudutS2=prefs.getInt("s2",0);
  sudutS3=prefs.getInt("s3",0); sudutS4=prefs.getInt("s4",0);
  sudutS5=prefs.getInt("s5",0);

  WiFi.setHostname(mdnsName);
  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED) delay(500);

  MDNS.begin(mdnsName);
  MDNS.addService("http","tcp",80);

  configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);

  dfSerial.begin(9600,SERIAL_8N1,16,17);
  mp3.begin(dfSerial);
  mp3.volume(25);

  s1.attach(27); s2.attach(26); s3.attach(25); s4.attach(33); s5.attach(32);
  s1.write(sudutS1); s2.write(sudutS2); s3.write(sudutS3);
  s4.write(sudutS4); s5.write(sudutS5);

  server.on("/", handleRoot);
  server.on("/kloter",handleKloter);
  server.on("/reset",handleReset);
  server.on("/setJadwal",handleSetJadwal);
  server.on("/lcd",handleSetLCD);
  server.on("/lcdDefault",handleLCDDefault);
  server.begin();
}

// ================= LOOP =================
void loop(){
  server.handleClient();

  struct tm t;
  if(!getLocalTime(&t)) return;

  if(t.tm_min!=lastMinute){
    done1=done2=done3=false;
    lastMinute=t.tm_min;
  }

  if(manualOverride && millis()-manualTime>30000)
    manualOverride=false;

  if(currentMode==MODE_JAM)
    tampilJam(t.tm_hour,t.tm_min,t.tm_sec);

  if(!manualOverride){
    if(t.tm_hour==k1_h&&t.tm_min==k1_m&&!done1){jalankanKloter(1);done1=true;}
    if(t.tm_hour==k2_h&&t.tm_min==k2_m&&!done2){jalankanKloter(2);done2=true;}
    if(t.tm_hour==k3_h&&t.tm_min==k3_m&&!done3){jalankanKloter(3);done3=true;}
  }

  if(currentMode==MODE_ALARM && millis()-alarmStart>10000)
    currentMode=MODE_JAM;

  delay(500);
}
