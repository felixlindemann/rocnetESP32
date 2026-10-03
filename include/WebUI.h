#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS / SOURCE ATTRIBUTION:
 * ==============================================================================
 * WebUI: Eingebettetes HTML5/CSS3/JavaScript-Frontend für die ESP32-Weboberfläche.
 * Dient der Weichenkalibrierung, Signalüberwachung und Systemkonfiguration.
 * ==============================================================================
 */

#include <pgmspace.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>RocNet ESP32 Servo Finetuning</title>
<style>
:root {
  --bg: #0f172a;
  --surface: #1e293b;
  --surface-hover: #334155;
  --border: #334155;
  --primary: #3b82f6;
  --primary-hover: #2563eb;
  --success: #10b981;
  --success-hover: #059669;
  --text: #f8fafc;
  --text-muted: #94a3b8;
  --accent: #f59e0b;
  --radius: 10px;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  background-color: var(--bg);
  color: var(--text);
  line-height: 1.5;
  padding: 16px;
}
.container { max-width: 1720px; width: 98%; margin: 0 auto; }
header {
  display: flex;
  flex-wrap: wrap;
  justify-content: space-between;
  align-items: center;
  background: var(--surface);
  padding: 14px 20px;
  border-radius: var(--radius);
  border: 1px solid var(--border);
  margin-bottom: 16px;
  gap: 12px;
}
h1 { font-size: 1.35rem; font-weight: 700; color: #fff; display: flex; align-items: center; gap: 8px; }
.badge {
  background: #334155;
  color: #38bdf8;
  padding: 3px 8px;
  border-radius: 999px;
  font-size: 0.75rem;
  font-weight: 600;
}
.badge-online { background: rgba(16,185,129,0.2); color: #34d399; border: 1px solid #10b981; }
.badge-offline { background: rgba(239,68,68,0.2); color: #f87171; border: 1px solid #ef4444; }
.hardware-bar {
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  padding: 12px 20px;
  margin-bottom: 16px;
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: space-between;
  gap: 14px;
}
.mod-select-group { display: flex; align-items: center; gap: 10px; flex-wrap: wrap; }
label { font-size: 0.85rem; color: var(--text-muted); font-weight: 600; }
select, input[type="number"] {
  background: #0f172a;
  color: var(--text);
  border: 1px solid var(--border);
  padding: 5px 10px;
  border-radius: 6px;
  font-size: 0.9rem;
}
.btn {
  background: var(--primary);
  color: #fff;
  border: none;
  padding: 7px 14px;
  border-radius: 6px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.15s ease;
  display: inline-flex;
  align-items: center;
  gap: 6px;
}
.btn:hover { background: var(--primary-hover); transform: translateY(-1px); }
.btn-success { background: var(--success); }
.btn-success:hover { background: var(--success-hover); }
.btn-outline { background: transparent; border: 1px solid var(--border); color: var(--text); }
.btn-outline:hover { background: var(--surface-hover); }
.btn-sm { padding: 4px 8px; font-size: 0.8rem; }
.module-status-tags { display: flex; gap: 8px; flex-wrap: wrap; }
.tabs { display: flex; gap: 8px; margin-bottom: 14px; flex-wrap: wrap; }
.tab-btn {
  background: var(--surface);
  color: var(--text-muted);
  border: 1px solid var(--border);
  padding: 6px 14px;
  border-radius: 8px;
  cursor: pointer;
  font-weight: 600;
  font-size: 0.85rem;
}
.tab-btn.active {
  background: var(--primary);
  color: #fff;
  border-color: var(--primary);
}
.servo-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 14px;
}
@media (max-width: 1400px) {
  .servo-grid {
    grid-template-columns: repeat(3, 1fr);
    gap: 12px;
  }
}
@media (max-width: 1050px) {
  .servo-grid {
    grid-template-columns: repeat(2, 1fr);
    gap: 10px;
  }
}
@media (max-width: 640px) {
  .servo-grid {
    grid-template-columns: 1fr;
    gap: 10px;
  }
}
.servo-card {
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  padding: 12px 14px;
  display: flex;
  flex-direction: column;
  gap: 10px;
  transition: border-color 0.2s ease, transform 0.15s ease;
}
.servo-card:hover { border-color: #475569; }
.servo-card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  border-bottom: 1px solid var(--border);
  padding-bottom: 6px;
}
.servo-title { font-weight: 700; font-size: 1.05rem; color: #fff; }
.servo-subtitle { font-size: 0.75rem; color: var(--text-muted); }
.angle-row {
  display: flex;
  flex-direction: column;
  gap: 2px;
}
.angle-header {
  display: flex;
  justify-content: space-between;
  font-size: 0.8rem;
}
.angle-label { font-weight: 600; color: #cbd5e1; }
.slider-group {
  display: flex;
  align-items: center;
  gap: 8px;
}
input[type="range"] {
  flex: 1;
  accent-color: var(--primary);
  height: 6px;
  background: #0f172a;
  border-radius: 3px;
  cursor: pointer;
}
.num-box {
  width: 52px;
  text-align: center;
  padding: 4px 6px;
  font-size: 0.85rem;
}
.toggle-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-size: 0.8rem;
  color: var(--text-muted);
  flex-wrap: wrap;
  gap: 6px;
}
.toggle-label { display: flex; align-items: center; gap: 6px; cursor: pointer; user-select: none; }
input[type="checkbox"] { width: 15px; height: 15px; accent-color: var(--primary); cursor: pointer; }
.card-actions {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-top: 4px;
  gap: 6px;
  flex-wrap: wrap;
}
.test-btn-group { display: flex; gap: 4px; }
.toast {
  position: fixed;
  bottom: 24px;
  right: 24px;
  background: #1e293b;
  border: 1px solid #3b82f6;
  color: #fff;
  padding: 12px 20px;
  border-radius: 8px;
  box-shadow: 0 10px 25px rgba(0,0,0,0.5);
  font-weight: 600;
  display: none;
  z-index: 1000;
}
.modal-overlay {
  position: fixed;
  inset: 0;
  background: rgba(15, 23, 42, 0.85);
  backdrop-filter: blur(5px);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 2000;
  padding: 16px;
}
.modal-overlay.active { display: flex; }
.modal-content {
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  max-width: 760px;
  width: 100%;
  max-height: 90vh;
  overflow-y: auto;
  box-shadow: 0 25px 50px -12px rgba(0,0,0,0.7);
  display: flex;
  flex-direction: column;
}
.modal-header {
  padding: 16px 20px;
  border-bottom: 1px solid var(--border);
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.modal-title { font-size: 1.2rem; font-weight: 700; color: #fff; display: flex; align-items: center; gap: 8px; }
.modal-close {
  background: transparent;
  border: none;
  color: var(--text-muted);
  font-size: 1.5rem;
  cursor: pointer;
  line-height: 1;
}
.modal-close:hover { color: #fff; }
.modal-body {
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 16px;
}
.modal-section {
  background: #0f172a;
  border: 1px solid var(--border);
  border-radius: 8px;
  padding: 14px 16px;
  display: flex;
  flex-direction: column;
  gap: 12px;
}
.modal-section-title {
  font-size: 0.95rem;
  font-weight: 700;
  color: #38bdf8;
  display: flex;
  align-items: center;
  gap: 6px;
}
.form-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 12px;
}
@media (max-width: 600px) {
  .form-grid { grid-template-columns: 1fr; }
}
.form-group {
  display: flex;
  flex-direction: column;
  gap: 4px;
}
.form-group label {
  font-size: 0.8rem;
  color: var(--text-muted);
  font-weight: 600;
}
.form-group input, .form-group select {
  background: #1e293b;
  color: var(--text);
  border: 1px solid var(--border);
  padding: 7px 10px;
  border-radius: 6px;
  font-size: 0.9rem;
}
.form-group input:focus, .form-group select:focus {
  border-color: var(--primary);
  outline: none;
}
.modal-footer {
  padding: 14px 20px;
  border-top: 1px solid var(--border);
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 8px;
}
.modal-footer-right {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}
.btn-warning { background: #f59e0b; color: #000; }
.btn-warning:hover { background: #d97706; }
.reboot-overlay {
  position: fixed;
  inset: 0;
  background: rgba(15, 23, 42, 0.96);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 3000;
  text-align: center;
  padding: 20px;
}
.reboot-overlay.active { display: flex; }
.reboot-box {
  background: var(--surface);
  border: 1px solid var(--primary);
  border-radius: var(--radius);
  padding: 30px;
  max-width: 440px;
  box-shadow: 0 20px 40px rgba(0,0,0,0.8);
}
.reboot-spinner {
  width: 48px;
  height: 48px;
  border: 4px solid var(--border);
  border-top: 4px solid var(--primary);
  border-radius: 50%;
  animation: spin 1s linear infinite;
  margin: 0 auto 16px auto;
}
@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }

.log-panel {
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  margin-top: 18px;
  margin-bottom: 24px;
  overflow: hidden;
  box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.2);
}
.log-header {
  padding: 10px 16px;
  border-bottom: 1px solid var(--border);
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 10px;
  background: rgba(15, 23, 42, 0.6);
}
.log-title {
  font-size: 0.95rem;
  font-weight: 700;
  color: #fff;
  display: flex;
  align-items: center;
  gap: 8px;
}
.log-controls {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}
.log-console {
  background: #090d16;
  padding: 12px 14px;
  height: 260px;
  overflow-y: auto;
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace;
  font-size: 0.8rem;
  line-height: 1.45;
  color: #cbd5e1;
}
.log-line {
  display: flex;
  align-items: baseline;
  gap: 8px;
  padding: 3px 0;
  border-bottom: 1px solid rgba(255, 255, 255, 0.03);
  word-break: break-word;
}
.log-ts {
  color: var(--text-muted);
  font-size: 0.72rem;
  min-width: 65px;
  user-select: none;
}
.log-tag {
  font-size: 0.68rem;
  font-weight: 700;
  padding: 1px 6px;
  border-radius: 4px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  user-select: none;
}
.tag-error {
  background: rgba(239, 68, 68, 0.2);
  color: #f87171;
  border: 1px solid #ef4444;
}
.tag-debug {
  background: rgba(56, 189, 248, 0.15);
  color: #38bdf8;
  border: 1px solid #0284c7;
}
.tag-info {
  background: rgba(148, 163, 184, 0.15);
  color: #94a3b8;
  border: 1px solid #475569;
}
.log-msg {
  flex: 1;
  white-space: pre-wrap;
}
</style>
</head>
<body>
<div class="container">
  <header>
    <div>
      <h1 id="headerTitle">RocNet ESP32 Servo Finetuning</h1>
      <span class="servo-subtitle" id="headerSubtitle">Offline Controller f&uuml;r Modellbahn-Weichensanftlauf &bull; Node 204</span>
    </div>
    <div style="display:flex; gap:10px; align-items:center; flex-wrap:wrap;">
      <span id="ipBadge" class="badge">10.10.78.68</span>
      <button class="btn btn-outline" style="border-color:#38bdf8; color:#38bdf8;" onclick="triggerIdentify()">📡 In Rocrail melden</button>
      <button class="btn btn-outline" onclick="openSettingsModal()">⚙️ Einstellungen</button>
      <button class="btn btn-success" onclick="saveAll()">Alle Servos speichern</button>
    </div>
  </header>

  <div class="hardware-bar">
    <div class="mod-select-group">
      <label for="moduleSelect">PCA9685 Module (je 16 Servos):</label>
      <select id="moduleSelect" onchange="changeModuleCount()">
        <option value="1">1 Modul (16 Servos: #1..16)</option>
        <option value="2">2 Module (32 Servos: #1..32)</option>
        <option value="3">3 Module (48 Servos: #1..48)</option>
        <option value="4">4 Module (64 Servos: #1..64)</option>
      </select>
      <button class="btn btn-outline btn-sm" onclick="fetchStatus()">Neu laden</button>
    </div>
    <div style="display:flex; gap:8px; align-items:center; flex-wrap:wrap;">
      <span class="badge" id="statsTurnoutBadge" style="background:rgba(59,130,246,0.15); color:#60a5fa; border:1px solid #3b82f6;">
        🔀 Weichen: 0 aktiv &bull; 🔄 0 Schaltungen
      </span>
      <span class="badge" id="statsWsBadge" style="background:rgba(16,185,129,0.15); color:#34d399; border:1px solid #10b981;">
        💡 16 WS2811 (48 LEDs)
      </span>
      <div class="module-status-tags" id="moduleBadges"></div>
    </div>
  </div>

  <div class="tabs" id="tabButtons"></div>

  <div class="servo-grid" id="servoGrid"></div>

  <div class="log-panel">
    <div class="log-header">
      <div class="log-title">
        <span>📋 Live-Systemprotokoll</span>
        <span class="badge" id="logCountBadge" style="background:#0f172a; color:#94a3b8; border:1px solid #334155;">0 Einträge</span>
      </div>
      <div class="log-controls">
        <label for="liveLogLevelSelect" style="font-size:0.8rem;">Logging-Level:</label>
        <select id="liveLogLevelSelect" onchange="setLiveLogLevel(this.value)">
          <option value="1">1: Fehler (Default)</option>
          <option value="2">2: Debug (Events)</option>
          <option value="3">3: Info (Verbose)</option>
        </select>
        <label class="toggle-label" style="font-size:0.8rem;">
          <input type="checkbox" id="logAutoscroll" checked>
          <span>Auto-Scroll</span>
        </label>
        <label class="toggle-label" style="font-size:0.8rem;">
          <input type="checkbox" id="logPause">
          <span>Pause</span>
        </label>
        <button class="btn btn-outline btn-sm" onclick="clearLiveLogs()">🗑️ Leeren</button>
      </div>
    </div>
    <div class="log-console" id="logConsole">
      <div style="color:var(--text-muted); font-style:italic; padding:6px 0;">Warte auf Systemprotokoll-Einträge...</div>
    </div>
  </div>
</div>

<div id="settingsModal" class="modal-overlay">
  <div class="modal-content">
    <div class="modal-header">
      <div class="modal-title">⚙️ Controller & Hardware Einstellungen</div>
      <button class="modal-close" onclick="closeSettingsModal()">&times;</button>
    </div>
    <div class="modal-body">
      <div class="modal-section">
        <div class="modal-section-title">🚂 RocNet Protokoll & Node</div>
        <div class="form-grid">
          <div class="form-group">
            <label for="sys_nodeId">RocNet Node ID (1..65535):</label>
            <input type="number" id="sys_nodeId" min="1" max="65535" required>
          </div>
          <div class="form-group">
            <label for="sys_nodeName">Node Name (UID-Name in Rocrail):</label>
            <input type="text" id="sys_nodeName" maxlength="31" required>
          </div>
          <div class="form-group" style="grid-column: span 2;">
            <label for="sys_pairedPorts">Weichen-Adressierungsmodus:</label>
            <select id="sys_pairedPorts">
              <option value="1">Paarweise Ports (Port 1=Gerade, Port 2=Abzweig) [Rocrail Standard]</option>
              <option value="0">1:1 Einzel-Port (Port 1=Servo 1, Port 2=Servo 2, Zustand=0/1)</option>
            </select>
          </div>
          <div class="form-group" style="grid-column: span 2;">
            <label for="sys_logLevel">Logging-Level (Fehler / Debug / Info):</label>
            <select id="sys_logLevel">
              <option value="1">1: Fehler (Standard - Nur Hardwarefehler &amp; Ausfälle)</option>
              <option value="2">2: Debug (Wesentliche Events: Weichen, Signale, Identify, Ping)</option>
              <option value="3">3: Info (Alle Details: Raw Hex-Pakete, Heartbeats, Web-Traffic)</option>
            </select>
          </div>
        </div>
      </div>

      <div class="modal-section">
        <div class="modal-section-title">🔌 Hardware Pins (GPIOs) & Signale</div>
        <div class="form-grid">
          <div class="form-group">
            <label for="sys_sdaPin">I2C SDA Pin (PCA9685):</label>
            <input type="number" id="sys_sdaPin" min="0" max="39" required>
          </div>
          <div class="form-group">
            <label for="sys_sclPin">I2C SCL Pin (PCA9685):</label>
            <input type="number" id="sys_sclPin" min="0" max="39" required>
          </div>
          <div class="form-group">
            <label for="sys_ws2811Pin">WS2811 Data Pin (Signale):</label>
            <input type="number" id="sys_ws2811Pin" min="0" max="39" required>
          </div>
          <div class="form-group">
            <label for="sys_ws2811Chips">Anzahl WS2811 (Default: 16):</label>
            <input type="number" id="sys_ws2811Chips" min="1" max="128" placeholder="16" required>
          </div>
          <div class="form-group" style="grid-column: span 2;">
            <label for="sys_signalBrightness">Signal Helligkeit (0..255):</label>
            <input type="range" id="sys_signalBrightness" min="10" max="255" style="width:100%;">
          </div>
        </div>
      </div>

      <div class="modal-section">
        <div class="modal-section-title">📶 WLAN & Netzwerk</div>
        <div class="form-grid">
          <div class="form-group">
            <label for="sys_wifiSsid">WLAN SSID:</label>
            <input type="text" id="sys_wifiSsid" maxlength="32" required>
          </div>
          <div class="form-group">
            <label for="sys_wifiPass">WLAN Passwort (leer = unverändert):</label>
            <input type="password" id="sys_wifiPass" maxlength="64" placeholder="********">
          </div>
          <div class="form-group" style="grid-column: span 2;">
            <label for="sys_mdnsName">mDNS Hostname (.local):</label>
            <input type="text" id="sys_mdnsName" maxlength="31" required>
          </div>
        </div>
      </div>

      <div class="modal-section" style="background:#090d16; font-size:0.85rem; color:var(--text-muted);">
        <div style="display:flex; justify-content:space-between; flex-wrap:wrap; gap:8px;">
          <span>IP: <strong id="info_ip" style="color:#fff;">-</strong></span>
          <span>MAC: <strong id="info_mac" style="color:#fff;">-</strong></span>
          <span>RAM: <strong id="info_heap" style="color:#fff;">-</strong></span>
          <span>Uptime: <strong id="info_uptime" style="color:#fff;">-</strong></span>
        </div>
      </div>
    </div>
    <div class="modal-footer">
      <button class="btn btn-outline" onclick="closeSettingsModal()">Schließen</button>
      <div class="modal-footer-right">
        <button class="btn btn-outline btn-warning" onclick="rebootESP()">⚡ Neu starten</button>
        <button class="btn btn-primary" onclick="saveSystemSettings(false)">💾 Speichern</button>
        <button class="btn btn-success" onclick="saveSystemSettings(true)">🔄 Speichern & Neu starten</button>
      </div>
    </div>
  </div>
</div>

<div id="rebootOverlay" class="reboot-overlay">
  <div class="reboot-box">
    <div class="reboot-spinner"></div>
    <h2 style="font-size:1.3rem; margin-bottom:10px; color:#fff;">ESP32 startet neu...</h2>
    <p style="color:var(--text-muted); margin-bottom:16px;">Bitte warten. Seite wird in <span id="rebootTimer" style="color:#38bdf8; font-weight:700;">6</span> Sekunden neu geladen.</p>
  </div>
</div>

<div id="toast" class="toast"></div>

<script>
let state = { modules: 1, total: 16, online: [true,false,false,false], servos: [] };
let activeTab = 0; // 0 = All, 1 = Mod 1, etc.
let dragTimer = null;

function showToast(msg, isSuccess=true) {
  const t = document.getElementById('toast');
  t.innerText = msg;
  t.style.borderColor = isSuccess ? '#10b981' : '#3b82f6';
  t.style.display = 'block';
  setTimeout(() => { t.style.display = 'none'; }, 2200);
}

function fetchStatus() {
  fetch('/api/status')
    .then(r => r.json())
    .then(data => {
      state = data;
      document.getElementById('moduleSelect').value = state.modules;
      renderModuleBadges();
      renderTabs();
      renderServos();
    })
    .catch(e => console.error("Status load failed", e));
}

function renderModuleBadges() {
  const mb = document.getElementById('moduleBadges');
  mb.innerHTML = '';
  for(let i=0; i<state.modules; i++) {
    const addr = '0x' + (0x40 + i).toString(16).toUpperCase();
    const isOnline = state.online && state.online[i];
    const span = document.createElement('span');
    span.className = 'badge ' + (isOnline ? 'badge-online' : 'badge-offline');
    span.innerText = `Modul ${i+1} (${addr}): ${isOnline ? 'Online' : 'Offline'}`;
    mb.appendChild(span);
  }

  const tb = document.getElementById('statsTurnoutBadge');
  if (tb) {
    const act = (state.activeTurnouts !== undefined) ? state.activeTurnouts : 0;
    const totSw = (state.totalSwitches !== undefined) ? state.totalSwitches : 0;
    tb.innerHTML = `🔀 Weichen: <strong>${act}</strong> aktiv (max. ${state.total}) &bull; 🔄 <strong>${totSw}</strong> Schaltungen`;
  }

  const wb = document.getElementById('statsWsBadge');
  if (wb) {
    const chips = (state.wsChips !== undefined) ? state.wsChips : 16;
    wb.innerHTML = `💡 <strong>${chips}</strong> WS2811 (${chips * 3} LEDs)`;
  }
}

function renderTabs() {
  const tb = document.getElementById('tabButtons');
  tb.innerHTML = '';
  
  const allBtn = document.createElement('button');
  allBtn.className = 'tab-btn ' + (activeTab === 0 ? 'active' : '');
  allBtn.innerText = `Alle (${state.total} Servos)`;
  allBtn.onclick = () => { activeTab = 0; renderTabs(); renderServos(); };
  tb.appendChild(allBtn);

  for(let i=1; i<=state.modules; i++) {
    const btn = document.createElement('button');
    btn.className = 'tab-btn ' + (activeTab === i ? 'active' : '');
    const start = (i-1)*16 + 1;
    const end = i*16;
    btn.innerText = `Modul ${i} (#${start}..#${end})`;
    btn.onclick = () => { activeTab = i; renderTabs(); renderServos(); };
    tb.appendChild(btn);
  }
}

function renderServos() {
  const grid = document.getElementById('servoGrid');
  grid.innerHTML = '';

  state.servos.forEach((s) => {
    if (activeTab > 0 && s.board !== activeTab) return;

    const card = document.createElement('div');
    card.className = 'servo-card';
    card.id = `card_${s.id}`;

    const boardAddr = '0x' + (0x40 + s.board - 1).toString(16).toUpperCase();

    card.innerHTML = `
      <div class="servo-card-header">
        <div>
          <div class="servo-title">Servo #${s.id}</div>
          <div class="servo-subtitle">Modul ${s.board} (${boardAddr}), Pin ${s.pin} &bull; RocNet Port ${s.port1}/${s.port2}</div>
        </div>
        <div style="display:flex; gap:6px; align-items:center;">
          <span class="badge" id="sw_cnt_${s.id}" style="background:rgba(148,163,184,0.15); color:#cbd5e1; border:1px solid #475569;" title="Schaltspiel-Z\u00e4hler (bisherige Schaltungen)">🔄 ${s.switches || 0}</span>
          <span class="badge" id="badge_${s.id}">${s.lastPos === 1 ? 'Rechts' : 'Links'}</span>
        </div>
      </div>

      <div class="angle-row">
        <div class="angle-header">
          <span class="angle-label">Grenzwinkel Links (Gerade)</span>
          <span style="color:#94a3b8; font-size:0.8rem;">Port ${s.port1}</span>
        </div>
        <div class="slider-group">
          <input type="range" id="left_range_${s.id}" min="0" max="180" value="${s.left}" 
                 oninput="syncAngle(${s.id}, 'left', this.value, true)">
          <input type="number" id="left_num_${s.id}" min="0" max="180" value="${s.left}" class="num-box"
                 onchange="syncAngle(${s.id}, 'left', this.value, false)">
          <span style="font-size:0.9rem;">&deg;</span>
        </div>
      </div>

      <div class="angle-row">
        <div class="angle-header">
          <span class="angle-label">Grenzwinkel Rechts (Abzweig)</span>
          <span style="color:#94a3b8; font-size:0.8rem;">Port ${s.port2}</span>
        </div>
        <div class="slider-group">
          <input type="range" id="right_range_${s.id}" min="0" max="180" value="${s.right}"
                 oninput="syncAngle(${s.id}, 'right', this.value, true)">
          <input type="number" id="right_num_${s.id}" min="0" max="180" value="${s.right}" class="num-box"
                 onchange="syncAngle(${s.id}, 'right', this.value, false)">
          <span style="font-size:0.9rem;">&deg;</span>
        </div>
      </div>

      <div class="angle-row">
        <div class="angle-header">
          <span class="angle-label">Fahrzeit (Laufzeit)</span>
          <span style="color:#94a3b8; font-size:0.8rem;">${s.spd} ms</span>
        </div>
        <div class="slider-group">
          <input type="range" id="spd_range_${s.id}" min="100" max="2000" step="50" value="${s.spd}"
                 oninput="syncSpd(${s.id}, this.value)">
          <input type="number" id="spd_num_${s.id}" min="100" max="2000" step="50" value="${s.spd}" class="num-box"
                 onchange="syncSpd(${s.id}, this.value)">
          <span style="font-size:0.85rem;">ms</span>
        </div>
      </div>

      <div class="toggle-row">
        <label class="toggle-label">
          <input type="checkbox" id="inv_${s.id}" ${s.inv ? 'checked' : ''} onchange="updateInv(${s.id}, this.checked)">
          <span>Richtung umkehren (Invertieren)</span>
        </label>
        <label class="toggle-label" style="font-size:0.8rem;">
          <input type="checkbox" id="live_${s.id}" checked>
          <span>Live testen beim Schieben</span>
        </label>
      </div>

      <div class="card-actions">
        <div class="test-btn-group">
          <button class="btn btn-outline btn-sm" onclick="testServo(${s.id}, 0)">&laquo; Links</button>
          <button class="btn btn-outline btn-sm" onclick="testServo(${s.id}, 1)">Rechts &raquo;</button>
        </div>
        <button class="btn btn-sm btn-success" onclick="saveServo(${s.id})">Speichern</button>
      </div>
    `;
    grid.appendChild(card);
  });
}

function syncAngle(id, side, val, fromSlider) {
  val = parseInt(val) || 0;
  if (val < 0) val = 0;
  if (val > 180) val = 180;

  document.getElementById(`${side}_range_${id}`).value = val;
  document.getElementById(`${side}_num_${id}`).value = val;

  const s = state.servos.find(x => x.id === id);
  if (s) s[side] = val;

  const live = document.getElementById(`live_${id}`).checked;
  if (live) {
    if (dragTimer) clearTimeout(dragTimer);
    dragTimer = setTimeout(() => {
      fetch(`/api/test?id=${id}&pos=2&angle=${val}`, { method: 'POST' });
    }, 25);
  }
}

function syncSpd(id, val) {
  val = parseInt(val) || 400;
  document.getElementById(`spd_range_${id}`).value = val;
  document.getElementById(`spd_num_${id}`).value = val;
  const s = state.servos.find(x => x.id === id);
  if (s) s.spd = val;
}

function updateInv(id, val) {
  const s = state.servos.find(x => x.id === id);
  if (s) s.inv = val ? 1 : 0;
}

function testServo(id, pos) {
  fetch(`/api/test?id=${id}&pos=${pos}`, { method: 'POST' })
    .then(() => {
      const s = state.servos.find(x => x.id === id);
      if (s) {
        s.lastPos = pos;
        s.switches = (s.switches || 0) + 1;
        state.totalSwitches = (state.totalSwitches || 0) + 1;
        const swEl = document.getElementById(`sw_cnt_${id}`);
        if (swEl) swEl.innerText = `🔄 ${s.switches}`;
        const tb = document.getElementById('statsTurnoutBadge');
        if (tb) {
          const act = state.servos.filter(x => (x.switches || 0) > 0).length;
          tb.innerHTML = `🔀 Weichen: <strong>${act}</strong> aktiv (max. ${state.total}) &bull; 🔄 <strong>${state.totalSwitches}</strong> Schaltungen`;
        }
      }
      const b = document.getElementById(`badge_${id}`);
      if (b) b.innerText = pos === 1 ? 'Rechts' : 'Links';
      showToast(`Servo #${id} auf ${pos === 1 ? 'Rechts' : 'Links'} gestellt`);
    });
}

function saveServo(id) {
  const s = state.servos.find(x => x.id === id);
  if (!s) return;

  const params = new URLSearchParams({
    id: s.id,
    left: s.left,
    right: s.right,
    spd: s.spd,
    inv: s.inv,
    save: 1
  });

  fetch('/api/servo', { method: 'POST', body: params })
    .then(r => r.json())
    .then(res => {
      showToast(`Servo #${id} erfolgreich im Flash gespeichert!`);
    })
    .catch(e => showToast(`Fehler beim Speichern von Servo #${id}`, false));
}

async function saveAll() {
  showToast('Speichere alle Servos...');
  try {
    for (const s of state.servos) {
      const params = new URLSearchParams({
        id: s.id,
        left: s.left,
        right: s.right,
        spd: s.spd,
        inv: s.inv,
        save: 1
      });
      await fetch('/api/servo', { method: 'POST', body: params });
    }
    await fetch('/api/save_all', { method: 'POST' });
    showToast(`Alle ${state.total} Servos dauerhaft im Flash gespeichert!`);
  } catch (e) {
    showToast('Fehler beim Speichern aller Servos', false);
  }
}

function changeModuleCount() {
  const cnt = document.getElementById('moduleSelect').value;
  if (!confirm(`Soll die Anzahl der PCA9685 Module auf ${cnt} geändert werden (${cnt * 16} Servos)?`)) {
    document.getElementById('moduleSelect').value = state.modules;
    return;
  }
  fetch(`/api/set_modules?count=${cnt}`, { method: 'POST' })
    .then(() => {
      showToast(`Modulanzahl auf ${cnt} gesetzt (${cnt * 16} Servos)`);
      fetchStatus();
    });
}

function openSettingsModal() {
  fetch('/api/system')
    .then(r => r.json())
    .then(data => {
      document.getElementById('sys_nodeId').value = data.nodeId;
      document.getElementById('sys_nodeName').value = data.nodeName;
      document.getElementById('sys_pairedPorts').value = data.pairedPorts ? '1' : '0';
      document.getElementById('sys_logLevel').value = data.logLevel || 1;
      document.getElementById('sys_sdaPin').value = data.sdaPin;
      document.getElementById('sys_sclPin').value = data.sclPin;
      document.getElementById('sys_ws2811Pin').value = data.ws2811Pin;
      document.getElementById('sys_ws2811Chips').value = data.ws2811Chips;
      document.getElementById('sys_signalBrightness').value = data.signalBrightness;
      document.getElementById('sys_wifiSsid').value = data.wifiSsid;
      document.getElementById('sys_wifiPass').value = '';
      document.getElementById('sys_mdnsName').value = data.mdnsName;
      document.getElementById('info_ip').innerText = data.ip;
      document.getElementById('info_mac').innerText = data.mac;
      document.getElementById('info_heap').innerText = Math.round(data.freeHeap / 1024) + ' KB frei';
      const up = data.uptime;
      const h = Math.floor(up / 3600);
      const m = Math.floor((up % 3600) / 60);
      const s = up % 60;
      document.getElementById('info_uptime').innerText = `${h}h ${m}m ${s}s`;
      document.getElementById('settingsModal').classList.add('active');
    })
    .catch(() => showToast('Fehler beim Laden der Systemeinstellungen', false));
}

function closeSettingsModal() {
  document.getElementById('settingsModal').classList.remove('active');
}

function saveSystemSettings(reboot) {
  const params = new URLSearchParams({
    nodeId: document.getElementById('sys_nodeId').value,
    nodeName: document.getElementById('sys_nodeName').value,
    pairedPorts: document.getElementById('sys_pairedPorts').value,
    logLevel: document.getElementById('sys_logLevel').value,
    sdaPin: document.getElementById('sys_sdaPin').value,
    sclPin: document.getElementById('sys_sclPin').value,
    ws2811Pin: document.getElementById('sys_ws2811Pin').value,
    ws2811Chips: document.getElementById('sys_ws2811Chips').value,
    signalBrightness: document.getElementById('sys_signalBrightness').value,
    wifiSsid: document.getElementById('sys_wifiSsid').value,
    wifiPassword: document.getElementById('sys_wifiPass').value,
    mdnsName: document.getElementById('sys_mdnsName').value,
    reboot: reboot ? 1 : 0
  });

  fetch('/api/system', { method: 'POST', body: params })
    .then(r => r.json())
    .then(res => {
      if (res.reboot) {
        closeSettingsModal();
        startRebootCountdown(6);
      } else {
        showToast('Systemeinstellungen im Flash gespeichert!');
        closeSettingsModal();
        const nid = document.getElementById('sys_nodeId').value;
        const nname = document.getElementById('sys_nodeName').value;
        const sub = document.getElementById('headerSubtitle');
        if (sub) sub.innerText = `Offline Controller für Modellbahn-Weichensanftlauf • Node ${nid} ("${nname}")`;
        fetchStatus();
      }
    })
    .catch(() => showToast('Fehler beim Speichern der Systemeinstellungen', false));
}

function rebootESP() {
  if (!confirm('Soll der ESP32 jetzt neu gestartet werden?')) return;
  fetch('/api/reboot', { method: 'POST' })
    .then(() => {
      closeSettingsModal();
      startRebootCountdown(6);
    })
    .catch(() => showToast('Fehler beim Neustart', false));
}

function startRebootCountdown(sec) {
  const overlay = document.getElementById('rebootOverlay');
  const timer = document.getElementById('rebootTimer');
  overlay.classList.add('active');
  let remaining = sec;
  timer.innerText = remaining;
  const interval = setInterval(() => {
    remaining--;
    timer.innerText = remaining;
    if (remaining <= 0) {
      clearInterval(interval);
      window.location.reload();
    }
  }, 1000);
}

function triggerIdentify() {
  fetch('/api/identify', { method: 'POST' })
    .then(r => r.json())
    .then(() => showToast('Node an Rocrail gemeldet (Identify & Show gesendet)!'))
    .catch(() => showToast('Fehler beim Melden an Rocrail', false));
}

// Live Log Console handlers
let lastLogId = 0;
let logEntriesCount = 0;
let isFirstLogPoll = true;

function setLiveLogLevel(lvl) {
  const params = new URLSearchParams({ level: lvl });
  fetch('/api/logs/level', { method: 'POST', body: params })
    .then(r => r.json())
    .then(data => {
      showToast(`Log-Level auf ${data.levelName || lvl} gesetzt`);
      const sysSel = document.getElementById('sys_logLevel');
      if (sysSel) sysSel.value = lvl;
      const liveSel = document.getElementById('liveLogLevelSelect');
      if (liveSel) liveSel.value = lvl;
    })
    .catch(() => showToast('Fehler beim Setzen des Log-Levels', false));
}

function clearLiveLogs() {
  fetch('/api/logs/clear', { method: 'POST' })
    .then(r => r.json())
    .then(() => {
      const consoleEl = document.getElementById('logConsole');
      if (consoleEl) {
        consoleEl.innerHTML = '<div style="color:var(--text-muted); font-style:italic; padding:6px 0;">Log geleert.</div>';
      }
      logEntriesCount = 0;
      updateLogCountBadge();
      showToast('Log-Puffer geleert');
    })
    .catch(() => showToast('Fehler beim Leeren des Logs', false));
}

function updateLogCountBadge() {
  const b = document.getElementById('logCountBadge');
  if (b) b.innerText = `${logEntriesCount} Einträge`;
}

function escapeHtml(str) {
  if (!str) return '';
  return str
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&#039;");
}

function pollLogs() {
  const paused = document.getElementById('logPause') && document.getElementById('logPause').checked;
  if (paused) return;

  fetch(`/api/logs?since=${lastLogId}`)
    .then(r => r.json())
    .then(data => {
      if (data.level !== undefined) {
        const liveSel = document.getElementById('liveLogLevelSelect');
        if (liveSel && document.activeElement !== liveSel) {
          liveSel.value = data.level;
        }
      }

      if (data.entries && data.entries.length > 0) {
        const consoleEl = document.getElementById('logConsole');
        if (isFirstLogPoll) {
          consoleEl.innerHTML = '';
          isFirstLogPoll = false;
        }

        const autoscroll = document.getElementById('logAutoscroll') && document.getElementById('logAutoscroll').checked;

        data.entries.forEach(e => {
          if (e.id > lastLogId) lastLogId = e.id;
          logEntriesCount++;

          const div = document.createElement('div');
          div.className = 'log-line';

          const tagClass = (e.level === 'FEHLER') ? 'tag-error' : (e.level === 'DEBUG') ? 'tag-debug' : 'tag-info';
          const sec = (e.ts / 1000).toFixed(2);

          div.innerHTML = `
            <span class="log-ts">[${sec}s]</span>
            <span class="log-tag ${tagClass}">${e.level}</span>
            <span class="log-msg">${escapeHtml(e.msg)}</span>
          `;
          consoleEl.appendChild(div);
        });

        while (consoleEl.children.length > 250) {
          consoleEl.removeChild(consoleEl.firstChild);
        }

        updateLogCountBadge();

        if (autoscroll) {
          consoleEl.scrollTop = consoleEl.scrollHeight;
        }
      } else if (isFirstLogPoll) {
        isFirstLogPoll = false;
      }
    })
    .catch(() => {});
}

// Initial fetch on load
window.addEventListener('DOMContentLoaded', () => {
  fetchStatus();
  pollLogs();
});

// Periodic log polling (every 1s)
setInterval(pollLogs, 1000);

// Periodic status polling (every 4s) to smoothly update live counters and positions from Rocrail
setInterval(() => {
  const modal = document.getElementById('settingsModal');
  const isModalActive = modal && modal.classList.contains('active');
  if (!dragTimer && !isModalActive) {
    fetch('/api/status')
      .then(r => r.json())
      .then(data => {
        const modulesChanged = (data.modules !== state.modules);
        state = data;
        if (modulesChanged) {
          document.getElementById('moduleSelect').value = state.modules;
          renderModuleBadges();
          renderTabs();
          renderServos();
        } else {
          renderModuleBadges();
          if (state.servos) {
            state.servos.forEach(s => {
              const swEl = document.getElementById(`sw_cnt_${s.id}`);
              if (swEl) swEl.innerText = `🔄 ${s.switches || 0}`;
              const b = document.getElementById(`badge_${s.id}`);
              if (b) b.innerText = (s.lastPos === 1) ? 'Rechts' : 'Links';
            });
          }
        }
      })
      .catch(() => {});
  }
}, 4000);
</script>
</body>
</html>)rawliteral";
