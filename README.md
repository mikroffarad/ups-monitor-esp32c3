# UPS Monitor — ESP32-C3 Super Mini

Web-based status monitor for DC UPS.  
Reads battery level LEDs and AC/charging state via GPIO, and exposes a live dashboard over Wi-Fi.

---

## Hardware

**Board:** ESP32-C3 Super Mini  
**UPS:** VIA Energy Mini

---

## Pin Mapping

| GPIO | UPS Signal | Description |
|------|-----------|-------------|
| 21   | R         | AC power connected / charging active |
| 0    | 100%      | Battery ≥ 100% LED |
| 1    | 75%       | Battery ≥ 75% LED |
| 3    | 50%       | Battery ≥ 50% LED |
| 4    | 25%       | Battery ≥ 25% LED |

---

## Inverted Signal Logic

The UPS LED outputs use **inverted logic**:

| Physical pin level | Meaning |
|--------------------|---------|
| `HIGH` | Indicator is **OFF** (level not reached / no AC) |
| `LOW`  | Indicator is **ON** (level active / AC present) |

In the code this is handled by the `inverted` field inside each `PinDef` struct:

```cpp
const PinDef PINS[] = {
  { PIN_R,   true, "AC / Charging" },  // true = inverted
  { PIN_100, true, "100%"          },
  ...
};
```

The helper `isActive()` transparently flips the reading when `inverted = true`, so the rest of the code always works with logical (not physical) levels. The web UI shows **green = active**, **red = inactive** — already accounting for the inversion.

To add a non-inverted pin in the future, simply set its `inverted` field to `false`.

---

## Web Interface

| URL | Description |
|-----|-------------|
| `http://<IP>/` | Live dashboard — battery level, AC status, GPIO table |
| `http://<IP>/update` | OTA firmware update page |
| `http://<IP>/api/status` | JSON API — battery %, pin states, uptime, version |

The dashboard auto-refreshes every **3 seconds** via `fetch`.

---

## Configuration

Open `main.cpp` and edit the top section:

```cpp
const char* SSID             = "YOUR_WIFI_SSID";
const char* PASSWORD         = "YOUR_WIFI_PASSWORD";
const char* FIRMWARE_VERSION = "1.1.0";   // bump this on every release
```

---

## First Flash (USB)

1. Make sure `platformio.ini` does **not** have `upload_protocol` set.
2. Connect the board via USB.
3. In PlatformIO: **Build** → **Upload**.
4. Open Serial Monitor at **115200 baud** — the IP address will be printed on boot.

---

## OTA Firmware Update (Web)

After the first USB flash, all subsequent updates can be done wirelessly through the browser.

### Step 1 — Build the binary

In PlatformIO press **Build** (or `Ctrl+Alt+B`).  
The output file will be located at:

```
.pio/build/esp32-c3-devkitm-1/firmware.bin
```

### Step 2 — Open the update page

Navigate to `http://<IP>/update` in your browser.  
The current firmware version is displayed at the top of the page.

### Step 3 — Upload

Drag and drop `firmware.bin` onto the upload area, or click to browse for the file.  
Press **Flash firmware**.

A progress bar shows the upload percentage.  
Once complete, the ESP32 reboots automatically and the browser redirects back to the dashboard after 8 seconds.

> **Note:** If the browser shows a connection error instead of the success message, the flash still likely succeeded — the ESP32 reboots mid-transfer which closes the HTTP connection. Wait a few seconds and refresh the main page to verify the new version number.

### Step 4 — Verify

Check the subtitle on the dashboard — it should show the new version:

```
VIA Energy Mini — ESP32-C3 · v1.2.0
```

---

## API Response Example

```json
{
  "version": "1.1.0",
  "battery_pct": 75,
  "uptime_s": 3842,
  "pins": [
    { "pin": 21, "label": "AC / Charging", "raw": 0, "inverted": true, "active": true  },
    { "pin": 0,  "label": "100%",          "raw": 1, "inverted": true, "active": false },
    { "pin": 1,  "label": "75%",           "raw": 0, "inverted": true, "active": true  },
    { "pin": 3,  "label": "50%",           "raw": 0, "inverted": true, "active": true  },
    { "pin": 4,  "label": "25%",           "raw": 0, "inverted": true, "active": true  }
  ]
}
```