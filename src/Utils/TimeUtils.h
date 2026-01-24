#ifndef TIME_UTILS_H
#define TIME_UTILS_H

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "esp_random.h"
#include "esp_log.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define ISO8601_MAX_LENGTH  32

#define S_TO_USECONDS(s)          ((s) * 1000000ULL)

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Parse ISO 8601 timestamp string to Unix timestamp
 * 
 * Parses a timestamp in ISO 8601 format (e.g., "2025-11-24T21:31:00.000Z")
 * and converts it to Unix timestamp (seconds since epoch)
 * 
 * @param iso8601Str ISO 8601 formatted timestamp string
 * @param timestamp Pointer to store the resulting Unix timestamp
 * @return true on success, false on error
 */
extern bool timeUtils_parseIso8601(const char *iso8601Str, time_t *timestamp);

/**
 * @brief Convert Unix timestamp to ISO 8601 string
 * 
 * Converts a Unix timestamp to ISO 8601 format string
 * 
 * @param timestamp Unix timestamp (seconds since epoch)
 * @param iso8601Str Buffer to store the ISO 8601 string (min ISO8601_MAX_LENGTH)
 * @param bufferSize Size of the buffer
 * @return true on success, false on error
 */
extern bool timeUtils_toIso8601(time_t timestamp, char *iso8601Str, size_t bufferSize);

/**
 * @brief Get current time as ISO 8601 string
 * 
 * @param iso8601Str Buffer to store the ISO 8601 string (min ISO8601_MAX_LENGTH)
 * @param bufferSize Size of the buffer
 * @return true on success, false on error
 */
extern bool timeUtils_getCurrentIso8601(char *iso8601Str, size_t bufferSize);

/**
 * @brief Calculate time difference in seconds
 * 
 * @param timestamp1 First Unix timestamp
 * @param timestamp2 Second Unix timestamp
 * @return Difference in seconds (timestamp1 - timestamp2)
 */
extern int32_t timeUtils_diffSeconds(time_t timestamp1, time_t timestamp2);

/**
 * @brief Check if a scheduled time has been reached
 * 
 * @param scheduledTime Unix timestamp of scheduled time
 * @return true if current time >= scheduled time, false otherwise
 */
extern bool timeUtils_isScheduledTimeReached(time_t scheduledTime);

/**
 * @brief Get current time as ISO 8601 string
 * 
 * Currently only returns a generated timestamp for testing purposes.
 */
extern void timeUtils_getCurrentTimeIso8601(char *outputString, size_t maxSize);

#endif // TIME_UTILS_H
