#include <Arduino.h>
#include <M5StickC.h>

#define STRING_BUF_LEN 100
#define HISTORY_COUNT  100

volatile uint32_t g_wom_count = 0;
volatile uint32_t g_wom_last_millis = 0;
void IRAM_ATTR mpu6886_wake_on_motion_isr(void) {
    g_wom_count++;
    g_wom_last_millis = millis();
}

RTC_DATA_ATTR uint16_t rollCount = 0;
RTC_DATA_ATTR uint8_t historyA[HISTORY_COUNT];
RTC_DATA_ATTR uint8_t historyB[HISTORY_COUNT];

void showRoll(uint8_t a, uint8_t b) {
  M5.Lcd.setTextDatum(CL_DATUM);
  M5.Lcd.drawNumber(a, 10, 50, 4);
  M5.Lcd.setTextDatum(CR_DATUM);
  M5.Lcd.drawNumber(b, 150, 50, 4);
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.drawNumber(a + b, 80, 50, 6);
}

void setup() {
  char buf[STRING_BUF_LEN];
  // put your setup code hee, to run once:

  M5.begin(false);
  M5.Axp.ScreenBreath(0);
  M5.Lcd.begin();
  pinMode(GPIO_NUM_35, INPUT);
  attachInterrupt(GPIO_NUM_35, mpu6886_wake_on_motion_isr, FALLING);
  M5.Mpu6886.enableWakeOnMotion(M5.Mpu6886.AFS_16G, 14);
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Axp.ScreenBreath(255);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextDatum(TL_DATUM);
  snprintf(buf, sizeof(buf), "Roll #%i", rollCount + 1);
  M5.Lcd.drawString(buf, 0, 0, 2);
  M5.Lcd.setTextDatum(CC_DATUM);
  while (millis() - g_wom_last_millis < 1000) {
    M5.Lcd.drawString("Rolling.  ", 80, 40, 4);
    delay(500);
    M5.Lcd.drawString("Rolling.. ", 80, 40, 4);
    delay(500);
    M5.Lcd.drawString("Rolling...", 80, 40, 4);
    delay(500);
  }
  M5.Lcd.fillScreen(BLACK);
  snprintf(buf, sizeof(buf), "Roll #%i", rollCount + 1);
  M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  M5.Lcd.drawString(buf, 0, 0, 2);
  uint8_t dicea = random(6) + 1;
  uint8_t diceb = random(6) + 1;
  historyA[rollCount % HISTORY_COUNT] = dicea;
  historyB[rollCount % HISTORY_COUNT] = diceb;
  rollCount++;
  showRoll(dicea, diceb);
}

void showHistory() {
  M5.Lcd.fillScreen(BLACK);
  char buf[STRING_BUF_LEN];
  M5.Lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
  M5.Lcd.setTextDatum(TL_DATUM);
  uint16_t curroll = rollCount - 1;
  snprintf(buf, sizeof(buf), "Roll #%i", curroll + 1);
  M5.Lcd.drawString(buf, 0, 0, 2);
  showRoll(historyA[curroll % HISTORY_COUNT], historyB[curroll % HISTORY_COUNT]);
  uint64_t lastact = millis();
  while (millis() - lastact < 5000) {
    M5.BtnA.read();
    if (M5.BtnA.wasPressed()) {
      lastact = millis();
      M5.Lcd.fillScreen(BLACK);
      if (curroll >= 1) curroll -= 1;
      snprintf(buf, sizeof(buf), "Roll #%i", curroll + 1);
      M5.Lcd.drawString(buf, 0, 0, 2);
      showRoll(historyA[curroll % HISTORY_COUNT], historyB[curroll % HISTORY_COUNT]);
    }
    delay(20);
  }
}

void loop() {
  M5.BtnA.read();
  M5.BtnB.read();
  if (M5.BtnA.wasPressed()) {
    showHistory();
  }
  if(M5.BtnB.wasPressed() || (millis() - g_wom_last_millis > 5000)) {
      Serial.println("Going to sleep now");
      
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, LOW);
      M5.Axp.SetSleep(); // conveniently turn off screen, etc.
      esp_deep_sleep_start();
  }
  delay(1000);
}