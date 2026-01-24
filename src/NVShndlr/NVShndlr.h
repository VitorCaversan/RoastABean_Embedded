/*******************************************************************************
 * @file NVShndlr.h
 * @brief NVS (Non-Volatile Storage) Handler Module
 * 
 * This module provides persistent storage functionality for roast profiles
 * using ESP32's NVS flash system. It handles saving, loading, and managing
 * coffee roast profiles received via Bluetooth in JSON format (max 2KB each).
 * 
 ******************************************************************************/

#ifndef NVS_H
#define NVS_H

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_log.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/
#define MAX_CHARTS_TO_STORE     10
#define NVS_MAX_PROFILE_SIZE    2048

#define MAX_CHART_NAMESIZE    64
#define MAX_ROAST_TIME_IN_MIN       200

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

typedef struct ST_storedChart
{
    char chartName[MAX_CHART_NAMESIZE];
    float tempProfile[MAX_ROAST_TIME_IN_MIN];
    float achievedProfile[MAX_ROAST_TIME_IN_MIN];
    uint32_t totalPoints;
    bool isScheduled;
    char scheduledTime[32];  // ISO 8601 format: "2025-11-24T21:31:00.000Z"
    char currentTime[32];    // ISO 8601 format: "2025-11-23T21:30:00.496Z"
    bool isFeedbackSent;
} ST_storedChart;

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Initialize NVS flash storage
 * 
 * @return ESP_OK on success, error code otherwise
 */
extern esp_err_t nvs_init(void);

/**
 * @brief Save roast profile to NVS
 * 
 * @param profileId Profile ID (0-based index)
 * @param data Pointer to JSON data to save
 * @param dataLen Length of data in bytes
 * @return ESP_OK on success, error code otherwise
 */
extern esp_err_t nvs_saveRoastProfile(uint8_t profileId, const char *data, size_t dataLen);

/**
 * @brief Load roast profile from NVS
 * 
 * @param profileId Profile ID (0-based index)
 * @param data Pointer to buffer where data will be loaded
 * @param maxLen Maximum size of buffer
 * @param outLen Pointer to store actual data length read
 * @return ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if profile doesn't exist, error code otherwise
 */
extern esp_err_t nvs_loadRoastProfile(uint8_t profileId, char *data, size_t maxLen, size_t *outLen);

/**
 * @brief Delete roast profile from NVS
 * 
 * @param profileId Profile ID (0-based index)
 * @return ESP_OK on success, error code otherwise
 */
extern esp_err_t nvs_deleteRoastProfile(uint8_t profileId);

/**
 * @brief Check if a roast profile exists in NVS
 * 
 * @param profileId Profile ID (0-based index)
 * @return true if profile exists, false otherwise
 */
extern bool nvs_profileExists(uint8_t profileId);

/**
 * @brief Get count of stored roast profiles
 * 
 * @return Number of profiles stored (0-255)
 */
extern uint8_t nvs_getProfileCount(void);

/**
 * @brief Erase all roast profiles from NVS
 * 
 * @return ESP_OK on success, error code otherwise
 */
extern esp_err_t nvs_eraseAllProfiles(void);

#endif // NVS_H
