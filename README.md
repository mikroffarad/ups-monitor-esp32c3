# UPS Monitor for ESP32-C3 Super Mini

Firmware for monitoring VIA Energy Mini UPS via ESP32-C3 Super Mini.

> **Note:** This is a PlatformIO project. For building and flashing, it is recommended to use VS Code with the PlatformIO IDE extension.

## Requirements

- [VS Code](https://code.visualstudio.com/)
- [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) (VS Code extension)
- USB cable for connecting ESP32-C3 (data cable, not charge-only)

## Quick Start with PlatformIO

### 1. Clone and Open Project

```bash
git clone <repository-url>
cd ups-monitor-esp32c3
```

Open the project folder in VS Code.

### 2. Build Project

**Method 1: Via VS Code**
- Click the PlatformIO icon on the sidebar (ant/alien icon)
- Select `esp32c3-supermini` → `Build`

**Method 2: Via Terminal**
```bash
pio run
```

### 3. Upload (Flash)

1. Connect ESP32-C3 via USB
2. Identify COM port (Windows: Device Manager, Linux/Mac: `ls /dev/tty*`)

**Method 1: Via VS Code**
- PlatformIO → `esp32c3-supermini` → `Upload`

**Method 2: Via Terminal**
```bash
pio run --target upload
```

### 4. Serial Monitor

**Method 1: Via VS Code**
- PlatformIO → `esp32c3-supermini` → `Monitor`

**Method 2: Via Terminal**
```bash
pio device monitor
```

### 5. Upload + Monitor (Single Command)

```bash
pio run --target upload --target monitor
```

Or via VS Code: PlatformIO → `esp32c3-supermini` → `Upload and Monitor`

## Serial Monitor Settings

- Baud rate: **115200**
- Line ending: Any or Both NL & CR

## GPIO Configuration
```
GPIO0 → UPS LED 25%
GPIO1 → UPS LED 50%
GPIO2 → (not used - internal pull-up)
GPIO3 → UPS LED 100%
GPIO4 → UPS PWR Mode (AC/Battery)
GPIO5 → UPS LED 75% (backup/alternative)

GPIO8 → 3.3V OUTPUT (for testing)
```

## Testing Without UPS

Connect dupont male-male jumpers:
- GPIO8 (3.3V source) → GPIO0, GPIO1, GPIO3, GPIO4, or GPIO5
- Observe changes in Serial Monitor

## Example Output
```
UPS Monitor - GPIO Test Started
GPIO8 = 3.3V OUTPUT for testing

--- GPIO States ---
GPIO0: LOW
GPIO1: LOW
GPIO2: HIGH
GPIO3: LOW
GPIO4: LOW
GPIO5: LOW

--- GPIO States ---
GPIO0: HIGH
GPIO1: LOW
GPIO2: HIGH
GPIO3: LOW
GPIO4: LOW
GPIO5: LOW
```

## Known Issues

- **GPIO2 always HIGH** - this is normal for ESP32-C3 (strapping/boot pin with internal pull-up)
- Dupont connections may have poor contact - verify connection quality
- GPIO8 and GPIO9 have I2C pull-ups on some boards - GPIO8 works for output but not recommended for input

## Hardware Notes

### Tested on ESP32-C3 Super Mini

**Recommended INPUT pins:**
- ✅ GPIO0, GPIO1, GPIO3, GPIO4, GPIO5 - stable, clean signals
- ⚠️ GPIO2 - strapping pin, always HIGH due to internal pull-up
- ❌ GPIO8, GPIO9 - I2C pins with hardware pull-ups, not suitable for clean input

**Connection diagram for testing:**
```
ESP32-C3 Super Mini
┌─────────────────┐
│                 │
│  GPIO8 (OUT) ───┼──┐ 3.3V test source
│                 │  │
│  GPIO0 (IN)  ───┼──┘ Connect with dupont
│  GPIO1 (IN)  ───┤
│  GPIO3 (IN)  ───┤
│  GPIO4 (IN)  ───┤
│  GPIO5 (IN)  ───┤
│                 │
│  GND ───────────┼──── Common ground
└─────────────────┘
```

## UPS Connection (Future)

When connecting real VIA Energy Mini UPS via 5V→3.3V converter board:
```
VIA Energy Mini UPS → 5V→3.3V Converter → ESP32-C3
LED 25%  (5V) ────────→ GPIO0 (3.3V)
LED 50%  (5V) ────────→ GPIO1 (3.3V)
LED 75%  (5V) ────────→ GPIO5 (3.3V)
LED 100% (5V) ────────→ GPIO3 (3.3V)
PWR Mode (5V) ────────→ GPIO4 (3.3V)
GND ──────────────────→ GND
```

**Note:** GPIO2 is skipped due to internal pull-up behavior.

## Troubleshooting

### Unable to Flash
- Try lower speed: add `upload_speed = 115200` to `platformio.ini`
- Hold BOOT button while connecting USB
- Check USB cable (must be data cable, not charge-only)
- Try a different USB port

### Serial Monitor Shows Garbage
- Verify baud rate is 115200
- Reset the board (RST button)

### Unstable GPIO Readings
- Check dupont wire connections
- Ensure common ground (GND) connection
- Try different dupont wires

### GPIO2 Always Shows HIGH
- This is normal behavior - GPIO2 has internal pull-up resistor
- Use GPIO5 as alternative input

| Command | Description |
|---------|-------------|
| `pio run` | Build project |
| `pio run -t upload` | Flash firmware |
| `pio device monitor` | Serial Monitor |
| `pio run -t upload -t monitor` | Flash + Monitor |
| `pio run -t clean` | Clean build files |
| `pio lib install <lib>` | Install library |