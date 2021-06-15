/*
** Purpose: Define platform configurations for the OpenSat Kit Command Ingest application
**
** Notes:
**   None
**
** License:
**   Written by David McComas, licensed under the copyleft GNU
**   General Public License (GPL). 
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
**
*/

#ifndef _kit_ci_platform_cfg_
#define _kit_ci_platform_cfg_

/*
** Includes
*/

#include "kit_ci_mission_cfg.h"
#include "kit_ci_msgids.h"
#include "kit_ci_perfids.h"

/******************************************************************************
** Platform Deployment Configurations
*/

#define KIT_CI_PLATFORM_REV  0
#define KIT_CI_INI_FILENAME  "/cf/kit_ci_ini.json"


/******************************************************************************
** Command Ingest Application Macros
*/

#define  KIT_CI_RUNLOOP_DELAY     500  /* Delay in milliseconds for main loop      */
#define  KIT_CI_RUNLOOP_MSG_READ   10  /* Max messages read in main loop iteration */

#define  KIT_CI_PORT  1234


/******************************************************************************
** UPLINK Object Macros
**
*/

#define UPLINK_RECV_BUFF_CNT     5
#define UPLINK_RECV_BUFF_LEN  1024
#define UPLINK_MSG_TUNNEL_CNT    8  /* Number of Message ID tunnels */
#define UPLINK_UNUSED_MSG_ID  (CFE_SB_INVALID_MSG_ID)


#endif /* _kit_ci_platform_cfg_ */
