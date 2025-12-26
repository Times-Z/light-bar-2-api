# Light bar 2 api

**Light bar 2 API** is a firmware solution that makes the Xiaomi Light Bar smart and connected through a RESTful API. Built on ESP32, it provides wireless control capabilities for your Xiaomi Light Bar via HTTP endpoints.

<!-- screen later -->

## Prerequisites

Before setting up the project, ensure you have the following installed:

### Software Requirements

- **Python 3** + `pip`
- **VS Code** with the following extensions:
  - [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
  - [ESP-IDF (5.4)](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension)
- _Optional_ **Docker/docker-compose** for running the devcontainer to avoid installing esp-idf on host

### Hardware Requirements

- **ESP32** microcontroller
- **NRF24L01** module transceiver
- **Mi Computer Monitor Light Bar**, model **MJGJD01YL** (without BLE/WiFi), the MJGJD02YL will not work

### Tested hardware

- [Mi Computer Monitor Light Bar](https://www.mi.com/fr/product/mi-computer-monitor-light-bar/)
- [ESP-32S ESP-WROOM-32E](https://fr.aliexpress.com/item/1005004275519535.html)
- [NRF24L01+PA+LNA](https://amzn.eu/d/drTwkjU)

### Partitions

The project uses a custom partition table tailored for persistent storage, application firmware, and static assets.

| Name      | Type | SubType | Offset   | Size     | Size (KB) | Description                |
| :-------- | :--- | :------ | :------- | :------- | :-------- | :------------------------- |
| `nvs`     | data | nvs     | 0x9000   | 0x80000  | 512 KB    | Non-volatile storage (NVS) |
| `factory` | app  | factory | 0x90000  | 0x200000 | 2048 KB   | Main application binary    |
| `storage` | data | spiffs  | 0x290000 | 0x170000 | 1472 KB   | SPIFFS for static assets   |

## Installation

1. **Clone the repository:**
   ```sh
   git clone https://github.com/Times-Z/light-bar-2-api.git
   cd light-bar-2-api
   ```

### If your don't use devcontainer

**Configure ESP-IDF:**

- Open VS Code and install the ESP-IDF extension.
- Follow the setup instructions to configure the ESP32 environment.

### Both

3. **Build and flash the firmware:**
   ```sh
   idf.py build
   idf.py flash
   ```

## Usage

- Power on the ESP32
- The device will create an access point `Lightbar2api` with password `$tr0ngWifi` to configure wifi network.
- Use the serial monitor for debugging:
  ```sh
  idf.py monitor
  ```

## API documentation

[SmartClimate API Swagger](./swagger.yml)

## Features

- [x] Embedded web server with HTTP endpoints
- [x] Wi-Fi station (STA) mode support
- [x] Captive portal for Wi-Fi access point (AP) mode
- [x] NVS-based persistent storage for configuration and credentials
- [x] Uptime tracking (seconds to days format)
- [x] Lightweight JSON API for system status and configuration

## License

This project is licensed under the MIT License.

## Contributions

Contributions are welcome! Feel free to submit a pull request or open an issue for discussion.

---

Happy Coding! 😊
