# Release Notes - RocNet ESP32 Controller v1.0.0

**Datum:** Oktober 2026  
**Ziel-Hardware:** ESP32 (Wemos Lolin32 Lite)  
**Protokoll:** RocNet v1.0 (UDP Multicast `224.0.0.1:4321`) über WiFi

---

## 🌟 Übersicht

Version 1.0.0 ist das erste offizielle Release des **RocNet ESP32 Controllers**. Das Projekt ermöglicht die Ansteuerung von Modellbahnweichen (bis zu 64 Servos via PCA9685) und Modellbahnsignalen (WS2811 / NeoPixel) direkt aus **Rocrail** über das standardisierte RocNet-Netzwerkprotokoll. 

Die Firmware arbeitet **100 % nicht-blockierend (`non-blocking`)**, bietet eine integrierte **WebUI zur Kalibrierung und Diagnose** im Browser und ein **3-stufiges Logging-System** mit Live-Streaming.

---

## 🚀 Hauptfunktionen & Features

### 1. Weichensteuerung mit Sanftanlauf (PCA9685 via I2C)
- **Kapazität:** Bis zu 64 Servos kaskadierbar (1 bis 4 PCA9685 Module à 16 Ports, I2C-Adressen `0x40`–`0x43`).
- **S-Kurven-Sanftanlauf (Smoothstep):** Ruckelfreier, realistischer Weichenumlauf ohne ruckartiges Reißen an der Stellschwelle.
- **Individuelle Kalibrierung:** Endlagen ($0^\circ$–$180^\circ$), Stellzeit (100–2000 ms) und Drehrichtungsumkehr pro Servo separat einstellbar.
- **RocNet-Endlagenrückmeldung:** Sobald ein Servo seine Zielposition erreicht hat, sendet der ESP32 automatisch ein `RN_STATIONARY_REPORT` (Gruppe 3, Aktion 7) an Rocrail.
- **Schaltspiel-Zähler:** Jeder Servo zählt seine Schaltungen und speichert sie im NVS-Flash.
- **Idle Power-Down:** Abschaltung des PWM-Signals 500 ms nach Erreichen der Endlage zur Vermeidung von Brummen und zur Stromeinsparung.

### 2. Signalsteuerung (WS2811 LED-Treiber via Hardware-RMT)
- **Signalbilder (Aspect Numbers):** Ansteuerung von Signalen über 16-Bit-Aspect-Masken (RocNet Gruppe 8 / Gruppe 9).
- **Hardware-RMT-Treiber:** Blockierungsfreie Signalgenerierung über den ESP32-RMT-Koprozessor (`NeoPixelBus`).
- **Skalierbar:** Bis zu 128 WS2811 Chips (384 Einzellampen) an Pin GPIO 32.
- **Globale Helligkeit:** Einstellbar von 10 bis 255.

### 3. RocNet-Protokollstandard (Vollständige Interoperabilität)
- **Empfangene Telegramme:**
  - `TURNOUT_SWITCH` (Gruppe 2 / Gruppe 3 / Gruppe 9): Klassische Weichenumläufe (Gerade/Abzweig).
  - `SIGNAL_ASPECT` (Gruppe 8 / Gruppe 9): Signalbegriffe und Aspect-Masken.
  - `SERVO_DIRECT` (Gruppe 3 / Gruppe 5): Direkte Winkel- und Impulsvorgaben.
  - `HOST_PING` (Gruppe 0, Aktion 2): Zentralen-Verbindungsprüfung.
  - `PORT_QUERY` (Gruppe 3, Aktionen 4 & 5): Abfrage des aktuellen Weichen-Istzustands.
  - `SHOW_IDENTIFY` (Gruppe 3, Aktion 11): Identifikation des Decoders (Status-LED blinkt).
  - `NODE_ID_WRITE` (Gruppe 5, Aktion 6 `WRNID`): Remote-Änderung der Node-ID über Rocrail.
  - `OPTIONS_READ` (Gruppe 5, Aktion 7 `RDOPT`): Hardware-Optionen und I2C-Scan-Ergebnis.
  - `SOFTWARE_RESET` (Gruppe 5, Aktion 11 `UPDATE`): Geordneter Software-Neustart.
- **Gesendete Telegramme:**
  - `RN_HOST_PONG` (Gruppe 0, Aktion 3): Antwort auf Ping mit Uptime und Status.
  - `RN_STATIONARY_REPORT` (Gruppe 3, Aktion 7): Weichenendlagen-Meldung und Port-Abfrageantwort.
  - `RN_STATIONARY_IDENTIFY` (Gruppe 3, Aktion 8): Anmeldung der Node bei Rocrail.

### 4. Integrierte WebUI (HTML5 / CSS3 / Vanilla JS)
- Erreichbar im lokalen Netzwerk per IP oder mDNS: `http://rocnet-servo.local`.
- Responsive Oberfläche im modernen Dark-Mode (optimiert für Desktop, Tablet und Smartphone).
- **Interaktive Servo-Schieberegler** mit Echtzeit-Vorschau ("Live testen beim Schieben").
- **Systemeinstellungs-Dialog:** Node-ID, Node-Name, Pins (SDA/SCL/WS2811), Adressierungsmodus und WLAN-Parameter.
- **Flash-Persistenz:** Dauerhafte Speicherung aller Einstellungen im ESP32 Non-Volatile Storage (NVS).

### 5. Dreistufiges Logging-System mit Live-Webkonsole
- **Stufe 1 (FEHLER / Standard):** Nur echte Hardware- und Systemfehler (I2C-Timeout, NVS-Fehler, Socketfehler).
- **Stufe 2 (DEBUG):** Relevante Betriebsevents (Weichenumläufe, Signalaspekte, Ping/Pong, Identify).
- **Stufe 3 (INFO):** Vollständige Hex-Dumps aller UDP-Pakete, Heartbeats und Webzugriffe.
- **Live-Terminal:** Monospace-Konsole direkt im WebUI-Dashboard mit Zeitstempel, Auto-Scroll, Pause-Modus und Schnellauswahl der Filterstufe.

---

## 📁 Repository- & Lizenzstruktur

- Alle Fremdreferenzen im Verzeichnis `literature/` sind eindeutig als **"Copied from Rocrail"** deklariert.
- Ausführlicher Quellennachweis und Lizenzdokumentation in `ATTRIBUTION.md`.
- Sensible Daten (`include/secrets.h`) sind strikt durch `.gitignore` vom Git-Tracking ausgeschlossen; Vorlage `include/secrets.h.example` liegt bereit.
- Vollständige, native Unit-Test-Suite (`test/test_rocnet_parser.cpp`) für alle Parser- und Logger-Funktionen.
