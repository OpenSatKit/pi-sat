/*
** Purpose: Define platform configurations for the GPIO Demo application
**
** Notes:
**   1. This is part of a refactoring prototype. The definitions in this file 
**      should come from a EDS or equivalent toolchain
**
** License:
**   Written by David McComas and licensed under the GNU
**   Lesser General Public License (LGPL).
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
**
*/

#ifndef _gpio_demo_platform_cfg_
#define _gpio_demo_platform_cfg_

/*
** Includes
*/

#include "gpio_demo_mission_cfg.h"


/******************************************************************************
** Platform Deployment Configurations
*/

#define GPIO_DEMO_PLATFORM_REV   0
#define GPIO_DEMO_INI_FILENAME   "/cf/gpio_demo_ini.json"


#endif /* _gpio_demo_platform_cfg_ */
