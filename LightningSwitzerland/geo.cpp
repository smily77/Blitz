#include "geo.h"
#include "config.h"

bool geoInView(float lat, float lon) {
  return lat >= Config::kMinLat && lat <= Config::kMaxLat &&
         lon >= Config::kMinLon && lon <= Config::kMaxLon;
}

ScreenPoint geoToScreen(float lat, float lon) {
  const float width = Config::kScreenWidth - 1.0f;
  const float mapHeight = Config::kScreenHeight - Config::kStatusHeight - 1.0f;
  ScreenPoint p;
  p.x = static_cast<int16_t>((lon - Config::kMinLon) * width /
                             (Config::kMaxLon - Config::kMinLon) + 0.5f);
  p.y = static_cast<int16_t>(Config::kStatusHeight +
      (Config::kMaxLat - lat) * mapHeight /
      (Config::kMaxLat - Config::kMinLat) + 0.5f);
  return p;
}

// Fast ray casting against the same simplified Swiss outline used by the map.
bool geoInSwitzerland(float lat, float lon) {
  static const float p[][2] = {
    {5.956f,46.132f},{6.077f,46.263f},{6.855f,46.443f},{6.842f,46.868f},
    {7.493f,47.066f},{7.594f,47.586f},{8.570f,47.806f},{9.566f,47.540f},
    {9.650f,47.050f},{10.492f,46.850f},{10.163f,46.227f},{9.280f,46.230f},
    {8.455f,45.820f},{7.860f,45.916f},{7.045f,45.922f},{5.956f,46.132f}
  };
  bool inside = false;
  for (unsigned i = 0, j = 15; i < 16; j = i++) {
    const float xi=p[i][0], yi=p[i][1], xj=p[j][0], yj=p[j][1];
    if (((yi > lat) != (yj > lat)) &&
        (lon < (xj-xi) * (lat-yi) / (yj-yi) + xi)) inside = !inside;
  }
  return inside;
}

