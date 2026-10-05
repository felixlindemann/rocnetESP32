# RocNet Protocol Specification / RocNet Protokollspezifikation

> **Quellen- und Urheberrechtsangabe / Source Attribution & Copyright:**  
> - **Quelle / Source:** [Rocrail Wiki - RocNet Protocol](https://www.rocrail-wiki.net/doku.php?id=rocnet:rocnet-prot-en)  
> - **Titel / Title:** `rocnet:rocnet-prot-en [Innovative Model Railroad Control System]`  
> - **Urheber / Author:** Copyright &copy; 2002–2026 Robert Jan Versluis, [rocrail.net](https://www.rocrail.net). Alle Rechte vorbehalten / All rights reserved.  
> - **Zweck / Purpose:** Technische Spezifikation zur Gewährleistung der Protokoll-Interoperabilität gemäß EU-Richtlinie 2009/24/EG Art. 6 / § 69e UrhG.  
>   *Technical specification to ensure protocol interoperability according to EU Directive 2009/24/EC Art. 6 and § 69e German Copyright Act (UrhG).*

---

## 1. Netzwerk-Topologie & Transport / Network Topology & Transport

- **Protokoll / Protocol:** UDP Multicast über Ethernet / WLAN (802.11 b/g/n)
- **Multicast IP:** `224.0.0.1`
- **Multicast Port:** `4321` (Standard-Port für alle RocNet-Knoten und Zentrale / Default port for all nodes & server)
- **Byte-Reihenfolge / Endianness:** Big-Endian (Most Significant Byte first)

---

## 2. Paketstruktur / Packet Layout

Jedes standardkonforme RocNet-Paket besteht aus einem **8-Byte Header**, gefolgt von bis zu **255 Datenbytes**:  
*Every standard RocNet packet consists of an **8-byte header**, followed by up to **255 data bytes**:*

| Byte | Feld / Field | Typ / Type | Beschreibung (DE) | Description (EN) |
|:---:|:---|:---|:---|:---|
| **0** | `netid` | `uint8` | Netzwerk-Kennung (Standard `1`, Bit 7 kann gesetzt sein `0x81` zur Synchronisation) | Network ID (default `1`, bit 7 may be set `0x81` for packet sync) |
| **1** | `rcptH` | `uint8` | Empfänger-Node-ID High-Byte | Recipient Node ID High Byte |
| **2** | `rcptL` | `uint8` | Empfänger-Node-ID Low-Byte (`0` = Broadcast an alle) | Recipient Node ID Low Byte (`0` = Broadcast to all nodes) |
| **3** | `sndrH` | `uint8` | Absender-Node-ID High-Byte | Sender Node ID High Byte |
| **4** | `sndrL` | `uint8` | Absender-Node-ID Low-Byte | Sender Node ID Low Byte |
| **5** | `group` | `uint8` | Funktionsgruppe (siehe Gruppenübersicht) | Function group (see group overview) |
| **6** | `action` | `uint8` | Bit 7..5: Aktionstyp, Bit 4..0: Aktionscode | Bit 7..5: Action type, Bit 4..0: Action code |
| **7** | `len` | `uint8` | Anzahl der Datenbytes (0..255) | Number of data bytes (0..255) |
| **8..n** | `data[...]` | `uint8[]` | Nutzdaten (Länge entspricht `len`) | Payload data (length matches `len`) |

### Aufbau des `action`-Bytes / Structure of the `action` Byte (Byte 6)

$$\text{action} = (\text{type} \ll 5) \mid (\text{code} \ \& \ 0\text{x}1\text{F})$$

- **Aktionstypen (Bits 7..5) / Action Types (Bits 7..5):**
  - `0` (`0x00`): **Request** – Befehl / Anforderung an den Knoten (*Command / Request to node*).
  - `1` (`0x20`): **Event** – Ereignis / Statusmeldung (*Event / Autonomous status change*).
  - `2` (`0x40`): **Reply** – Antwort auf eine Abfrage (*Reply to a previous request*).
- **Aktionscode (Bits 4..0) / Action Code (Bits 4..0):**
  - Befehls-ID innerhalb der jeweiligen Gruppe (Werte `0`–`31`) / *Command identifier within group (`0`–`31`)*.

---

## 3. Übersicht der Gruppen / Functional Groups Overview (`group`, Byte 5)

| Gruppe / Group | Konstante in `rocnet-const.h` | Beschreibung (DE) | Description (EN) |
|:---:|:---|:---|:---|
| **0** | `RN_GROUP_HOST` | Host- und Zentralkommandos (Ping, Pong, Shutdown) | Host & server commands (Ping, Pong, Shutdown) |
| **1** | `RN_GROUP_CS` | Zentrale-Befehle | Command station commands |
| **2** | `RN_GROUP_MOBILE` | Lokomotiv- & Fahrzeugsteuerung | Mobile decoders (locomotives, speed, functions) |
| **3** | `RN_GROUP_STATIONARY` | Stationäre Decoder (Weichen, Servos, Relais) | Stationary decoders (turnouts, servos, relays) |
| **4** | `RN_GROUP_PT_MOBILE` | Programmierung mobiler Decoder | Mobile decoder programming |
| **5** | `RN_GROUP_PT_STATIONARY` | Programmierung stationärer Decoder | Stationary programming (ports, channels, node ID) |
| **6** | `RN_GROUP_GP_DATA` | Allgemeine Datentelegramme | General purpose data frames |
| **7** | `RN_GROUP_CLOCK` | Modellzeit-Synchronisation (Modelluhr) | Fast clock model time synchronization |
| **8** | `RN_GROUP_SENSOR` | Rückmelder, Gleiskontakte, Besetztmelder | Sensors, feedback contacts, block occupancy |
| **9** | `RN_GROUP_OUTPUT` | Standard-Ausgänge, Licht & Signal-Aspekte | Outputs, lighting & signal aspect masks |
| **10** | `RN_GROUP_INPUT` | *(Nicht verwendet, stattdessen Gruppe 8 nutzen)* | *(Unused, use Group 8 instead)* |
| **11** | `RN_GROUP_SOUND` | Audio- & Soundmodule | Sound & audio player control |
| **12** | `RN_GROUP_DISPLAY` | Textanzeigen und LCD-Displays | Text displays & LCD controllers |

---

## 4. Detaillierte Aktionsdefinitionen / Detailed Action Definitions

### 4.1 Gruppe 0: Host / Server Commands (`RN_GROUP_HOST`)

| Code | Aktion / Action | Typ / Type | Datenbytes / Data | Beschreibung (DE) / Description (EN) |
|:---:|:---|:---:|:---|:---|
| **0** | `RN_HOST_NOP` | Request | - | Keine Operation / No operation |
| **1** | `RN_HOST_SHUTDOWN` | Request | `data[0]`: 1 | Systemweites Herunterfahren / System-wide shutdown |
| **2** | `RN_HOST_PING` | Request | - | Erreichbarkeitsprüfung / Host presence ping |
| **3** | `RN_HOST_PONG` | Reply | `statusH, statusL, uptime[4]` | Antwort auf Ping mit Uptime / Reply with uptime & status |

---

### 4.2 Gruppe 3: Stationäre Decoder / Stationary Decoders (`RN_GROUP_STATIONARY`)

| Code | Aktion / Action | Typ / Type | Request Data | Reply / Event Data | Beschreibung (DE) / Description (EN) |
|:---:|:---|:---:|:---|:---|:---|
| **0** | `RN_STATIONARY_NOP` | Req | - | - | Keine Operation / No operation |
| **1** | `RN_STATIONARY_PORT` | Req | `state, type, delay, port` | - | Einzelport schalten (0=aus, 1=ein) / Switch single port |
| **2** | `RN_STATIONARY_PORT_PAIR` | Req | `state, type, delay, port` | - | Weichenpaar schalten (0=Gerade, 1=Abzweig) / Switch turnout |
| **4** | `RN_STATIONARY_QUERY_SINGLE_PORT` | Req | `port` | `addrH, addrL, status` | Abfrage Einzelport / Query single port status |
| **5** | `RN_STATIONARY_QUERY_PORT_PAIR` | Req | `port` | `addrH, addrL, status` | Abfrage Weichenpaar / Query turnout status |
| **7** | `RN_STATIONARY_REPORT` | Event/Reply | - | `addrH, addrL, status1, status2` | Weichenendlage / End-position report |
| **8** | `RN_STATIONARY_IDENTIFY` | Event/Reply | - | `class, manuId, verH, verL, ioCount, subipH, subipL` | Node-Anmeldung bei Rocrail / Node registration |
| **11** | `RN_STATIONARY_SHOW` | Req | - | - | Node-Identifikation (LED blinken) / Identify show command |

---

### 4.3 Gruppe 5: Programmierung stationärer Decoder (`RN_GROUP_PT_STATIONARY`)

| Code | Aktion / Action | Typ / Type | Datenbytes / Data | Beschreibung (DE) / Description (EN) |
|:---:|:---|:---:|:---|:---|
| **6** | `RN_PROGRAMMING_WRNID` | Req | `idH, idL, subipH, subipL` | Neue Node-ID zuweisen / Set and write new Node ID |
| **7** | `RN_PROGRAMMING_RDOPT` | Req / Reply | Reply: 14 Bytes, Byte 17 = I2C-Maske | Hardware-Optionen & I2C-Scan / Read options & I2C scan |
| **11** | `RN_PROGRAMMING_UPDATE` | Req | - | Remote Software-Neustart / Remote software reboot |
| **15** | `RN_PROGRAMMING_RCHANNEL` | Req | `fromChannel, toChannel` | Servo-Kanalkonfiguration lesen / Read channel configuration |
| **16** | `RN_PROGRAMMING_WCHANNEL` | Req | `channel, offH, offL, onH, onL, offSteps, onSteps, options` | Endlagen & Laufzeit schreiben / Write channel limits |
| **18** | `RN_PROGRAMMING_SETCHANNEL` | Req | `channel, valH, valL` | Direkte Servopositionierung / Direct servo angle / pulse |

---

### 4.4 Gruppe 7: Modellzeit / Fast Clock (`RN_GROUP_CLOCK`)

| Code | Aktion / Action | Typ / Type | Datenbytes / Data | Beschreibung (DE) / Description (EN) |
|:---:|:---|:---:|:---|:---|
| **1** | `RN_CLOCK_SET` | Req | `yearH, yearL, month, day, hour, min, temp, divider, bright` | Modellzeit setzen / Set model clock time |
| **2** | `RN_CLOCK_SYNC` | Req | `yearH, yearL, month, day, hour, min, temp, divider, bright` | Modellzeit synchronisieren / Sync model clock time |

---

### 4.5 Gruppe 8: Sensoren & Rückmelder / Sensors & Feedback (`RN_GROUP_SENSOR`)

| Code | Aktion / Action | Typ / Type | Datenbytes / Data | Beschreibung (DE) / Description (EN) |
|:---:|:---|:---:|:---|:---|
| **1** | `RN_SENSOR_REPORT` | Event | `addrH, addrL, status, port, [id...]` | Rückmeldung (1=belegt, 0=frei) / Feedback state report |
| **2** | `RN_SENSORID_REPORT`| Event | `addrH, addrL, status, port, idBytes...` | Rückmeldung mit RFID / Lok-ID / Sensor ID report |
| **3** | `RN_SENSORID_ACK` | Reply | `port, rc` | Quittung der Zentrale / Server acknowledge |

---

### 4.6 Gruppe 9: Ausgänge & Signal-Aspekte / Outputs & Signals (`RN_GROUP_OUTPUT`)

| Code | Aktion / Action | Typ / Type | Datenbytes / Data | Beschreibung (DE) / Description (EN) |
|:---:|:---|:---:|:---|:---|
| **0** | `RN_OUTPUT_OFF` | Req | `state, type, delay, port` | Ausgang ausschalten / Turn output off |
| **1** | `RN_OUTPUT_ON` | Req | `state, type, delay, port` | Ausgang einschalten / Turn output on |
| **1 (Macro)** | Aspect-Befehl | Req | `6, 0, aspectMask, port` | Signalbegriff (Aspect) schalten / Set signal aspect mask |
| **14** | Native Aspects | Req | `data... (Aspect Index & Pattern)` | Rocrail Native Extended Aspect Numbers |

---

## 5. Referenzdatei / Reference Header File

Die originalen C-Präprozessordefinitionen sind in [`lib/rocnet-const.h`](../lib/rocnet-const.h) hinterlegt.  
*The original C preprocessor definitions are preserved in [`lib/rocnet-const.h`](../lib/rocnet-const.h).*
