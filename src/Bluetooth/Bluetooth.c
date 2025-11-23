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
static void bluetooth_onSync(void);
static void bluetooth_onReset(int reason);
static int bluetooth_gapEvent(struct ble_gap_event *event, void *arg);
static void bluetooth_hostTask(void *param);
static int bluetooth_gattCharAccessCb(uint16_t conn_handle, uint16_t attr_handle,
                                      struct ble_gatt_access_ctxt *ctxt, void *arg);

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/
static const char *TAG = "BLUETOOTH";
static bool isConnected = false;
static uint16_t connHandle = 0;
static uint8_t receivedData[BLE_MAX_DATA_LEN];
static uint16_t receivedDataLen = 0;

// GATT service definition
static const ble_uuid128_t gatt_svr_svc_uuid =
    BLE_UUID128_INIT(ROASTABEAN_SERVICE_UUID);

static const ble_uuid128_t gatt_svr_chr_json_uuid =
    BLE_UUID128_INIT(ROASTABEAN_CHAR_JSON_UUID);

static uint16_t gatt_svr_chr_json_handle;

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_svr_chr_json_uuid.u,
                .access_cb = bluetooth_gattCharAccessCb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                .val_handle = &gatt_svr_chr_json_handle,
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
    
    // Initialize NVS for bonding
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize NimBLE
    ESP_ERROR_CHECK(nimble_port_init());
    
    // Initialize the NimBLE host configuration
    ble_hs_cfg.sync_cb = bluetooth_onSync;
    ble_hs_cfg.reset_cb = bluetooth_onReset;
    
    // Set device name
    ble_svc_gap_device_name_set(BLE_DEVICE_NAME);
    
    // Initialize GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();
    
    // Register custom GATT service
    int rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to count GATT configuration: %d", rc);
        return;
    }
    
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to add GATT services: %d", rc);
        return;
    }
    
    // Start NimBLE host task
    nimble_port_freertos_init(bluetooth_hostTask);
    
    ESP_LOGI(TAG, "NimBLE initialized");
}

extern void bluetooth_startAdvertising(void)
{
    struct ble_gap_adv_params advParams;
    struct ble_hs_adv_fields fields;
    
    memset(&advParams, 0, sizeof(advParams));
    memset(&fields, 0, sizeof(fields));
    
    // Set advertising parameters
    advParams.conn_mode = BLE_GAP_CONN_MODE_UND;
    advParams.disc_mode = BLE_GAP_DISC_MODE_GEN;
    
    // Set advertising data
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)BLE_DEVICE_NAME;
    fields.name_len = strlen(BLE_DEVICE_NAME);
    fields.name_is_complete = 1;
    
    ble_gap_adv_set_fields(&fields);
    
    // Start advertising
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                      &advParams, bluetooth_gapEvent, NULL);
    
    ESP_LOGI(TAG, "Advertising started");
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

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

static void bluetooth_onSync(void)
{
    ESP_LOGI(TAG, "BLE Host synchronized");
    
    // Start advertising when host is ready
    bluetooth_startAdvertising();
}

static void bluetooth_onReset(int reason)
{
    ESP_LOGE(TAG, "BLE Host reset, reason: %d", reason);
}

static int bluetooth_gapEvent(struct ble_gap_event *event, void *arg)
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
            
            // Resume advertising after disconnect
            bluetooth_startAdvertising();
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

static void bluetooth_hostTask(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static int bluetooth_gattCharAccessCb(uint16_t conn_handle, uint16_t attr_handle,
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
