# RocNet Protocol Specification (Technical Reference)

> **Quellen- und Urheberrechtsangabe / Source Attribution:**  
> - **Quelle:** [Rocrail Wiki - RocNet Protocol](https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en)  
> - **Titel:** `rocnet:rocnet-prot-en [Innovative Model Railroad Control System]`  
> - **Urheber:** Copyright &copy; 2002–2026 Robert Jan Versluis, [rocrail.net](https://www.rocrail.net). Alle Rechte vorbehalten.  
> - **Zweck:** Technische Spezifikation zur Gewährleistung der Protokoll-Interoperabilität gemäß EU-Richtlinie 2009/24/EG Art. 6 / § 69e UrhG.

---

## 1. Netzwerk-Topologie & Transport

- **Protokoll:** UDP-Multicast über Ethernet oder WLAN (802.11 b/g/n)
- **Multicast-IP-Adresse:** `224.0.0.1`
- **Multicast-Port:** `4321` (Standard-Port für alle RocNet-Knoten und die Zentrale)
- **Byte-Reihenfolge (Endianness):** Big-Endian (Most Significant Byte zuerst)

---

## 2. Paketstruktur (Packet Layout)

Jedes standardkonforme RocNet-Paket besteht aus einem **8-Byte Header**, gefolgt von bis zu **255 Datenbytes**:

| Byte-Index | Feld | Datentyp | Beschreibung |
|:---|:---|:---|:---|
| **0** | `netid` | `uint8` | Netzwerk-Kennung (Standard: `1`, Bit 7 kann gesetzt sein `0x81` zur Paket-Synchronisation) |
| **1** | `rcptH` | `uint8` | Empfänger-Knoten-ID High-Byte |
| **2** | `rcptL` | `uint8` | Empfänger-Knoten-ID Low-Byte (`0` = Broadcast an alle Knoten) |
| **3** | `sndrH` | `uint8` | Absender-Knoten-ID High-Byte |
| **4** | `sndrL` | `uint8` | Absender-Knoten-ID Low-Byte |
| **5** | `group` | `uint8` | Funktionsgruppe (siehe Gruppenübersicht) |
| **6** | `action` | `uint8` | Bit 5..7: Aktionstyp, Bit 0..4: Aktionscode (siehe unten) |
| **7** | `len` | `uint8` | Anzahl der nachfolgenden Datenbytes (0..255) |
| **8..n** | `data[0..n-1]` | `uint8[]` | Nutzdaten (Länge entspricht dem Feld `len`) |

### Aufbau des `action`-Bytes (Byte 6)

Das Byte `action` setzt sich aus zwei Bitfeldern zusammen:

$$\text{action} = (\text{type} \ll 5) \mid (\text{code} \ \& \ 0\text{x}1\text{F})$$

- **Aktionstypen (Bits 7..5):**
  - `0` (`0x00`): **Request** – Befehl / Anforderung an den Knoten.
  - `1` (`0x20`): **Event** – Ereignis / Statusänderung (z. B. Sensor-Auslösung, Weichenendlage).
  - `2` (`0x40`): **Reply** – Antwort auf eine Abfrage (z. B. Pong, Statusreport, Konfigurationsdaten).
- **Aktionscode (Bits 4..0):**
  - Spezifischer Befehl innerhalb der jeweiligen Gruppe (Werte `0` bis `31`).

---

## 3. Übersicht der Gruppen (`group`, Byte 5)

| Gruppe (Dezimal) | Konstante in `rocnet-const.h` | Beschreibung |
|:---:|:---|:---|
| **0** | `RN_GROUP_HOST` | Host- und Zentralkommandos (Ping, Pong, Shutdown) |
| **1** | `RN_GROUP_CS` | Zentrale-Befehle |
| **2** | `RN_GROUP_MOBILE` | Lokomotiv- & Fahrzeugsteuerung (Fahrstufe, Funktionen) |
| **3** | `RN_GROUP_STATIONARY` | Stationäre Decoder (Weichen, Servos, Relais, Signale) |
| **4** | `RN_GROUP_PT_MOBILE` | Programmierung mobiler Decoder |
| **5** | `RN_GROUP_PT_STATIONARY` | Programmierung stationärer Decoder (Kanäle, Servo-Werte, Node-ID) |
| **6** | `RN_GROUP_GP_DATA` | Allgemeine Datentelegramme |
| **7** | `RN_GROUP_CLOCK` | Modellzeit-Synchronisation (Modelluhr) |
| **8** | `RN_GROUP_SENSOR` | Rückmelder, Gleiskontakte, Besetztmelder |
| **9** | `RN_GROUP_OUTPUT` | Standard-Ausgänge, Lichtschaltungen und Signal-Aspekte |
| **10** | `RN_GROUP_INPUT` | *(Nicht verwendet, stattdessen Sensor-Gruppe nutzen)* |
| **11** | `RN_GROUP_SOUND` | Audio- & Soundmodule |
| **12** | `RN_GROUP_DISPLAY` | Textanzeigen und LCD-Displays |

---

## 4. Detaillierte Aktionsdefinitionen

### 4.1 Gruppe 0: Host (`RN_GROUP_HOST`)

| Code | Aktion | Typ | Datenbytes | Beschreibung |
|:---:|:---|:---:|:---|:---|
| **0** | `RN_HOST_NOP` | Request | - | Keine Operation |
| **1** | `RN_HOST_SHUTDOWN` | Request | `data[0]`: 1 = alle Knoten herunterfahren | Systemweites Herunterfahren |
| **2** | `RN_HOST_PING` | Request | - | Abfrage der Erreichbarkeit durch die Zentrale |
| **3** | `RN_HOST_PONG` | Reply | `data[0..1]`: Status, `data[2..5]`: Uptime in Sek. | Antwort des Knotens auf Ping |

---

### 4.2 Gruppe 3: Stationäre Decoder (`RN_GROUP_STATIONARY`)

| Code | Aktion | Typ | Datenbytes (Request) | Datenbytes (Reply/Event) | Beschreibung |
|:---:|:---|:---:|:---|:---|:---|
| **0** | `RN_STATIONARY_NOP` | Req | - | - | Keine Operation |
| **1** | `RN_STATIONARY_PORT` | Req | `state, type, delay, port` | - | Einzelport schalten (0=aus, 1=ein) |
| **2** | `RN_STATIONARY_PORT_PAIR` | Req | `state, type, delay, port` | - | Weichenpaar schalten (0=Gerade, 1=Abzweig) |
| **4** | `RN_STATIONARY_QUERY_SINGLE_PORT` | Req | `port` | `addrH, addrL, status` | Abfrage Einzelport-Zustand |
| **5** | `RN_STATIONARY_QUERY_PORT_PAIR` | Req | `port` | `addrH, addrL, status` | Abfrage Weichenpaar-Zustand |
| **7** | `RN_STATIONARY_REPORT` | Event/Reply | - | `addrH, addrL, status1, status2` | Statusbericht / Weichenendlagenmeldung |
| **8** | `RN_STATIONARY_IDENTIFY` | Event/Reply | - | `class, manuId, verH, verL, ioCount, subipH, subipL` | Knotenanmeldung bei Rocrail |
| **11** | `RN_STATIONARY_SHOW` | Req | - | - | Identify-Befehl: Lässt Status-LED am Knoten blinken |

---

### 4.3 Gruppe 5: Programmierung stationärer Decoder (`RN_GROUP_PT_STATIONARY`)

| Code | Aktion | Typ | Datenbytes | Beschreibung |
|:---:|:---|:---:|:---|:---|
| **6** | `RN_PROGRAMMING_WRNID` | Req | `idH, idL, subipH, subipL` | Schreibt neue Node-ID in den Decoder |
| **7** | `RN_PROGRAMMING_RDOPT` | Req / Reply | Reply: 14 Bytes, Byte 17 = I2C Scan-Maske | Hardware-Optionen und I2C-Busbericht |
| **11** | `RN_PROGRAMMING_UPDATE` | Req | - | Remote Software-Neustart (Reboot) |
| **15** | `RN_PROGRAMMING_RCHANNEL` | Req | `fromChannel, toChannel` | Liest Servo-Kanalkonfiguration |
| **16** | `RN_PROGRAMMING_WCHANNEL` | Req | `channel, offH, offL, onH, onL, offSteps, onSteps, options` | Schreibt Servogrenzen und Stellzeit |
| **18** | `RN_PROGRAMMING_SETCHANNEL` | Req | `channel, valH, valL` | Direkte Positionierung (Winkel/Impuls) |

---

### 4.4 Gruppe 7: Modellzeit (`RN_GROUP_CLOCK`)

| Code | Aktion | Typ | Datenbytes | Beschreibung |
|:---:|:---|:---:|:---|:---|
| **1** | `RN_CLOCK_SET` | Req | `yearH, yearL, month, day, hour, min, temp, divider, bright` | Setzt die Modellzeit |
| **2** | `RN_CLOCK_SYNC` | Req | `yearH, yearL, month, day, hour, min, temp, divider, bright` | Synchronisiert periodisch die Modellzeit |

---

### 4.5 Gruppe 8: Sensoren & Rückmelder (`RN_GROUP_SENSOR`)

| Code | Aktion | Typ | Datenbytes | Beschreibung |
|:---:|:---|:---:|:---|:---|
| **1** | `RN_SENSOR_REPORT` | Event | `addrH, addrL, status, port, [id...]` | Rückmeldung: status 1 = belegt, 0 = frei |
| **2** | `RN_SENSORID_REPORT`| Event | `addrH, addrL, status, port, idBytes...` | Rückmeldung mit RFID- / Lok-Identifikation |
| **3** | `RN_SENSORID_ACK` | Reply | `port, rc` | Quittung der Zentrale |

---

### 4.6 Gruppe 9: Ausgänge & Signal-Aspekte (`RN_GROUP_OUTPUT`)

| Code | Aktion | Typ | Datenbytes | Beschreibung |
|:---:|:---|:---:|:---|:---|
| **0** | `RN_OUTPUT_OFF` | Req | `state, type, delay, port` | Ausgang ausschalten |
| **1** | `RN_OUTPUT_ON` | Req | `state, type, delay, port` | Ausgang einschalten |
| **1 (Macro)** | Aspect-Befehl | Req | `6, 0, aspectMask, port` | Signalbegriff (Aspect) mit Bitmaske schalten |
| **14** | Native Aspects | Req | `data... (Aspect Index & Pattern)` | Rocrail Native Extended Aspect Numbers |

---

## 5. Referenzdatei

Die originalen C-Präprozessordefinitionen sind in [`lib/rocnet-const.h`](../lib/rocnet-const.h) hinterlegt.
