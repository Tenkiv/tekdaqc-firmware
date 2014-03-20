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
 * @file Digital_Input.h
 * @brief Header file for the Digital_Input_t data structure.
 *
 * Contains public definitions and data types for the Digital_Input_t data structure.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TEKDAQC_DIGITALINPUT_H_
#define TEKDAQC_DIGITALINPUT_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "Tekdaqc_Config.h"
#include "Tekdaqc_Error.h"
#include <boolean.h>

/** @addtogroup tekdaqc_firmware Tekdaqc Firmware
 * @{
 */

/** @addtogroup digital_input Digital Input
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @def MAX_DIGITAL_INPUT_NAME_LENGTH
 * @brief The maximum number of characters for the name of an digital input.
 */
#define MAX_DIGITAL_INPUT_NAME_LENGTH 24



/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED TYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Data structure used to store the state and requirements of a digital input to the Tekdaqc.
 * This data structure contains all the information related to a particular input to the Tekdaqc. Please
 * note that while there is nothing to stop you from manipulating the values of the structure directly, it
 * is not recommended as it could put the structure in an indeterminate state. Instead, manipulation
 * functions are provided which will ensure that all state related implications are addressed.
 */
typedef struct {
	ChannelAdded_t added; /**< Addition status of the input. */
	GPI_TypeDef input; /**< An integer id for this input. */
	char name[MAX_DIGITAL_INPUT_NAME_LENGTH]; /**< Pointer to a C string name for this input. */
	DigitalLevel_t level; /**< The recorded status of this input. */
	uint64_t timestamp; /**< The timestamp of the measurement in UNIX epoch format. */
} Digital_Input_t;



/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Initializes the boards digital input data structures.
 */
void DigitalInputsInit(void);

/**
 * @brief Retrieves the requested digital input.
 */
Digital_Input_t* GetDigitalInputByNumber(uint8_t number);

/**
 * @brief Adds a digital input, marking it for inclusion in the state machine.
 */
Tekdaqc_Function_Error_t AddDigitalInput(Digital_Input_t* input);

/**
 * @brief Removes a digital input, marking it for exclusion from the state machine.
 */
Tekdaqc_Function_Error_t RemoveDigitalInput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], int count);

/**
 * @brief Prints a representation of all the added digital inputs.
 */
Tekdaqc_Function_Error_t ListDigitalInputs(void);

/**
 * @brief Configures a digital input with the specified parameters.
 */
Tekdaqc_Function_Error_t CreateDigitalInput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], int count);

/**
 * @brief Sets the pointer to the function to invoke when digital input data needs to be written.
 */
void SetDigitalInputWriteFunction(WriteFunction writeFunction);

/**
 * @brief Samples the specified digital input's level and writes out the result.
 */
void SampleDigitalInput(Digital_Input_t* input);

/**
 * @brief Samples the digital input level of all added digital inputs, writing out the results.
 */
void SampleAllDigitalInputs(void);

/**
 * @brief Writes out the data for the specified digital input.
 */
void WriteDigitalInput(Digital_Input_t* input);

/**
 * @brief Writes out the data for all added digital inputs.
 */
void WriteAllDigitalInputs(void);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* TEKDAQC_DIGITALINPUT_H_ */
