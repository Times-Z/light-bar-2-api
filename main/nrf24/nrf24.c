#include "nrf24.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

static const char* TAG = "NRF24";

// Mutex to protect SPI access from multiple tasks
static SemaphoreHandle_t nrf24_mutex = NULL;

// NRF24L01+ pin definitions
// Pinout for ESP32 DevKitC
// MISO - GPIO19
// MOSI - GPIO23
// SCK  - GPIO18
// CS   - GPIO5
// CE   - GPIO4
// GND  - GND
// VCC  - 3.3V

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 18
#define PIN_NUM_CS 5
#define PIN_NUM_CE 4

#define NRF_SPI_HOST SPI3_HOST

static spi_device_handle_t nrf_spi;
static bool spi_bus_initialized = false;

// Register map
#define NRF_REG_CONFIG 0x00
#define NRF_REG_EN_AA 0x01
#define NRF_REG_EN_RXADDR 0x02
#define NRF_REG_SETUP_AW 0x03
#define NRF_REG_SETUP_RETR 0x04
#define NRF_REG_RF_CH 0x05
#define NRF_REG_RF_SETUP 0x06
#define NRF_REG_STATUS 0x07
#define NRF_REG_RX_ADDR_P0 0x0A
#define NRF_REG_RX_ADDR_P1 0x0B
#define NRF_REG_RX_PW_P0 0x11
#define NRF_REG_RX_PW_P1 0x12
#define NRF_REG_FIFO_STATUS 0x17
#define NRF_REG_DYNPD 0x1C
#define NRF_REG_FEATURE 0x1D

// Commands
#define NRF_CMD_R_REGISTER 0x00
#define NRF_CMD_W_REGISTER 0x20
#define NRF_CMD_R_RX_PAYLOAD 0x61
#define NRF_CMD_FLUSH_RX 0xE2

// Bit helpers
#define NRF_STATUS_RX_DR (1 << 6)
#define NRF_FIFO_RX_EMPTY 0x01

static xiaomi_scan_result_t last_scan_result = {0};

typedef struct {
    uint32_t id;
    uint16_t cmd;
    uint8_t seq;
} xiaomi_packet_t;

typedef enum {
    XCMD_UNKNOWN = 0,
    XCMD_ONOFF = 1,
    XCMD_COOLER = 2,
    XCMD_WARMER = 3,
    XCMD_HIGHER = 4,
    XCMD_LOWER = 5,
    XCMD_RESET = 6,
} xiaomi_cmd_t;

/// @brief Writes a value to a single register of the nRF24L01+ module
/// @param reg The register address to write to
/// @param value The byte value to write to the register
/// @param status Pointer to store the status byte returned by the nRF24L01+ (can be NULL if not needed)
/// @return ESP_OK on success, error code on failure
static esp_err_t nrf24_write_register(uint8_t reg, uint8_t value, uint8_t* status) {
    uint8_t tx_data[2] = {(uint8_t)(0x20 | (reg & 0x1F)), value};
    uint8_t rx_data[2] = {0};

    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    esp_err_t err = spi_device_transmit(nrf_spi, &t);
    if (err != ESP_OK) {
        return err;
    }
    if (status) {
        *status = rx_data[0];
    }

    return ESP_OK;
}

/// @brief Sends a command to the nRF24L01+ module
/// @param cmd The command byte to send
/// @param status Pointer to store the status byte returned by the nRF24L01+ (can be NULL if not needed)
/// @return ESP_OK on success, error code on failure
static esp_err_t nrf24_command(uint8_t cmd, uint8_t* status) {
    uint8_t rx = 0;
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
        .rx_buffer = &rx,
    };
    esp_err_t err = spi_device_transmit(nrf_spi, &t);
    if (err != ESP_OK) {
        return err;
    }
    if (status) {
        *status = rx;
    }

    return ESP_OK;
}

