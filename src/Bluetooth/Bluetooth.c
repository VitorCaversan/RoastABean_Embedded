/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "Bluetooth.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/
static void onSync(void);
static void onReset(int reason);
static int gapEvent(struct ble_gap_event *event, void *arg);
static void hostTask(void *param);
static int gattCharAccessCb(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg);

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/
static const char *TAG = "BLUETOOTH";
static bool isConnected = false;
static uint16_t connHandle = 0;
static uint8_t receivedData[BLE_MAX_DATA_LEN];
static uint16_t receivedDataLen = 0;

// Nordic UART Service (NUS) UUIDs
static const ble_uuid128_t nusSvcUuid =
    BLE_UUID128_INIT(BLE_SVC_NUS_UUID128);

static const ble_uuid128_t nusChrRxUuid =
    BLE_UUID128_INIT(BLE_SVC_NUS_CHR_RX_UUID128);

static const ble_uuid128_t nusChrTxUuid =
    BLE_UUID128_INIT(BLE_SVC_NUS_CHR_TX_UUID128);

static uint16_t nusRxHandle;
static uint16_t nusTxHandle;

static const struct ble_gatt_svc_def gattSvrSvcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &nusSvcUuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                // RX characteristic - phone writes data to ESP32
                .uuid = &nusChrRxUuid.u,
                .access_cb = gattCharAccessCb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                .val_handle = &nusRxHandle,
            },
            {
                // TX characteristic - ESP32 notifies phone
                .uuid = &nusChrTxUuid.u,
                .access_cb = gattCharAccessCb,
                .flags = BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &nusTxHandle,
            },
            {0} // No more characteristics
        }
    },
    {0} // No more services
};

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

extern void bluetooth_task(void *arg)
{
    ST_bleMsg msg = {0};

    while (1)
    {
        if (xQueueReceive(OS_bleEventQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            switch (msg.event)
            {
                case BLE_EVENT_CONNECTED:
                    ESP_LOGI(TAG, "Device connected");
                break;
                case BLE_EVENT_DISCONNECTED:
                    ESP_LOGI(TAG, "Device disconnected");
                break;
                case BLE_EVENT_DATA_RECEIVED:
                    ESP_LOGI(TAG, "Data received over BLE, length: %d bytes", msg.dataLen);
                break;
                default:
                    ESP_LOGW(TAG, "Unknown BLE event: %d", msg.event);
                break;
            }
        }
    }
    
}

extern void bluetooth_init(void)
{
    ESP_LOGI(TAG, "Initializing NimBLE");
    
    // NVS already initialized by nvs_init() in main
    
    // Initialize NimBLE
    ESP_ERROR_CHECK(nimble_port_init());
    
    // Initialize the NimBLE host configuration
    ble_hs_cfg.sync_cb = onSync;
    ble_hs_cfg.reset_cb = onReset;
    
    // Set device name
    ble_svc_gap_device_name_set(BLE_DEVICE_NAME);
    
    // Initialize GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();
    
    // Register Nordic UART Service (NUS)
    int rc = ble_gatts_count_cfg(gattSvrSvcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to count GATT configuration: %d", rc);
        return;
    }
    
    rc = ble_gatts_add_svcs(gattSvrSvcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to add GATT services: %d", rc);
        return;
    }
    
    ESP_LOGI(TAG, "Nordic UART Service registered");
    
    // Start NimBLE host task
    nimble_port_freertos_init(hostTask);
    
    ESP_LOGI(TAG, "NimBLE initialized");
}

extern void bluetooth_startAdvertising(void)
{
    struct ble_gap_adv_params advParams;
    struct ble_hs_adv_fields advertiseFields;
    struct ble_hs_adv_fields responseFields;
    
    memset(&advParams, 0, sizeof(advParams));
    memset(&advertiseFields, 0, sizeof(advertiseFields));
    memset(&responseFields, 0, sizeof(responseFields));
    
    // Set advertising parameters
    advParams.conn_mode = BLE_GAP_CONN_MODE_UND;
    advParams.disc_mode = BLE_GAP_DISC_MODE_GEN;
    
    // Set advertising data (31 bytes max)
    advertiseFields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    
    // Advertise NUS service UUID in advertising packet
    advertiseFields.uuids128 = &nusSvcUuid;
    advertiseFields.num_uuids128 = 1;
    advertiseFields.uuids128_is_complete = 1;
    
    ble_gap_adv_set_fields(&advertiseFields);
    
    // Put device name in scan response data (separate 31-byte packet)
    responseFields.name = (uint8_t *)BLE_DEVICE_NAME;
    responseFields.name_len = strlen(BLE_DEVICE_NAME);
    responseFields.name_is_complete = 1;
    
    ble_gap_adv_rsp_set_fields(&responseFields);
    
    // Start advertising
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                      &advParams, gapEvent, NULL);
    
    ESP_LOGI(TAG, "Advertising started with NUS UUID and device name");
}

