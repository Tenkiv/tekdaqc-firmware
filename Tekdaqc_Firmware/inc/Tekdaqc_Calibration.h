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
 * @file Tekdaqc_Calibration.h
 * @brief Header file for the Tekdaqc's calibration functions.
 *
 * Contains public function definitions for the Tekdaqc's calibration functions..
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TEKDAQC_CALIBRATION_H_
#define TEKDAQC_CALIBRATION_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_BSP.h"
#include "Tekdaqc_Error.h"
#include "boolean.h"

/** @addtogroup tekdaqc_firmware Tekdaqc Firmware
 * @{
 */

/** @addtogroup tekdaqc_calibration Calibration
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Performs a system gain calibration with specified parameters.
 */
Tekdaqc_Function_Error_t PerformSystemGainCalibration(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @brief Performs a self system calibration, determining offset values and base gain values.
 */
Tekdaqc_Function_Error_t PerformSystemCalibration(void);

/**
 * @brief Determines if the calibration data in the FLASH memory is valid or not.
 */
bool isTekdaqc_CalibrationValid(void);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* TEKDAQC_CALIBRATION_H_ */