/// @brief Writes multiple bytes to a specified register on the nRF24L01+ device
/// @param reg The register address to write to
/// @param data Pointer to the data buffer containing bytes to write
/// @param len The number of bytes to write (maximum 5)
/// @return ESP_OK on success, error code on failure
static esp_err_t nrf24_write_register_buf(uint8_t reg, const uint8_t* data, size_t len) {
    uint8_t tx_data[1 + 5] = {0};
    uint8_t rx_data[1 + 5] = {0};
    if (len > 5) {
        return ESP_ERR_INVALID_ARG;
    }
    tx_data[0] = NRF_CMD_W_REGISTER | (reg & 0x1F);
    for (size_t i = 0; i < len; i++) {
        tx_data[1 + i] = data[i];
    }

    spi_transaction_t t = {
        .length = (1 + len) * 8,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    return spi_device_transmit(nrf_spi, &t);
}

/// @brief Initializes the NRF24L01+ wireless transceiver module
/// @return ESP_OK on success, error code on failure
static esp_err_t nrf24_init_spi(void) {
    if (nrf24_mutex == NULL) {
        nrf24_mutex = xSemaphoreCreateMutex();
        if (nrf24_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create nrf24 mutex");
            return ESP_ERR_NO_MEM;
        }
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(PIN_NUM_CE),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(PIN_NUM_CE, 0);

    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000,  // 1 MHz for bring-up
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };

    esp_err_t err;
    if (!spi_bus_initialized) {
        err = spi_bus_initialize(NRF_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "SPI bus init failed: %d", err);
            return err;
        }
        spi_bus_initialized = true;
    }

    if (nrf_spi == NULL) {
        err = spi_bus_add_device(NRF_SPI_HOST, &devcfg, &nrf_spi);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "Device add failed: %d", err);
            return err;
        }
    }

    return ESP_OK;
}

/// @brief Writes a single byte value to a specified register on the nRF24L01+ device
/// @param reg The register address to write to
/// @param value The byte value to write to the register
/// @param status Pointer to store the status byte returned by the device (can be NULL if not needed)
/// @return ESP_OK on success, error code on failure
static esp_err_t nrf24_read_register(uint8_t reg, uint8_t* value, uint8_t* status) {
    uint8_t tx_data[2] = {(uint8_t)(reg & 0x1F), 0xFF};
    uint8_t rx_data[2] = {0};

    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    esp_err_t err = spi_device_transmit(nrf_spi, &t);
    if (err != ESP_OK) {
        return err;
    }

    if (value) {
        *value = rx_data[1];
    }
    if (status) {
        *status = rx_data[0];
    }

    return ESP_OK;
}

/// @brief Computes CRC-16-CCITT checksum for given data
/// @param data Pointer to the data buffer
/// @param len Length of the data buffer in bytes
/// @return Computed CRC-16 checksum
static uint16_t nrf24_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFE;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

/// @brief Searches for and decodes a Xiaomi packet from raw received data
/// @param raw Pointer to the raw data buffer
/// @param len Length of the raw data buffer in bytes
/// @param out Pointer to xiaomi_packet_t structure to store decoded packet (if found)
/// @return true if a valid packet was found and decoded, false otherwise
static bool nrf24_find_and_decode_packet(const uint8_t* raw, size_t len, xiaomi_packet_t* out) {
    if (!raw || !out || len < 17) return false;

    const uint8_t expected_preamble[8] = {0x53, 0x39, 0x14, 0xDD, 0x1C, 0x49, 0x34, 0x12};
    for (size_t byte_off = 0; byte_off + 17 <= len; byte_off++) {
        for (int shift = 0; shift <= 7; shift++) {
            uint8_t aligned[17];
            for (int i = 0; i < 17; i++) {
                uint8_t a = raw[byte_off + i];
                uint8_t b = (byte_off + i + 1 < len) ? raw[byte_off + i + 1] : 0;
                aligned[i] = (shift == 0) ? a : (uint8_t)((a << shift) | (b >> (8 - shift)));
            }

            bool preamble_ok = true;
            for (int i = 0; i < 8; i++) {
                if (aligned[i] != expected_preamble[i]) {
                    preamble_ok = false;
                    break;
                }
            }
            if (!preamble_ok || aligned[11] != 0xFF) continue;

            uint16_t crc_calc = nrf24_crc16(aligned, 15);
            uint16_t crc_rx = ((uint16_t)aligned[15] << 8) | aligned[16];
            if (crc_calc != crc_rx) continue;

            out->id = ((uint32_t)aligned[8] << 16) | ((uint32_t)aligned[9] << 8) | aligned[10];
            out->cmd = ((uint16_t)aligned[12] << 8) | aligned[13];
            out->seq = aligned[14];
            return true;
        }
    }

    return false;
}

