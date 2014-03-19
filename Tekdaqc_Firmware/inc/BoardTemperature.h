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
 * @file BoardTemperature.h
 * @brief Header file for the Tekdaqc board temperature monitor.
 *
 * Contains public definitions and data types for the Tekdaqc's board temperature monitor.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BOARDTEMPERATURE_H_
#define BOARDTEMPERATURE_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Analog_Input.h"

/** @addtogroup tekdaqc_firmware Tekdaqc Firmware
 * @{
 */

/** @addtogroup board_temperature Board Temperature Monitor
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Updates the current board temperature reading.
 */
void updateBoardTemperature(Analog_Input_t* input, int32_t code);

/**
 * @brief Retrieves the current board temperature reading.
 */
float getBoardTemperature(void);

/**
 * @brief Retrieves the maximum temperature this board has been exposed to.
 */
float getMaximumBoardTemperature(void);

/**
 * @brief Retrieves the minimum temperature this board has been exposed to.
 */
float getMinimumBoardTemperature(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

/**
 * @}
 */

#endif /* BOARDTEMPERATURE_H_ */
