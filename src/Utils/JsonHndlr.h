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
#include "TimeUtils.h"
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
 *   "curveName": "string",
 *   "pointsQuantity": number,
 *   "temperatures": [array of floats],
 *   "achievedTemperatures": [array of floats] (optional),
 *   "isScheduled": boolean,
 *   "scheduledTime": "ISO 8601 string",
 *   "currentTime": "ISO 8601 string",
 *   "isFeedbackSent": boolean
 * }
 * 
 * @param jsonStr JSON string to parse
 * @param chart Pointer to ST_storedChart structure to fill
 * @return true on success, false on error
 */
extern bool json_jsonToStoredChart(const char *jsonStr, ST_storedChart *chart);

/**
 * @brief Convert ST_storedChart structure to JSON string
 * 
 * Creates a JSON object with the following structure:
 * {
 *   "curveName": "string",
 *   "pointsQuantity": number,
 *   "isScheduled": boolean,
 *   "scheduledTime": "ISO 8601 string",
 *   "currentTime": "ISO 8601 string",
 *   "temperatures": [array of floats],
 *   "achievedTemperatures": [array of floats],
 *   "isFeedbackSent": boolean
 * }
 * 
 * @param chart Pointer to ST_storedChart structure to convert
 * @param jsonStr Buffer to store the resulting JSON string
 * @param maxLen Maximum length of the buffer
 * @return true on success, false on error
 */
extern bool json_storedChartToJson(const ST_storedChart *chart, char *jsonStr, size_t maxLen);

/**
 * @brief Generate feedback JSON from a stored chart
 * 
 * Creates a JSON object with the following structure:
 * {
 *   "temperatures": [array of achieved temperature floats],
 *   "finishTime": "ISO 8601 string",
 *   "curveName": "string"
 * }
 * 
 * @param chart Pointer to ST_storedChart structure
 * @param jsonStr Buffer to store the resulting JSON string
 * @param maxLen Maximum length of the buffer
 * @return true on success, false on error
 */
extern bool json_generateFeedbackJson(const ST_storedChart *chart, char *jsonStr, size_t maxLen);

/**
 * @brief Set all stored profiles' isFeedbackSent flag to false and resave them
 * 
 * @return ESP_OK on success, error code otherwise
 */
extern esp_err_t json_setAllProfilesAsNotSent(void);

#endif // JSON_HNDLR_H
