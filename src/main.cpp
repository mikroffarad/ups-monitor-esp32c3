#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

// ─── Wi-Fi ────────────────────────────────────────────────────────────────────
const char* SSID     = "YOUR_WIFI_SSID";
const char* PASSWORD = "YOUR_WIFI_PASSWORD";

// ─── Firmware version ─────────────────────────────────────────────────────────
const char* FIRMWARE_VERSION = "1.1.0";

// ─── Pin definitions ──────────────────────────────────────────────────────────
// Signal logic is INVERTED: HIGH = indicator OFF, LOW = indicator ON
#define PIN_R    21   // AC power / charging indicator
#define PIN_100   0   // Battery 100%
#define PIN_75    1   // Battery 75%
#define PIN_50    3   // Battery 50%
#define PIN_25    4   // Battery 25%

struct PinDef {
  int         pin;
  bool        inverted;  // true = HIGH means inactive (inverted logic)
  const char* label;
};

const PinDef PINS[] = {
  { PIN_R,   true, "AC / Charging" },
  { PIN_100, true, "100%"          },
  { PIN_75,  true, "75%"           },
  { PIN_50,  true, "50%"           },
  { PIN_25,  true, "25%"           },
};
const int PIN_COUNT = sizeof(PINS) / sizeof(PINS[0]);

// Returns true if the indicator is logically active (accounts for inversion)
bool isActive(const PinDef& p) {
  return p.inverted ? !digitalRead(p.pin) : digitalRead(p.pin);
}

// Returns battery level in % based on highest active LED
int batteryPercent() {
  if (isActive(PINS[1])) return 100;
  if (isActive(PINS[2])) return 75;
  if (isActive(PINS[3])) return 50;
  if (isActive(PINS[4])) return 25;
  return 0;
}

// ─── Web Server ───────────────────────────────────────────────────────────────
WebServer server(80);

