/* 
** Purpose: Generic entry point function for MIPEA library
**
** Notes:
**   None
**
** License:
**   Written by David McComas, licensed under the copyleft GNU
**   General Public License (GPL). 
**
*/

/*
** Includes
*/

#include "cfe.h"

/*
** Exported Functions
*/

/******************************************************************************
** Entry function
**
*/
uint32 MIPEA_LibInit(void)
{

   OS_printf("MIPEA Library Initialized. Version 2.1.1\n");
   
   return OS_SUCCESS;

} /* End MIPEA_LibInit() */

