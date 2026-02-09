# TapeBackarr CYD Monitor

📼 **ESP32 Cheap Yellow Display monitor for [TapeBackarr](https://github.com/RoseOO/TapeBackarr) tape drive backup system**

A compact monitoring display that sits on top of your tape drive, showing real-time job stats, drive status, and tape change alerts.

## Hardware

- **Board**: [ESP32 Cheap Yellow Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display) — CYD2USB variant (USB-C)
- **Display**: 2.8" 320×240 ILI9341 TFT with XPT2046 resistive touch
- **MCU**: ESP32-WROOM-32 (WiFi + Bluetooth)

## Features

### Display Screens
- **Dashboard** — Total tapes, active/full counts, job stats, storage usage bar
- **Active Jobs** — Running backup jobs with file count and bytes processed
- **Drives** — Drive status, loaded tape info, online/offline/error indicators
- **Tape Alert** — Full-screen alert with flashing red LED when a tape change is needed

### Touch Navigation
- Tap bottom tab bar to switch between Dashboard, Jobs, and Drives screens
- Tap anywhere to dismiss tape change alerts

### Web Configuration
- WiFi network scanning and selection
- TapeBackarr server host, port, and API key settings
- Display brightness and poll interval controls
- Device name customization
- Reboot and factory reset options

### WiFi
- Connects to your configured WiFi network (STA mode)
- Falls back to access point mode if WiFi is not configured or connection fails
- AP name shown on display for easy setup

### LED Indicators
- 🟢 Green — Connected and operating normally
- 🔵 Blue — AP setup mode active
- 🔴 Red (blinking) — Tape change alert

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- ESP32 CYD2USB board connected via USB-C

### Build & Flash

```bash
# Clone this repository
git clone https://github.com/RoseOO/TapeBackarrCYD.git
cd TapeBackarrCYD

# Build
pio run

# Upload to board
pio run --target upload

# Monitor serial output
pio device monitor
```

### Initial Setup

1. Power on the CYD — it will start in AP mode on first boot
2. Connect to the **TapeBackarr-CYD** WiFi network from your phone/computer
3. Open **http://192.168.4.1** in a browser
4. Configure:
   - **WiFi** — Select your network and enter the password
   - **Server** — Enter your TapeBackarr server IP and port (default: 8080)
   - **API Key** — Paste an API key generated from TapeBackarr's web UI (Settings → API Keys)
5. Click **Save Settings** and then **Reboot**

### API Key

Generate an API key in the TapeBackarr web interface:
1. Log in to TapeBackarr
2. Go to **Settings → API Keys**
3. Create a new key with at least **read-only** permissions
4. Copy the key and paste it into the CYD web setup page

## API Endpoints Used

| Endpoint | Purpose |
|---|---|
| `GET /api/v1/health` | Connection test |
| `GET /api/v1/dashboard` | Dashboard statistics |
| `GET /api/v1/jobs/active` | Active job list |
| `GET /api/v1/drives` | Drive status and tape info |
| `GET /api/v1/events` | Tape change notifications |

All requests include the `X-API-Key` header for authentication.

## Project Structure

```
├── platformio.ini          # PlatformIO build configuration
├── src/
│   ├── main.cpp            # Application entry point and main loop
│   ├── settings.h/cpp      # Persistent configuration (Preferences)
│   ├── wifi_manager.h/cpp  # WiFi STA/AP management
│   ├── api_client.h/cpp    # TapeBackarr REST API client
│   ├── display.h/cpp       # TFT display rendering and touch
│   └── web_server.h/cpp    # Configuration web interface
└── readme.md
```

## Configuration

All settings are stored in ESP32 non-volatile storage (Preferences) and persist across reboots.

| Setting | Default | Description |
|---|---|---|
| WiFi SSID | — | Your WiFi network name |
| WiFi Password | — | Your WiFi password |
| Server Host | — | TapeBackarr IP or hostname |
| Server Port | 8080 | TapeBackarr API port |
| API Key | — | TapeBackarr API key |
| Use HTTPS | false | Enable HTTPS for API calls |
| Brightness | 100 | Display brightness (0–100) |
| Poll Interval | 5 | Data refresh interval in seconds |
| Device Name | TapeBackarr-CYD | WiFi hostname and AP name |

## CYD2USB Pin Map

| Function | GPIO |
|---|---|
| TFT MISO | 12 |
| TFT MOSI | 13 |
| TFT SCLK | 14 |
| TFT CS | 15 |
| TFT DC | 2 |
| TFT Backlight | 21 |
| Touch CS | 33 |
| Touch IRQ | 36 |
| Touch MOSI | 32 |
| Touch MISO | 39 |
| Touch CLK | 25 |
| LED Red | 4 |
| LED Green | 16 |
| LED Blue | 17 |

## License

MIT License — see [LICENSE](https://github.com/RoseOO/TapeBackarr/blob/main/LICENSE) for details.
