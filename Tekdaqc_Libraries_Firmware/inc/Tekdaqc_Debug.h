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
 * @file Tekdaqc_Debug.h
 * @brief Header file to define precompiler variables for debugging sections of code.
 *
 * Contains pre-processor directives to enable/disable sections of debug code.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TEKDAQC_DEBUG_H
#define TEKDAQC_DEBUG_H

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

/** @addtogroup driver_debug Driver Debug
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @def ADS1256_DEBUG
 * @brief Used to turn on debugging `printf` statements for the ADS1256 ADC driver.
 */
/*#define ADS1256_DEBUG */
/*#define ADS1256_SPI_DEBUG */

/**
 * @internal
 * @def TLE7232_DEBUG
 * @brief Used to turn on debugging `printf` statements for the TLE7232 driver.
 */
//#define TLE7232_DEBUG
//#define TLE7232_SPI_DEBUG

/**
 * @internal
 * @def ANALOGINPUT_DEBUG
 * @brief Used to turn on debugging `printf` statements for the Tekdaqc_Input data type.
 */
//#define ANALOGINPUT_DEBUG

/**
 * @internal
 * @def ADC_STATE_MACHINE_DEBUG
 * @brief Used to turn on debugging `printf` statements for the ADC state machine.
 */
//#define ADC_STATE_MACHINE_DEBUG

/**
 * @internal
 * @def BOARD_TEMPERATURE_DEBUG
 * @brief Used to turn on debugging `printf` statements for board temperature sampling.
 */
/*#define BOARD_TEMPERATURE_DEBUG */

/**
 * @internal
 * @def DI_STATE_MACHINE_DEBUG
 * @brief Used to turn on debugging `printf` statements for the DI state machine.
 */
//#define DI_STATE_MACHINE_DEBUG

/**
 * @internal
 * @def DO_STATE_MACHINE_DEBUG
 * @brief Used to turn on debugging `printf` statements for the DO state machine.
 */
//#define DO_STATE_MACHINE_DEBUG

/**
 * @internal
 * @def DIGITALINPUT_DEBUG
 * @brief Used to turn on debugging `printf` statements for the Tekdaqc_Input data type.
 */
//#define DIGITALINPUT_DEBUG

/**
 * @internal
 * @def DIGITALOUTPUT_DEBUG
 * @brief Used to turn on debugging `printf` statements for the Tekdaqc_Output data type.
 */
//#define DIGITALOUTPUT_DEBUG

/**
 * @internal
 * @def COMMAND_DEBUG
 * @brief Used to turn on debugging `printf` statements for the Tekdaqc command parser.
 */
//#define COMMAND_DEBUG

/**
 * @internal
 * @def TEKDAQC_OUTPUT_DEBUG
 * @brief
 */
//#define TEKDAQC_OUTPUT_DEBUG

/**
 * @internal
 * @def INPUT_MULTIPLEXER_DEBUG
 * @brief Used to turn on debugging `printf` statements for the Analog Input multiplexer.
 */
//#define INPUT_MULTIPLEXER_DEBUG

/**
 * @internal
 * @def TELNET_DEBUG
 * @brief Used to turn on debugging `printf` statements for the telnet server.
 */
/* #define TELNET_DEBUG */

/**
 * @internal
 * @def TELNET_CHAR_DEBUG
 * @brief Used to turn on debugging `printf` statements for the telnet server on an individual character level.
 */
/*#define TELNET_CHAR_DEBUG */

/**
 * @internal
 * @def CALIBRATION_DEBUG
 * @brief Used to turn on debugging `printf` statements for the calibration process.
 */
/*#define CALIBRATION_DEBUG */

/**
 * @internal
 * @def CALIBRATION_TABLE_DEBUG
 * @brief Used to turn on debugging `printf` statements for the flash calibration table.
 */
//#define CALIBRATION_TABLE_DEBUG

/**
 * @internal
 * @def LOCATOR_DEBUG
 * @brief Used to turn on debugging `printf` statements for the Tekdaqc locator service.
 */
/* #define LOCATOR_DEBUG */

/**
 * @internal
 * @def SERIAL_DEBUG
 * @brief Used to enable retarget of printf to serial port for debug purpose
 */
#define SERIAL_DEBUG

/**
 * @internal
 * @def SWV_DEBUG
 * @brief Used to enable retarget of `printf` to SWV ITM Port 0.
 */
/*#define SWV_DEBUG */

/**
 * @}
 */

/**
 * @}
 */

void Debug_Hexdump(char* desc, void* addr, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* TEKDAQC_DEBUG_H */
