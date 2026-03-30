#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// 2 PCA9685
Adafruit_PWMServoDriver pca1 = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pca2 = Adafruit_PWMServoDriver(0x41);

// Giới hạn servo (có thể chỉnh)
#define SERVOMIN  150
#define SERVOMAX  600

void setup() {
  Serial.begin(115200);
  Wire.begin(); // ESP32: SDA=21, SCL=22

  pca1.begin();
  pca2.begin();

  pca1.setPWMFreq(50); // servo 50Hz
  pca2.setPWMFreq(50);

  delay(1000);
  Serial.println("Start test servo...");
}

// Hàm quay servo
void setServo(Adafruit_PWMServoDriver &pca, int channel, int pulse) {
  pca.setPWM(channel, 0, pulse);
}

void loop() {

  // ===== TEST PCA1 (0-15) =====
  // for (int ch = 13; ch < 15; ch++) {
  //   Serial.print("PCA1 - Channel: ");
  //   Serial.println(ch);

  //   // quay trái
  //   setServo(pca1, ch, SERVOMIN);
  //   delay(500);

  //   // quay phải
  //   setServo(pca1, ch, SERVOMAX);
  //   delay(500);

  //   // về giữa
  //   setServo(pca1, ch, (SERVOMIN + SERVOMAX)/2);
  //   delay(500);
  // }

  setServo(pca1, 15, SERVOMIN);
  delay(500);

  // quay phải
  setServo(pca1, 15, SERVOMAX);
  delay(500);

  setServo(pca1, 15, (SERVOMIN + SERVOMAX)/2);
  delay(500);

  Serial.println("Cycle done!");
  delay(2000);
}