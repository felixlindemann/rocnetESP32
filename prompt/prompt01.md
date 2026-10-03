# Rolle & Ziel
Du bist ein erfahrener Embedded-C++-Entwickler für ESP32 und Modelleisenbahn-Automatisierung.
Erstelle ein vollständiges, kompilierbares PlatformIO-Projekt für das Board **Wemos Lolin32 Lite**.

Der ESP32 soll als **RocNet-Knoten (UDP-Multicast)** arbeiten und zwei Aufgaben simultan, nicht-blockierend erfüllen:
1. **Signale steuern** über WS2811 an **GPIO 32** (mittels Rocrail Aspect-Bitmasken).
2. **Weichen/Servos steuern** über einen **PCA9685** via Hardware-I2C (**SDA: GPIO 21, SCL: GPIO 22**).

---

## 1. Hardware & Framework
* **Platform:** espressif32
* **Board:** `lolin32_lite`
* **Framework:** arduino
* **Libraries:**
  * `Adafruit PWM Servo Driver Library` (für PCA9685)
  * `NeoPixelBus` von Makuna (für WS2811 an GPIO32, dynamische Chip-Anzahl zur Laufzeit instanziierbar via RMT)
  * `WiFi` & `WiFiUdp`

---

## 2. RocNet-Netzwerkarchitektur
* **Multicast-IP:** `224.0.0.1`
* **Multicast-Port:** `1281`
* **Eigene Node-ID:** Als Konstante konfigurierbar (Default: `204`).
* Alle eingehenden UDP-Pakete müssen gefiltert werden:
  * Nur Pakete verarbeiten, deren Empfänger-Node-ID mit der konfigurierten `MY_NODE_ID` übereinstimmt (oder Broadcasts, falls relevant).

---

## 3. Signalsteuerung (WS2811 & Rocrail Aspects)
* **Pin:** GPIO 32.
* **Protokoll-Handling:**
  * Rocrail sendet für Signalschaltungen im Modus *Aspect numbers* UDP-Frames der Gruppe **Group 0x08** (Aspect / Extended Output).
  * Paket-Struktur:
    - Byte 1..2: Node-ID
    - Byte 3: Group (`0x08`)
    - Byte 5: Port / Signaladresse (1-basiert)
    - Byte 6: Aspect Value High Byte
    - Byte 7: Aspect Value Low Byte (ergibt eine 16-Bit-Bitmaske)
* **Mapping-Logik:**
  * Jeder physische WS2811-Treiberbaustein stellt 3 Ausgänge bereit (R, G, B).
  * Port $N$ mappt auf die entsprechenden WS2811-Pixel.
  * Bit 0 der Maske steuert Kanal R (LED 1), Bit 1 steuert Kanal G (LED 2), Bit 2 steuert Kanal B (LED 3). Bei mehrbegriffigen Signalen über mehrere WS2811 werden nachfolgende Bits fortlaufend auf die nächsten Kanäle verteilt.
  * Zustand `1` schaltet den Ausgang auf volle Helligkeit (oder konfigurierbaren Dimmwert), `0` schaltet ab.

---

## 4. Weichensteuerung (PCA9685 via I2C)
* **Bus:** I2C an `SDA = 21`, `SCL = 22`. I2C-Adresse: `0x40`. PWM-Frequenz: `50 Hz`.
* **Protokoll-Handling:**
  * Rocrail-Weichenkommandos (Group `0x02` für klassische Weichenbefehle `turnout straight/turnout thrown` oder Group `0x03` für Servo-Direktansteuerung).
  * Umsetzung von Weichenport (1..16) auf PCA9685-Kanal (0..15).
  * Servofahrt muss sanft und ruckelfrei (interpolierte Bewegung per Software-Timer, keine blockierenden `delay()`) von `MIN_PULSE` zu `MAX_PULSE` fahren.

---

## 5. Code-Anforderungen
* Strukturierte, saubere Trennung:
  * `main.cpp`: Setup, WiFi-Verbindung, Task/Loop-Scheduling.
  * `RocNetParser.h/.cpp`: Dekodierung der Binärtelegramme.
  * `SignalOutput.h/.cpp`: Kapselung von NeoPixelBus und Bitmasken-Übersetzung.
  * `ServoController.h/.cpp`: Kapselung des PCA9685 und sanfte Schritt-Interpolation.
* Vollständig non-blocking implementiert (Verwendung von `millis()` für Servolaufzeiten und LED-Aktualisierungen).
* Füge eine passende `platformio.ini` mit allen benötigten Abhängigkeiten bei.