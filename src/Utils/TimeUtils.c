/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "TimeUtils.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Generate a random ISO 8601 timestamp string
 * 
 * @param buffer Buffer to store the timestamp string (must be at least 25 bytes)
 * @param bufferSize Size of the buffer
 */
static void generateRandomTimestamp(char *buffer, size_t bufferSize);

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static const char *TAG = "TIME_UTILS";

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

extern bool timeUtils_parseIso8601(const char *iso8601Str, time_t *timestamp)
{
    if (iso8601Str == NULL || timestamp == NULL)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return false;
    }

    struct tm timeInfo = {0};
    int milliseconds = 0;

    // Parse ISO 8601 format: "2025-11-24T21:31:00.000Z"
    int parsed = sscanf(iso8601Str, "%d-%d-%dT%d:%d:%d.%dZ",
                       &timeInfo.tm_year, &timeInfo.tm_mon, &timeInfo.tm_mday,
                       &timeInfo.tm_hour, &timeInfo.tm_min, &timeInfo.tm_sec,
                       &milliseconds);

    if (parsed < 6)
    {
        ESP_LOGE(TAG, "Failed to parse ISO 8601 timestamp: %s", iso8601Str);
        return false;
    }

    // Adjust values for struct tm
    timeInfo.tm_year -= 1900;  // Years since 1900
    timeInfo.tm_mon -= 1;       // Months since January [0-11]
    timeInfo.tm_isdst = -1;     // Let mktime determine DST

    // Convert to Unix timestamp (UTC)
    *timestamp = mktime(&timeInfo);

    if (*timestamp == -1)
    {
        ESP_LOGE(TAG, "mktime failed for timestamp: %s", iso8601Str);
        return false;
    }

    ESP_LOGD(TAG, "Parsed ISO 8601 '%s' to timestamp: %lld", iso8601Str, (long long)*timestamp);
    return true;
}

extern bool timeUtils_toIso8601(time_t timestamp, char *iso8601Str, size_t bufferSize)
{
    if (iso8601Str == NULL || bufferSize < ISO8601_MAX_LENGTH)
    {
        ESP_LOGE(TAG, "Invalid parameters or buffer too small");
        return false;
    }

    struct tm timeInfo;
    if (gmtime_r(&timestamp, &timeInfo) == NULL)
    {
        ESP_LOGE(TAG, "gmtime_r failed");
        return false;
    }

    // Format as ISO 8601: "2025-11-24T21:31:00.000Z"
    int written = snprintf(iso8601Str, bufferSize, "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
                          timeInfo.tm_year + 1900,
                          timeInfo.tm_mon + 1,
                          timeInfo.tm_mday,
                          timeInfo.tm_hour,
                          timeInfo.tm_min,
                          timeInfo.tm_sec);

    if (written < 0 || written >= bufferSize)
    {
        ESP_LOGE(TAG, "Failed to format ISO 8601 string");
        return false;
    }

    return true;
}

extern bool timeUtils_getCurrentIso8601(char *iso8601Str, size_t bufferSize)
{
    if (iso8601Str == NULL || bufferSize < ISO8601_MAX_LENGTH)
    {
        ESP_LOGE(TAG, "Invalid parameters or buffer too small");
        return false;
    }

    time_t now;
    time(&now);

    return timeUtils_toIso8601(now, iso8601Str, bufferSize);
}

extern int32_t timeUtils_diffSeconds(time_t timestamp1, time_t timestamp2)
{
    return (int32_t)difftime(timestamp1, timestamp2);
}

extern bool timeUtils_isScheduledTimeReached(time_t scheduledTime)
{
    time_t now;
    time(&now);

    return difftime(now, scheduledTime) >= 0;
}

extern void timeUtils_getCurrentTimeIso8601(char *outputString, size_t maxSize)
{
    generateRandomTimestamp(outputString, maxSize);
}

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

 static void generateRandomTimestamp(char *buffer, size_t bufferSize)
{
    if (buffer == NULL || bufferSize < 25) {
        return;
    }
    
    // Generate random date/time components
    // Year: 2024-2026
    uint32_t year = 2024 + (esp_random() % 3);
    
    // Month: 1-12
    uint32_t month = 1 + (esp_random() % 12);
    
    // Day: 1-28 (simplified to avoid invalid dates)
    uint32_t day = 1 + (esp_random() % 28);
    
    // Hour: 0-23
    uint32_t hour = esp_random() % 24;
    
    // Minute: 0-59
    uint32_t minute = esp_random() % 60;
    
    // Second: 0-59
    uint32_t second = esp_random() % 60;
    
    // Millisecond: 0-999
    uint32_t millisecond = esp_random() % 1000;
    
    // Format as ISO 8601: "YYYY-MM-DDTHH:MM:SS.sssZ"
    snprintf(buffer, bufferSize, "%04lu-%02lu-%02luT%02lu:%02lu:%02lu.%03luZ",
             (unsigned long)year, (unsigned long)month, (unsigned long)day,
             (unsigned long)hour, (unsigned long)minute, (unsigned long)second,
             (unsigned long)millisecond);
}
