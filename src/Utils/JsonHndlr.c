/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "JsonHndlr.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static const char *TAG = "JSON_HNDLR";

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

extern bool json_jsonToStoredChart(const char *jsonStr, ST_storedChart *chart)
{
    if (jsonStr == NULL || chart == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return false;
    }

    // Initialize chart structure
    memset(chart, 0, sizeof(ST_storedChart));

    // Parse JSON string
    cJSON *root = cJSON_Parse(jsonStr);
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "JSON parse error before: %s", error_ptr);
        }
        return false;
    }

    bool success = true;

    // Parse curveName
    cJSON *curveName = cJSON_GetObjectItem(root, "curveName");
    if (cJSON_IsString(curveName) && (curveName->valuestring != NULL)) {
        strncpy(chart->chartName, curveName->valuestring, MAX_CHART_NAMESIZE - 1);
        chart->chartName[MAX_CHART_NAMESIZE - 1] = '\0';
    } else {
        ESP_LOGW(TAG, "curveName not found or invalid");
        strcpy(chart->chartName, "Unnamed");
    }

    // Parse pointsQuantity
    cJSON *pointsQuantity = cJSON_GetObjectItem(root, "pointsQuantity");
    if (cJSON_IsNumber(pointsQuantity)) {
        chart->totalPoints = (uint32_t)pointsQuantity->valueint;
        if (chart->totalPoints > MAX_ROAST_TIME_IN_MIN) {
            ESP_LOGW(TAG, "pointsQuantity (%lu) exceeds MAX_ROAST_TIME_IN_MIN (%d), truncating",
                     chart->totalPoints, MAX_ROAST_TIME_IN_MIN);
            chart->totalPoints = MAX_ROAST_TIME_IN_MIN;
        }
    } else {
        ESP_LOGE(TAG, "pointsQuantity not found or invalid");
        success = false;
    }

    // Parse temperatures array
    if (success) {
        cJSON *temperatures = cJSON_GetObjectItem(root, "temperatures");
        if (cJSON_IsArray(temperatures)) {
            int arraySize = cJSON_GetArraySize(temperatures);
            if (arraySize != (int)chart->totalPoints) {
                ESP_LOGW(TAG, "temperatures array size (%d) doesn't match pointsQuantity (%lu)",
                         arraySize, chart->totalPoints);
            }

            int minSize = (arraySize < (int)chart->totalPoints) ? arraySize : (int)chart->totalPoints;
            for (int i = 0; i < minSize; i++) {
                cJSON *temp = cJSON_GetArrayItem(temperatures, i);
                if (cJSON_IsNumber(temp)) {
                    chart->tempProfile[i] = (float)temp->valuedouble;
                } else {
                    ESP_LOGW(TAG, "Invalid temperature at index %d", i);
                    chart->tempProfile[i] = 0.0f;
                }
            }

            // Fill remaining with zeros if array was shorter
            for (int i = minSize; i < (int)chart->totalPoints; i++) {
                chart->tempProfile[i] = 0.0f;
            }
        } else {
            ESP_LOGE(TAG, "temperatures array not found or invalid");
            success = false;
        }
    }

    if (success) { // OPTIONAL: Parse achievedTemp array if present
        cJSON *achievedTemp = cJSON_GetObjectItem(root, "achievedTemperatures");
        if (cJSON_IsArray(achievedTemp)) {
            int arraySize = cJSON_GetArraySize(achievedTemp);
            if (arraySize != (int)chart->totalPoints) {
                ESP_LOGW(TAG, "achievedTemp array size (%d) doesn't match pointsQuantity (%lu)",
                         arraySize, chart->totalPoints);
            }

            int minSize = (arraySize < (int)chart->totalPoints) ? arraySize : (int)chart->totalPoints;
            for (int i = 0; i < minSize; i++) {
                cJSON *temp = cJSON_GetArrayItem(achievedTemp, i);
                if (cJSON_IsNumber(temp)) {
                    chart->achievedProfile[i] = (float)temp->valuedouble;
                } else {
                    ESP_LOGW(TAG, "Invalid temperature at index %d", i);
                    chart->achievedProfile[i] = 0.0f;
                }
            }

            // Fill remaining with zeros if array was shorter
            for (int i = minSize; i < (int)chart->totalPoints; i++) {
                chart->achievedProfile[i] = 0.0f;
            }
        } else {
            for (uint32_t i = 0; i < chart->totalPoints; i++){
                chart->achievedProfile[i] = 0.0f;
            }
            ESP_LOGW(TAG, "achievedTemp array not found or invalid");
        }
    }

    // Parse isScheduled
    if (success) {
        cJSON *isScheduled = cJSON_GetObjectItem(root, "isScheduled");
        if (cJSON_IsBool(isScheduled)) {
            chart->isScheduled = cJSON_IsTrue(isScheduled);
        } else {
            chart->isScheduled = false;
        }
    }

    // Parse scheduledTime
    if (success) {
        cJSON *scheduledTime = cJSON_GetObjectItem(root, "scheduledTime");
        if (cJSON_IsString(scheduledTime) && (scheduledTime->valuestring != NULL)) {
            strncpy(chart->scheduledTime, scheduledTime->valuestring, sizeof(chart->scheduledTime) - 1);
            chart->scheduledTime[sizeof(chart->scheduledTime) - 1] = '\0';
        } else {
            chart->scheduledTime[0] = '\0';
        }
    }

    // Parse currentTime
    if (success) {
        cJSON *currentTime = cJSON_GetObjectItem(root, "currentTime");
        if (cJSON_IsString(currentTime) && (currentTime->valuestring != NULL)) {
            strncpy(chart->currentTime, currentTime->valuestring, sizeof(chart->currentTime) - 1);
            chart->currentTime[sizeof(chart->currentTime) - 1] = '\0';
        } else {
            chart->currentTime[0] = '\0';
        }
    }

    if (success) {
        cJSON *isFeedbackSent = cJSON_GetObjectItem(root, "isFeedbackSent");
        if (cJSON_IsBool(isFeedbackSent)) {
            chart->isFeedbackSent = cJSON_IsTrue(isFeedbackSent);
        } else {
            chart->isFeedbackSent = true;
        }
    }

    if (success) {
        ESP_LOGI(TAG, "Successfully parsed chart: '%s' with %lu points", 
                 chart->chartName, chart->totalPoints);
    }

    cJSON_Delete(root);
    return success;
}

