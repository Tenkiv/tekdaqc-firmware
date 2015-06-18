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
 * @file Tekdaqc_Config.h
 * @brief Header file for the configuring the Tekdaqc.
 *
 * Contains public definitions and data types for configuring the Tekdaqc.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM324xG_EVAL_H
#define __STM324xG_EVAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "stm32f4xx.h"
#include "stm32f4xx_usart.h"
#include "Tekdaqc_BSP.h"
#include "stm32f4x7_eth_bsp.h"
#include "ADS1256_Driver.h"
#include "Tekdaqc_MessageHeaders.h"

/** @addtogroup tekdaqc_firmware_libraries Tekdaqc Firmware Libraries
 * @{
 */

/** @addtogroup tekdaqc_configuration Tekdaqc Configuration
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

extern __IO uint32_t LSIPeriodValue;
extern __IO uint32_t LSICaptureNumber;

extern char TOSTRING_BUFFER[SIZE_TOSTRING_BUFFER];
extern unsigned char TEKDAQC_BOARD_SERIAL_NUM[BOARD_SERIAL_NUM_LENGTH + 1]; /* 32 chars plus NULL termination */

extern __IO bool isSelfCalibrated;

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED TYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief A common function pointer for specifying a function to write string data.
 *
 * A common function which is used to specify a function to write string data to an arbitrary destination.
 * This abstracts the code doing the writing from the code wanting to write.
 */
typedef void (*WriteFunction)(char* str);

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Initializes the Tekdaqc's watchdog timer.
 */
void Watchdog_Init(void);

/**
 * @brief Initializes the Tekdaqc's ethernet communications and lwIP stack.
 */
void Communication_Init(void);

/**
 * @brief Initializes the Tekdaqc's FLASH disk.
 */
void FlashDiskInit(void);

/**
 * @brief Retrieves the serial number of this Tekdaqc.
 */
void GetSerialNumber(unsigned char* ptr);

/**
 * @brief Initializes the specified COM port.
 */
void COMInit(COM_TypeDef COM, USART_InitTypeDef* USART_InitStruct);

/**
 * @brief Converts a digital logic level enum value to a human readable string.
 */
const char* DigitalLevelToString(DigitalLevel_t level);

/**
 * @brief Clears the global TOSTRING_BUFFER data structure.
 */
void ClearToStringBuffer(void);

/**
 * @brief Disables all interrupts used by the board.
 */
void DisableBoardInterrupts(void);

/**
 * @brief Enables all interrupts used by the board.
 */
void EnableBoardInterrupts(void);

#ifdef DEBUG
/**
 * @internal
 * @brief Initializes the specified test pin.
 */
void TestPin_Init(TestPin_TypeDef pin);

/**
 * @internal
 * @brief Turns the specified test pin on.
 */
void TestPin_On(TestPin_TypeDef pin);

/**
 * @internal
 * @brief Turns the specified test pin off.
 */
void TestPin_Off(TestPin_TypeDef pin);

/**
 * @internal
 * @brief Toggles the state of the specified test pin.
 */
void TestPin_Toggle(TestPin_TypeDef pin);

/**
 * @internal
 * @brief Initialize COM3 interface for serial debug.
 */
void DebugComPort_Init(void);
#endif /* DEBUG */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __STM324xG_EVAL_H */
