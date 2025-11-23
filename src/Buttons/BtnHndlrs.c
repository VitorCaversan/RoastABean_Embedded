/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "BtnHndlrs.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Calls respective button press handler based on button pressed
 * 
 * @param button The button that was pressed
 */
static void onBtnPress(EN_buttons button);

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

static void onBackFromRoast(void);
static void onConfirmEndRoast(void);
static void onCancelEndRoast(void);

/**
 * @brief Create a confirmation popup with message and sets button handlers
 * 
 * @param message The message to display in the popup. \0 terminated string.
 * @param onConfirm Callback function when the confirm action is selected
 * @param onCancel Callback function when the cancel action is selected
 */
static void createConfirmationPopupWithHndlrs(const char *message,
                                              void (*onConfirm)(void),
                                              void (*onCancel)(void),
                                              lv_color_t currScrBckgnd);

/**
 * @brief Ends roast by stopping the timer and going back to the main menu
 */
static void endRoast(void);

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static const char *TAG = "BTN_HNDLRS";

static ST_btnsFunc btnsFunc = {0};

static ST_storedChart storedCharts[MAX_CHARTS_TO_SHOW] = {0};

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

extern void btnHndlrs_task(void *arg)
{
    ST_extEventMsg msg;

    while (1)
    {
        if (xQueueReceive(OS_btnHndlrsTaskQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            switch (msg.event)
            {
                case EXT_EVENT_UPDATE_CHART:
                    ESP_LOGI(TAG, "Update chart");
                    updateChart((ST_chartUpdateData *)msg.data);
                break;
                case EXT_EVENT_END_ROAST:
                    ESP_LOGI(TAG, "Update chart");
                    endRoast();
                break;
                case EXT_EVENT_BTN_1_PRESSED:
                    ESP_LOGI(TAG, "Button 1 pressed");
                    onBtnPress(BTN_1_PRESSED);
                break;
                case EXT_EVENT_BTN_2_PRESSED:
                    ESP_LOGI(TAG, "Button 2 pressed");
                    onBtnPress(BTN_2_PRESSED);
                break;
                case EXT_EVENT_BTN_3_PRESSED:
                    ESP_LOGI(TAG, "Button 3 pressed");
                    onBtnPress(BTN_3_PRESSED);
                break;
                case EXT_EVENT_BTN_4_PRESSED:
                    ESP_LOGI(TAG, "Button 4 pressed");
                    onBtnPress(BTN_4_PRESSED);
                break;
                default:
                    ESP_LOGW(TAG, "Unknown screen event: %d", msg.event);
                break;
            }
        }
    }
}

extern void btnHndlrs_btnHndlrsInit(void)
{
    setBtnsCallbacks(NULL, moveSelectionUpMainMenu, moveSelectionDownMainMenu, onSelectMainMenu);
}

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

static void onBtnPress(EN_buttons button)
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
            bluetooth_startAdvertising();
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
        if (i == 0)
        {
            // Realistic roast profile for Test Chart 1
            // Medium roast: 14 minutes total (MAX_ROAST_TIME_IN_MIN / 4 = 15 points)
            // Typical coffee roasting temperatures:
            // - Start: ~180°C (after preheat)
            // - Yellowing phase: 160-180°C (minutes 0-3)
            // - First crack: ~196°C (around minute 8-9)
            // - Development: 196-210°C (minutes 9-12)
            // - Second crack: ~210-220°C (minute 13+)
            // - End: ~215°C for medium roast
            
            float profile[] = {
                180.0f,  // 0 min - Starting temperature after preheat
                175.0f,  // 1 min - Initial drying phase
                170.0f,  // 2 min - Continued drying
                168.0f,  // 3 min - End of drying phase
                175.0f,  // 4 min - Maillard reaction begins
                182.0f,  // 5 min - Browning accelerates
                188.0f,  // 6 min - Pre-first crack
                194.0f,  // 7 min - Approaching first crack
                198.0f,  // 8 min - First crack begins (~196°C)
                202.0f,  // 9 min - First crack ongoing
                206.0f,  // 10 min - Development phase
                210.0f,  // 11 min - Continued development
                213.0f,  // 12 min - Late development
                215.0f,  // 13 min - End of roast (medium)
                215.0f   // 14 min - Hold temperature
            };
            
            for (j = 0; j < (sizeof(profile) / sizeof(profile[0])); j++)
            {
                storedCharts[i].tempProfile[j] = profile[j];
            }

            storedCharts[i].totalMins = j;

            sprintf(storedCharts[i].chartName, "Medium Roast");
        }
        else if (i == 1)
        {
            float profile[] = {
                30.0f
            };
            
            for (j = 0; j < (sizeof(profile) / sizeof(profile[0])); j++)
            {
                storedCharts[i].tempProfile[j] = profile[j];
            }

            storedCharts[i].totalMins = j;

            sprintf(storedCharts[i].chartName, "Testo rosto");
        }
        else
        {
            for (j = 0; j < (MAX_ROAST_TIME_IN_MIN / 4); j++)
            {
                storedCharts[i].tempProfile[j] = currTemp + random() % 20;
            }

            storedCharts[i].totalMins = j;
            
            sprintf(storedCharts[i].chartName, "Test Chart %ld", i + 1);
        }
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
    
    char message[128];
    snprintf(message, sizeof(message), "Start roasting with\n%s?", selectedChart->chartName);
    createConfirmationPopupWithHndlrs(message,
                                      onConfirmStartRoast,
                                      onCancelStartRoast,
                                      lv_color_black());
}

