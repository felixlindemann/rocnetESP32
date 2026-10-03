# RocNet ESP32 Controller

[![Release v1](https://img.shields.io/badge/Release-v1-blue.svg)](RELEASE_NOTES.md)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange.svg)](platformio.ini)

> 📦 **Aktuelles Release:** [Release Notes v1 (v1.0.0)](RELEASE_NOTES.md)

Ein vollständiges, hochperformantes und nicht-blockierendes **PlatformIO-Projekt** für das Board **Wemos Lolin32 Lite** zur Steuerung einer Modelleisenbahn über das **RocNet-Protokoll (UDP-Multicast)** in Verbindung mit **Rocrail**.

Der ESP32 übernimmt dabei simultan drei zentrale Aufgaben:
1. **Signalsteuerung via WS2811** an **GPIO 32** unter Verwendung von Rocrail *Aspect*-Bitmasken (Group `0x08`).
2. **Weichen- und Servosteuerung via PCA9685** über Hardware-I2C (**SDA: GPIO 21, SCL: GPIO 22**) für bis zu 64 Servos mit sanfter, ruckelfreier S-Kurven-Schritt-Interpolation (Group `0x02` und Group `0x03`) und automatischer Endlagenrückmeldung (Group `0x03` Action 7).
3. **Eingebettete WebUI & 3-Stufen-Logging:** Feintuning aller Servos per Web-Schieberegler im Browser, NVS-Speicherung, Host Ping/Pong, Identify/Show und ein dreistufiges Logging-System (Fehler, Debug, Info) mit Live-Streaming-Konsole.

---

## Inhaltsverzeichnis
- [Release Notes (v1)](RELEASE_NOTES.md)
- [Hardware & Pinbelegung](#hardware--pinbelegung)
- [Systemarchitektur & Non-Blocking Design](#systemarchitektur--non-blocking-design)
- [RocNet-Netzwerkarchitektur](#rocnet-netzwerkarchitektur)
- [Signalsteuerung (WS2811 & Rocrail Aspects)](#signalsteuerung-ws2811--rocrail-aspects)
- [Weichensteuerung (PCA9685 via I2C)](#weichensteuerung-pca9685-via-i2c)
- [WebUI & 3-stufiges Live-Logging](#webui--3-stufiges-live-logging)
- [Projektstruktur](#projektstruktur)
- [Konfiguration](#konfiguration)
- [Installation & Flashen](#installation--flashen)
- [Rocrail Konfigurationsanleitung](#rocrail-konfigurationsanleitung)
- [Lizenz & Referenzen](#lizenz--referenzen)

---

## Hardware & Pinbelegung

### Board: Wemos Lolin32 Lite (ESP32)

| Funktion | ESP32 GPIO | Peripherie | Beschreibung |
|---|---|---|---|
| **WS2811 Data** | **GPIO 32** | WS2811 Signal-Kette | RMT-Hardware-Treiber (NeoPixelBus) |
| **I2C SDA** | **GPIO 21** | PCA9685 SDA | Hardware-I2C Datenleitung |
| **I2C SCL** | **GPIO 22** | PCA9685 SCL | Hardware-I2C Taktleitung |
| **GND** | GND | Common GND | Gemeinsame Masse für ESP32, WS2811 & PCA9685 |
| **5V / VCC** | 5V / Externe Quelle | WS2811 & Servos | **Achtung:** Servos und LEDs über separates 5V-Netzteil versorgen! |

> [!IMPORTANT]
> **Stromversorgung:** Schließe die Servos und die WS2811-LEDs **niemals** direkt an die 3.3V-Stromversorgung des ESP32 an! Verwende ein ausreichend dimensioniertes 5V-Schaltnetzteil (z.B. 5V / 3A–5A) für Servos und LEDs und verbinde die Massen (**GND**) aller Komponenten miteinander.

---

## Systemarchitektur & Non-Blocking Design

Das gesamte System ist **100% blockierungsfrei (non-blocking)** konzipiert:
- **Keine `delay()`-Aufrufe** im laufenden Betrieb: Servofahrten und LED-Updates arbeiten zeitgesteuert über `millis()`.
- **Hardware-RMT-Peripherie** für WS2811: Das Generieren der hochfrequenten WS2811-Signale wird vollständig an die ESP32-RMT-Hardware delegiert, ohne die CPU zu blockieren.
- **Hardware-I2C** für PCA9685: 50 Hz PWM-Frequenz mit 12-Bit Auflösung (4096 Schritte).
- **UDP-Multicast-Verarbeitung**: Eingehende Telegramme werden direkt aus dem Socket-Puffer gelesen, dekodiert und an die jeweiligen Treiber weitergeleitet.

---

## RocNet-Netzwerkarchitektur

* **Multicast-IP:** `224.0.0.1`
* **Multicast-Port:** `4321` (Offizieller RocNet Standard)
* **Eigene Node-ID:** Standardmäßig `204` (konfigurierbar in [`Config.h`](include/Config.h)).
* **Telegramm-Filterung:**
  - Jedes UDP-Paket enthält in Byte 1..2 die Empfänger-Node-ID (Big-Endian).
  - Pakete werden nur verarbeitet, wenn die Empfänger-Node-ID mit der konfigurierten `MY_NODE_ID` übereinstimmt oder an die Broadcast-Adresse `0` bzw. `0xFFFF` gesendet wurde. Pakete für fremde Knoten werden sofort verworfen.

---

## Signalsteuerung (WS2811 & Rocrail Aspects)

Rocrail sendet im Modus **Aspect numbers** UDP-Frames der Gruppe **0x08** (Aspect / Extended Output):
- **Byte 1..2:** Node-ID
- **Byte 3:** Group (`0x08`)
- **Byte 5:** Port / Signaladresse (1-basiert)
- **Byte 6:** Aspect Value High Byte
- **Byte 7:** Aspect Value Low Byte (16-Bit Aspect Bitmaske)

### Mapping-Logik
Jeder physische WS2811-Chip besitzt **3 Ausgänge** (R, G, B):
- **Port $N$** (1-basiert) startet standardmäßig bei Kanal $(N - 1) \times 3$.
- **Bit 0** steuert Kanal R (LED 1 / Lampe 1).
- **Bit 1** steuert Kanal G (LED 2 / Lampe 2).
- **Bit 2** steuert Kanal B (LED 3 / Lampe 3).
- Bei mehrbegriffigen Signalen mit mehr als 3 Lampen (z.B. Haupt-/Vorsignale mit 4 bis 6 Lampen) werden die nachfolgenden Bits fortlaufend auf die nächsten WS2811-Treiberbausteine verteilt:
  - Bit 3 -> Nächster WS2811, Kanal R
  - Bit 4 -> Nächster WS2811, Kanal G
  - Bit 5 -> Nächster WS2811, Kanal B
- Wert `1` schaltet den entsprechenden Ausgang mit der eingestellten Helligkeit ein (Standard: `255`), Wert `0` schaltet ihn aus.

---

## Weichensteuerung (PCA9685 via I2C)

* **I2C-Bus:** `SDA = GPIO 21`, `SCL = GPIO 22`, I2C-Adresse: `0x40`.
* **PWM-Frequenz:** `50 Hz` (20 ms Periodendauer).
* **Port-Mapping:** Weichenport $1..16$ mappt direkt auf PCA9685-Kanal $0..15$ (`Kanal = Port - 1`).

### Ruckelfreie Schritt-Interpolation
Servos dürfen bei Modellbahnweichen nicht ruckartig umschalten, sondern müssen vorbildgerecht langsam und geschmeidig umlaufen:
- **Interpolation:** Ausführung alle 20 ms (passend zum 50 Hz PWM-Zyklus).
- **S-Kurven-Beschleunigung (Smoothstep):** 
  $$\text{eased}(t) = 3t^2 - 2t^3 \quad (t \in [0, 1])$$
  Dies sorgt für ein sanftes Anfahren und sanftes Abbremsen an den Endlagen.
- **Konfigurierbare Endlagen & Laufzeiten:**
  - Standard `MIN_PULSE`: $1000\,\mu\text{s}$ (Gerade / Straight)
  - Standard `MAX_PULSE`: $2000\,\mu\text{s}$ (Abzweig / Thrown)
  - Standard-Laufzeit: $1200\,\text{ms}$
- **Idle Power-Down:** Nach Erreichen der Endlage wird das PWM-Signal nach 500 ms abgeschaltet (`sleepOnIdle`). Dadurch wird das typische Servo-Brummen und der Ruhestromverbrauch komplett eliminiert!

### Unterstützte RocNet-Befehle
1. **Group `0x02` (Klassische Weichenbefehle):**
   - Schaltet Weiche auf Gerade (`0`) bzw. Abzweig (`1`).
2. **Group `0x03` (Servo-Direktansteuerung):**
   - Direkte Winkelvorgabe ($0..180^\circ$) oder Pulsbreitenvorgabe in Mikrosekunden.

---

## WebUI & 3-stufiges Live-Logging

Über die integrierte Weboberfläche (erreichbar per Browser über `http://<esp32-ip>` oder `http://rocnet-servo.local`) stehen umfangreiche Diagnose- und Einstellungsfunktionen zur Verfügung:
- **Servo-Finetuning:** Kalibrierung der Endlagen (Gerade/Abzweig), Stellzeiten (100–2000 ms), Drehrichtungsumkehr und Live-Testen für bis zu 64 Servos.
- **Hardware-Konfiguration:** Node-ID, Modulanzahl (1–4 PCA9685), WS2811-Chipanzahl, Signalhelligkeit und Pins dauerhaft im Flash (NVS) speichern.
- **RocNet-Interaktion:** Manueller Node-Identify-Befehl an Rocrail, automatische Weichenendlagen-Meldung (`RN_STATIONARY_REPORT`) und Host-Ping-Beantwortung (`RN_HOST_PONG`).
- **3-Stufen-Logging:**
  - **Stufe 1 (FEHLER / Standard):** Nur echte Hardware- und Systemfehler (I2C-Ausfälle, Socketfehler).
  - **Stufe 2 (DEBUG):** Wesentliche Events wie Weichenumläufe, Signalbildwechsel und Pings.
  - **Stufe 3 (INFO):** Vollständige Hexdumps aller UDP-Pakete, Heartbeats und Webzugriffe.
- **Live-Protokollkonsole:** Terminal im Webdashboard mit Zeitstempeln, Auto-Scroll, Pause-Funktion, Pufferleerung und Stufenauswahl.

---

## Projektstruktur

```
rocNetESP-Controller/
├── include/
│   ├── Config.h             # Zentrale Hardware-Voreinstellungen & Timings
│   ├── secrets.h            # WLAN-Zugangsdaten (lokal, nicht in Git)
│   ├── secrets.h.example    # Vorlage für WLAN-Zugangsdaten (in Git)
│   ├── AppLog.h             # 3-stufiges Logging- & Ringspeichersystem
│   ├── SystemConfig.h       # Persistente NVS-Flash-Systemkonfiguration
│   ├── RocNetParser.h       # Dekodierung der RocNet-Binärtelegramme
│   ├── ServoController.h    # Kapselung PCA9685, Sanftanlauf & Endlagenrückmeldung
│   ├── SignalOutput.h       # Kapselung NeoPixelBus & Aspect-Bitmasken-Mapping
│   ├── WebServerManager.h   # HTTP-Webserver & REST-API-Endpunkte
│   └── WebUI.h              # Eingebettete HTML5/CSS3/JS Web-Applikation
├── src/
│   ├── main.cpp             # Setup, WiFi, UDP-Multicast, Loop & RocNet-Dispatch
│   ├── AppLog.cpp           # Ringspeicher-Pufferung, Serial-Output & JSON-Export
│   ├── SystemConfig.cpp     # NVS Preferences-Speicherung & Laden
│   ├── RocNetParser.cpp     # Parsing kompakter & Standard-RocNet-Pakete
│   ├── ServoController.cpp  # Schrittweiser Servolauf ohne delay()
│   ├── SignalOutput.cpp     # Dynamische WS2811-Verwaltung via RMT
│   └── WebServerManager.cpp # REST-Endpoints für Status, Logs, Kalibrierung & Reboot
├── test/
│   └── test_rocnet_parser.cpp # Native Unit-Tests für Protokollparser & Logging
├── lib/
│   └── rocnet-const.h       # Rocrail C-Header (Copied from Rocrail)
├── literature/              # Referenzdokumentation (Copied from Rocrail)
│   ├── README.md            # Dokumentation & Herkunftshinweise
│   ├── NOTICE               # Lizenz- und Copyright-Hinweis
│   └── ROCNET_SPECIFICATION.md # Protokollspezifikation (Markdown)
├── platformio.ini           # PlatformIO Build-Konfiguration & Abhängigkeiten
├── ATTRIBUTION.md           # Ausführlicher Quellennachweis & Herkunft
├── LICENSE                  # MIT-Lizenz (für den Projektquellcode)
├── RELEASE_NOTES.md         # Release Notes v1.0.0
├── .gitignore               # Ignoriert u.a. secrets.h und Build-Ordner
└── README.md                # Projektdokumentation
```

---

## Konfiguration

### 1. WLAN-Zugangsdaten (`secrets.h`)
Trage deine WLAN-Zugangsdaten in [`include/secrets.h`](include/secrets.h) ein (bzw. erstelle die Datei basierend auf der Vorlage `include/secrets.h.example`):

```cpp
#define WIFI_SSID "DeinWLANName"
#define WIFI_PASSWORD "DeinWLANPasswort"
```

### 2. Controller- & Hardware-Parameter (`Config.h`)
Passe bei Bedarf in [`include/Config.h`](include/Config.h) die Parameter an:

```cpp
// RocNet Node-ID
constexpr uint16_t DEFAULT_ROCNET_NODE_ID = 204;

// WS2811 Anzahl Chips (16 Chips = 48 Ausgänge)
constexpr uint16_t DEFAULT_WS2811_CHIP_COUNT = 16;
constexpr uint8_t DEFAULT_SIGNAL_BRIGHTNESS = 255;

// Servo-Laufzeiten und Endlagen
constexpr uint16_t SERVO_DEFAULT_MIN_PULSE_US = 1000;
constexpr uint16_t SERVO_DEFAULT_MAX_PULSE_US = 2000;
constexpr uint32_t SERVO_DEFAULT_TRAVEL_TIME_MS = 1200;
```

---

## Installation & Flashen

### Mit PlatformIO (VSCode oder CLI)

1. Projektordner in **VSCode** mit installierter **PlatformIO-Erweiterung** öffnen.
2. In der PlatformIO-Statusleiste auf **Build** (`✓`) klicken.
3. Wemos Lolin32 Lite per USB anschließen und auf **Upload** (`→`) klicken.
4. Den **Serial Monitor** auf `115200` Baud öffnen, um den Start-Banner und Verbindungsstatus zu prüfen.

---

## Rocrail Konfigurationsanleitung

### 1. Zentrale einrichten
1. In Rocrail: **Rocrail Eigenschaften** -> **Zentralen** -> **Neu** -> Typ: `rocnet`.
2. Port: `4321` (Standard-Port für RocNet)
3. Subnetz / Multicast: `224.0.0.1`

### 2. Weichen konfigurieren
1. Weiche auswählen -> **Eigenschaften** -> **Schnittstelle**.
2. **Schnittstellenkennung (IID):** Name der RocNet-Zentrale.
3. **Bus:** `204` (entspricht der `MY_NODE_ID` des ESP32).
4. **Adresse:** `1` bis `16` (entspricht PCA9685 Kanal 0 bis 15).
5. **Protokoll:** `Default`.

### 3. Signale mit Aspect Numbers konfigurieren
1. Signal auswählen -> **Eigenschaften** -> **Schnittstelle**.
2. **Schnittstellenkennung (IID):** Name der RocNet-Zentrale.
3. **Bus:** `204` (`MY_NODE_ID`).
4. **Adresse / Port:** Signalport (1-basiert).
5. **Steuerung:** `Aspect numbers`.
6. **Begriffe (Aspects):**
   - Unter **Begriffe** jedem Signalbild (z.B. Hp0, Hp1, Hp2, Sh1) die gewünschte Bitmaske zuweisen.
   - Bit 0 = Lampe 1 (Rot)
   - Bit 1 = Lampe 2 (Grün)
   - Bit 2 = Lampe 3 (Gelb / Weiß)
   - Folgende Bits für weitere Lampen.

---

## Lizenz & Referenzen

- **Eigene Software:** Der Quellcode dieses Projekts steht unter der freien [MIT-Lizenz](LICENSE) (&copy; 2026 Felix Lindemann).
- **RocNet Protokollspezifikation:** Sämtliche Protokolldefinitionen in [`lib/rocnet-const.h`](lib/rocnet-const.h) und [`literature/ROCNET_SPECIFICATION.md`](literature/ROCNET_SPECIFICATION.md) basieren auf der offiziellen Spezifikation von Rocrail (**Copied from Rocrail**):
  - Quelle: [https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en](https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en)
  - Urheberrecht: &copy; 2002–2026 Robert Jan Versluis, [rocrail.net](https://www.rocrail.net). Alle Rechte vorbehalten.
  - Ausführliche Hinweise siehe [`literature/README.md`](literature/README.md) und [`literature/NOTICE`](literature/NOTICE).
- **Vollständiger Quellennachweis:** Eine detaillierte Übersicht aller Lizenzen, Quellen und Drittbibliotheken befindet sich in [`ATTRIBUTION.md`](ATTRIBUTION.md).
- Die Nutzung dieser Spezifikationen und Schnittstelleninformationen dient ausschließlich der Gewährleistung technischer Interoperabilität (EU-Richtlinie 2009/24/EG Art. 6 / § 69e UrhG).


