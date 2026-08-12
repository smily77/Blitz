#include "config.h"
#include "display.h"
#include "lightning.h"
#include "mqtt_client.h"
void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("Boot");
  strikeStore.begin();
  if (!displayBegin()) {
    Serial.println("FATAL: display initialization failed or not 1024x600");
    while (true) delay(1000);
  }
  Serial.println("Display initialized (1024x600)");
  randomSeed(esp_random());
  networkBegin();
}

void loop() {
  static uint32_t nextFrame = 0;
  const uint32_t now = millis();
  networkLoop(now);
  demoLoop(now);
  strikeStore.expire(now);
  displayUpdatePower(now);
  if (static_cast<int32_t>(now - nextFrame) >= 0) {
    nextFrame = now + Config::kRenderIntervalMs;
    displayRender(now, networkWifiConnected(), networkMqttConnected());
  }
  delay(1);
}
