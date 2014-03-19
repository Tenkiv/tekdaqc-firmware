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
 * @file ethernetif.h
 * @brief Header file for the Ethernet interface.
 *
 * Contains public definitions and data types for the Ethernet interface.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "lwip/err.h"
#include "lwip/netif.h"

/** @addtogroup tekdaqc_firmware_libraries Tekdaqc Firmware Libraries
 * @{
 */

/** @addtogroup ethernet_driver Ethernet Driver
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* INTERFACE METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Initialize the Ethernet interface.
 */
err_t ethernetif_init(struct netif *netif);

/**
 * @brief Called when a packet is ready to be read from the interface.
 */
err_t ethernetif_input(struct netif *netif);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ETHERNETIF_H__ */
