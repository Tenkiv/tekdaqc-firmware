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
 * @file Digital_Output.h
 * @brief Header file for the Digital_Output_t data structure.
 *
 * Contains public definitions and data types for the Digital_Output_t data structure.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef DIGITAL_OUTPUT_H_
#define DIGITAL_OUTPUT_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "Tekdaqc_Config.h"
#include "TLE7232_RelayDriver.h"
#include "Tekdaqc_Error.h"
#include "Tekdaqc_BSP.h"
#include "boolean.h"

/** @addtogroup tekdaqc_firmware Tekdaqc Firmware
 * @{
 */

/** @addtogroup digital_output Digital Output
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @def MAX_DIGITAL_OUTPUT_NAME_LENGTH
 * @brief The maximum number of characters for the name of an digital output.
 */
#define MAX_DIGITAL_OUTPUT_NAME_LENGTH 24

/**
 * @def OUTPUT_ON
 * @brief Redefinition of the LOGIC_LOW definition. Used as a code readability convenience.
 */
#define OUTPUT_ON	(LOGIC_HIGH)

/**
 * @def OUTPUT_OFF
 * @brief Redefinition of the LOGIC_HIGH definition. Used as a code readability convenience.
 */
#define OUTPUT_OFF	(LOGIC_LOW)



/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED TYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Data structure used to store the state and requirements of a digital output of the Tekdaqc.
 * This data structure contains all the information related to a particular input to the Tekdaqc. Please
 * note that while there is nothing to stop you from manipulating the values of the structure directly, it
 * is not recommended as it could put the structure in an indeterminate state. Instead, manipulation
 * functions are provided which will ensure that all state related implications are addressed.
 */
typedef struct {
	ChannelAdded_t added; /**< Addition status of the input. */
	GPO_TypeDef output; /**< An integer id for this input. */
	uint8_t physicalChannel; /**< An integer id for the chip output */
	char name[MAX_DIGITAL_OUTPUT_NAME_LENGTH]; /**< Pointer to a C string name for this input. */
	DigitalLevel_t level; /**< The recorded status of this input. */
	uint64_t timestamp; /**< The timestamp of the measurement in UNIX epoch format. */
	uint64_t fault_timestamp; /**< The timestamp of the first occurrence of a fault. */
	TLE7232_Status_t fault_status; /**< The current fault status of the output. */
} Digital_Output_t;



/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Initializes the boards digital output data structures.
 */
void DigitalOutputsInit(void);

/**
 * @brief Retrieves the requested digital output.
 */
Digital_Output_t* GetDigitalOutputByNumber(uint8_t number);

/**
 * @brief Adds a digital output, marking it for inclusion in the state machine.
 */
Tekdaqc_Function_Error_t AddDigitalOutput(Digital_Output_t* output);

/**
 * @brief Removes a digital output, marking it for exclusion from the state machine.
 */
Tekdaqc_Function_Error_t RemoveDigitalOutput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], int count);

/**
 * @brief Prints a representation of all the added digital outputs.
 */
Tekdaqc_Function_Error_t ListDigitalOutputs(void);

/**
 * @brief Configures a digital output with the specified parameters.
 */
Tekdaqc_Function_Error_t CreateDigitalOutput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], int count);

/**
 * @brief Sets a digital output to the specified state.
 */
Tekdaqc_Function_Error_t SetDigitalOutput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @brief Sets the pointer to the function to invoke when digital output data needs to be written.
 */
void SetDigitalOutputWriteFunction(WriteFunction writeFunction);

/**
 * @brief Samples the specified digital output's level and writes out the result.
 */
void SampleDigitalOutput(Digital_Output_t* output);

/**
 * @brief Samples the digital output level of all added digital outputs, writing out the results.
 */
void SampleAllDigitalOutputs(void);

/**
 * @brief Writes out the data for the specified digital output.
 */
void WriteDigitalOutput(Digital_Output_t* output);

/**
 * @brief Writes out the data for all added digital outputs.
 */
void WriteAllDigitalOutputs(void);

/**
 * @brief Checks the stored status register contents for any errors.
 */
bool CheckDigitalOutputStatus(void);

/**
 * @brief Sets the fault status register on the specified output driver.
 */
bool SetDigitalOutputFaultStatus(TLE7232_Status_t status, uint8_t chip_id, uint8_t channel);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* DIGITAL_OUTPUT_H_ */
