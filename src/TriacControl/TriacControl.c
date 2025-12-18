/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "TriacControl.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define TRIAC_GPIO            GPIO_NUM_17
#define ZERO_CROSS_GPIO       GPIO_NUM_21
#define TRIAC_FREQ_HZ         25000                 // e.g., 25 kHz (above audible)

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static const char *TAG = "TRIAC";

dimmertyp *triacDimmer = NULL;

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/


/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

extern void triac_triacInit(void)
{
    triacDimmer = createDimmer(TRIAC_GPIO, ZERO_CROSS_GPIO);
    begin(triacDimmer, NORMAL_MODE, OFF, 60);
    triac_setPwrPercent(7.0f);
}

extern void triac_setState(bool onOff)
{
    if (onOff)
    {
        ESP_LOGI(TAG, "Set triac state to ON");
        setState(triacDimmer, ON);
    }
    else
    {
        ESP_LOGI(TAG, "Set triac state to OFF");
        setState(triacDimmer, OFF);
    }
}

extern void triac_setPwrPercent(float pwr)
{
    if (pwr < 0.0f)
        pwr = 0.0f;
    if (pwr > 100.0f)
        pwr = 100.0f;
    
    ESP_LOGI(TAG, "Set triac power to %.1f%%", pwr);
    setPower(triacDimmer, (int)pwr);
}
