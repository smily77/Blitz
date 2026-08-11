#include "display.h"
#include <Arduino_GFX_Library.h>
#include <esp_heap_caps.h>
#include <time.h>
#include "config.h"
#include "geo.h"
#include "lightning.h"
#include "map_data.h"
// Arduino_GFX 1.6.4 provides the JC1060P470 ESP32-P4 DSI defaults. No hardware
// pins are duplicated here: the library owns the fixed DSI/hosted configuration.
static Arduino_ESP32DSIPanel dsiBus;
static Arduino_DSI_Display gfx(Config::kScreenWidth, Config::kScreenHeight, &dsiBus);
static LightningStrike *frameStrikes=nullptr;
static uint16_t rgb(uint8_t r,uint8_t g,uint8_t b){return gfx.color565(r,g,b);}
bool displayBegin(){
 if(!gfx.begin()) return false;
 if(gfx.width()!=Config::kScreenWidth||gfx.height()!=Config::kScreenHeight)return false;
 frameStrikes=static_cast<LightningStrike *>(heap_caps_malloc(Config::kStrikeCapacity*sizeof(LightningStrike),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
 if(!frameStrikes)frameStrikes=static_cast<LightningStrike *>(malloc(Config::kStrikeCapacity*sizeof(LightningStrike)));
 gfx.fillScreen(rgb(7,12,20)); return frameStrikes!=nullptr;
}
static void mapLine(const MapLine &l){uint16_t c=l.lake?rgb(25,78,105):(l.swiss?rgb(155,180,194):rgb(69,91,105));for(uint16_t i=1;i<l.count;++i){ScreenPoint a=geoToScreen(l.points[i-1].lat1000/1000.0f,l.points[i-1].lon1000/1000.0f),b=geoToScreen(l.points[i].lat1000/1000.0f,l.points[i].lon1000/1000.0f);gfx.drawLine(a.x,a.y,b.x,b.y,c);if(l.swiss)gfx.drawLine(a.x,a.y+1,b.x,b.y+1,c);}}
static void status(uint32_t now,bool wifi,bool mqtt){
 StrikeStats s=strikeStore.stats(now); char line[96]; gfx.fillRect(0,0,Config::kScreenWidth,Config::kStatusHeight,rgb(11,20,31));gfx.setTextColor(rgb(225,235,240));gfx.setTextSize(2);gfx.setCursor(14,10);gfx.print("BLITZE SCHWEIZ");snprintf(line,sizeof(line),"1 min: %u   5 min: %u   15 min: %u",s.minute1,s.minute5,s.minute15);gfx.setCursor(385,10);gfx.print(line);gfx.setTextSize(1);gfx.setCursor(14,40);if(strikeStore.hasStrikes())snprintf(line,sizeof(line),"Letzter: vor %lu s",(unsigned long)((now-strikeStore.lastReceivedMillis())/1000));else snprintf(line,sizeof(line),"Letzter: --");gfx.print(line);gfx.setCursor(385,40);gfx.print("WiFi ");gfx.setTextColor(wifi?rgb(50,220,110):rgb(230,65,65));gfx.print(wifi?"ON":"OFF");gfx.setTextColor(rgb(225,235,240));gfx.setCursor(500,40);gfx.print("MQTT ");gfx.setTextColor(mqtt?rgb(50,220,110):rgb(230,65,65));gfx.print(mqtt?"ON":"OFF");time_t n=time(nullptr);struct tm local;localtime_r(&n,&local);strftime(line,sizeof(line),"%H:%M:%S",&local);gfx.setTextColor(rgb(225,235,240));gfx.setCursor(920,40);gfx.print(n>1600000000?line:"--:--:--");
}
void displayRender(uint32_t now,bool wifi,bool mqtt){gfx.fillRect(0,Config::kStatusHeight,Config::kScreenWidth,Config::kScreenHeight-Config::kStatusHeight,rgb(7,12,20));for(uint8_t i=0;i<kMapLineCount;++i)mapLine(kMapLines[i]);uint16_t n=strikeStore.snapshot(frameStrikes,Config::kStrikeCapacity,now);for(uint16_t i=0;i<n;++i){uint32_t age=strikeAgeMs(frameStrikes[i],now);ScreenPoint p=geoToScreen(frameStrikes[i].lat,frameStrikes[i].lon);if(age<=60000){gfx.fillCircle(p.x,p.y,10,rgb(75,75,35));gfx.fillCircle(p.x,p.y,7,rgb(255,250,180));}else if(age<=300000)gfx.fillCircle(p.x,p.y,5,rgb(255,156,40));else gfx.fillCircle(p.x,p.y,3,rgb(170,35,30));}status(now,wifi,mqtt);gfx.flush();}
