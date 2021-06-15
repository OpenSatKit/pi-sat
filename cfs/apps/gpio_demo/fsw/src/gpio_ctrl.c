/*
** Purpose: Implement the GPIO Controller Class methods
**
** Notes:
**   None
**
** License:
**   Written by David McComas, licensed under the copyleft GNU General Public
**   Public License (GPL).
**
** References:
**   1. OpenSatKit Object-based Application Developers Guide.
**   2. cFS Application Developer's Guide.
*/

/*
** Include Files:
*/

#include <string.h>

#include "app_cfg.h"
#include "initbl.h"
#include "childmgr.h"
#include "gpio_ctrl.h"
#include "gpio.h"

/**********************/
/** Global File Data **/
/**********************/

static GPIO_CTRL_Class*  GpioCtrl = NULL;


/*******************************/
/** Local Function Prototypes **/
/*******************************/


/******************************************************************************
** Function: GPIO_CTRL_Constructor
**
** Initialize the GPIO Controller object to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void GPIO_CTRL_Constructor(GPIO_CTRL_Class *GpioCtrlPtr, INITBL_Class* IniTbl)
{
   
   GpioCtrl = GpioCtrlPtr;
   
   memset(GpioCtrl, 0, sizeof(GPIO_CTRL_Class));
   
   if (gpio_map() < 0) { // map peripherals
   
      CFE_EVS_SendEvent (GPIO_CTRL_CONSTRUCTOR_EID, CFE_EVS_EventType_ERROR, "GPIO map failed");
      GpioCtrl->IsMapped = false;

   }
   else {
      
      GpioCtrl->IsMapped = true;
      
   }
   
   GpioCtrl->OutPin  = INITBL_GetIntConfig(IniTbl, CFG_CTRL_OUT_PIN);
   gpio_out(GpioCtrl->OutPin);

   GpioCtrl->OnTime  = INITBL_GetIntConfig(IniTbl, CFG_CTRL_ON_TIME);
   GpioCtrl->OffTime = INITBL_GetIntConfig(IniTbl, CFG_CTRL_OFF_TIME);
   
} /* End GPIO_CTRL_Constructor() */


/******************************************************************************
** Function: GPIO_CTRL_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void GPIO_CTRL_ResetStatus(void)
{
   

} /* End GPIO_CTRL_ResetStatus() */


/******************************************************************************
** Function: GPIO_CTRL_SetOnTimeCmd
**
** Notes:
**   1. No limits placed on commanded value.
**
*/
bool GPIO_CTRL_SetOnTimeCmd(void* DataObjPtr, const CFE_SB_Buffer_t* SbBufPtr)
{
   
   const GPIO_CTRL_SetOnTimeCmdMsg *Cmd = (const GPIO_CTRL_SetOnTimeCmdMsg *) SbBufPtr;

   bool RetStatus = true;
  
   GpioCtrl->OnTime = Cmd->OnTime;
   CFE_EVS_SendEvent (GPIO_CTRL_SET_ON_TIME_EID, CFE_EVS_EventType_INFORMATION, "GPIO on time set to %ul milliseconds", GpioCtrl->OnTime);
  
   return RetStatus;   
   
} /* End GPIO_CTRL_SetOnTimeCmd() */


/******************************************************************************
** Function: GPIO_CTRL_SetOffTimeCmd
**
** Notes:
**   1. No limits placed on commanded value.
**
*/
bool GPIO_CTRL_SetOffTimeCmd(void* DataObjPtr, const CFE_SB_Buffer_t* SbBufPtr)
{
   
   const GPIO_CTRL_SetOffTimeCmdMsg *Cmd = (const GPIO_CTRL_SetOffTimeCmdMsg *) SbBufPtr;

   bool RetStatus = true;
  
   GpioCtrl->OffTime = Cmd->OffTime;  
   CFE_EVS_SendEvent (GPIO_CTRL_SET_OFF_TIME_EID, CFE_EVS_EventType_INFORMATION, "GPIO off time set to %ul milliseconds", GpioCtrl->OffTime);
  
   return RetStatus;   
   
} /* End GPIO_CTRL_SetOffTimeCmd() */


/******************************************************************************
** Function: GPIO_CTRL_ChildTask
**
*/
bool GPIO_CTRL_ChildTask(CHILDMGR_Class* ChildMgr)
{
   
   if (GpioCtrl->IsMapped) {
      
      gpio_set(GpioCtrl->OutPin);
      CFE_EVS_SendEvent (GPIO_CTRL_CHILD_TASK_EID, CFE_EVS_EventType_DEBUG, "GPIO pin %d on for %ul milliseconds", GpioCtrl->OutPin, GpioCtrl->OnTime);
      OS_TaskDelay(GpioCtrl->OnTime);
    
      gpio_clr(GpioCtrl->OutPin);
      CFE_EVS_SendEvent (GPIO_CTRL_CHILD_TASK_EID, CFE_EVS_EventType_DEBUG, "GPIO pin %d off for %ul milliseconds", GpioCtrl->OutPin, GpioCtrl->OffTime);
      OS_TaskDelay(GpioCtrl->OffTime);
   
   } /* End if mapped */
   else {
      
      OS_TaskDelay(2000);
   
   }
   
   return true;

} /* End GPIO_CTRL_ChildTask() */

