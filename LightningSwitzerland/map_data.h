#pragma once
#include <stdint.h>
struct MapVertex { int32_t lon1000; int32_t lat1000; };
struct MapLine { const MapVertex *points; uint16_t count; bool swiss; bool lake; };
extern const MapLine kMapLines[];
extern const uint8_t kMapLineCount;
