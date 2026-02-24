# UPS Monitor — ESP32-C3 Super Mini

Web-based live monitor for DC UPS.  
ESP32-C3 reads battery LEDs and AC/charging state via GPIO and exposes a real-time dashboard over Wi-Fi.

The device starts in **Access Point mode**, allows Wi-Fi configuration, then switches to Station mode for monitoring and OTA updates.

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

⚠ ESP32-C3 is **NOT 5V tolerant**.  
All GPIO inputs must be limited to **3.3V maximum**.

Use:
- Voltage divider (recommended)
- Series resistor (temporary testing only)
- Level shifter or optocoupler (best for permanent installation)

---

## Inverted Signal Logic

UPS LED outputs use **inverted logic**:

| Physical pin level | Meaning |
|--------------------|---------|
| `HIGH` | Indicator is OFF |
| `LOW`  | Indicator is ON |

Firmware handles inversion internally:

```cpp
bool readLogicalPin(int pin)
{
    return !digitalRead(pin); // inverted logic
}
```

The web UI always shows:

- 🟢 Green = Active
- 🔴 Red = Inactive

---

## Boot Behavior

On every power-up:

1. ESP32 starts in **Access Point mode**
2. SSID: `UPS_Config`
3. Open `http://192.168.4.1`
4. Enter your Wi-Fi credentials
5. Device switches to Station mode
6. Dashboard becomes available at `http://ups-monitor.local` (or via assigned IP)

Wi-Fi credentials are **NOT stored**.  
After reboot → configuration resets → AP mode again.

---

## Web Interface

| URL | Description |
|-----|-------------|
| `http://192.168.4.1/` | Wi-Fi configuration page (AP mode) |
| `http://ups-monitor.local/` | Live UPS dashboard (Station mode) |
| `http://ups-monitor.local/update` | OTA firmware upload |
| `http://<IP>/` | Same as above, via direct IP |

Dashboard updates in real time using **WebSocket** (no page refresh required).

---

## First Flash (USB)

1. Connect ESP32-C3 via USB.
2. Open PlatformIO.
3. Click **Build**.
4. Click **Upload**.
5. Open Serial Monitor (115200 baud).

---

## OTA Firmware Update

After the first USB flash, all updates can be done wirelessly.

### Step 1 — Build firmware

In PlatformIO:

```
pio run
```

The binary will be located at:

```
.pio/build/esp32c3/firmware.bin
```

---

### Step 2 — Open OTA page

Navigate to:

```
http://ups-monitor.local/update
```

---

### Step 3 — Upload firmware

1. Select `firmware.bin`
2. Press **Upload**
3. Wait for completion
4. ESP32 reboots automatically

If the browser shows a connection error after upload, this is normal — the device reboots during HTTP transfer.

Wait a few seconds and refresh the main page.

---

## mDNS — Local DNS

After connecting to Wi-Fi, the device registers itself via **mDNS (Multicast DNS)**:

```
http://ups-monitor.local
```

No need to know the device's IP address.

| Platform | Requirements |
|----------|-------------|
| macOS / iOS | Built-in (Bonjour) |
| Linux | `avahi-daemon` |
| Windows | [Apple Bonjour](https://support.apple.com/kb/DL999) or iTunes |

The hostname is defined in firmware:

```cpp
#define MDNS_HOSTNAME "ups-monitor"
```

Change it before building to avoid conflicts in networks with multiple devices.

---

## WebSocket Live Updates

The dashboard connects to:

```
ws://ups-monitor.local/ws
```

On every state change, ESP32 sends JSON like:

```json
{
  "R": true,
  "100": false,
  "75": true,
  "50": true,
  "25": true
}
```

The page updates instantly without refresh.

---

## PlatformIO Configuration

Example `platformio.ini`:

```ini
[env:esp32c3]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200

lib_deps =
    https://github.com/mathieucarbou/AsyncTCP.git
    https://github.com/mathieucarbou/ESPAsyncWebServer.git
    bblanchon/ArduinoJson
```

---

## Safety Notes

- Never apply 5V directly to ESP32-C3 GPIO.
- Avoid using GPIO0 if external signals may hold it LOW during reset (boot mode issue).
- Prefer opto-isolation when interfacing with UPS internal circuitry.

---

## Future Improvements

- Battery percentage auto-calculation
- Password-protected OTA
- NVS storage for Wi-Fi
- Signal debounce filtering
- Voltage measurement via ADC
- Historical logging

---

## Project Purpose

This firmware turns a basic DC UPS into a network-monitored device with:

- Live status view
- OTA firmware upgrades
- Temporary Wi-Fi configuration mode
- Clean GPIO abstraction for inverted signals

---

**VIA Energy Mini — ESP32-C3 UPS Monitor**