extern void bluetooth_stopAdvertising(void)
{
    ble_gap_adv_stop();
    ESP_LOGI(TAG, "Advertising stopped");
}

extern bool bluetooth_isConnected(void)
{
    return isConnected;
}

extern uint16_t bluetooth_getReceivedData(uint8_t *buffer, uint16_t bufferSize)
{
    if (buffer == NULL || bufferSize == 0 || receivedDataLen == 0) {
        return 0;
    }
    
    uint16_t copyLen = (receivedDataLen < bufferSize) ? receivedDataLen : bufferSize;
    memcpy(buffer, receivedData, copyLen);
    
    return copyLen;
}

extern int bluetooth_sendData(const uint8_t *data, uint16_t dataLen)
{
    if (!isConnected) {
        ESP_LOGW(TAG, "Cannot send data: device not connected");
        return -1;
    }
    
    if (data == NULL || dataLen == 0) {
        ESP_LOGW(TAG, "Invalid data or length");
        return -1;
    }
    
    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, dataLen);
    if (om == NULL) {
        ESP_LOGE(TAG, "Failed to allocate mbuf for notification");
        return -1;
    }
    
    int rc = ble_gattc_notify_custom(connHandle, nusTxHandle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to send notification: %d", rc);
        return rc;
    }
    
    ESP_LOGI(TAG, "Sent %d bytes via NUS TX", dataLen);
    return 0;
}

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

static void onSync(void)
{
    ESP_LOGI(TAG, "BLE Host synchronized");
}

static void onReset(int reason)
{
    ESP_LOGE(TAG, "BLE Host reset, reason: %d", reason);
}

static int gapEvent(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "Connection %s; status=%d",
                     event->connect.status == 0 ? "established" : "failed",
                     event->connect.status);
            
            if (event->connect.status == 0) {
                isConnected = true;
                connHandle = event->connect.conn_handle;
                
                // Send connected event to queue
                if (OS_bleEventQueue != NULL) {
                    ST_bleMsg msg = {.event = BLE_EVENT_CONNECTED, .dataLen = 0};
                    xQueueSend(OS_bleEventQueue, &msg, 0);
                }
            } else {
                // Connection failed, resume advertising
                bluetooth_startAdvertising();
            }
            break;
            
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Disconnected; reason=%d", event->disconnect.reason);
            isConnected = false;
            connHandle = 0;
            
            // Send disconnected event to queue
            if (OS_bleEventQueue != NULL) {
                ST_bleMsg msg = {.event = BLE_EVENT_DISCONNECTED, .dataLen = 0};
                xQueueSend(OS_bleEventQueue, &msg, 0);
            }
            break;
            
        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "Advertising complete");
            bluetooth_startAdvertising();
            break;
            
        case BLE_GAP_EVENT_MTU:
            ESP_LOGI(TAG, "MTU update event; conn_handle=%d mtu=%d",
                     event->mtu.conn_handle, event->mtu.value);
            break;
            
        default:
            break;
    }
    
    return 0;
}

static void hostTask(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static int gattCharAccessCb(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            ESP_LOGI(TAG, "Data received: %d bytes", OS_MBUF_PKTLEN(ctxt->om));
            
            // Get the length of received data
            uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
            
            if (len > BLE_MAX_DATA_LEN) {
                ESP_LOGW(TAG, "Received data too large: %d bytes, max: %d", len, BLE_MAX_DATA_LEN);
                len = BLE_MAX_DATA_LEN;
            }
            
            // Copy data from mbuf to our buffer
            receivedDataLen = 0;
            struct os_mbuf *om = ctxt->om;
            while (om != NULL && receivedDataLen < len) {
                uint16_t copyLen = om->om_len;
                if (receivedDataLen + copyLen > BLE_MAX_DATA_LEN) {
                    copyLen = BLE_MAX_DATA_LEN - receivedDataLen;
                }
                memcpy(&receivedData[receivedDataLen], om->om_data, copyLen);
                receivedDataLen += copyLen;
                om = SLIST_NEXT(om, om_next);
            }
            
            // Null-terminate if it's JSON string data
            if (receivedDataLen < BLE_MAX_DATA_LEN) {
                receivedData[receivedDataLen] = '\0';
            }
            
            ESP_LOGI(TAG, "JSON data stored: %d bytes", receivedDataLen);
            
            // Send data received event to queue
            if (OS_bleEventQueue != NULL) {
                ST_bleMsg msg = {.event = BLE_EVENT_DATA_RECEIVED, .dataLen = receivedDataLen};
                xQueueSend(OS_bleEventQueue, &msg, 0);
            }
            
            return 0;
            
        default:
            ESP_LOGW(TAG, "Unsupported GATT operation: %d", ctxt->op);
            return BLE_ATT_ERR_UNLIKELY;
    }
}