static void onConfirmStartRoast(void)
{
    ESP_LOGI(TAG, "Roast confirmed!");

    display_hideConfirmationPopup();
    
    setBtnsCallbacks(onBackFromRoast, NULL, NULL, NULL);

    ST_VerticalMenuUi *ui = display_getSelectRoastMenuUi();
    ST_storedChart *selectedChart = &storedCharts[ui->selectedIndex];
    
    display_createRoastChart(selectedChart->tempProfile, selectedChart->totalMins, NAN, NAN);
    display_showRoastChart();
    pid_ctrlLoopStart(selectedChart->tempProfile, selectedChart->totalMins);

    DCMotor_rampSpeedUp(TB_BOARD_1, MOTOR_A, 0, 30, 8);
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

static void createConfirmationPopupWithHndlrs(const char *message,
                                              void (*onConfirm)(void),
                                              void (*onCancel)(void),
                                              lv_color_t currScrBckgnd)
{
    setBtnsCallbacks(onCancel, NULL, NULL, onConfirm);
    
    display_createConfirmationPopup(message, currScrBckgnd);
    display_showConfirmationPopup();
}

static void onBackFromRoast(void)
{
    char message[128];
    snprintf(message, sizeof(message), "Are you sure to end the roast?");
    createConfirmationPopupWithHndlrs(message,
                                      onConfirmEndRoast,
                                      onCancelEndRoast,
                                      lv_palette_main(LV_PALETTE_NONE));
}

static void onConfirmEndRoast(void)
{
    ESP_LOGI(TAG, "End roast confirmed!");

    display_hideConfirmationPopup();

    pid_ctrlLoopStop();

    setBtnsCallbacks(NULL,
                     moveSelectionUpMainMenu,
                     moveSelectionDownMainMenu,
                     onSelectMainMenu);

    display_showMainMenu();
}

static void onCancelEndRoast(void)
{
    ESP_LOGI(TAG, "End roast cancelled");

    display_hideConfirmationPopup();
    
    setBtnsCallbacks(onBackFromRoast, NULL, NULL, NULL);
}

static void endRoast(void)
{
    pid_ctrlLoopStop();

    ST_motorControlContext *motorCtrlCtx = DCMotor_getContextFromMotor(TB_BOARD_2);

    tb6612_setSpeed(&motorCtrlCtx->motor, MOTOR_B, 100);

    vTaskDelay(pdMS_TO_TICKS(500));

    tb6612_setSpeed(&motorCtrlCtx->motor, MOTOR_B, 0);

    vTaskDelay(pdMS_TO_TICKS(5000));

    motorCtrlCtx = DCMotor_getContextFromMotor(TB_BOARD_1);

    tb6612_setSpeed(&motorCtrlCtx->motor, MOTOR_A, 0);

    setBtnsCallbacks(NULL,
                     moveSelectionUpMainMenu,
                     moveSelectionDownMainMenu,
                     onSelectMainMenu);

    display_showMainMenu();
}