/// @brief Classifies a Xiaomi command based on its command code
/// from https://github.com/lamperez/xiaomi-lightbar-nrf24?tab=readme-ov-file#command-codes
/// @param cmd The command code to classify
/// @return The classified xiaomi_cmd_t value
static xiaomi_cmd_t classify_cmd(uint16_t cmd) {
    uint8_t hi = (uint8_t)(cmd >> 8);
    uint8_t lo = (uint8_t)(cmd & 0xFF);
    if (hi == 0x01) return XCMD_ONOFF;
    if (hi == 0x06 || hi == 0x07) return XCMD_RESET;

    // Cooler ranges: 0x0201..0x020F or 0x0301..0x030F
    if ((hi == 0x02 && lo >= 0x01 && lo <= 0x0F) || (hi == 0x03 && lo >= 0x01 && lo <= 0x0F)) return XCMD_COOLER;
    // Warmer ranges: 0x02F1..0x02FF or 0x03F1..0x03FF
    if ((hi == 0x02 && lo >= 0xF1) || (hi == 0x03 && lo >= 0xF1)) return XCMD_WARMER;
    // Higher ranges: 0x0401..0x040F or 0x0501..0x050F
    if ((hi == 0x04 && lo >= 0x01 && lo <= 0x0F) || (hi == 0x05 && lo >= 0x01 && lo <= 0x0F)) return XCMD_HIGHER;
    // Lower ranges: 0x04F1..0x04FF or 0x05F1..0x05FF
    if ((hi == 0x04 && lo >= 0xF1) || (hi == 0x05 && lo >= 0xF1)) return XCMD_LOWER;

    return XCMD_UNKNOWN;
}

/// @brief Checks the connection to the NRF24L01+ module by reading and writing its CONFIG register
/// @return ESP_OK on success, error code on failure
esp_err_t nrf24_check_connection(void) {
    esp_err_t err = nrf24_init_spi();
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(5));

    uint8_t status_nop = 0;
    uint8_t nop_cmd = 0xFF;
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &nop_cmd,
        .rx_buffer = &status_nop,
    };
    err = spi_device_transmit(nrf_spi, &t);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NOP failed: %d", err);
        return err;
    }

    uint8_t config = 0;
    uint8_t status = 0;
    err = nrf24_read_register(0x00, &config, &status);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CONFIG read failed: %d", err);
        return err;
    }

    ESP_LOGI(TAG, "status(nop)=0x%02X status(read)=0x%02X CONFIG=0x%02X (expected default 0x08)", status_nop, status,
             config);

    if (config == 0x00 || config == 0xFF || status_nop == 0x00 || status_nop == 0xFF) {
        ESP_LOGW(TAG, "CONFIG/status suspect; tentative write+readback to confirm SPI");

        uint8_t write_status = 0;
        err = nrf24_write_register(0x00, 0x0B, &write_status);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "CONFIG write failed: %d", err);
            return err;
        }

        uint8_t config_after = 0;
        uint8_t status_after = 0;
        err = nrf24_read_register(0x00, &config_after, &status_after);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "CONFIG readback failed: %d", err);
            return err;
        }

        ESP_LOGI(TAG, "After write: status=0x%02X write_status=0x%02X CONFIG=0x%02X", status_after, write_status,
                 config_after);

        if (config_after != 0x0B) {
            ESP_LOGW(TAG,
                     "Write/readback mismatch; check wiring (VCC 3.3V, GND, CE=4, CSN=5, SCK=18, MOSI=23, MISO=19) and "
                     "add 10uF near module");
            return ESP_FAIL;
        }
    } else if (config == 0x08) {
        ESP_LOGI(TAG, "NRF24 appears responsive; wiring/power OK");
    }

    return ESP_OK;
}

