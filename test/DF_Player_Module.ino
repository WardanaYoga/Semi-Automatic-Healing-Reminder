#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

HardwareSerial mySerial(1);
DFRobotDFPlayerMini player;

void setup() {
  Serial.begin(115200);

  mySerial.begin(9600, SERIAL_8N1, 16, 17);

  Serial.println("Initializing DFPlayer...");

  if (!player.begin(mySerial)) {
    Serial.println("DFPlayer NOT detected!");
    while (true);
  }

  Serial.println("DFPlayer Ready");

  player.volume(20);   // 0–30
  player.play(1);      // mainkan 0001.mp3
}

void loop() {
}
