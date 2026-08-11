# Blitzanzeige Schweiz – GUITION JC1060P470

Autarke Near-Realtime-Anzeige für Blitzortung-Ereignisse auf dem 7-Zoll-Board
**GUITION JC1060P470 / JC1060P470C_I_W** (ESP32-P4, ESP32-C6 über ESP-Hosted,
1024 × 600 MIPI-DSI). Die Anwendung braucht zur Laufzeit keine Kartenserver.

## Hardware und bewusst gewählte Initialisierung

Das Projekt zielt auf die Board-Konfiguration **ESP32-P4 Function EV Board** mit der
JC1060P470-Unterstützung aus Arduino_GFX 1.6.4. `display.cpp` verwendet
`Arduino_ESP32DSIPanel`/`Arduino_DSI_Display` mit den vom Treiber bereitgestellten
P4-DSI-Standards. Es enthält absichtlich **keine kopierten oder geratenen GPIOs**.
`WiFi.h` benutzt beim P4-Core transparent den ESP-Hosted-Netif zum C6; es ist keine
manuelle SDIO-Pinbelegung im Sketch erforderlich. Der C6 muss die zur verwendeten
Core-Version passende ESP-Hosted-Firmware besitzen (bei fabrikneuen Guition-Boards
normalerweise vorinstalliert).

Beim Start wird geprüft, dass der Treiber tatsächlich 1024 × 600 meldet. Touch ist
in Version 1 nicht aktiv. Direktes Arduino_GFX-Rendering vermeidet den für diese
reine Anzeige unnötigen LVGL-Overhead.

## Arduino IDE und Bibliotheken

Das Projekt ist ausschließlich als normaler **Arduino-IDE-Sketch** aufgebaut. Eine
PlatformIO-Konfiguration ist weder vorhanden noch erforderlich. Getestete bzw. für
dieses Projekt festgelegte Versionen:

* Arduino IDE **2.x**
* Espressif Arduino-ESP32-Core mit ESP32-P4- und ESP-Hosted-Unterstützung
* GFX Library for Arduino (**Arduino_GFX**) **1.6.4**
* PubSubClient **2.8**
* ArduinoJson **7.4.2**

### ESP32-Boardpaket installieren

1. In **Datei → Voreinstellungen → Zusätzliche Boardverwalter-URLs** die offizielle
   Espressif-URL eintragen:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. **Werkzeuge → Board → Boardverwalter** öffnen, nach `esp32` suchen und das
   Espressif-Boardpaket mit ESP32-P4-Unterstützung installieren.
3. Unter **Werkzeuge → Board → esp32** das vom Guition-Hersteller bereitgestellte
   JC1060P470-Profil auswählen. Falls dieses nicht separat installiert ist,
   `ESP32P4 Dev Module` verwenden.

### Bibliotheken installieren

Unter **Werkzeuge → Bibliotheken verwalten** nacheinander suchen und installieren:

1. `GFX Library for Arduino` von Moon On Our Nation, Version 1.6.4
2. `PubSubClient` von Nick O'Leary, Version 2.8
3. `ArduinoJson` von Benoit Blanchon, Version 7.4.2

Zusätzlich muss die persönliche, nicht im Repository enthaltene lokale Library
`Credentials` vorhanden sein; ihr erwarteter Header und die Variablen werden im
Abschnitt **WLAN** beschrieben.

Empfohlene Boardoptionen sind OPI-PSRAM aktiviert, Partitionierung `Default`,
`USB CDC On Boot: Enabled` und zunächst 921600 Baud Uploadgeschwindigkeit. Die
Portauswahl hängt vom Betriebssystem und verwendeten USB-Anschluss ab.

## WLAN

Die Zugangsdaten liegen **nicht im Sketch und nicht in diesem Repository**. Der
Sketch bindet stattdessen die bereits lokal installierte Datei `Credentials.h`
aus dem persönlichen Arduino-Libraries-Verzeichnis ein:

```cpp
#pragma once
const char* ssid = "mein-wlan";
const char* password = "mein-passwort";
```

Eine übliche lokale Ablage ist beispielsweise
`Dokumente/Arduino/libraries/Credentials/src/Credentials.h`. Je nach bestehender
Arduino-Library kann die Headerdatei auch direkt im Ordner `Credentials` liegen.
Entscheidend ist, dass `#include <Credentials.h>` über den Arduino-Library-Suchpfad
gefunden wird und genau die beiden oben gezeigten Variablen `ssid` und `password`
bereitstellt. Weil die Datei außerhalb des Projektordners liegt, wird sie weder
von diesem Repository erfasst noch auf GitHub hochgeladen.

Falls die Arduino IDE beim Kompilieren `Credentials.h: No such file or directory`
meldet, muss die lokale Credentials-Library installiert bzw. an einen der genannten
Orte verschoben und die Arduino IDE danach neu gestartet werden.

WLAN- und MQTT-Verbindungen verwenden exponentiellen Backoff (1 bis 60 Sekunden),
der Hauptloop und das Display laufen weiter. Nach WLAN-Verbindung startet NTP mit
der POSIX-Zeitzone `CET-1CEST,M3.5.0,M10.5.0/3` (Europe/Zurich einschließlich DST).
Vor erfolgreichem NTP wird das Alter aus `millis()` seit Empfang bestimmt.

