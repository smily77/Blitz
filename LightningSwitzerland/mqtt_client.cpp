#include "mqtt_client.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "geo.h"
#include "lightning.h"
#include <Credentials.h>
static WiFiClient tcp; static PubSubClient mqtt(tcp);
static uint32_t nextWifiAttempt=0,nextMqttAttempt=0,wifiBackoff=1000,mqttBackoff=1000;
static bool ntpStarted=false,wasWifi=false,wasMqtt=false;
// Precision-two cells intersecting the viewport. Upstream encodes every
// geohash character as a separate MQTT level.
static const char *const topics[]={"blitzortung/1.1/u/0/#","blitzortung/1.1/u/2/#"};
static void message(char *,byte *payload,unsigned int length){
 if(!length||length>1024){Serial.println("MQTT JSON rejected: invalid size");return;}
 JsonDocument doc;DeserializationError err=deserializeJson(doc,payload,length);if(err){Serial.printf("MQTT JSON rejected: %s\n",err.c_str());return;}
 if(!doc["lat"].is<float>()||!doc["lon"].is<float>()||!doc["time"].is<uint64_t>()){Serial.println("MQTT JSON rejected: lat/lon/time missing");return;}
 float lat=doc["lat"].as<float>(),lon=doc["lon"].as<float>();uint64_t stamp=doc["time"].as<uint64_t>();if(!geoInView(lat,lon)||!stamp)return;
 if(strikeStore.add(lat,lon,stamp,millis()))Serial.printf("Strike lat=%.5f lon=%.5f time=%llu\n",lat,lon,(unsigned long long)stamp);
}
void networkBegin(){WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(false);mqtt.setServer(Config::kMqttHost,Config::kMqttPort);mqtt.setCallback(message);mqtt.setBufferSize(1152);mqtt.setKeepAlive(30);mqtt.setSocketTimeout(1);}
bool networkWifiConnected(){return WiFi.status()==WL_CONNECTED;} bool networkMqttConnected(){return mqtt.connected();}
void networkLoop(uint32_t now){
 bool wifi=networkWifiConnected();if(wifi!=wasWifi){Serial.println(wifi?"WiFi connected":"WiFi disconnected");if(wifi)Serial.println(WiFi.localIP());wasWifi=wifi;}
 if(!wifi){if(static_cast<int32_t>(now-nextWifiAttempt)>=0){Serial.println("WiFi connecting...");WiFi.disconnect();WiFi.begin(ssid,password);nextWifiAttempt=now+wifiBackoff;wifiBackoff=min<uint32_t>(wifiBackoff*2,60000);}return;}wifiBackoff=1000;
 if(!ntpStarted){configTzTime(Config::kTimezone,"pool.ntp.org","time.cloudflare.com");ntpStarted=true;Serial.println("NTP synchronization started");}
 if(!mqtt.connected()&&static_cast<int32_t>(now-nextMqttAttempt)>=0){char id[32];snprintf(id,sizeof(id),"ch-p4-%08lx",(unsigned long)ESP.getEfuseMac());Serial.println("MQTT connecting...");if(mqtt.connect(id)){Serial.println("MQTT connected");mqttBackoff=1000;for(const char *t:topics){mqtt.subscribe(t,0);Serial.printf("Subscribed: %s\n",t);}}else{nextMqttAttempt=now+mqttBackoff;mqttBackoff=min<uint32_t>(mqttBackoff*2,60000);}}
 if(mqtt.connected())mqtt.loop();if(wasMqtt&&!mqtt.connected())Serial.println("MQTT disconnected");wasMqtt=mqtt.connected();
}
