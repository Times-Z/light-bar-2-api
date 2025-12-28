# Light bar 2 API

**Light bar 2 API** is a firmware solution that makes the Xiaomi Light Bar smart and connected through a RESTful API. Built on ESP32, it provides wireless control capabilities for your Xiaomi Light Bar via HTTP endpoints.

<div align="center">
  <br/>

<a href="https://github.com/Times-Z/light-bar-2-api"><img src="https://img.shields.io/github/v/release/Times-Z/light-bar-2-api?label=Latest%20Version&color=c56a90&style=for-the-badge&logo=star)" alt="Latest Version" /></a>
<a href="https://github.com/Times-Z/light-bar-2-api"><img src="https://img.shields.io/github/actions/workflow/status/Times-Z/light-bar-2-api/.github/workflows/build.yml?branch=main&label=Pipeline%20Status&color=c56a90&style=for-the-badge&logo=star" alt="Latest Version" /></a>

  <br/>
  <br/>

<a href="https://github.com/Times-Z/light-bar-2-api"><img height="400" src="./.github/assets/01_login.gif" alt="gif_login" /></a>
<a href="https://github.com/Times-Z/light-bar-2-api"><img height="400" src="./.github/assets/02_website.png" alt="website" /></a>
<a href="https://github.com/Times-Z/light-bar-2-api"><img height="400" src="./.github/assets/03_logs.png" alt="logs" /></a>

</div>

## Acknowledgements

This project is based on the reverse engineering work by [Lamperez](https://github.com/lamperez/xiaomi-lightbar-nrf24) and [Benallen](https://github.com/benallen-dev/xiaomi-lightbar).

Thanks for your works!

## Prerequisites

Before setting up the project, ensure you have the following installed:

### Software Requirements

- **Python 3** + **pip**
- **VS Code** with the following extensions:
  - [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
  - [ESP-IDF](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension)
- [ESP-IDF framework 5.5](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32/get-started/index.html)
- **Optional**: **Docker/docker-compose** for running the devcontainer to avoid installing esp-idf on host

### Hardware Requirements

- **ESP32** microcontroller
- **NRF24L01** module transceiver
- **Mi Computer Monitor Light Bar**, model **MJGJD01YL** (without BLE/WiFi), the MJGJD02YL will not work

#### Pinout

| nRF24     |  ESP32 |
| :-------- | -----: |
| VCC       |    3V3 |
| GND       |    GND |
| CE        |  Pin 4 |
| CSN       |  Pin 5 |
| SCK       | Pin 18 |
| MOSI (MO) | Pin 23 |
| MISO (M1) | Pin 19 |

### Tested Hardware

- [Mi Computer Monitor Light Bar](https://www.mi.com/fr/product/mi-computer-monitor-light-bar/)
- [ESP-32S ESP-WROOM-32E](https://fr.aliexpress.com/item/1005004275519535.html)
- [NRF24L01+PA+LNA](https://amzn.eu/d/drTwkjU)

### Partitions

The project uses a custom partition table tailored for persistent storage, application firmware, and static assets.

| Name      | Type | SubType | Offset   | Size     | Size (KB) | Description                          |
| :-------- | :--- | :------ | :------- | :------- | :-------- | :----------------------------------- |
| `nvs`     | data | nvs     | 0x9000   | 0x80000  | 512 KB    | Non-volatile storage (NVS)           |
| `factory` | app  | factory | 0x90000  | 0x200000 | 2048 KB   | Main application binary              |
| `config`  | data | spiffs  | 0x290000 | 0x20000  | 128 KB    | SPIFFS for JSON configuration files  |
| `www`     | data | spiffs  | 0x2B0000 | 0x150000 | 1344 KB   | SPIFFS for static assets (HTML, CSS) |

## Installation

### 1. Clone the repository

```sh
git clone https://github.com/Times-Z/light-bar-2-api.git
cd light-bar-2-api
```

### 2. Configure ESP-IDF (if you don't use devcontainer)

- Open VS Code and install the ESP-IDF extension
- Follow the setup instructions to configure the ESP32 environment

### 3. Configure the project

Two configuration methods are available:

**1) Using a JSON file (first boot-time only)**

- Copy the template: `cp main/config/default.json main/config/config.json`
- Fill in `main/config/config.json` with:
  - `wifi_ssid` and `wifi_password`
  - `api_key`
  - `ntp_server`
  - `xiaomi_remote_id` if already known
- If the file exists and the Wi-Fi credentials are valid, the ESP32 will connect, store the config in NVS, and will not read the file again (used only at init)

**2) Using the Web UI or the API**

- All routes are described in the swagger (`swagger.yml`).
- Configuration entered via the UI/API is persisted directly in NVS if valid

If Wi-Fi is not set in `config.json`, or the configured network is unreachable/invalid, the ESP32 falls back to AP mode and creates the access point `Lightbar2api` with password `$tr0ngWifi`, exposing a captive portal so setup works on iPhone/Android as well

### 4. Build and flash the firmware

```sh
idf.py build
idf.py flash
```

## API Documentation

[Light bar 2 API Swagger](./swagger.yml)

## Features

- [x] Embedded web server with HTTP endpoints
- [x] Wi-Fi station (STA) mode support with auto-connect from `config.json`
- [x] Captive portal for Wi-Fi access point (AP) mode
- [x] JSON-based configuration file (`config.json`) for boot-time settings
- [x] NVS-based persistent storage for credentials (fallback & persistence)
- [x] Uptime tracking (seconds to days format)
- [x] Lightweight JSON API for system status and configuration
- [x] NTP sync (default => build date)
- [x] X-API-Key based header protection for needed endpoint
- [x] Scan and get the Xiaomi remote ID
- [x] Show ESP32 logs from web UI
- [x] Store the Xiaomi remote ID in the NVS storage
- [x] Send commands power on / off
- [] Send others commands to the bar

## Data Whitening Logic

The Whitening process is a data transformation technique used in radio communications (nRF24, Bluetooth, etc.) to ensure signal reliability

Why is it used?
Clock Recovery: Long sequences of identical bits (e.g., 00000000) make it difficult for the receiver to stay synchronized with the transmitter's clock

DC Balancing: It prevents a "DC Offset" by ensuring an equal distribution of 0s and 1s, which keeps the radio frequency stable

### How it works

The algorithm uses a Linear Feedback Shift Register (LFSR) to generate a pseudo-random bit sequence (PRBS) based on a specific polynomial and a starting value called a Seed

XOR Operation: Each byte of the plaintext packet (including the CRC) is combined with the LFSR output using a bitwise XOR operation

Symmetry: Because XOR is reversible, the receiver applies the exact same algorithm with the same seed to "de-whiten" the data and recover the original message

### Implementation in this project

In [`nrf24_build_xiaomi_frame`](main/nrf24/nrf24.c#L298), whitening is the final step before transmission. It is applied to the entire 18-byte frame (Preamble + Payload + CRC) to scramble the data pattern across the airwaves while maintaining the integrity of the underlying protocol

## License

This project is licensed under the MIT License.

## Contributions

Contributions are welcome! Feel free to submit a pull request or open an issue for discussion.

---

Happy Coding! 😊
