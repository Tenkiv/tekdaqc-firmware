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
 * @file Tekdaqc_Timers.h
 * @brief Header file for the various timers used by the Tekdaqc.
 *
 * Contains public definitions and data types for the times used by the Tekdaqc.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TEKDAQC_TIMERS_H_
#define TEKDAQC_TIMERS_H_

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

/** @addtogroup tekdaqc_timers Tekdaqc Timers
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @def SYSTEMTICK_PERIOD_US
 * @brief Defines the time period in microseconds for SYSTICK interrupts.
 */
#define SYSTEMTICK_PERIOD_US 			1000

/**
 * @def SYSTEMTICK_DIVIDER_US
 * @brief Defines the clock divider for the SYSTICK timer.
 */
#define SYSTEMTICK_DIVIDER_US 			(1000000 / SYSTEMTICK_PERIOD_US)

/**
 * @def SYSTEMTICK_PERIOD
 * @brief Defines the time period to use for the SYSTICK interrupts. This is what is used by the setup code.
 */
#define SYSTEMTICK_PERIOD 				(SYSTEMTICK_PERIOD_US)

/**
 * @def SYSTEMTICK_DIVIDER
 * @brief Defines the clock divider for the SYSTICK timer. This is what is used by the setup code.
 */
#define SYSTEMTICK_DIVIDER 				(SYSTEMTICK_DIVIDER_US)

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Configure the timers.
 */
void Timer_Config(void);

/**
 * @brief Called by the SYSTICK interrupt handler.
 */
void Time_Update(void);

/**
 * @brief Retrieve the local time stamp.
 */
uint64_t GetLocalTime(void);

/**
 * @brief Blocking delay, measured in fractional milliseconds.
 */
void Delay_ms(float ms);

/**
 * @brief Blocking delay, measured in microseconds.
 */
void Delay_us(uint64_t us);

/**
 * @brief Blocking delay, measured in 10 millisecond periods.
 */
void Delay_Periods_10MS(uint32_t nCount);

/**
 * @brief Blocking delay, measured in SYSTICK periods.
 */
void Delay_Periods(uint32_t nCount);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* TEKDAQC_TIMERS_H_ */
