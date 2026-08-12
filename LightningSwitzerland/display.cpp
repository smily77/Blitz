#include "display.h"

#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <esp_heap_caps.h>
#include <time.h>

#include "config.h"
#include "geo.h"
#include "lightning.h"
#include "map_data.h"

// Timing and reset values from Arduino_GFX 1.6.7's JC1060P470 profile.
static Arduino_ESP32DSIPanel dsiBus(40, 160, 160, 10, 23, 12, 48000000);
// auto_flush=false keeps CPU cache changes invisible until the complete frame is
// ready. A single flush below eliminates the visible clear/redraw flicker.
static Arduino_DSI_Display panel(
    Config::kScreenWidth, Config::kScreenHeight, &dsiBus, 0, false, 27,
    jd9165_init_operations,
    sizeof(jd9165_init_operations) / sizeof(lcd_init_cmd_t));
// Render into a separate PSRAM-backed canvas. This prevents individual map
// drawing operations from ever touching the framebuffer currently scanned out.
static Arduino_Canvas gfx(Config::kScreenWidth, Config::kScreenHeight, &panel);

static LightningStrike *frameStrikes = nullptr;
static uint8_t touchAddress = 0;
static uint32_t keepAwakeUntil = 0;
static bool backlightOn = true;

static uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return gfx.color565(r, g, b);
}

static bool i2cRead(uint8_t address, uint16_t reg, uint8_t *data, size_t length) {
  Wire.beginTransmission(address);
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg));
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(address, static_cast<uint8_t>(length)) != length) return false;
  for (size_t i = 0; i < length; ++i) data[i] = Wire.read();
  return true;
}

static bool i2cWrite(uint8_t address, uint16_t reg, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg));
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static void touchBegin() {
  pinMode(Config::kTouchInterruptPin, INPUT);
  Wire.begin(Config::kTouchSdaPin, Config::kTouchSclPin, 400000);
  uint8_t productId[4];
  for (uint8_t address : {uint8_t(0x5D), uint8_t(0x14)}) {
    if (i2cRead(address, 0x8140, productId, sizeof(productId))) {
      touchAddress = address;
      Serial.printf("Touch initialized: GT%.4s at 0x%02X\n", productId, address);
      return;
    }
  }
  Serial.println("WARNING: GT911 touch controller not found");
}

static bool touchPressed() {
  if (!touchAddress) return false;
  uint8_t status = 0;
  if (!i2cRead(touchAddress, 0x814E, &status, 1)) return false;
  if (!(status & 0x80)) return false;
  i2cWrite(touchAddress, 0x814E, 0);
  return (status & 0x0F) != 0;
}

