#ifndef JSON_HNDLR_H
#define JSON_HNDLR_H

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "cJSON.h"
#include "esp_log.h"
#include "NVShndlr.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define JSON_MAX_SIZE    4096

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Parse JSON string into ST_storedChart structure
 * 
 * Parses a JSON object with the following structure:
 * {
 *   "temperatures": [array of floats],
 *   "pointsQuantity": number,
 *   "isScheduled": boolean,
 *   "scheduledTime": "ISO 8601 string",
 *   "currentTime": "ISO 8601 string",
 *   "curveName": "string"
 * }
 * 
 * @param jsonStr JSON string to parse
 * @param chart Pointer to ST_storedChart structure to fill
 * @return true on success, false on error
 */
extern bool json_parseToStoredChart(const char *jsonStr, ST_storedChart *chart);

/**
 * @brief Convert ST_storedChart structure to JSON string
 * 
 * Creates a JSON object with the following structure:
 * {
 *   "temperatures": [array of floats],
 *   "pointsQuantity": number,
 *   "curveName": "string",
 *   "isScheduled": boolean,
 *   "scheduledTime": "ISO 8601 string",
 *   "currentTime": "ISO 8601 string"
 * }
 * 
 * @param chart Pointer to ST_storedChart structure to convert
 * @param jsonStr Buffer to store the resulting JSON string
 * @param maxLen Maximum length of the buffer
 * @return true on success, false on error
 */
extern bool json_storedChartToJson(const ST_storedChart *chart, char *jsonStr, size_t maxLen);

#endif // JSON_HNDLR_H
