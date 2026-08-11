#pragma once
#include <Arduino.h>
bool displayBegin();
void displayRender(uint32_t nowMs, bool wifiConnected, bool mqttConnected);
