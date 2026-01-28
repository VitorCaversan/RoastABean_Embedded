#ifndef BLUETOOTH_H
#define BLUETOOTH_H

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "OSConfig.h"
#include "JsonHndlr.h"
#include "NVShndlr.h"
#include "TimeUtils.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/
#define BLE_DEVICE_NAME         "RoastABean"

// Nordic UART Service (NUS) UUIDs
#define BLE_SVC_NUS_UUID16                                  0x0001
#define BLE_SVC_NUS_CHR_RX_UUID16                          0x0002  // Write (phone -> ESP32)
#define BLE_SVC_NUS_CHR_TX_UUID16                          0x0003  // Notify (ESP32 -> phone)
#define BLE_SVC_NUS_UUID128                                 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E
#define BLE_SVC_NUS_CHR_RX_UUID128                         0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E
#define BLE_SVC_NUS_CHR_TX_UUID128                         0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Bluetooth task to handle BLE events
 */
extern void bluetooth_task(void *arg);

/**
 * @brief Initialize Bluetooth (NimBLE stack)
 */
extern void bluetooth_init(void);

/**
 * @brief Start BLE advertising
 */
extern void bluetooth_startAdvertising(void);

/**
 * @brief Stop BLE advertising
 */
extern void bluetooth_stopAdvertising(void);

/**
 * @brief Disconnect from the currently connected device
 * 
 * Terminates the active BLE connection if one exists.
 * Does nothing if no device is connected.
 */
extern void bluetooth_disconnect(void);

/**
 * @brief Check if a device is connected
 * 
 * @return true if connected, false otherwise
 */
extern bool bluetooth_isConnected(void);

/**
 * @brief Get received JSON data (call this after receiving BLE_EVENT_DATA_RECEIVED)
 * 
 * @param buffer Pointer to buffer where data will be copied
 * @param bufferSize Size of the provided buffer
 * @return Number of bytes copied, 0 if no data available
 */
extern uint16_t bluetooth_getReceivedData(uint8_t *buffer, uint16_t bufferSize);

/**
 * @brief Send data to connected device via NUS TX characteristic
 * 
 * @param data Pointer to data to send
 * @param dataLen Length of data in bytes
 * @return 0 on success, error code otherwise
 */
extern int bluetooth_sendData(const uint8_t *data, uint16_t dataLen);

/**
 * @brief Check all stored charts and send pending feedback
 * 
 * Iterates through all stored charts, checks if isFeedbackSent is false,
 * sends feedback JSON for those charts, and marks them as sent.
 */
extern void bluetooth_sendPendingFeedback(void);

#endif // BLUETOOTH_H
