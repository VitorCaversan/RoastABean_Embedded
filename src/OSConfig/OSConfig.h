#ifndef MAIN_H
#define MAIN_H

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define BUTTON_DEBUG                 0
#define NVS_DEBUG                    0

#define OS_MAIN_TASK_QUEUE_SIZE     32

#define BLE_MAX_DATA_LEN        512

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

typedef enum EN_extEvents
{
    EXT_EVENT_NONE = 0,
    EXT_EVENT_UPDATE_CHART,
    EXT_EVENT_END_ROAST,
    EXT_EVENT_5S_TIMER_TO_START_ROAST,
    EXT_EVENT_SCHEDULE_ROAST,
    EXT_EVENT_BTN_1_PRESSED,
    EXT_EVENT_BTN_2_PRESSED,
    EXT_EVENT_BTN_3_PRESSED,
    EXT_EVENT_BTN_4_PRESSED,
    
    EXT_EVENT_QTY // Must be the last element
} EN_extEvents;

typedef struct ST_extEventMsg
{
    EN_extEvents event;
    void *data;
} ST_extEventMsg;

typedef enum {
    BLE_EVENT_CONNECTED = 0,
    BLE_EVENT_DISCONNECTED,
    BLE_EVENT_DATA_RECEIVED,
    BLE_EVENT_SEND_PENDING_FEEDBACK
} EN_bleEvent;

typedef struct {
    EN_bleEvent event;
    uint16_t dataLen;
    // Don't embed large buffer - will be handled separately
} ST_bleMsg;

extern QueueHandle_t OS_mainTaskQueue;
extern QueueHandle_t OS_btnHndlrsTaskQueue;
extern QueueHandle_t OS_bleEventQueue;
extern TaskHandle_t OS_tempSensTaskHandle;

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/


#endif // MAIN_H