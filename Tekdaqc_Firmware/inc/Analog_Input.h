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
 * @file Analog_Input.h
 * @brief Header file for the Tekdaqc_Input data structure.
 *
 * Contains public definitions and data types for the Tekdaqc_Input data structure.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 *
 * @modified by Pachia Cha (pcha@tenkiv.com)
 * @since v1.2.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TEKDAQC_ANALOGINPUT_H_
#define TEKDAQC_ANALOGINPUT_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "Tekdaqc_Error.h"
#include "stm32f4xx.h"
#include "Tekdaqc_Config.h"
#include "ADS1256_Driver.h"
#include "boolean.h"

/** @addtogroup tekdaqc_firmware Tekdaqc Firmware
 * @{
 */

/** @addtogroup analog_input Analog Input
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @def MAX_ANALOG_INPUT_NAME_LENGTH
 * @brief The maximum number of characters for the name of an analog input.
 */
#define MAX_ANALOG_INPUT_NAME_LENGTH 24

/**
 * @def ANALOG_INPUT_BUFFER_SIZE
 * @brief The number or readings to store in the circular buffer for the input.
 */
#define ANALOG_INPUT_BUFFER_SIZE	50U /* 50 samples. Other code expects it to <= 255. */


//lfao-defines the size of the buffer where samples taken from the DRDY interrupt handler are written
#define ANALOG_SAMPLES_BUFFER_SIZE 100
/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED TYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Tekdaqc input status enumeration.
 * Defines the possible status states of an input to the Tekdaqc, both analog and digital.
 */
typedef enum {
	BELOW_RANGE, /**< The current value is below the minimum of the range. */
	IN_RANGE, /**< The current value is in the allowable range. */
	ABOVE_RANGE, /**< The current value is above the maximum of the range. */
} AnalogInputStatus_t;

/**
 * @brief Data structure used to store the state and requirements of an analog input to the Tekdaqc.
 * This data structure contains all the information related to a particular input to the Tekdaqc, including values and allowable range.
 * Please note that while there is nothing to stop you from manipulating the values of the structure directly, it is not recommended as
 * it could put the structure in an indeterminate state. Instead, manipulation functions are provided which will ensure that all state
 * related implications are addressed.
 */
typedef struct {
	ChannelAdded_t added; /**< Addition status of the input. */
	PhysicalAnalogInput_t physicalInput; /**< The physical input for this input. */
	ExternalMuxedInput_t externalInput; /**< If an external input, which channel. */
	InternalAnalogInput_t internalInput; /**< If an internal input, which channel. */
	char name[MAX_ANALOG_INPUT_NAME_LENGTH]; /**< Pointer to a C string name for this input. */
	uint8_t bufferReadIdx; /**< The index of the buffer to read data from. */
	uint8_t bufferWriteIdx; /**< The index of the buffer to write data to. */
	AnalogInputStatus_t status; /**< The current status of this input. */
	ADS1256_BUFFER_t buffer; /**< Analog buffer state to use. */
	ADS1256_PGA_t gain; /**< Gain setting to use for analog measurements. */
	ADS1256_SPS_t rate; /**< Sample rate to use for measurements. */
	int32_t min; /**< The low value of the allowable range of this input. */
	int32_t max; /**< The high value of the allowable range of this input. */
	int32_t values[ANALOG_INPUT_BUFFER_SIZE]; /**< The recorded values of this input (ADC Counts). */
	uint64_t timestamps[ANALOG_INPUT_BUFFER_SIZE]; /**< The timestamps of the measurements in UNIX epoch format. */
} Analog_Input_t;


typedef struct {
int iChannel;
uint32_t iReading;
uint64_t ui64TimeStamp;
} Analog_Samples_t;

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* INITIALIZATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Initialize all of the submodules for analog inputs.
 */
void AnalogInputsInit(void);
void InitAnalogSamplesBuffer(void);
int WriteSampleToBuffer(Analog_Samples_t *Data);
int ReadSampleFromBuffer(Analog_Samples_t *Data);
void WriteToTelnet_Analog(void);
void AnalogChannelHandler(void);
void AnalogHalt(void);
/*--------------------------------------------------------------------------------------------------------*/
/* INPUT ADD/REMOVE METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Creates a new analog input structure and adds it to the board's list.
 */
Tekdaqc_Function_Error_t CreateAnalogInput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @brief Adds an analog input to the board's list.
 */
Tekdaqc_Function_Error_t AddAnalogInput(Analog_Input_t* input);

/**
 * @brief Removes an analog input from the board's list.
 */
Tekdaqc_Function_Error_t RemoveAnalogInput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/*--------------------------------------------------------------------------------------------------------*/
/* UTILITY METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Prints a representation of all the added analog inputs.
 */
Tekdaqc_Function_Error_t ListAnalogInputs(void);

/**
 * @brief Retrieve the analog input structure corresponding to a physical channel.
 */
Analog_Input_t* GetAnalogInputByNumber(uint8_t number);

/**
 * @brief Prints data from the analog input structure to the data connection.
 */
void WriteAnalogInput(Analog_Input_t* input);

/**
 * @brief Sets the function to use for writing strings to the data connection.
 */
void SetAnalogInputWriteFunction(WriteFunction writeFunction);

/**
 * @brief Returns the string representation of an externally muxed analog input.
 */
const char* ExtAnalogInputToString(ExternalMuxedInput_t input);

/**
 * @brief Returns the string representation of an internally muxed analog input.
 */
const char* IntAnalogInputToString(InternalAnalogInput_t input);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

/**
 * @}
 */

#endif /* TEKDAQC_ANALOGINPUT_H_ */