bool displayBegin() {
  pinMode(Config::kBacklightPin, OUTPUT);
  digitalWrite(Config::kBacklightPin, HIGH);
  if (!panel.begin()) return false;
  if (panel.width() != Config::kScreenWidth ||
      panel.height() != Config::kScreenHeight) return false;
  if (!gfx.begin(GFX_SKIP_OUTPUT_BEGIN)) {
    Serial.println("FATAL: display backbuffer allocation failed");
    return false;
  }
  frameStrikes = static_cast<LightningStrike *>(heap_caps_malloc(
      Config::kStrikeCapacity * sizeof(LightningStrike),
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!frameStrikes) frameStrikes = static_cast<LightningStrike *>(
      malloc(Config::kStrikeCapacity * sizeof(LightningStrike)));
  gfx.fillScreen(rgb(7, 12, 20));
  gfx.flush();
  panel.flush();
  keepAwakeUntil = millis() + Config::kTouchWakeMs;
  touchBegin();
  return frameStrikes != nullptr;
}

void displayUpdatePower(uint32_t now) {
  if (touchPressed()) keepAwakeUntil = now + Config::kTouchWakeMs;
  const StrikeStats s = strikeStore.stats(now);
  const bool strikesActive = s.minute1 || s.minute5 || s.minute15;
  const bool touchActive = static_cast<int32_t>(keepAwakeUntil - now) > 0;
  const bool shouldBeOn = strikesActive || touchActive;
  if (shouldBeOn != backlightOn) {
    backlightOn = shouldBeOn;
    digitalWrite(Config::kBacklightPin, backlightOn ? HIGH : LOW);
    Serial.printf("Backlight %s\n", backlightOn ? "ON" : "OFF");
  }
}

static void mapLine(const MapLine &line) {
  const uint16_t color = line.lake ? rgb(35, 112, 151)
      : (line.swiss ? rgb(175, 204, 218) : rgb(69, 91, 105));
  for (uint16_t i = 1; i < line.count; ++i) {
    const ScreenPoint a = geoToScreen(line.points[i - 1].lat10000 / 10000.0f,
                                      line.points[i - 1].lon10000 / 10000.0f);
    const ScreenPoint b = geoToScreen(line.points[i].lat10000 / 10000.0f,
                                      line.points[i].lon10000 / 10000.0f);
    gfx.drawLine(a.x, a.y, b.x, b.y, color);
    if (line.swiss) gfx.drawLine(a.x, a.y + 1, b.x, b.y + 1, color);
  }
}

struct City { float lat; float lon; };
static const City cities[] = {
    {47.3860f, 9.2794f},  // Herisau
    {47.0471f, 9.4410f},  // Sargans
    {47.3769f, 8.5417f},  // Zurich
    {46.9480f, 7.4474f},  // Bern
    {46.0037f, 8.9511f},  // Lugano
    {46.9896f, 6.9293f},  // Neuchatel
};

static void drawCities() {
  const uint16_t halo = rgb(7, 12, 20);
  const uint16_t green = rgb(45, 225, 105);
  for (const City &city : cities) {
    const ScreenPoint p = geoToScreen(city.lat, city.lon);
    gfx.fillCircle(p.x, p.y, 5, halo);
    gfx.fillCircle(p.x, p.y, 3, green);
  }
}

static void drawStatus(uint32_t now, bool wifi, bool mqtt) {
  const StrikeStats s = strikeStore.stats(now);
  char line[96];
  gfx.fillRect(0, 0, Config::kScreenWidth, Config::kStatusHeight, rgb(11, 20, 31));
  gfx.setTextColor(rgb(225, 235, 240));
  gfx.setTextSize(2);
  gfx.setCursor(14, 10);
  gfx.print("BLITZE SCHWEIZ");
  snprintf(line, sizeof(line), "1 min: %u   5 min: %u   15 min: %u",
           s.minute1, s.minute5, s.minute15);
  gfx.setCursor(385, 10);
  gfx.print(line);
  gfx.setTextSize(1);
  gfx.setCursor(14, 40);
  if (strikeStore.hasStrikes())
    snprintf(line, sizeof(line), "Letzter: vor %lu s",
             (unsigned long)((now - strikeStore.lastReceivedMillis()) / 1000));
  else
    snprintf(line, sizeof(line), "Letzter: --");
  gfx.print(line);
  gfx.setCursor(385, 40);
  gfx.print("WiFi ");
  gfx.setTextColor(wifi ? rgb(50, 220, 110) : rgb(230, 65, 65));
  gfx.print(wifi ? "ON" : "OFF");
  gfx.setTextColor(rgb(225, 235, 240));
  gfx.setCursor(500, 40);
  gfx.print("MQTT ");
  gfx.setTextColor(mqtt ? rgb(50, 220, 110) : rgb(230, 65, 65));
  gfx.print(mqtt ? "ON" : "OFF");
  time_t current = time(nullptr);
  struct tm local;
  localtime_r(&current, &local);
  strftime(line, sizeof(line), "%H:%M:%S", &local);
  gfx.setTextColor(rgb(225, 235, 240));
  gfx.setCursor(920, 40);
  gfx.print(current > 1600000000 ? line : "--:--:--");
}

void displayRender(uint32_t now, bool wifi, bool mqtt) {
  gfx.fillRect(0, Config::kStatusHeight, Config::kScreenWidth,
               Config::kScreenHeight - Config::kStatusHeight, rgb(7, 12, 20));
  for (uint16_t i = 0; i < kMapLineCount; ++i) mapLine(kMapLines[i]);
  drawCities();
  const uint16_t count = strikeStore.snapshot(
      frameStrikes, Config::kStrikeCapacity, now);
  for (uint16_t i = 0; i < count; ++i) {
    const uint32_t age = strikeAgeMs(frameStrikes[i], now);
    const ScreenPoint p = geoToScreen(frameStrikes[i].lat, frameStrikes[i].lon);
    if (age <= 60000) {
      gfx.fillCircle(p.x, p.y, 10, rgb(75, 75, 35));
      gfx.fillCircle(p.x, p.y, 7, rgb(255, 250, 180));
    } else if (age <= 300000) {
      gfx.fillCircle(p.x, p.y, 5, rgb(255, 156, 40));
    } else {
      gfx.fillCircle(p.x, p.y, 3, rgb(170, 35, 30));
    }
  }
  drawStatus(now, wifi, mqtt);
  gfx.flush();
  panel.flush();
}