extern bool json_storedChartToJson(const ST_storedChart *chart, char *jsonStr, size_t maxLen)
{
    if (chart == NULL || jsonStr == NULL || maxLen == 0) {
        ESP_LOGE(TAG, "Invalid parameters");
        return false;
    }

    // Create root JSON object
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return false;
    }

    bool success = true;

    // Add curveName
    if (cJSON_AddStringToObject(root, "curveName", chart->chartName) == NULL) {
        ESP_LOGE(TAG, "Failed to add curveName");
        success = false;
    }

    // Add pointsQuantity
    if (success && cJSON_AddNumberToObject(root, "pointsQuantity", chart->totalPoints) == NULL) {
        ESP_LOGE(TAG, "Failed to add pointsQuantity");
        success = false;
    }

    // Add isScheduled
    if (success && cJSON_AddBoolToObject(root, "isScheduled", chart->isScheduled) == NULL) {
        ESP_LOGE(TAG, "Failed to add isScheduled");
        success = false;
    }

    // Add scheduledTime
    if (success && cJSON_AddStringToObject(root, "scheduledTime", chart->scheduledTime) == NULL) {
        ESP_LOGE(TAG, "Failed to add scheduledTime");
        success = false;
    }

    // Add currentTime
    if (success && cJSON_AddStringToObject(root, "currentTime", chart->currentTime) == NULL) {
        ESP_LOGE(TAG, "Failed to add currentTime");
        success = false;
    }

    // Create temperatures array
    cJSON *temperatures = NULL;
    if (success) {
        temperatures = cJSON_CreateArray();
        if (temperatures == NULL) {
            ESP_LOGE(TAG, "Failed to create temperatures array");
            success = false;
        }
    }
    if (success) {
        for (uint32_t i = 0; i < chart->totalPoints && i < MAX_ROAST_TIME_IN_MIN; i++) {
            cJSON *temp = cJSON_CreateNumber(chart->tempProfile[i]);
            if (temp == NULL) {
                ESP_LOGE(TAG, "Failed to create temperature number at index %lu", i);
                cJSON_Delete(temperatures);
                success = false;
                break;
            }
            cJSON_AddItemToArray(temperatures, temp);
        }
    }
    if (success) {
        cJSON_AddItemToObject(root, "temperatures", temperatures);
    }

    if (success) {
        temperatures = cJSON_CreateArray();
        if (temperatures == NULL) {
            ESP_LOGE(TAG, "Failed to create achieved temperatures array");
            success = false;
        }
    }
    if (success) {
        for (uint32_t i = 0; i < chart->totalPoints && i < MAX_ROAST_TIME_IN_MIN; i++) {
            cJSON *temp = cJSON_CreateNumber(chart->achievedProfile[i]);
            if (temp == NULL) {
                ESP_LOGE(TAG, "Failed to create achieved temperature number at index %lu", i);
                cJSON_Delete(temperatures);
                success = false;
                break;
            }
            cJSON_AddItemToArray(temperatures, temp);
        }
    }
    if (success) {
        cJSON_AddItemToObject(root, "achievedTemperatures", temperatures);
    }

    // Add isFeedbackSent
    if (success && cJSON_AddBoolToObject(root, "isFeedbackSent", chart->isFeedbackSent) == NULL) {
        ESP_LOGE(TAG, "Failed to add isFeedbackSent");
        success = false;
    }

    // Convert to string
    char *jsonOutput = NULL;
    if (success) {
        jsonOutput = cJSON_PrintUnformatted(root);
        if (jsonOutput == NULL) {
            ESP_LOGE(TAG, "Failed to print JSON");
            success = false;
        }
    }

    // Check if output fits in buffer
    if (success) {
        size_t jsonLen = strlen(jsonOutput);
        if (jsonLen >= maxLen) {
            ESP_LOGE(TAG, "JSON output (%zu bytes) exceeds buffer size (%zu bytes)", 
                     jsonLen, maxLen);
            cJSON_free(jsonOutput);
            success = false;
        } else {
            // Copy to output buffer
            strcpy(jsonStr, jsonOutput);
            cJSON_free(jsonOutput);
            ESP_LOGI(TAG, "Successfully converted chart '%s' to JSON (%zu bytes)", 
                     chart->chartName, jsonLen);
        }
    }

    cJSON_Delete(root);
    return success;
}

