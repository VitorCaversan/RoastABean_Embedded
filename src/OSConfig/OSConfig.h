#ifndef MAIN_H
#define MAIN_H

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define OS_MAIN_TASK_QUEUE_SIZE     32

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

typedef enum EN_screenEvents
{
    SCR_EVENT_NONE = 0,
    SCR_EVENT_UPDATE_CHART,
    SCR_EVENT_END_ROAST,
    
    SCR_EVENT_QTY // Must be the last element
} EN_screenEvents;

typedef struct ST_screenMsg
{
    EN_screenEvents event;
    void *data;
} ST_screenMsg;

extern QueueHandle_t OS_mainTaskQueue;
extern QueueHandle_t OS_btnHndlrsTaskQueue;

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/


#endif // MAIN_H