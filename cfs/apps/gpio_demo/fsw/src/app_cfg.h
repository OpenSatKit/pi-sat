/*
** Purpose: Define application configurations for the GPIO Demo
**          application
**
** Notes:
**   1. These macros can only be built with the application and can't
**      have a platform scope because the same app_cfg.h filename is used for
**      all applications following the object-based application design.
**
** License:
**   Written by David McComas, licensed under the copyleft GNU
**   General Public License (GPL). 
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
*/
#ifndef _app_cfg_
#define _app_cfg_

/*
** Includes
*/

#include "gpio_demo_platform_cfg.h"
#include "osk_c_fw.h"


/******************************************************************************
** Application Macros
*/

/*
** Versions:
**
** 1.0 - Initial release
*/

#define  GPIO_DEMO_MAJOR_VER   1
#define  GPIO_DEMO_MINOR_VER   0


/******************************************************************************
** Init File declarations create:
**
**  typedef enum {
**     CMD_PIPE_DEPTH,
**     CMD_PIPE_NAME
**  } INITBL_ConfigEnum;
**    
**  typedef struct {
**     CMD_PIPE_DEPTH,
**     CMD_PIPE_NAME
**  } INITBL_ConfigStruct;
**
**   const char *GetConfigStr(value);
**   ConfigEnum GetConfigVal(const char *str);
**
** XX(name,type)
*/

#define CFG_APP_CFE_NAME       APP_CFE_NAME
#define CFG_APP_PERF_ID        APP_PERF_ID

#define CFG_CHILD_NAME         CHILD_NAME
#define CFG_CHILD_PERF_ID      CHILD_PERF_ID
#define CFG_CHILD_STACK_SIZE   CHILD_STACK_SIZE
#define CFG_CHILD_PRIORITY     CHILD_PRIORITY

#define CFG_CMD_PIPE_NAME      CMD_PIPE_NAME
#define CFG_CMD_PIPE_DEPTH     CMD_PIPE_DEPTH

#define CFG_CMD_MID            CMD_MID
#define CFG_SEND_HK_MID        SEND_HK_MID
#define CFG_HK_TLM_MID         HK_TLM_MID

#define CFG_CTRL_OUT_PIN       CTRL_OUT_PIN
#define CFG_CTRL_ON_TIME       CTRL_ON_TIME
#define CFG_CTRL_OFF_TIME      CTRL_OFF_TIME

#define APP_CONFIG(XX) \
   XX(APP_CFE_NAME,char*) \
   XX(APP_PERF_ID,uint32) \
   XX(CHILD_NAME,char*) \
   XX(CHILD_PERF_ID,uint32) \
   XX(CHILD_STACK_SIZE,uint32) \
   XX(CHILD_PRIORITY,uint32) \
   XX(CMD_PIPE_NAME,char*) \
   XX(CMD_PIPE_DEPTH,uint32) \
   XX(CMD_MID,uint32) \
   XX(SEND_HK_MID,uint32) \
   XX(HK_TLM_MID,uint32) \
   XX(CTRL_OUT_PIN,uint32) \
   XX(CTRL_ON_TIME,uint32) \
   XX(CTRL_OFF_TIME,uint32) \
   
DECLARE_ENUM(Config,APP_CONFIG)


/******************************************************************************
** Command Macros
** - Load/dump table definitions are placeholders for a JSON table
*/

#define GPIO_DEMO_TBL_LOAD_CMD_FC      (CMDMGR_APP_START_FC + 0)
#define GPIO_DEMO_TBL_DUMP_CMD_FC      (CMDMGR_APP_START_FC + 1)

#define GPIO_CTRL_SET_ON_TIME_CMD_FC   (CMDMGR_APP_START_FC + 2)
#define GPIO_CTRL_SET_OFF_TIME_CMD_FC  (CMDMGR_APP_START_FC + 3)


/******************************************************************************
** Event Macros
**
** Define the base event message IDs used by each object/component used by the
** application. There are no automated checks to ensure an ID range is not
** exceeded so it is the developer's responsibility to verify the ranges. 
*/

#define GPIO_DEMO_BASE_EID  (OSK_C_FW_APP_BASE_EID +  0)
#define GPIO_CTRL_BASE_EID  (OSK_C_FW_APP_BASE_EID + 20)


#endif /* _app_cfg_ */
