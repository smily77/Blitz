#include "lightning.h"
#include "geo.h"
#include <esp_heap_caps.h>
#include <time.h>

LightningStore strikeStore;

void LightningStore::begin() {
  strikes_ = static_cast<LightningStrike *>(heap_caps_calloc(
      Config::kStrikeCapacity, sizeof(LightningStrike), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  dedupe_ = static_cast<DedupeEntry *>(heap_caps_calloc(
      Config::kDedupeCapacity, sizeof(DedupeEntry), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!strikes_) strikes_ = static_cast<LightningStrike *>(calloc(Config::kStrikeCapacity, sizeof(LightningStrike)));
  if (!dedupe_) dedupe_ = static_cast<DedupeEntry *>(calloc(Config::kDedupeCapacity, sizeof(DedupeEntry)));
  if (!strikes_ || !dedupe_) { Serial.println("FATAL: strike storage allocation failed"); abort(); }
}

bool LightningStore::duplicate(uint64_t key, uint32_t nowMs) {
  if (!key) return false;
  uint16_t slot = static_cast<uint16_t>((key ^ (key >> 32)) * 2654435761UL) & (Config::kDedupeCapacity - 1);
  for (uint16_t n=0; n<Config::kDedupeCapacity; ++n) {
    DedupeEntry &e=dedupe_[slot];
    if (!e.key || static_cast<uint32_t>(nowMs-e.seenMillis)>Config::kRetentionMs) {
      e={key,nowMs}; return false;
    }
    if (e.key==key) { e.seenMillis=nowMs; return true; }
    slot=(slot+1)&(Config::kDedupeCapacity-1);
  }
  dedupe_[slot]={key,nowMs};
  return false;
}

bool LightningStore::add(float lat, float lon, uint64_t timestampNs, uint32_t receivedMillis) {
  if (duplicate(timestampNs,receivedMillis)) return false;
  const uint16_t pos=(head_+count_)%Config::kStrikeCapacity;
  if (count_==Config::kStrikeCapacity) { head_=(head_+1)%Config::kStrikeCapacity; }
  else ++count_;
  strikes_[pos]={lat,lon,timestampNs,receivedMillis};
  lastReceivedMillis_=receivedMillis;
  return true;
}

uint32_t LightningStore::ageMs(const LightningStrike &s, uint32_t nowMs) const {
  const time_t now=time(nullptr);
  if (s.timestampNs && now > 1600000000) {
    const uint64_t nowNs=static_cast<uint64_t>(now)*1000000000ULL;
    if (nowNs >= s.timestampNs) {
      const uint64_t ms=(nowNs-s.timestampNs)/1000000ULL;
      return ms>UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(ms);
    }
  }
  return nowMs-s.receivedMillis;
}

void LightningStore::expire(uint32_t nowMs) {
  while (count_ && ageMs(strikes_[head_],nowMs)>Config::kRetentionMs) {
    head_=(head_+1)%Config::kStrikeCapacity; --count_;
  }
}

uint16_t LightningStore::snapshot(LightningStrike *out,uint16_t capacity,uint32_t nowMs) const {
  uint16_t written=0;
  for (uint16_t i=0;i<count_ && written<capacity;++i) {
    const LightningStrike &s=strikes_[(head_+i)%Config::kStrikeCapacity];
    if (ageMs(s,nowMs)<=Config::kRetentionMs) out[written++]=s;
  }
  return written;
}

StrikeStats LightningStore::stats(uint32_t nowMs) const {
  StrikeStats r={0,0,0};
  for(uint16_t i=0;i<count_;++i) {
    uint32_t a=ageMs(strikes_[(head_+i)%Config::kStrikeCapacity],nowMs);
    if(a<=60000) ++r.minute1;
    if(a<=300000) ++r.minute5;
    if(a<=Config::kRetentionMs) ++r.minute15;
  }
  return r;
}

uint32_t strikeAgeMs(const LightningStrike &s,uint32_t nowMs){ return strikeStore.ageMs(s,nowMs); }

void injectTestStrike(float lat,float lon,uint32_t ageSeconds) {
  const uint32_t now=millis(); const time_t epoch=time(nullptr);
  const uint64_t ns=epoch>1600000000 ? (static_cast<uint64_t>(epoch)-ageSeconds)*1000000000ULL :
      (static_cast<uint64_t>(now)+1)*1000000ULL;
  strikeStore.add(lat,lon,ns,now-ageSeconds*1000UL);
}

void demoLoop(uint32_t nowMs) {
#if DEMO_MODE
  static uint32_t next=0;
  if(static_cast<int32_t>(nowMs-next)>=0) {
    float lat,lon; do { lat=random(45800,47750)/1000.0f; lon=random(6000,10400)/1000.0f; } while(!geoInSwitzerland(lat,lon));
    injectTestStrike(lat,lon,random(0,900)); next=nowMs+random(700,2200);
  }
#else
  (void)nowMs;
#endif
}
