#pragma once
#include <stdint.h>

struct ScreenPoint { int16_t x; int16_t y; };
bool geoInView(float lat, float lon);
ScreenPoint geoToScreen(float lat, float lon);
bool geoInSwitzerland(float lat, float lon);

