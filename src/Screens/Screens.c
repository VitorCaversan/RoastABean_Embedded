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
static void display_setBtnsCallbacks(void (*onBtn1Pressed)(void),
                                     void (*onBtn2Pressed)(void),
                                     void (*onBtn3Pressed)(void),
                                     void (*onBtn4Pressed)(void));

/**
 * @defgroup MainMenuButtons Main menu button handlers
 * @brief Navigation and selection logic for the start menu.
 * 
 * Functions: moveSelectionUpMainMenu(), moveSelectionDownMainMenu(), onSelectMainMenu(), setSelectRoastMenu().
 */
static void moveSelectionUpMainMenu(void);
static void moveSelectionDownMainMenu(void);
static void onSelectMainMenu(void);
static void setSelectRoastMenu(void);
static void moveSelection(ST_VerticalMenuUi *ui, int delta);

static void moveSelectionUpSelectRoastMenu(void);
static void moveSelectionDownSelectRoastMenu(void);
static void onSelectSelectRoastMenu(void);
static void onBackFromSelectRoast(void);

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
    display_setBtnsCallbacks(NULL, moveSelectionUpMainMenu, moveSelectionDownMainMenu, onSelectMainMenu);
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

static void display_setBtnsCallbacks(void (*onBtn1Pressed)(void),
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
    ST_VerticalMenuUi *ui = display_getStartMenuUi();
    if (ui == NULL || ui->screen == NULL) {
        ESP_LOGW(TAG, "Start menu UI not initialized");
        return;
    }
    moveSelection(ui, -1);
}

static void moveSelectionDownMainMenu(void)
{
    ST_VerticalMenuUi *ui = display_getStartMenuUi();
    if (ui == NULL || ui->screen == NULL) {
        ESP_LOGW(TAG, "Start menu UI not initialized");
        return;
    }
    moveSelection(ui, 1);
}

static void moveSelection(ST_VerticalMenuUi *ui, int delta)
{
    int next = ui->selectedIndex + delta;
    if (next < 0) next = (MAIN_MENU_OPTION_COUNT - 1);
    if (next >= MAIN_MENU_OPTION_COUNT) next = 0;
    if (next != ui->selectedIndex)
    {
        ui->selectedIndex = next;
        display_applySelectionStyle(ui);
    }
}

static void onSelectMainMenu(void)
{
    ST_VerticalMenuUi *ui = display_getStartMenuUi();
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

    display_setBtnsCallbacks(onBackFromSelectRoast,
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
    moveSelection(ui, -1);
}

static void moveSelectionDownSelectRoastMenu(void)
{
    ST_VerticalMenuUi *ui = display_getSelectRoastMenuUi();
    if (ui == NULL || ui->screen == NULL) {
        ESP_LOGW(TAG, "Select roast menu UI not initialized");
        return;
    }
    moveSelection(ui, 1);
}

static void onSelectSelectRoastMenu(void)
{
    ST_VerticalMenuUi *ui = display_getSelectRoastMenuUi();
    if (ui == NULL || ui->screen == NULL) {
        ESP_LOGW(TAG, "Select roast menu UI not initialized");
        return;
    }
    ESP_LOGI(TAG, "Selected roast menu option %d", ui->selectedIndex);
    
    ST_storedChart *selectedChart = &storedCharts[ui->selectedIndex];
    ESP_LOGI(TAG, "Starting roast with chart: %s", selectedChart->chartName);
    
    // TODO: Navigate to roast screen with selected chart
}

static void onBackFromSelectRoast(void)
{
    ESP_LOGI(TAG, "Going back to main menu");
    
    display_setBtnsCallbacks(NULL,
                             moveSelectionUpMainMenu,
                             moveSelectionDownMainMenu,
                             onSelectMainMenu);
    
    display_showStartMenu();
}