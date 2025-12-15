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

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/
