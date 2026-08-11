#pragma once
#include <Arduino.h>
void networkBegin();
void networkLoop(uint32_t nowMs);
bool networkWifiConnected();
bool networkMqttConnected();
