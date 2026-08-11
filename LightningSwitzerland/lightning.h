#pragma once
#include <Arduino.h>
#include "config.h"

struct LightningStrike {
  float lat;
  float lon;
  uint64_t timestampNs;
  uint32_t receivedMillis;
};

struct StrikeStats { uint16_t minute1, minute5, minute15; };

class LightningStore {
 public:
  void begin();
  bool add(float lat, float lon, uint64_t timestampNs, uint32_t receivedMillis);
  void expire(uint32_t nowMs);
  uint16_t snapshot(LightningStrike *out, uint16_t capacity, uint32_t nowMs) const;
  StrikeStats stats(uint32_t nowMs) const;
  uint32_t ageMs(const LightningStrike &strike, uint32_t nowMs) const;
  uint32_t lastReceivedMillis() const { return lastReceivedMillis_; }
  bool hasStrikes() const { return count_ != 0; }
 private:
  struct DedupeEntry { uint64_t key; uint32_t seenMillis; };
  LightningStrike *strikes_ = nullptr;
  DedupeEntry *dedupe_ = nullptr;
  uint16_t head_ = 0, count_ = 0;
  uint32_t lastReceivedMillis_ = 0;
  bool duplicate(uint64_t key, uint32_t nowMs);
};

extern LightningStore strikeStore;
uint32_t strikeAgeMs(const LightningStrike &strike, uint32_t nowMs);
void injectTestStrike(float lat, float lon, uint32_t ageSeconds);
void demoLoop(uint32_t nowMs);