## MQTT-Datenquelle und Topics

Quelle ist das öffentliche Relay des Projekts
[`mrk-its/homeassistant-blitzortung`](https://github.com/mrk-its/homeassistant-blitzortung):
`blitzortung.ha.sed.pl:1883`. In Version 1.1 wird jedes Geohash-Zeichen als eigene
MQTT-Pfadkomponente codiert. Für den Ausschnitt 5.0–11.5° E / 45.3–48.3° N werden
nur die zwei überlappenden Präzision-2-Zellen abonniert:

```
blitzortung/1.1/u/0/#
blitzortung/1.1/u/2/#
```

Jede Nachricht wird danach nochmals exakt gegen den Kartenausschnitt gefiltert.
ArduinoJson liest `lat`, `lon` und `time`; `time` bleibt durchgehend `uint64_t`
(Nanosekunden). Ein offenes Hash-Table mit 8192 Plätzen dedupliziert in erwarteter
O(1)-Zeit, ohne den 4096-Einträge-Ringpuffer zu durchsuchen.

## Karte und Lizenz

Die kompakten Polylinien in `map_data.cpp` sind für diesen Ausschnitt vereinfachte,
auf 0,001° quantisierte Ableitungen aus **Natural Earth 1:10m Admin-0 Countries**
und **Natural Earth Lakes**. Natural Earth stellt diese Daten in die Public Domain:
<https://www.naturalearthdata.com/about/terms-of-use/>. Die abgeleiteten Arrays
bleiben ebenfalls gemeinfrei. Grenzen, Seen und Blitze nutzen dieselbe lineare
Equirektangular-Abbildung in `geo.cpp`; die zentralen Bounds stehen in `config.h`.
Die Karte ist bewusst schematisch und nicht für Navigation bestimmt.

## In der Arduino IDE bauen und flashen

1. Dieses Repository herunterladen oder klonen.
2. `LightningSwitzerland/LightningSwitzerland.ino` mit der Arduino IDE öffnen.
   Die IDE lädt automatisch alle `.h`- und `.cpp`-Dateien des Sketchordners als
   weitere Tabs; Dateien müssen nicht einzeln zu einem Projekt hinzugefügt werden.
3. Prüfen, dass die lokale `Credentials.h` wie im Abschnitt **WLAN** beschrieben
   über den Arduino-Library-Suchpfad verfügbar ist.
4. Das JC1060P470- beziehungsweise ESP32-P4-Board und den richtigen Port unter
   **Werkzeuge** auswählen.
5. Mit **Sketch → Überprüfen/Kompilieren** kompilieren.
6. Mit **Sketch → Hochladen** flashen.
7. **Werkzeuge → Serieller Monitor** öffnen und 115200 Baud einstellen. Nach dem
   Neustart müssen mindestens `Boot` und `Display initialized (1024x600)` erscheinen.

## Betrieb und Darstellung

Der MQTT-Callback validiert und speichert nur. Mit 4 Hz wird unabhängig davon die
Karte neu gezeichnet; Statistik und lokale Uhr werden dabei aktualisiert. Marker:
hell/Radius 7 plus Halo bis 60 s, orange/Radius 5 bis 5 min, dunkelrot/Radius 3 bis
15 min. Ältere Ereignisse werden weder gezählt noch gezeichnet und aus dem Puffer
entfernt. Der Status zeigt 1/5/15-Minuten-Zähler, letzten Empfang, WLAN, MQTT und
Ortszeit. Der feste Speicher liegt bevorzugt in PSRAM; es gibt keine Allokation pro
Ereignis.

## DEMO_MODE

In `config.h` steht standardmäßig `#define DEMO_MODE 0`. Für einen Offline-Test
in der Arduino IDE auf `1` setzen und den Sketch erneut kompilieren und hochladen.
Dann entstehen fortlaufend zufällige
Treffer innerhalb des vereinfachten Schweiz-Polygons mit Altern zwischen 0 und
15 Minuten. WLAN/MQTT dürfen dabei fehlen; Ringpuffer, Alterung, Statistik und
Rendering bleiben vollständig aktiv. `injectTestStrike(lat, lon, ageSeconds)` ist
zusätzlich öffentlich verfügbar.

## Bekannte Einschränkungen

* Der öffentliche, unverschlüsselte Broker bietet keine Verfügbarkeitsgarantie und
  keine TLS-Vertraulichkeit.
* PubSubClient führt einen Verbindungsversuch synchron aus; der Socket-Timeout ist
  deshalb auf eine Sekunde begrenzt. Versuche werden durch Backoff selten ausgeführt.
* Die lokale Karte ist stark vereinfacht; kleine Enklaven und Uferdetails fehlen.
* Touch, Helligkeitssteuerung und Zeitraumumschaltung sind bewusst für eine spätere
  Version vorgesehen.
* Vor dem ersten Hardwareeinsatz sollte die ESP-Hosted-C6-Firmware entsprechend der
  Anleitung des Boardlieferanten/Core-Releases abgeglichen werden. Der Sketch nimmt
  keine C6- oder SDIO-Pins in Besitz.
