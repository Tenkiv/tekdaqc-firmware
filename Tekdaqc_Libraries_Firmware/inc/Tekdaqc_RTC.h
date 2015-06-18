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
 * @file Tekdaqc_RTC.h
 * @brief Header file for the Tekdaqc RTC driver.
 *
 * Contains public definitions and data types for the Tekdaqc RTC driver.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TEKDAQC_RTC_H_
#define TEKDAQC_RTC_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif
  
/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "stm32f4xx.h"

/** @addtogroup tekdaqc_firmware_libraries Tekdaqc Firmware Libraries
 * @{
 */

/** @addtogroup rtc_driver RTC Driver
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* RTC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Configure the RTC domain of the processor.
 */
void RTC_Config(uint32_t synch_prediv, uint32_t asynch_prediv);

/**
 * @brief Initialize the RTC time to 0.
 */
void RTC_ZeroTime(void);
/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* TEKDAQC_RTC_H_ */
