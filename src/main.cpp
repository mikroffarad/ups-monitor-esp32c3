#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Update.h>

#define PIN_R    21
#define PIN_100  0
#define PIN_75   1
#define PIN_50   3
#define PIN_25   4

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool wifiConnected = false;

/* ================= GPIO ================= */

bool readLogicalPin(int pin)
{
    return !digitalRead(pin); // inverted signals
}

/* ================= UPS STATE ================= */

void sendUPSState()
{
    StaticJsonDocument<256> doc;

    doc["R"]   = readLogicalPin(PIN_R);
    doc["100"] = readLogicalPin(PIN_100);
    doc["75"]  = readLogicalPin(PIN_75);
    doc["50"]  = readLogicalPin(PIN_50);
    doc["25"]  = readLogicalPin(PIN_25);

    String output;
    serializeJson(doc, output);

    ws.textAll(output);
}

/* ================= WIFI CONFIG PAGE ================= */

const char wifi_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<body>
<h2>WiFi Setup</h2>
<form action="/connect" method="POST">
SSID:<br>
<input name="ssid"><br>
Password:<br>
<input name="pass" type="password"><br><br>
<input type="submit" value="Connect">
</form>
</body>
</html>
)rawliteral";

/* ================= MONITOR PAGE ================= */

const char monitor_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<style>
body { font-family: Arial; background:#111; color:white; text-align:center; }
.ind { width:80px; margin:10px auto; padding:10px; border-radius:6px; }
.on { background:#2ecc71; }
.off { background:#e74c3c; }
</style>
</head>
<body>

<h2>UPS Monitor</h2>

<div id="100" class="ind">100%</div>
<div id="75" class="ind">75%</div>
<div id="50" class="ind">50%</div>
<div id="25" class="ind">25%</div>
<div id="R" class="ind">R</div>

<h3>OTA Update</h3>
<form method="POST" action="/update" enctype="multipart/form-data">
<input type="file" name="update">
<input type="submit" value="Upload">
</form>

<script>
let ws = new WebSocket(`ws://${location.host}/ws`);
ws.onmessage = function(event)
{
    let data = JSON.parse(event.data);
    for (let key in data)
    {
        let el = document.getElementById(key);
        el.className = "ind " + (data[key] ? "on" : "off");
    }
};
</script>

</body>
</html>
)rawliteral";

/* ================= SETUP ================= */

void startAP()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP("UPS_Config");
}

void setup()
{
    Serial.begin(115200);

    pinMode(PIN_R, INPUT_PULLUP);
    pinMode(PIN_100, INPUT_PULLUP);
    pinMode(PIN_75, INPUT_PULLUP);
    pinMode(PIN_50, INPUT_PULLUP);
    pinMode(PIN_25, INPUT_PULLUP);

    startAP();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        if (wifiConnected)
            request->send_P(200, "text/html", monitor_html);
        else
            request->send_P(200, "text/html", wifi_html);
    });

    server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request)
    {
        String ssid = request->getParam("ssid", true)->value();
        String pass = request->getParam("pass", true)->value();

        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());

        request->send(200, "text/plain", "Connecting...");

        int timeout = 0;
        while (WiFi.status() != WL_CONNECTED && timeout < 20)
        {
            delay(500);
            timeout++;
        }

        if (WiFi.status() == WL_CONNECTED)
            wifiConnected = true;
    });

    /* ===== WebSocket ===== */

    ws.onEvent([](AsyncWebSocket * server,
                  AsyncWebSocketClient * client,
                  AwsEventType type,
                  void * arg,
                  uint8_t * data,
                  size_t len)
    {
        if (type == WS_EVT_CONNECT)
            sendUPSState();
    });

    server.addHandler(&ws);

    /* ===== OTA ===== */

    server.on("/update", HTTP_POST,
        [](AsyncWebServerRequest *request)
        {
            request->send(200, "text/plain", Update.hasError() ? "FAIL" : "OK");
            delay(1000);
            ESP.restart();
        },
        [](AsyncWebServerRequest *request,
           String filename,
           size_t index,
           uint8_t *data,
           size_t len,
           bool final)
        {
            if (!index)
                Update.begin(UPDATE_SIZE_UNKNOWN);

            Update.write(data, len);

            if (final)
                Update.end(true);
        });

    server.begin();
}

/* ================= LOOP ================= */

void loop()
{
    if (wifiConnected)
        sendUPSState();

    ws.cleanupClients();
    delay(1000);
}