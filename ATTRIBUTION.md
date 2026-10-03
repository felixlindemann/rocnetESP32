# Quellennachweis & Urheberrechtsangaben (Attribution & Licenses)

Dieses Dokument gibt einen vollständigen Überblick über sämtliche verwendeten Quellen, Lizenzen, externe Bibliotheken und Urheberrechte im Projekt **RocNet ESP32 Controller**.

---

## 1. Lizenz dieses Projekts (MIT License)

Der Quellcode des Projekts **RocNet ESP32 Controller** (Verzeichnisse `src/`, `include/`, `test/`) steht unter der freien **MIT-Lizenz**:
- **Copyright:** &copy; 2026 Felix Lindemann
- **Lizenztext:** Siehe [`LICENSE`](LICENSE)

---

## 2. Aus Rocrail übernommene Spezifikationen ("Copied from Rocrail")

Die folgenden Dateien basieren direkt auf der offiziellen Dokumentation bzw. den Code-Beigaben des **Rocrail**-Projekts ([rocrail.net](https://www.rocrail.net)):

| Datei / Pfad | Herkunft / Quelle | Urheber / Copyright | Zweck |
|---|---|---|---|
| [`lib/rocnet-const.h`](lib/rocnet-const.h) | `rocnet-const.h.zip` via Rocrail Wiki | &copy; 2002–2017 Robert Jan Versluis, Rocrail.net | C-Header mit Protokollkonstanten, Gruppen- und Aktionscodes |
| [`literature/ROCNET_SPECIFICATION.md`](literature/ROCNET_SPECIFICATION.md) | [RocNet Protocol Specification](https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en) | &copy; 2002–2026 Robert Jan Versluis, Rocrail.net | Technische Spezifikation des RocNet-Protokolls (Paketstruktur, Gruppen, Aktionscodes) |

*Ausführliche Hinweise hierzu siehe auch [`literature/README.md`](literature/README.md) und [`literature/NOTICE`](literature/NOTICE).*

---

## 3. Auf der RocNet-Spezifikation basierende Eigenentwicklungen (Interoperabilität)

Die nachfolgenden Quellcodedateien wurden eigenständig entwickelt, um die Kommunikation mit Rocrail über das binäre RocNet-Protokoll (UDP-Multicast) herzustellen. Die Paketstrukturen, Offsets, Gruppen und Aktionscodes orientieren sich an der unter (2) genannten Spezifikation:

| Datei / Pfad | Beschreibung & Referenzbezug |
|---|---|
| [`include/RocNetParser.h`](include/RocNetParser.h) & [`src/RocNetParser.cpp`](src/RocNetParser.cpp) | Dekodierung von RocNet-Telegrammen: Header-Struktur (NetID, Empfänger, Sender, Gruppe, Aktion, Länge), Gruppe 0x01 (Weichen), Gruppe 0x03 (Stationary), Gruppe 0x05 (Programmierung), Gruppe 0x08 (Aspects) und Gruppe 0x09 (Output). |
| [`include/Config.h`](include/Config.h) | Netzwerk- und Protokollkonfiguration: Multicast-IP `224.0.0.1`, Standard-Port `4321`, Standard Node-ID `204`, Gruppen-Definitionen und Aspect-Bitmasken-Standards. |
| [`src/main.cpp`](src/main.cpp) | Multicast UDP Socket-Handling, Antworttelegramme für Knotenidentifikation (`sendIdentifyReply`), Start-of-Day/Show-Events (`sendShowEvent`) und Kanalabfragen (`sendChannelConfigReply`). |
| [`include/SignalOutput.h`](include/SignalOutput.h) & [`src/SignalOutput.cpp`](src/SignalOutput.cpp) | Umsetzung von Rocrail Aspect-Bitmasken auf physische WS2811-LED-Kanäle ([Rocrail Aspects](https://wiki.rocrail.net/doku.php?id=aspects-en)). |
| [`include/ServoController.h`](include/ServoController.h) & [`src/ServoController.cpp`](src/ServoController.cpp) | Weichensteuerung mit Paired-Ports-Mapping und Programmierung von Endlagen und Schritten gemäß RocNet Gruppe 5. |
| [`test/test_rocnet_parser.cpp`](test/test_rocnet_parser.cpp) | Testfälle mit Beispiel-Binärtelegrammen gemäß Spezifikation. |

**Rechtlicher Rahmen:** Gemäß EU-Richtlinie 2009/24/EG über den Rechtsschutz von Computerprogrammen (Art. 6) sowie § 69e UrhG sind Schnittstellen- und Kommunikationsprotokolle nicht urheberrechtlich schutzfähig, soweit ihre Verwendung der Herstellung der Interoperabilität unabhängig geschaffener Software und Hardware dient.

---

## 4. Verwendete externe Open-Source-Bibliotheken

| Bibliothek | Autor / Organisation | Lizenz | Verwendung |
|---|---|---|---|
| **Adafruit PWM Servo Driver Library** | Adafruit Industries | BSD-Lizenz | Hardware-Treiber für den I2C-PWM-Baustein PCA9685 |
| **NeoPixelBus** | Michael C. Miller (Makuna) | LGPLv3 | Hochpräziser, hardwaregestützter RMT-Treiber für WS2811 an ESP32 |
| **Arduino Core for ESP32** | Espressif Systems / Arduino Team | LGPLv2.1 / Apache 2.0 | Framework, WiFi, WiFiUDP, Preferences (NVS), WebServer, ESPmDNS |

---

## 5. Sicherheitshinweis für öffentliche Repositories (Public Archive)

- **WLAN-Zugangsdaten:** Die Datei `include/secrets.h` enthält vertrauliche WLAN-Passwörter und ist in [`.gitignore`](.gitignore) eingetragen, damit sie **nicht** in öffentliche Git-Repositories gepusht wird.
- **Vorlage:** Für öffentliche Commits steht die Bereitstellungsvorlage [`include/secrets.h.example`](include/secrets.h.example) zur Verfügung.
