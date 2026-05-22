<a href="https://github.com/G4MEOVER18/usb-army-penetrator/blob/master/LICENSE"><img alt="License" src="https://img.shields.io/badge/license-MIT-blue.svg"></a>
[![PlatformIO](https://img.shields.io/badge/build-PlatformIO-orange.svg)](https://platformio.org)

# USB Army Penetrator

**Fork of [USB Army Knife by i-am-shodan](https://github.com/i-am-shodan/USBArmyKnife)** with extended hardware support and enhanced G4MEOVER RadioRemote UI.

<div align="center">
  <img src="./docs/images/t-dongle-s3-side.png" width="400px">
</div>

A compact, concealable USB/WiFi penetration testing platform for red teamers and security researchers. Plug into a target, run payloads via DuckyScript, exfiltrate over WiFi, or leave behind and control remotely — all from a phone or browser.

> **For authorized security testing and penetration testing only.** Only use on systems you own or have explicit written authorization to test.

---

## What's different from upstream?

This fork adds:

| Addition | Description |
|----------|-------------|
| **CYD board support** | Full TFT dashboard UI for the ESP32-2432S028R (Cheap Yellow Display) — status page, quick-launch buttons, payload monitoring, touch input |
| **Enhanced T-Dongle UI** | Improved boot screen and multi-page status display for the LilyGo T-Dongle S3 |
| **G4MEOVER RadioRemote** | Remote-control extensions in WebServer.cpp for WiFi-triggered payload execution |
| **Improved WebUI** | Extended Bootstrap interface (`ui/content/index.html`) with additional controls |
| **USB NCM fixes** | Additional stability patches in USBCore.cpp and USBMSC.cpp |
| **Agent improvements** | SerialComms.cs and Program.cs enhancements for better agent communication |

---

## Features

- **USB HID Attacks** — DuckyScript BadUSB, supports multiple keyboard layouts
- **Mass Storage Emulation** — appear as USB drive or CDROM
- **USB Network Device** — USB Ethernet / NCM adapter, PCAP capture
- **WiFi & Bluetooth Attacks** — via ESP32 Marauder (deauth, evil AP, pcap)
- **Remote Agent** — deploy an agent, execute commands over serial even on locked machines
- **VNC Screen Pull** — view victim screen over device WiFi via noVNC
- **Hot Mic** — stream microphone audio over WiFi
- **Custom TFT UI** — run scripts with custom loading screens and progress bars

---

## Supported Hardware

| Hardware | Notes |
|----------|-------|
| **LilyGo T-Dongle S3** ⭐ | Recommended. USB-A form factor, screen, hidden SD card |
| **CYD ESP32-2432S028R** | New in this fork — 320×240 touch TFT, standalone dashboard |
| **Evil Crow Cable Wind** | Covert USB cable with hidden ESP32-S3 |
| **T-Watch S3** | Smart watch form factor |
| **Waveshare ESP32-S3 1.47"** | Large screen variant |
| **M5Stack AtomS3U** | Small form, no SD |
| **ESP32 UDisk / ESP32 Key** | Budget option, no web interface |
| **Waveshare RP2040-GEEK** | No Marauder, no NCM |

---

## Getting Started

### Prerequisites

- [Visual Studio Code](https://code.visualstudio.com/) + [PlatformIO extension](https://platformio.org/install/ide?install=vscode)
- Or PlatformIO Core CLI: `pip install platformio`

### Build & Flash

```bash
# Clone this repo
git clone https://github.com/G4MEOVER18/usb-army-penetrator.git
cd usb-army-penetrator

# Open in VSCode
code .

# In PlatformIO: select your board environment, then click Build → Upload
# CLI:
pio run -e LILYGO-T-Dongle-S3 -t upload
pio run -e CYD-ESP32-2432S028 -t upload
```

Available PlatformIO environments (in `platformio.ini`):
```
LILYGO-T-Dongle-S3
CYD-ESP32-2432S028
M5Stack-AtomS3U
Waveshare-ESP32-S3-GEEK
Waveshare-RP2040-GEEK
EvilCrowWind
TWatchS3
```

### First Use

1. **Flash** the firmware for your board (see above)
2. **Insert** a microSD card with your payloads (DuckyScript `.ds` files) — optional
3. **Plug** the device into a USB port
4. **Connect** to the WiFi access point the device creates (default SSID configured at runtime via web UI)
5. **Open** the web interface: `http://4.3.2.1:8080`
6. **Load** or write a DuckyScript payload and click Run

---

## Usage Guide

### Web Interface

After connecting to the device's WiFi AP and opening `http://4.3.2.1:8080`:

| Section | Description |
|---------|-------------|
| **Status** | Current USB mode, WiFi state, uptime, payload status |
| **Payload Editor** | Write/load/run DuckyScript attacks |
| **File Manager** | Browse SD card, upload/download files |
| **Settings** | WiFi credentials, AP name, USB mode |
| **VNC** | Screen view (requires agent deployment) |

### DuckyScript Quick Reference

```duckyscript
# Type text
STRING Hello World
ENTER

# Key combos
CTRL ALT DELETE
GUI r
STRING cmd
ENTER

# Delay (ms)
DELAY 1000

# WiFi — connect to AP
WIFI_CONNECT ssid password

# Execute Marauder command
ESP32M scan -w -t 5

# Display image on TFT
SHOW_IMAGE /myimage.png

# Self-destruct payload files
WIPE_STORAGE
```

Full DuckyScript reference: [USB Army Knife Wiki](https://github.com/i-am-shodan/USBArmyKnife/wiki)

### CYD Dashboard (ESP32-2432S028R)

The CYD board shows a 4-page TFT interface:

| Page | Content |
|------|---------|
| **Page 0** | Dashboard — WiFi, SD, USB mode, RAM, uptime, network info |
| **Page 1** | Quick Launch — touch 6 payload slots to run instantly |
| **Page 2** | Payload status — current running script, last output |
| **Page 3** | About / system info |

- **Touch** anywhere on page 0 → opens Quick Launch
- **Button** → cycles pages

### Agent Deployment

The C# agent (`tools/Agent/`) provides covert command execution:

```duckyscript
# Deploy agent (Windows)
INCLUDE install_agent_and_run_command/us-autorun.ds

# After deploy — send commands via web interface
# Commands execute on target, output appears in WebUI
```

The agent runs over the USB serial interface — no network socket opened on the target, no outbound connections, very low detection footprint.

---

## Examples

| Example | Description |
|---------|-------------|
| [Covert Storage](./examples/covertstorage/) | Appear as two different USB drives — full SD on first plug, benign drive after |
| [Progress Bar](./examples/progressbar/) | Hollywood-style attack UI with animated progress |
| [Ultimate RickRoll](./examples/rickroll/) | Keystroke inject + Marauder WiFi beacon spam |
| [USB Ethernet PCAP](./examples/usb_ethernet_pcap/) | USB NIC + automatic PCAP capture |
| [Install Agent](./examples/install_agent_and_run_command/) | Deploy agent, execute commands remotely |
| [VNC Screen Pull](./examples/vnc/) | Deploy agent with VNC server, view screen via WebUI |
| [Simple UI](./examples/simple_ui/) | Button-driven script launcher with custom screen |
| [Hot Mic](./examples/hotmic/) | Stream microphone audio over WiFi |
| [Linux Panic](./examples/linux_panic/) | Bad filesystem triggers kernel panic on automount |
| [Evil USB NIC](./examples/malicious_ethernet_adapter/) | Fake NIC that presents a driver CDROM |
| [EvilAP](./examples/evilap/) | Deceptive WiFi access point via Marauder |
| [WiFi Deauth + PCAP](./examples/wifi_deauth_and_crypt_capture/) | Deauth clients, capture handshake |

---

## Build from Source (advanced)

```bash
# Install PlatformIO CLI
pip install platformio

# Build all environments
pio run

# Build specific board
pio run -e LILYGO-T-Dongle-S3

# Flash
pio run -e LILYGO-T-Dongle-S3 -t upload
```

---

## Upstream / Credits

This project is a fork of **[USB Army Knife](https://github.com/i-am-shodan/USBArmyKnife)** by [@i-am-shodan](https://github.com/i-am-shodan), licensed MIT.

WiFi/Bluetooth attack capability via **[ESP32 Marauder](https://github.com/justcallmekoko/ESP32Marauder)** by justcallmekoko.

---

## License

MIT License — see [LICENSE](LICENSE)

Original work copyright (c) 2024 i-am-shodan  
Fork modifications copyright (c) 2026 G4MEOVER18

---

## Support

**Bitcoin:** `39vZWmnUwDReQ15BwqQXzyqVQ6U8LardEf`
