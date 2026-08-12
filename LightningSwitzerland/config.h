#pragma once
#include <stdint.h>

#ifndef DEMO_MODE
#define DEMO_MODE 0
#endif

namespace Config {
constexpr uint16_t kScreenWidth = 1024;
constexpr uint16_t kScreenHeight = 600;
constexpr uint16_t kStatusHeight = 62;
constexpr float kMinLon = 5.0f;
constexpr float kMaxLon = 11.5f;
constexpr float kMinLat = 45.3f;
constexpr float kMaxLat = 48.3f;
constexpr uint16_t kMqttPort = 1883;
constexpr char kMqttHost[] = "blitzortung.ha.sed.pl";
constexpr uint32_t kRetentionMs = 15UL * 60UL * 1000UL;
constexpr uint32_t kRenderIntervalMs = 250;
constexpr uint32_t kTouchWakeMs = 30UL * 1000UL;
constexpr uint8_t kBacklightPin = 23;
constexpr uint8_t kTouchSdaPin = 7;
constexpr uint8_t kTouchSclPin = 8;
constexpr uint8_t kTouchInterruptPin = 21;
constexpr uint16_t kStrikeCapacity = 4096;
constexpr uint16_t kDedupeCapacity = 8192;
constexpr char kTimezone[] = "CET-1CEST,M3.5.0,M10.5.0/3";
}