// ── Main page ─────────────────────────────────────────────────────────────────
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>UPS Monitor</title>
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: 'Segoe UI', sans-serif;
      background: #0d0d1a;
      color: #dde0ff;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 32px 16px;
    }
    h1 { font-size: 22px; color: #8888ff; margin-bottom: 6px; letter-spacing: 1px; }
    .subtitle { font-size: 12px; color: #5555aa; margin-bottom: 32px; }
    .battery-wrap { display: flex; flex-direction: column; align-items: center; margin-bottom: 32px; }
    .battery-shell {
      width: 120px; height: 220px;
      border: 3px solid #4444aa; border-radius: 12px;
      position: relative; background: #111128; overflow: hidden;
    }
    .battery-shell::before {
      content: '';
      position: absolute; top: -14px; left: 50%;
      transform: translateX(-50%);
      width: 40px; height: 14px;
      background: #4444aa; border-radius: 4px 4px 0 0;
    }
    .battery-fill { position: absolute; bottom: 0; left: 0; right: 0; transition: height .6s ease, background .6s ease; }
    .battery-label {
      position: absolute; inset: 0;
      display: flex; align-items: center; justify-content: center;
      font-size: 28px; font-weight: bold; color: #fff;
      text-shadow: 0 0 8px #000a; z-index: 1;
    }
    .battery-pct { margin-top: 12px; font-size: 14px; color: #8888cc; }
    .status-row {
      display: flex; gap: 12px; align-items: center;
      background: #161630; border: 1px solid #2222aa;
      border-radius: 10px; padding: 12px 20px;
      margin-bottom: 28px; width: 100%; max-width: 360px;
    }
    .dot { width: 12px; height: 12px; border-radius: 50%; flex-shrink: 0; }
    .dot.on  { background: #44ff88; box-shadow: 0 0 8px #44ff88; }
    .dot.off { background: #ff4444; box-shadow: 0 0 8px #ff4444; }
    .status-text { font-size: 14px; }
    .gpio-table { width: 100%; max-width: 360px; border-collapse: collapse; }
    .gpio-table th { text-align: left; font-size: 11px; color: #5555aa; padding: 0 8px 8px; text-transform: uppercase; }
    .gpio-table td { padding: 8px; font-size: 13px; border-top: 1px solid #1e1e3a; }
    .gpio-table td:last-child { text-align: right; }
    .badge { display: inline-block; padding: 2px 10px; border-radius: 20px; font-size: 12px; font-weight: bold; }
    .badge.active   { background: #1a3a1a; color: #44ff88; border: 1px solid #44ff88; }
    .badge.inactive { background: #3a1a1a; color: #ff6666; border: 1px solid #ff4444; }
    .refresh-note { margin-top: 24px; font-size: 11px; color: #33336a; }
    .ota-link {
      margin-top: 28px; display: inline-block;
      padding: 8px 20px; border: 1px solid #3333aa;
      border-radius: 8px; color: #7777cc; font-size: 13px;
      text-decoration: none; transition: background .2s;
    }
    .ota-link:hover { background: #1a1a40; color: #aaaaff; }
  </style>
</head>
<body>
  <h1>&#9889; UPS Monitor</h1>
  <p class="subtitle" id="ver-badge">VIA Energy Mini &mdash; ESP32-C3</p>

  <div class="battery-wrap">
    <div class="battery-shell">
      <div class="battery-fill" id="fill"></div>
      <div class="battery-label" id="pct-label">&mdash;</div>
    </div>
    <div class="battery-pct" id="pct-sub"></div>
  </div>

  <div class="status-row">
    <div class="dot" id="ac-dot"></div>
    <div class="status-text" id="ac-text">Fetching data&hellip;</div>
  </div>

  <table class="gpio-table">
    <thead><tr><th>GPIO</th><th>Signal</th><th>State</th></tr></thead>
    <tbody id="gpio-body"></tbody>
  </table>

  <div class="refresh-note" id="refresh-note"></div>
  <a class="ota-link" href="/update">&#11014; Update firmware</a>

  <script>
    function batteryColor(pct) {
      if (pct >= 75) return '#44ff88';
      if (pct >= 50) return '#aaff44';
      if (pct >= 25) return '#ffcc00';
      return '#ff4444';
    }
    async function refresh() {
      try {
        const data = await fetch('/api/status').then(r => r.json());
        document.getElementById('ver-badge').textContent =
          'VIA Energy Mini \u2014 ESP32-C3 \u00b7 v' + (data.version ?? '?');

        const pct = data.battery_pct;
        const fill = document.getElementById('fill');
        fill.style.height     = pct + '%';
        fill.style.background = batteryColor(pct);
        document.getElementById('pct-label').textContent = pct + '%';
        document.getElementById('pct-sub').textContent   =
          pct === 0 ? 'Critical level' : 'Battery charge';

        const acOn = data.pins.find(p => p.label === 'AC / Charging')?.active ?? false;
        document.getElementById('ac-dot').className    = 'dot ' + (acOn ? 'on' : 'off');
        document.getElementById('ac-text').textContent =
          acOn ? '\uD83D\uDD0C AC connected / Charging' : '\uD83D\uDD0B Running on battery';

        document.getElementById('gpio-body').innerHTML = data.pins.map(p => `
          <tr>
            <td>GPIO ${p.pin}</td>
            <td>${p.label}</td>
            <td><span class="badge ${p.active ? 'active' : 'inactive'}">
              ${p.active ? 'Active' : 'Inactive'}
            </span></td>
          </tr>`).join('');

        const now = new Date().toLocaleTimeString('en-US');
        document.getElementById('refresh-note').textContent =
          'Updated: ' + now + ' \u00b7 every 3s';
      } catch(e) {
        document.getElementById('refresh-note').textContent = '\u26A0 Connection error';
      }
    }
    refresh();
    setInterval(refresh, 3000);
  </script>
</body>
</html>
)rawliteral";

// ── OTA update page ───────────────────────────────────────────────────────────
const char UPDATE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>OTA Update</title>
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: 'Segoe UI', sans-serif;
      background: #0d0d1a; color: #dde0ff;
      min-height: 100vh;
      display: flex; align-items: center; justify-content: center;
      padding: 24px;
    }
    .card {
      background: #161630; border: 1px solid #3333aa;
      border-radius: 16px; padding: 36px;
      width: 100%; max-width: 440px;
    }
    h2 { color: #8888ff; margin-bottom: 6px; }
    .ver { font-size: 12px; color: #5555aa; margin-bottom: 28px; }
    .drop-zone {
      border: 2px dashed #3333aa; border-radius: 10px;
      padding: 40px 20px; text-align: center;
      cursor: pointer; transition: border-color .2s, background .2s;
      margin-bottom: 20px;
    }
    .drop-zone.hover { border-color: #7777ff; background: #1a1a40; }
    .drop-zone .icon { font-size: 36px; margin-bottom: 10px; }
    .drop-zone p { font-size: 14px; color: #8888cc; }
    .drop-zone .filename { margin-top: 8px; font-size: 13px; color: #aaaaff; }
    input[type=file] { display: none; }
    button {
      width: 100%; padding: 12px;
      background: #3333aa; color: #fff;
      border: none; border-radius: 8px;
      font-size: 15px; cursor: pointer; transition: background .2s;
    }
    button:hover:not(:disabled) { background: #5555cc; }
    button:disabled { opacity: .4; cursor: not-allowed; }
    .progress-wrap { display: none; margin-top: 20px; }
    .progress-bar-bg {
      background: #111128; border-radius: 20px;
      height: 10px; overflow: hidden; margin-bottom: 8px;
    }
    .progress-bar {
      height: 100%; width: 0%;
      background: linear-gradient(90deg, #4444ff, #44aaff);
      transition: width .3s; border-radius: 20px;
    }
    .progress-text { font-size: 13px; color: #8888cc; text-align: center; }
    .result {
      margin-top: 20px; padding: 12px 16px;
      border-radius: 8px; font-size: 14px;
      display: none; text-align: center;
    }
    .result.ok  { background: #1a3a1a; color: #44ff88; border: 1px solid #44ff88; }
    .result.err { background: #3a1a1a; color: #ff6666; border: 1px solid #ff4444; }
    .back { display: block; text-align: center; margin-top: 20px; color: #5555aa; font-size: 13px; text-decoration: none; }
    .back:hover { color: #8888ff; }
  </style>
</head>
<body>
<div class="card">
  <h2>&#11014; OTA Firmware Update</h2>
  <div class="ver" id="cur-ver">Current version: loading&hellip;</div>

  <div class="drop-zone" id="drop" onclick="document.getElementById('fileInput').click()">
    <div class="icon">&#128230;</div>
    <p>Drag &amp; drop a .bin file here or click to browse</p>
    <div class="filename" id="fname"></div>
  </div>
  <input type="file" id="fileInput" accept=".bin">

  <button id="uploadBtn" disabled onclick="doUpload()">Flash firmware</button>

  <div class="progress-wrap" id="prog-wrap">
    <div class="progress-bar-bg"><div class="progress-bar" id="prog-bar"></div></div>
    <div class="progress-text" id="prog-text">0%</div>
  </div>

  <div class="result" id="result"></div>
  <a class="back" href="/">&larr; Back to monitoring</a>
</div>

<script>
  fetch('/api/status').then(r => r.json()).then(d => {
    document.getElementById('cur-ver').textContent =
      'Current firmware version: ' + (d.version ?? '\u2014');
  });

  const drop  = document.getElementById('drop');
  const input = document.getElementById('fileInput');
  let file = null;

  input.addEventListener('change', () => selectFile(input.files[0]));
  drop.addEventListener('dragover',  e => { e.preventDefault(); drop.classList.add('hover'); });
  drop.addEventListener('dragleave', ()  => drop.classList.remove('hover'));
  drop.addEventListener('drop', e => {
    e.preventDefault();
    drop.classList.remove('hover');
    selectFile(e.dataTransfer.files[0]);
  });

  function selectFile(f) {
    if (!f || !f.name.endsWith('.bin')) {
      showResult('err', '\u26A0 Please select a .bin file');
      return;
    }
    file = f;
    document.getElementById('fname').textContent =
      f.name + ' (' + (f.size / 1024).toFixed(1) + ' KB)';
    document.getElementById('uploadBtn').disabled = false;
    document.getElementById('result').style.display = 'none';
  }

  function doUpload() {
    if (!file) return;
    const btn  = document.getElementById('uploadBtn');
    const wrap = document.getElementById('prog-wrap');
    const bar  = document.getElementById('prog-bar');
    const txt  = document.getElementById('prog-text');

    btn.disabled = true;
    wrap.style.display = 'block';

    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/update');

    xhr.upload.addEventListener('progress', e => {
      if (e.lengthComputable) {
        const pct = Math.round(e.loaded / e.total * 100);
        bar.style.width = pct + '%';
        txt.textContent = pct + '%';
      }
    });

    xhr.onload = () => {
      if (xhr.status === 200) {
        showResult('ok', '\u2705 Flashed successfully! Device is rebooting\u2026');
        setTimeout(() => location.href = '/', 8000);
      } else {
        showResult('err', '\u274C Error: ' + xhr.responseText);
        btn.disabled = false;
      }
    };

    // ESP32 reboots after flashing — connection drop is expected
    xhr.onerror = () => {
      showResult('ok', '\u2705 Firmware sent! Device is rebooting\u2026');
      setTimeout(() => location.href = '/', 8000);
    };

    const form = new FormData();
    form.append('firmware', file, file.name);
    xhr.send(form);
  }

  function showResult(type, msg) {
    const el = document.getElementById('result');
    el.className = 'result ' + type;
    el.textContent = msg;
    el.style.display = 'block';
  }
</script>
</body>
</html>
)rawliteral";

// ─── Handlers ─────────────────────────────────────────────────────────────────
void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleUpdatePage() {
  server.send(200, "text/html", UPDATE_HTML);
}

void handleUpdate() {
  server.sendHeader("Connection", "close");
  if (Update.hasError()) {
    server.send(500, "text/plain", Update.errorString());
  } else {
    server.send(200, "text/plain", "OK");
  }
  delay(500);
  ESP.restart();
}

void handleUpdateUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("[OTA] Start: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("[OTA] Success! %u bytes written\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void handleStatus() {
  String json = "{";
  json += "\"version\":\"" + String(FIRMWARE_VERSION) + "\",";
  json += "\"battery_pct\":" + String(batteryPercent()) + ",";
  json += "\"uptime_s\":"    + String(millis() / 1000)  + ",";
  json += "\"pins\":[";
  for (int i = 0; i < PIN_COUNT; i++) {
    bool active = isActive(PINS[i]);
    bool raw    = digitalRead(PINS[i].pin);
    json += "{";
    json += "\"pin\":"      + String(PINS[i].pin)                       + ",";
    json += "\"label\":\""  + String(PINS[i].label)                     + "\",";
    json += "\"raw\":"      + String(raw ? 1 : 0)                       + ",";
    json += "\"inverted\":" + String(PINS[i].inverted ? "true":"false") + ",";
    json += "\"active\":"   + String(active ? "true" : "false");
    json += "}";
    if (i < PIN_COUNT - 1) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// ─── Setup ────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_R,   INPUT);
  pinMode(PIN_100, INPUT);
  pinMode(PIN_75,  INPUT);
  pinMode(PIN_50,  INPUT);
  pinMode(PIN_25,  INPUT);

  Serial.printf("\nConnecting to %s", SSID);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[OK] Wi-Fi connected!");
  Serial.print("[OK] IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("[OK] Firmware version: %s\n", FIRMWARE_VERSION);

  server.on("/",           HTTP_GET,  handleRoot);
  server.on("/update",     HTTP_GET,  handleUpdatePage);
  server.on("/update",     HTTP_POST, handleUpdate, handleUpdateUpload);
  server.on("/api/status", HTTP_GET,  handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[OK] HTTP server started");
}

// ─── Loop ─────────────────────────────────────────────────────────────────────
void loop() {
  server.handleClient();

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    Serial.printf("[UPS] Battery: %d%%  AC: %s\n",
      batteryPercent(), isActive(PINS[0]) ? "ON" : "OFF");
  }
}