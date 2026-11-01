#include <SPI.h>
#include "RF24.h"
#include "esp_bt.h"
#include "esp_wifi.h"

// ==================== PINS & HARDWARE ====================
// HSPI (Module 1): SCK=14, MISO=12, MOSI=13, CS=15, CE=16
// VSPI (Module 2): SCK=18, MISO=19, MOSI=23, CS=21, CE=22

RF24 radio(16, 15);   // HSPI Module (CE, CSN)
RF24 radio1(22, 21);  // VSPI Module (CE, CSN)

// ==================== HOP STATE ====================
int ch1 = 45, ch2 = 45;
int dir1 = 1, dir2 = -1;  // Opposite directions for maximum spectrum coverage
int step1 = 2, step2 = 4; // Different speeds create crossing paths

SPIClass *hp = nullptr;
SPIClass *sp = nullptr;

unsigned long lastHopTime = 0;
const unsigned long hopIntervalUs = 130; // Optimized PLL lock delay

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(500);

  // Kill ESP32 internal radios to free CPU and prevent background interference
  esp_bt_controller_deinit();
  esp_wifi_stop();
  esp_wifi_deinit();

  // ---- Initialize HSPI Bus (Module 1) ----
  hp = new SPIClass(HSPI);
  hp->begin(14, 12, 13, 15); // SCK, MISO, MOSI, SS
  if (radio.begin(hp, 16, 15)) {
    radio.setAutoAck(false);
    radio.stopListening();
    radio.setRetries(0, 0);
    radio.setPALevel(RF24_PA_MAX, true);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.startConstCarrier(RF24_PA_MAX, 45);
    Serial.println("[HSPI] Module 1 Initialized");
  } else {
    Serial.println("[HSPI] Module 1 Failed");
  }

  // ---- Initialize VSPI Bus (Module 2) ----
  sp = new SPIClass(VSPI);
  sp->begin(18, 19, 23, 21); // SCK, MISO, MOSI, SS
  if (radio1.begin(sp, 22, 21)) {
    radio1.setAutoAck(false);
    radio1.stopListening();
    radio1.setRetries(0, 0);
    radio1.setPALevel(RF24_PA_MAX, true);
    radio1.setDataRate(RF24_2MBPS);
    radio1.setCRCLength(RF24_CRC_DISABLED);
    radio1.startConstCarrier(RF24_PA_MAX, 45);
    Serial.println("[VSPI] Module 2 Initialized");
  } else {
    Serial.println("[VSPI] Module 2 Failed");
  }

  Serial.println("=== FULL PERFORMANCE SWEEP ACTIVE ===");
}

// ==================== LOOP ====================
void loop() {
  unsigned long currentMicros = micros();
  if (currentMicros - lastHopTime >= hopIntervalUs) {
    lastHopTime = currentMicros;

    // Module 1 Sweep (Step 2)
    ch1 += step1 * dir1;
    if (ch1 > 79) { ch1 = 79; dir1 = -1; }
    if (ch1 < 2)  { ch1 = 2;  dir1 = 1; }
    radio.setChannel(ch1);

    // Module 2 Sweep (Step 4, opposite direction)
    ch2 += step2 * dir2;
    if (ch2 > 79) { ch2 = 79; dir2 = 1; }
    if (ch2 < 2)  { ch2 = 2;  dir2 = -1; }
    radio1.setChannel(ch2);
  }
}