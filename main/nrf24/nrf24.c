#include "nrf24.h"

#include <stdint.h>

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "NRF24";

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

/// @brief Writes a value to a single register of the nRF24L01+ module
/// @param reg The register address to write to
/// @param value The byte value to write to the register
/// @param status Pointer to store the status byte returned by the nRF24L01+ (can be NULL if not needed)
/// @return Returns 0 on success, non-zero error code on failure
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

/// @brief Initializes the NRF24L01+ wireless transceiver module
/// @return Returns 0 on success, -1 on failure to initialize SPI communication,
///         -2 on failure to detect NRF24 module, or -3 on configuration error
static esp_err_t nrf24_init_spi(void) {
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

    esp_err_t err = spi_bus_initialize(NRF_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (err == ESP_ERR_INVALID_STATE) {
        err = ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %d", err);
        return err;
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
/// @return Returns 0 on success, or an error code on failure
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
