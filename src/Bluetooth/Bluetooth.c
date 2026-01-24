/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "Bluetooth.h"
#include "SpiConfig.h"
#include "esp_heap_caps.h"

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

/**
 * @brief Checks is entire payload is received and treats it if so
 * 
 * @param data Pointer to received data
 * @param len Length of the received data
 */
static void treatReceivedPayload(const uint8_t *data, uint16_t len);

/**
 * @brief Parses the received json data and takes appropriate actions
 * 
 * @param data Pointer to received data in JSON format
 * @param len Length of the received data
 * @return true if data was treated successfully, false otherwise
 */
static bool treatReceivedJson(const uint8_t *data, uint16_t len);

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/
static const char *TAG = "BLUETOOTH";
static bool isConnected = false;
static uint16_t connHandle = 0;

// Static buffer for accumulating BLE data (shared, not on stack)
static uint8_t totalReceivedData[2048];
static uint16_t totalReceivedDataLen = 0;
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
                    ESP_LOGI(TAG, "Received data: %.*s", msg.dataLen, receivedData);
                    treatReceivedPayload(receivedData, msg.dataLen);
                break;
                case BLE_EVENT_SEND_PENDING_FEEDBACK:
                    vTaskDelay(pdMS_TO_TICKS(3000));
                    ESP_LOGI(TAG, "Sending pending feedback");
                    bluetooth_sendPendingFeedback();
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
    ESP_LOGI(TAG, "Initializing NimBLE - Free heap: %lu bytes", esp_get_free_heap_size());
    
    // NVS already initialized by nvs_init() in main
    
    // Free SPI bus before BLE init to avoid cache conflicts during PHY calibration
    spiConfig_freeSpiBus();

    // Initialize NimBLE
    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init() failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "NimBLE port initialized, free heap: %lu bytes", esp_get_free_heap_size());
    
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
    
    int rc = ble_gatts_notify_custom(connHandle, nusTxHandle, om);
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
    
    // Start advertising when host is ready
    bluetooth_startAdvertising();
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
                
                // Send pending feedback event to be handled in bluetooth_task context (with larger stack)
                if (OS_bleEventQueue != NULL) {
                    ST_bleMsg msg = {.event = BLE_EVENT_SEND_PENDING_FEEDBACK, .dataLen = 0};
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
            
            ESP_LOGI(TAG, "Data stored: %d bytes", receivedDataLen);
            
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

static void treatReceivedPayload(const uint8_t *data, uint16_t len)
{
    if (totalReceivedDataLen + len >= sizeof(totalReceivedData))
    {
        ESP_LOGE(TAG, "Total received data buffer overflow, resetting");
        totalReceivedDataLen = 0;
        return;
    }

    memcpy(&totalReceivedData[totalReceivedDataLen], data, len);
    totalReceivedDataLen += len;
    
    totalReceivedData[totalReceivedDataLen] = '\0';
    
    ESP_LOGI(TAG, "Accumulated %d bytes so far", totalReceivedDataLen);

    int braceCount = 0;
    bool foundOpenBrace = false;
    
    for (uint16_t i = 0; i < totalReceivedDataLen; i++)
    {
        if (totalReceivedData[i] == '{')
        {
            braceCount++;
            foundOpenBrace = true;
        }
        else if (totalReceivedData[i] == '}')
        {
            braceCount--;
        }
    }

    if (foundOpenBrace && braceCount == 0)
    {
        ESP_LOGI(TAG, "Complete JSON received (%d bytes), processing...", totalReceivedDataLen);
        
        if (treatReceivedJson(totalReceivedData, totalReceivedDataLen))
        {
            ESP_LOGI(TAG, "JSON processed successfully");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to process JSON");
        }
        
        totalReceivedDataLen = 0;
        memset(totalReceivedData, 0, sizeof(totalReceivedData));
    }
    else if (braceCount < 0)
    {
        ESP_LOGE(TAG, "Invalid JSON structure detected, resetting buffer");
        totalReceivedDataLen = 0;
        memset(totalReceivedData, 0, sizeof(totalReceivedData));
    }
    else
    {
        ESP_LOGI(TAG, "Waiting for more data (brace count: %d)", braceCount);
    }
}

static bool treatReceivedJson(const uint8_t *data, uint16_t len)
{
    ST_storedChart *chart = heap_caps_malloc(sizeof(ST_storedChart), MALLOC_CAP_8BIT);
    if (chart == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for chart");
        return false;
    }
    memset(chart, 0, sizeof(ST_storedChart));

    if (!json_jsonToStoredChart((const char *)data, chart))
    {
        free(chart);
        return false;
    }

    esp_err_t err = nvs_saveRoastProfile(nvs_getProfileCount(), (const char *)data, len);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save roast profile to NVS: %d", err);
        free(chart);
        return false;
    }

    if (chart->isScheduled)
    {
        time_t currentTime = 0;
        if (!timeUtils_parseIso8601(chart->currentTime, &currentTime))
        {
            ESP_LOGE(TAG, "Failed to parse current time");
            free(chart);
            return false;
        }

        time_t scheduledTime = 0;
        if (!timeUtils_parseIso8601(chart->scheduledTime, &scheduledTime))
        {
            ESP_LOGE(TAG, "Failed to parse scheduled time");
            free(chart);
            return false;
        }

        ESP_LOGI(TAG, "Current time: %s", chart->currentTime);
        ESP_LOGI(TAG, "Scheduled time: %s", chart->scheduledTime);
        ESP_LOGI(TAG, "Current time (epoch): %ld", currentTime);
        ESP_LOGI(TAG, "Scheduled time (epoch): %ld", scheduledTime);
        if (currentTime >= scheduledTime)
        {
            ESP_LOGW(TAG, "Scheduled time is in the past current time");
            free(chart);
            return false;
        }

        // Allocate on heap to safely pass through queue
        int32_t *secsToStartRoast = heap_caps_malloc(sizeof(int32_t), MALLOC_CAP_8BIT);
        if (secsToStartRoast == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate memory for delay");
            free(chart);
            return false;
        }
        *secsToStartRoast = timeUtils_diffSeconds(scheduledTime, currentTime);

        ST_extEventMsg screenMsg = {0};
        screenMsg.event = EXT_EVENT_SCHEDULE_ROAST;
        screenMsg.data = secsToStartRoast;  // Receiver MUST free this!

        if (xQueueSend(OS_btnHndlrsTaskQueue, &screenMsg, pdMS_TO_TICKS(100)) != pdTRUE)
        {
            ESP_LOGE(TAG, "Failed to send schedule roast event");
            free(secsToStartRoast);
            free(chart);
            return false;
        }
    }
    else
    {
        ST_extEventMsg screenMsg = {0};
        screenMsg.event = EXT_EVENT_5S_TIMER_TO_START_ROAST;
        screenMsg.data = NULL;

        if (xQueueSend(OS_btnHndlrsTaskQueue, &screenMsg, pdMS_TO_TICKS(100)) != pdTRUE)
        {
            ESP_LOGE(TAG, "Failed to send start roast event");
            free(chart);
            return false;
        }
    }

    free(chart);
    return true;
}

