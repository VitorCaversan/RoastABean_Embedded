/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "NVShndlr.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/
#define NVS_NAMESPACE           "roast_profiles"
#define NVS_PROFILE_KEY_PREFIX  "profile_"
#define NVS_COUNT_KEY           "profile_count"

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/
static void buildProfileKey(uint8_t profileId, char *key);
static esp_err_t updateProfileCount(void);

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/
static const char *TAG = "NVS";
static uint8_t profileCount = 0;

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

extern esp_err_t nvs_init(void)
{
    ESP_LOGI(TAG, "Initializing NVS");
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Load profile count
    nvs_handle_t handle;
    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret == ESP_OK)
    {
        ret = nvs_get_u8(handle, NVS_COUNT_KEY, &profileCount);
        if (ret == ESP_ERR_NVS_NOT_FOUND)
        {
            profileCount = 0;
            ret = ESP_OK;
        }
        nvs_close(handle);
    }
    
    ESP_LOGI(TAG, "NVS initialized, %d profiles stored", profileCount);
    return ESP_OK;
}

extern esp_err_t nvs_saveRoastProfile(uint8_t profileId, const char *data, size_t dataLen)
{
    if (data == NULL || dataLen == 0 || dataLen > NVS_MAX_PROFILE_SIZE)
    {
        ESP_LOGE(TAG, "Invalid data or length: %d bytes", dataLen);
        return ESP_ERR_INVALID_ARG;
    }
    
    char key[NVS_MAX_KEY_LENGTH + 1];
    buildProfileKey(profileId, key);
    
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Check if this is a new profile
    bool isNewProfile = !nvs_profileExists(profileId);
    
    // Save the profile data
    ret = nvs_set_blob(handle, key, data, dataLen);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save profile %d: %s", profileId, esp_err_to_name(ret));
        nvs_close(handle);
        return ret;
    }
    
    ret = nvs_commit(handle);
    nvs_close(handle);
    
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    if (isNewProfile)
    {
        updateProfileCount();
    }
    
    ESP_LOGI(TAG, "Profile %d saved successfully (%d bytes)", profileId, dataLen);
    return ESP_OK;
}

extern esp_err_t nvs_loadRoastProfile(uint8_t profileId, char *data, size_t maxLen, size_t *outLen)
{
    if (data == NULL || maxLen == 0)
    {
        ESP_LOGE(TAG, "Invalid buffer");
        return ESP_ERR_INVALID_ARG;
    }
    
    char key[NVS_MAX_KEY_LENGTH + 1];
    buildProfileKey(profileId, key);
    
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    size_t requiredSize;
    ret = nvs_get_blob(handle, key, NULL, &requiredSize);
    if (ret != ESP_OK)
    {
        if (ret == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGW(TAG, "Profile %d not found", profileId);
        } else {
            ESP_LOGE(TAG, "Failed to get profile %d size: %s", profileId, esp_err_to_name(ret));
        }
        nvs_close(handle);
        return ret;
    }
    
    if (requiredSize > maxLen)
    {
        ESP_LOGE(TAG, "Buffer too small: need %d bytes, have %d", requiredSize, maxLen);
        nvs_close(handle);
        return ESP_ERR_INVALID_SIZE;
    }
    
    ret = nvs_get_blob(handle, key, data, &requiredSize);
    nvs_close(handle);
    
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to load profile %d: %s", profileId, esp_err_to_name(ret));
        return ret;
    }
    
    if (outLen != NULL)
    {
        *outLen = requiredSize;
    }
    
    ESP_LOGI(TAG, "Profile %d loaded successfully (%d bytes)", profileId, requiredSize);
    return ESP_OK;
}

extern esp_err_t nvs_deleteRoastProfile(uint8_t profileId)
{
    char key[NVS_MAX_KEY_LENGTH + 1];
    buildProfileKey(profileId, key);
    
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = nvs_erase_key(handle, key);
    if (ret != ESP_OK)
    {
        if (ret != ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to delete profile %d: %s", profileId, esp_err_to_name(ret));
        }
        nvs_close(handle);
        return ret;
    }
    
    ret = nvs_commit(handle);
    nvs_close(handle);
    
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    updateProfileCount();
    
    ESP_LOGI(TAG, "Profile %d deleted successfully", profileId);
    return ESP_OK;
}

extern bool nvs_profileExists(uint8_t profileId)
{
    char key[NVS_MAX_KEY_LENGTH + 1];
    buildProfileKey(profileId, key);
    
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK)
    {
        return false;
    }
    
    size_t requiredSize;
    ret = nvs_get_blob(handle, key, NULL, &requiredSize);
    nvs_close(handle);
    
    return (ret == ESP_OK);
}

extern uint8_t nvs_getProfileCount(void)
{
    return profileCount;
}

extern esp_err_t nvs_eraseAllProfiles(void)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = nvs_erase_all(handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to erase all profiles: %s", esp_err_to_name(ret));
        nvs_close(handle);
        return ret;
    }
    
    ret = nvs_commit(handle);
    nvs_close(handle);
    
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    profileCount = 0;
    
    ESP_LOGI(TAG, "All profiles erased");
    return ESP_OK;
}

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

static void buildProfileKey(uint8_t profileId, char *key)
{
    snprintf(key, NVS_MAX_KEY_LENGTH + 1, "%s%d", NVS_PROFILE_KEY_PREFIX, profileId);
}

static esp_err_t updateProfileCount(void)
{
    uint8_t count = 0;
    
    for (uint8_t i = 0; i < UINT8_MAX; i++)
    {
        if (nvs_profileExists(i))
        {
            count++;
        }
    }
    
    profileCount = count;
    
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret == ESP_OK)
    {
        nvs_set_u8(handle, NVS_COUNT_KEY, profileCount);
        nvs_commit(handle);
        nvs_close(handle);
    }
    
    return ESP_OK;
}
