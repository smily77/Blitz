#pragma once
#include <stdint.h>
struct MapVertex { int32_t lon10000; int32_t lat10000; };
struct MapLine { const MapVertex *points; uint16_t count; bool swiss; bool lake; };
extern const MapLine kMapLines[];
extern const uint16_t kMapLineCount;
