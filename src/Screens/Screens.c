/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "Screens.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Update the roast chart with new data.
 *
 * @param data Pointer to the chart update data.
 */
static void updateChart(const ST_chartUpdateData *data);

/**
 * @brief Set the callbacks for the 4 buttons
 * 
 * @param onBtn1Pressed Callback for button 1 (can be NULL)
 * @param onBtn2Pressed Callback for button 2 (can be NULL)
 * @param onBtn3Pressed Callback for button 3 (can be NULL)
 * @param onBtn4Pressed Callback for button 4 (can be NULL)
 */
static void setBtnsCallbacks(void (*onBtn1Pressed)(void),
                             void (*onBtn2Pressed)(void),
                             void (*onBtn3Pressed)(void),
                             void (*onBtn4Pressed)(void));

/**
 * @defgroup MainMenuButtons Main menu button handlers
 * @brief Navigation and selection logic for the start menu.
 * 
 * Functions: moveSelectionUpMainMenu(),
 * moveSelectionDownMainMenu(),
 * onSelectMainMenu(),
 * setSelectRoastMenu().
 */
static void moveSelectionUpMainMenu(void);
static void moveSelectionDownMainMenu(void);
static void onSelectMainMenu(void);
static void setSelectRoastMenu(void);
static void moveSelection(ST_VerticalMenuUi *ui, int delta, int optionQty);

/**
 * @defgroup SelectRoastMenuButtons Select roast menu button handlers
 * @brief Navigation and selection logic for the select roast menu.
 * 
 * Functions: moveSelectionUpSelectRoastMenu(),
 * moveSelectionDownSelectRoastMenu(),
 * onSelectSelectRoastMenu(),
 * onConfirmStartRoast(),
 * onCancelStartRoast(),
 * onBackFromSelectRoast().
 */
static void moveSelectionUpSelectRoastMenu(void);
static void moveSelectionDownSelectRoastMenu(void);
static void onSelectSelectRoastMenu(void);
static void onConfirmStartRoast(void);
static void onCancelStartRoast(void);
static void onBackFromSelectRoast(void);

/**
 * @brief Create a confirmation popup with message and callbacks
 * 
 * @param message The message to display in the popup. \0 terminated string.
 * @param onConfirm Callback function when the confirm action is selected
 * @param onCancel Callback function when the cancel action is selected
 */
static void createConfirmationPopup(const char *message,
                                    void (*onConfirm)(void),
                                    void (*onCancel)(void));

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static const char *TAG = "SCREENS";

static ST_btnsFunc btnsFunc = {0};

static ST_storedChart storedCharts[MAX_CHARTS_TO_SHOW] = {0};

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

