/*
 * Copyright 2013 Tenkiv, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 */

/**
 * @file Tekdaqc_Locator.h
 * @brief Header file for the Tekdaqc Locator service.
 *
 * Contains public definitions and data types for the Tekdaqc Locator service.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TEKDAQC_LOCATOR_H_
#define TEKDAQC_LOCATOR_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "stm32f4xx.h"

/** @addtogroup tekdaqc_firmware_libraries Tekdaqc Firmware Libraries
 * @{
 */

/** @addtogroup tekdaqc_locator Tekdaqc Locator Service
 * @{
 */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Initializes the Tekdaqc locator service.
 */
void Tekdaqc_LocatorInit(void);

/**
 * @brief Sets the board type in the locator response packet.
 */
void Tekdaqc_LocatorBoardTypeSet(uint8_t type);

/**
 * @brief Sets the board ID in the locator response packet.
 */
void Tekdaqc_LocatorBoardIDSet(const unsigned char* id);

/**
 * @brief Sets the client IP address in the locator response packet.
 */
void Tekdaqc_LocatorClientIPSet(uint32_t ip);

/**
 * @brief Sets the MAC address in the locator response packet.
 */
void Tekdaqc_LocatorMACAddrSet(const unsigned char MAC[]);

/**
 * @brief Sets the firmware version in the locator response packet.
 */
void Tekdaqc_LocatorVersionSet(uint32_t version);

/**
 * @brief Sets the application title in the locator response packet.
 */
void Tekdaqc_LocatorAppTitleSet(const unsigned char *title);

/**
 * @brief Retrieves the board type from the locator packet data.
 */
uint8_t Tekdaqc_GetLocatorBoardType(void);

/**
 * @brief Retrieves the board ID from the locator packet data.
 */
unsigned char* Tekdaqc_GetLocatorBoardID(void);

/**
 * @brief Retrieves the board IP address from the locator packet data.
 */
uint32_t Tekdaqc_GetLocatorIp(void);

/**
 * @brief Retrieves the board MAC address from the locator packet data.
 */
unsigned char* Tekdaqc_GetLocatorMAC(void);

/**
 * @brief Retrieves the board firmware version from the locator packet data.
 */
uint32_t Tekdaqc_GetLocatorVersion(void);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* TEKDAQC_LOCATOR_H_ */