extern bool json_generateFeedbackJson(const ST_storedChart *chart, char *jsonStr, size_t maxLen)
{
    if (chart == NULL || jsonStr == NULL || maxLen == 0) {
        ESP_LOGE(TAG, "Invalid parameters");
        return false;
    }

    // Create root JSON object
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return false;
    }

    bool success = true;

    // Add curveName
    if (cJSON_AddStringToObject(root, "curveName", chart->chartName) == NULL) {
        ESP_LOGE(TAG, "Failed to add curveName");
        success = false;
    }

    // Add finishTime (random timestamp)
    char finishTime[32];
    timeUtils_getCurrentTimeIso8601(finishTime, sizeof(finishTime));
    if (success && cJSON_AddStringToObject(root, "finishTime", finishTime) == NULL) {
        ESP_LOGE(TAG, "Failed to add finishTime");
        success = false;
    }

    // Create temperatures array (using achievedProfile)
    cJSON *temperatures = NULL;
    if (success) {
        temperatures = cJSON_CreateArray();
        if (temperatures == NULL) {
            ESP_LOGE(TAG, "Failed to create temperatures array");
            success = false;
        }
    }
    if (success) {
        for (uint32_t i = 0; i < chart->totalPoints && i < MAX_ROAST_TIME_IN_MIN; i++) {
            cJSON *temp = cJSON_CreateNumber(chart->achievedProfile[i]);
            if (temp == NULL) {
                ESP_LOGE(TAG, "Failed to create temperature number at index %lu", i);
                cJSON_Delete(temperatures);
                success = false;
                break;
            }
            cJSON_AddItemToArray(temperatures, temp);
        }
    }
    if (success) {
        cJSON_AddItemToObject(root, "temperatures", temperatures);
    }

    // Convert to string
    char *jsonOutput = NULL;
    if (success) {
        jsonOutput = cJSON_PrintUnformatted(root);
        if (jsonOutput == NULL) {
            ESP_LOGE(TAG, "Failed to print JSON");
            success = false;
        }
    }

    // Check if output fits in buffer
    if (success) {
        size_t jsonLen = strlen(jsonOutput);
        if (jsonLen >= maxLen) {
            ESP_LOGE(TAG, "JSON output (%zu bytes) exceeds buffer size (%zu bytes)", 
                     jsonLen, maxLen);
            cJSON_free(jsonOutput);
            success = false;
        } else {
            // Copy to output buffer
            strcpy(jsonStr, jsonOutput);
            cJSON_free(jsonOutput);
            ESP_LOGI(TAG, "Successfully generated feedback JSON for '%s' (%zu bytes)", 
                     chart->chartName, jsonLen);
        }
    }

    cJSON_Delete(root);
    return success;
}

