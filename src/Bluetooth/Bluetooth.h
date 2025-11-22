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

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/
#define BLE_DEVICE_NAME         "RoastABean"
#define BLE_MAX_DATA_LEN        2048

// Custom GATT Service UUID (128-bit): 12345678-1234-5678-1234-56789abcdef0
#define ROASTABEAN_SERVICE_UUID     0xDEF0, 0x9ABC, 0x5678, 0x1234, 0x5678, 0x1234, 0x5678, 0x1234
// Characteristic UUID for JSON data: 12345678-1234-5678-1234-56789abcdef1
#define ROASTABEAN_CHAR_JSON_UUID   0xDEF1, 0x9ABC, 0x5678, 0x1234, 0x5678, 0x1234, 0x5678, 0x1234

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

typedef enum {
    BLE_EVENT_CONNECTED = 0,
    BLE_EVENT_DISCONNECTED,
    BLE_EVENT_DATA_RECEIVED
} EN_bleEvent;

typedef struct {
    EN_bleEvent event;
    uint16_t dataLen;  // For DATA_RECEIVED event
} ST_bleMsg;

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Initialize Bluetooth (NimBLE stack)
 * 
 * @param eventQueue Queue handle where BLE events will be posted (can be NULL if not needed)
 */
extern void bluetooth_init(QueueHandle_t eventQueue);

/**
 * @brief Start BLE advertising
 */
extern void bluetooth_startAdvertising(void);

/**
 * @brief Stop BLE advertising
 */
extern void bluetooth_stopAdvertising(void);

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

#endif // BLUETOOTH_H
