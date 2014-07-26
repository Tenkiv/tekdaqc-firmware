/*
 * TekDAQC_Bootloader.h
 *
 *  Created on: Dec 3, 2013
 *      Author: web_000
 */

#ifndef TEKDAQC_BOOTLOADER_H_
#define TEKDAQC_BOOTLOADER_H_

#include "stm32f4xx.h"

/* Flash user area definition *************************************************/
/*
   IMPORTANT NOTE:
   ==============
   Be sure that USER_FLASH_FIRST_PAGE_ADDRESS do not overlap with IAP code.
   For example, with all option enabled the total readonly memory size using EWARM
   compiler v6.10, with high optimization for size, is 39928 bytes (32894 bytes
   of readonly code memory and 7034 bytes of readonly data memory).
   However, with TrueSTUDIO, RIDE and TASKING toolchains the total readonly memory
   size exceeds 48 Kbytes, with this result four sectors of 16 Kbytes each will
   be used to store the IAP code, so the user Flash address will start from Sector4.

   In this example the define USER_FLASH_FIRST_PAGE_ADDRESS is set to 64K boundary,
   but it can be changed to any other address outside the 1st 64 Kbytes of the Flash.
   */
#define USER_FLASH_FIRST_PAGE_ADDRESS 0x08010000
#define USER_FLASH_LAST_PAGE_ADDRESS  0x081A0000
#define USER_FLASH_END_ADDRESS        0x081BFFFF

#endif /* TEKDAQC_BOOTLOADER_H_ */