extern void scr_screensTask(void *arg)
{
    ST_screenMsg msg;

    while (1)
    {
        if (xQueueReceive(OS_screensTaskQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            switch (msg.event)
            {
                case SCR_EVENT_UPDATE_CHART:
                    ESP_LOGI(TAG, "Update chart");
                    updateChart((ST_chartUpdateData *)msg.data);
                break;
                default:
                    ESP_LOGW(TAG, "Unknown screen event: %d", msg.event);
                break;
            }
        }
    }
}

extern void scr_screensInit(void)
{
    setBtnsCallbacks(NULL, moveSelectionUpMainMenu, moveSelectionDownMainMenu, onSelectMainMenu);
}

extern void scr_onBtnPress(EN_buttons button)
{
    switch (button)
    {
        case BTN_1_PRESSED:
            if (btnsFunc.onBtn1Pressed) btnsFunc.onBtn1Pressed();
        break;
        case BTN_2_PRESSED:
            if (btnsFunc.onBtn2Pressed) btnsFunc.onBtn2Pressed();
        break;
        case BTN_3_PRESSED:
            if (btnsFunc.onBtn3Pressed) btnsFunc.onBtn3Pressed();
        break;
        case BTN_4_PRESSED:
            if (btnsFunc.onBtn4Pressed) btnsFunc.onBtn4Pressed();
        break;
        case BTN_QTY:
        default:
            ESP_LOGW(TAG, "Unknown button press: %d", button);
        break;
    }
}

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

static void updateChart(const ST_chartUpdateData *data)
{
    display_updateRoastChart(data->currTargetTemp, data->elapsedSecs, data->currentTemp);
}

static void setBtnsCallbacks(void (*onBtn1Pressed)(void),
                             void (*onBtn2Pressed)(void),
                             void (*onBtn3Pressed)(void),
                             void (*onBtn4Pressed)(void))
{
    btnsFunc.onBtn1Pressed = onBtn1Pressed;
    btnsFunc.onBtn2Pressed = onBtn2Pressed;
    btnsFunc.onBtn3Pressed = onBtn3Pressed;
    btnsFunc.onBtn4Pressed = onBtn4Pressed;
}

static void moveSelectionUpMainMenu(void)
{
    ST_VerticalMenuUi *ui = display_getMainMenuUi();
    if (ui == NULL || ui->screen == NULL) {
        ESP_LOGW(TAG, "Start menu UI not initialized");
        return;
    }
    moveSelection(ui, -1, MAIN_MENU_OPTION_COUNT);
}

static void moveSelectionDownMainMenu(void)
{
    ST_VerticalMenuUi *ui = display_getMainMenuUi();
    if (ui == NULL || ui->screen == NULL) {
        ESP_LOGW(TAG, "Start menu UI not initialized");
        return;
    }
    moveSelection(ui, 1, MAIN_MENU_OPTION_COUNT);
}

static void moveSelection(ST_VerticalMenuUi *ui, int delta, int optionQty)
{
    int next = ui->selectedIndex + delta;
    if (next < 0) next = (optionQty - 1);
    if (next >= optionQty) next = 0;
    if (next != ui->selectedIndex)
    {
        ui->selectedIndex = next;
        display_applySelectionStyle(ui, optionQty);
    }
}

static void onSelectMainMenu(void)
{
    ST_VerticalMenuUi *ui = display_getMainMenuUi();
    if (ui == NULL || ui->screen == NULL) {
        ESP_LOGW(TAG, "Start menu UI not initialized");
        return;
    }
    ESP_LOGI(TAG, "Selected main menu option %d", ui->selectedIndex);
    
    switch (ui->selectedIndex)
    {
        case 0: // "Roast"
            ESP_LOGI(TAG, "Going to select roast...");
            setSelectRoastMenu();
        break;
        case 1: // "Manage Roast Curves"
            ESP_LOGI(TAG, "Managing roast curves...");
            // TODO: Navigate to roast curves management screen
        break;
        case 2: // "Connect"
            ESP_LOGI(TAG, "Connecting...");
            // TODO: Navigate to connection/settings screen
        break;
        default:
            ESP_LOGW(TAG, "Unknown menu option: %d", ui->selectedIndex);
        break;
    }
}

static void setSelectRoastMenu(void)
{
    float currTemp = tempSens_getTemperature();
    uint32_t i = 0;
    uint32_t j = 0;
    for (i = 0; i < MAX_CHARTS_TO_SHOW; i++)
    {
        for (j = 0; j < (MAX_ROAST_TIME_IN_MIN / 4); j++)
        {
            storedCharts[i].tempProfile[j] = currTemp + random() % 20;
        }

        sprintf(storedCharts[i].chartName, "Test Chart %ld", i + 1);
    }

    setBtnsCallbacks(onBackFromSelectRoast,
                     moveSelectionUpSelectRoastMenu,
                     moveSelectionDownSelectRoastMenu,
                     onSelectSelectRoastMenu);

    display_showSelectRoastMenu(storedCharts, MAX_CHARTS_TO_SHOW);
}

static void moveSelectionUpSelectRoastMenu(void)
{
    ST_VerticalMenuUi *ui = display_getSelectRoastMenuUi();
    if (ui == NULL || ui->screen == NULL) {
        ESP_LOGW(TAG, "Select roast menu UI not initialized");
        return;
    }
    moveSelection(ui, -1, SELECT_ROAST_MENU_OPTION_COUNT);
}

static void moveSelectionDownSelectRoastMenu(void)
{
    ST_VerticalMenuUi *ui = display_getSelectRoastMenuUi();
    if (ui == NULL || ui->screen == NULL) {
        ESP_LOGW(TAG, "Select roast menu UI not initialized");
        return;
    }
    moveSelection(ui, 1, SELECT_ROAST_MENU_OPTION_COUNT);
}

static void onSelectSelectRoastMenu(void)
{
    ST_VerticalMenuUi *ui = display_getSelectRoastMenuUi();
    if (ui == NULL || ui->screen == NULL) {
        ESP_LOGW(TAG, "Select roast menu UI not initialized");
        return;
    }
    
    ST_storedChart *selectedChart = &storedCharts[ui->selectedIndex];
    ESP_LOGI(TAG, "Starting roast with chart: %s", selectedChart->chartName);
    
    // Show confirmation popup
    char message[128];
    snprintf(message, sizeof(message), "Start roasting with\n%s?", selectedChart->chartName);
    createConfirmationPopup(message, onConfirmStartRoast, onCancelStartRoast);
}

static void onConfirmStartRoast(void)
{
    ESP_LOGI(TAG, "Roast confirmed!");

    display_hideConfirmationPopup();
    
    ST_VerticalMenuUi *ui = display_getSelectRoastMenuUi();
    ST_storedChart *selectedChart = &storedCharts[ui->selectedIndex];
    
    display_createRoastChart(selectedChart->tempProfile, (MAX_ROAST_TIME_IN_MIN / 4), NAN, NAN);
    pid_ctrlLoopStart(selectedChart->tempProfile, (MAX_ROAST_TIME_IN_MIN / 4));
}

static void onCancelStartRoast(void)
{
    ESP_LOGI(TAG, "Roast cancelled");

    display_hideConfirmationPopup();
    
    setBtnsCallbacks(onBackFromSelectRoast,
                     moveSelectionUpSelectRoastMenu,
                     moveSelectionDownSelectRoastMenu,
                     onSelectSelectRoastMenu);
}

static void onBackFromSelectRoast(void)
{
    ESP_LOGI(TAG, "Going back to main menu");
    
    setBtnsCallbacks(NULL,
                     moveSelectionUpMainMenu,
                     moveSelectionDownMainMenu,
                     onSelectMainMenu);
    
    display_showMainMenu();
}

static void createConfirmationPopup(const char *message,
                                    void (*onConfirm)(void),
                                    void (*onCancel)(void))
{
    setBtnsCallbacks(onCancelStartRoast, NULL, NULL, onConfirmStartRoast);
    
    display_createConfirmationPopup(message);
    display_showConfirmationPopup();
}