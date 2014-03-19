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
 * @file AnalogInput_Multiplexer.h
 * @brief Header file for the analog input multiplexer.
 *
 * Contains the public function prototypes for driving the input multiplexer.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef ANALOGINPUT_MUTLIPLEXER_H
#define ANALOGINPUT_MUTLIPLEXER_H

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "Tekdaqc_Config.h"
#include "Analog_Input.h"
#include "boolean.h"

/** @addtogroup tekdaqc_firmware Tekdaqc Firmware
 * @{
 */

/** @addtogroup analog_input_multiplexer Analog Input Multiplexer
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED TYPES */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Determines if the specified input is an external analog input.
 */
bool isExternalInput(PhysicalAnalogInput_t input);

/**
 * @brief Determines if the specified input is an internal analog input.
 */
bool isInternalInput(PhysicalAnalogInput_t input);

/**
 * @brief Checks to see if the external input muxing process has completed.
 */
bool isExternalMuxingComplete(void);

/**
 * @brief Initializes the analog input multiplexer.
 */
void InputMultiplexerInit(void);

/**
 * @brief Selects the specified analog input.
 */
void SelectAnalogInput(Analog_Input_t* input);

/**
 * @brief Selects the specified physical input.
 */
void SelectPhysicalInput(PhysicalAnalogInput_t input);

/**
 * @brief Selects the designated offset calibration input.
 */
void SelectCalibrationInput(void);

/**
 * @brief Selects the designated cold junction sensor input.
 */
void SelectColdJunctionInput(void);

/**
 * @brief Reselects the selected input if we swithced away from it.
 */
void ResetSelectedInput(void);

/**
 * @brief Determines the external input for the specified input index.
 */
ExternalMuxedInput_t GetExternalMuxedInputByNumber(uint8_t input);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

/**
 * @}
 */

#endif /* ANALOGINPUT_MUTLIPLEXER_H */