extern void bluetooth_sendPendingFeedback(void)
{
    ESP_LOGI(TAG, "Checking for pending feedback to send");
    
    uint8_t profileCount = nvs_getProfileCount();
    ESP_LOGI(TAG, "Found %d stored profiles", profileCount);
    
    for (uint8_t i = 0; i < profileCount; i++)
    {
        // Load profile from NVS
        char *buffer = calloc(NVS_MAX_PROFILE_SIZE, sizeof(char));
        if (buffer == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for loading profile %d", i);
            continue;
        }
        
        size_t loadedLen = 0;
        esp_err_t ret = nvs_loadRoastProfile(i, buffer, NVS_MAX_PROFILE_SIZE, &loadedLen);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to load profile %d: %s", i, esp_err_to_name(ret));
            free(buffer);
            continue;
        }
        
        // Parse JSON to chart structure
        ST_storedChart chart = {0};
        if (!json_jsonToStoredChart(buffer, &chart)) {
            ESP_LOGE(TAG, "Failed to parse profile %d", i);
            free(buffer);
            continue;
        }
        
        // Check if feedback needs to be sent
        if (!chart.isFeedbackSent) {
            ESP_LOGI(TAG, "Profile %d '%s' has pending feedback", i, chart.chartName);
            
            // Generate feedback JSON
            char *feedbackJson = calloc(JSON_MAX_SIZE, sizeof(char));
            if (feedbackJson == NULL) {
                ESP_LOGE(TAG, "Failed to allocate memory for feedback JSON");
                free(buffer);
                continue;
            }
            
            if (json_generateFeedbackJson(&chart, feedbackJson, JSON_MAX_SIZE)) {
                // Send feedback
                int sendResult = bluetooth_sendData((uint8_t *)feedbackJson, strlen(feedbackJson));
                if (sendResult == 0) {
                    ESP_LOGI(TAG, "Feedback sent for profile %d", i);
                    
                    // Mark as sent and save back to NVS
                    chart.isFeedbackSent = true;
                    if (json_storedChartToJson(&chart, buffer, NVS_MAX_PROFILE_SIZE)) {
                        ret = nvs_saveRoastProfile(i, buffer, strlen(buffer));
                        if (ret == ESP_OK) {
                            ESP_LOGI(TAG, "Profile %d updated with isFeedbackSent=true", i);
                        } else {
                            ESP_LOGE(TAG, "Failed to save updated profile %d: %s", i, esp_err_to_name(ret));
                        }
                    } else {
                        ESP_LOGE(TAG, "Failed to convert chart to JSON for profile %d", i);
                    }
                    
                    // Add delay between sends to avoid overwhelming the BLE stack
                    vTaskDelay(pdMS_TO_TICKS(100));
                } else {
                    ESP_LOGE(TAG, "Failed to send feedback for profile %d", i);
                }
            } else {
                ESP_LOGE(TAG, "Failed to generate feedback JSON for profile %d", i);
            }
            
            free(feedbackJson);
        }
        
        free(buffer);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    ESP_LOGI(TAG, "Finished checking for pending feedback");
}