extern esp_err_t json_setAllProfilesAsNotSent(void)
{
    ESP_LOGI(TAG, "Setting all profiles as not sent");
    
    uint8_t count = nvs_getProfileCount();
    if (count == 0) {
        ESP_LOGI(TAG, "No profiles to update");
        return ESP_OK;
    }
    
    uint8_t successCount = 0;
    uint8_t failCount = 0;
    
    for (uint8_t i = 0; i < count; i++) {
        // Load profile from NVS
        char *buffer = calloc(NVS_MAX_PROFILE_SIZE, sizeof(char));
        if (buffer == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for profile %d", i);
            failCount++;
            continue;
        }
        
        size_t loadedLen = 0;
        esp_err_t ret = nvs_loadRoastProfile(i, buffer, NVS_MAX_PROFILE_SIZE, &loadedLen);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to load profile %d: %s", i, esp_err_to_name(ret));
            free(buffer);
            failCount++;
            continue;
        }
        
        // Parse JSON to chart structure
        ST_storedChart chart = {0};
        if (!json_jsonToStoredChart(buffer, &chart)) {
            ESP_LOGE(TAG, "Failed to parse profile %d", i);
            free(buffer);
            failCount++;
            continue;
        }
        
        // Set isFeedbackSent to false
        chart.isFeedbackSent = false;
        
        // Convert back to JSON
        if (!json_storedChartToJson(&chart, buffer, NVS_MAX_PROFILE_SIZE)) {
            ESP_LOGE(TAG, "Failed to convert profile %d to JSON", i);
            free(buffer);
            failCount++;
            continue;
        }
        
        // Save back to NVS
        ret = nvs_saveRoastProfile(i, buffer, strlen(buffer));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to save profile %d: %s", i, esp_err_to_name(ret));
            free(buffer);
            failCount++;
            continue;
        }
        
        ESP_LOGI(TAG, "Profile %d updated: isFeedbackSent=false", i);
        successCount++;
        free(buffer);
    }
    
    ESP_LOGI(TAG, "Updated %d profiles, %d failed", successCount, failCount);
    return (failCount == 0) ? ESP_OK : ESP_FAIL;
}

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/