/// @brief Reads the payload data from the nRF24L01+ module
/// @param data Pointer to the buffer to store the received payload
/// @param len The length of the payload to read (maximum 32 bytes)
/// @return Returns ESP_OK on success, or an error code on failure
static esp_err_t nrf24_read_payload(uint8_t* data, size_t len) {
    if (len > 32) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t tx[1 + 32] = {0};
    uint8_t rx[1 + 32] = {0};
    tx[0] = NRF_CMD_R_RX_PAYLOAD;
    for (size_t i = 0; i < len; i++) {
        tx[1 + i] = 0xFF;
    }
    spi_transaction_t t = {
        .length = (len + 1) * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    esp_err_t err = spi_device_transmit(nrf_spi, &t);
    if (err != ESP_OK) {
        return err;
    }
    for (size_t i = 0; i < len; i++) {
        data[i] = rx[1 + i];
    }

    return ESP_OK;
}

/// @brief Quick scan for Xiaomi lightbar patterns and save results for API access
/// @param duration_ms Duration of scan in milliseconds (e.g., 10000 for 10 seconds)
/// @return ESP_OK if patterns found, ESP_ERR_NOT_FOUND otherwise
esp_err_t nrf24_scan_xiaomi(uint32_t duration_ms) {
    if (nrf24_mutex == NULL || xSemaphoreTake(nrf24_mutex, pdMS_TO_TICKS(5000)) == pdFALSE) {
        ESP_LOGE(TAG, "Failed to acquire nrf24 mutex");
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err;

    memset(&last_scan_result, 0, sizeof(last_scan_result));
    last_scan_result.last_scan_time = esp_log_timestamp();
    last_scan_result.id_found = 0;
    last_scan_result.remote_id = 0;
    last_scan_result.commands_mask = 0;

    ESP_LOGI(TAG, "Starting quick Xiaomi scan for %u ms", duration_ms);

    nrf24_write_register(NRF_REG_EN_AA, 0x00, NULL);
    nrf24_write_register(NRF_REG_SETUP_RETR, 0x00, NULL);
    nrf24_write_register(NRF_REG_EN_RXADDR, 0x01, NULL);
    nrf24_write_register(NRF_REG_SETUP_AW, 0x03, NULL);
    nrf24_write_register(NRF_REG_DYNPD, 0x00, NULL);
    nrf24_write_register(NRF_REG_FEATURE, 0x00, NULL);
    nrf24_write_register(NRF_REG_RF_SETUP, 0x0E, NULL);
    nrf24_write_register(NRF_REG_RX_PW_P0, 32, NULL);
    nrf24_write_register_buf(NRF_REG_RX_ADDR_P0, (const uint8_t[]){0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, 5);
    nrf24_command(NRF_CMD_FLUSH_RX, NULL);
    nrf24_write_register(NRF_REG_CONFIG, 0x03, NULL);

    vTaskDelay(pdMS_TO_TICKS(5));

    static const uint8_t channels[] = {6, 15, 43, 68};
    uint32_t start_ms = esp_log_timestamp();

    for (;;) {
        uint32_t elapsed = esp_log_timestamp() - start_ms;
        if (elapsed >= duration_ms) {
            break;
        }

        for (size_t c = 0; c < sizeof(channels); c++) {
            nrf24_write_register(NRF_REG_RF_CH, channels[c], NULL);
            nrf24_command(NRF_CMD_FLUSH_RX, NULL);
            gpio_set_level(PIN_NUM_CE, 1);
            vTaskDelay(pdMS_TO_TICKS(20));

            uint8_t status = 0;
            nrf24_read_register(NRF_REG_STATUS, &status, NULL);
            if ((status & NRF_STATUS_RX_DR) == 0) {
                gpio_set_level(PIN_NUM_CE, 0);
                continue;
            }

            ESP_LOGI(TAG, "Data detected on channel %u (status=0x%02X)", channels[c], status);

            uint8_t fifo = 0;
            xiaomi_packet_t pkt;
            do {
                static uint8_t raw[32] = {0};
                err = nrf24_read_payload(raw, sizeof(raw));
                if (err != ESP_OK) {
                    gpio_set_level(PIN_NUM_CE, 0);
                    xSemaphoreGive(nrf24_mutex);
                    return err;
                }

                if (nrf24_find_and_decode_packet(raw, sizeof(raw), &pkt)) {
                    last_scan_result.found_count++;
                    last_scan_result.remote_id = pkt.id;
                    last_scan_result.id_found = 1;
                    xiaomi_cmd_t kind = classify_cmd(pkt.cmd);
                    switch (kind) {
                        case XCMD_ONOFF:
                            last_scan_result.commands_mask |= (1 << 0);
                            break;
                        case XCMD_COOLER:
                            last_scan_result.commands_mask |= (1 << 1);
                            break;
                        case XCMD_WARMER:
                            last_scan_result.commands_mask |= (1 << 2);
                            break;
                        case XCMD_HIGHER:
                            last_scan_result.commands_mask |= (1 << 3);
                            break;
                        case XCMD_LOWER:
                            last_scan_result.commands_mask |= (1 << 4);
                            break;
                        case XCMD_RESET:
                            last_scan_result.commands_mask |= (1 << 5);
                            break;
                        default:
                            break;
                    }
                    ESP_LOGD(TAG, "Decoded pkt: ID=0x%06X CMD=0x%04X SEQ=%u mask=0x%02X", pkt.id, pkt.cmd, pkt.seq,
                             last_scan_result.commands_mask);
                }

                if ((last_scan_result.commands_mask & 0x3F) == 0x3F) {
                    ESP_LOGI(TAG, "All six Xiaomi command patterns observed");
                    nrf24_read_register(NRF_REG_FIFO_STATUS, &fifo, NULL);
                    while ((fifo & NRF_FIFO_RX_EMPTY) == 0) {
                        nrf24_read_payload(raw, sizeof(raw));
                        nrf24_read_register(NRF_REG_FIFO_STATUS, &fifo, NULL);
                    }
                    nrf24_write_register(NRF_REG_STATUS, NRF_STATUS_RX_DR, NULL);
                    gpio_set_level(PIN_NUM_CE, 0);
                    xSemaphoreGive(nrf24_mutex);
                    return ESP_OK;
                }

                nrf24_read_register(NRF_REG_FIFO_STATUS, &fifo, NULL);
            } while ((fifo & NRF_FIFO_RX_EMPTY) == 0);

            nrf24_write_register(NRF_REG_STATUS, NRF_STATUS_RX_DR, NULL);
            gpio_set_level(PIN_NUM_CE, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    ESP_LOGI(TAG, "Quick Xiaomi scan complete. Found %u packets%s", last_scan_result.found_count,
             last_scan_result.id_found ? ", ID decoded" : "");

    xSemaphoreGive(nrf24_mutex);

    return last_scan_result.id_found ? ESP_OK : (last_scan_result.found_count > 0 ? ESP_OK : ESP_ERR_NOT_FOUND);
}

/// @brief Get last scan result (for API access)
/// @return Pointer to last scan result structure
const xiaomi_scan_result_t* nrf24_get_last_scan_result(void) { return &last_scan_result; }