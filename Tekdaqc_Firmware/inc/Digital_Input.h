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
 *
 * @modified by Pachia Cha (pcha@tenkiv.com)
 * @since v1.2.0.0
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

#define DIGITAL_SAMPLES_BUFFER_SIZE 100

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


typedef struct {
int iChannel;
DigitalLevel_t iLevel;
uint64_t ui64TimeStamp;
} Digital_Samples_t;

//purpose:
//calculate new digital sampling rate
//send slow network message
//prevent missing samples
typedef struct {
	uint32_t digiRate;		//digital sampling rate
	uint8_t digiInput;		//total number of digital inputs
	uint16_t serverFull;	//consecutive failure to send - adjust sampling rate accordingly
	uint16_t serverTrack;	//hold highest serverFull value
	uint8_t bufScale;		//estimate value to calc digiRate
	bool bufferFree;		//flag server buffer is full
	bool sentMessage;		//flag to send slow network message
	bool slowAnalog;		//flag to send analog network message
	bool slowDigi;			//flag to send digital network message
} slowNet_t;

//pwm input params
typedef struct {
	uint64_t average;					//average sampling time
	uint64_t stopTime;				//time to send average results
	uint64_t prevTime;				//time of last transition
	uint64_t totalTimeOn;			//total time sample was a '1'
	uint64_t totalTimeOff;			//total time sample was a '0'
	int32_t totalTransitions;		//total transition within average time
	DigitalLevel_t startLevel;		//starting reading - '0' / '1'
	DigitalLevel_t level;			//last reading - '0' / '1'
	int64_t samples;				//number of samples to take
	char name[MAX_DIGITAL_INPUT_NAME_LENGTH];	//name of pwm input
} pwmInput_t;

typedef struct {
	uint8_t channel;
	float dutycycle;
	uint16_t totalTransitions;
	uint64_t timeStamp;
} pwmInputBuffer_t;
	
/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Initializes the boards digital input data structures.
 */
void DigitalInputsInit(void);
void WriteToTelnet_Digital(void);
void ReadDigitalInputs(void);
void DigitalInputHalt(void);

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
 * @brief Configures a pwm input with the specified parameters.
 */
Tekdaqc_Function_Error_t CreatePwmInput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @brief Removes a pwm input, marking it for exclusion from the state machine.
 */
Tekdaqc_Function_Error_t removePwmInput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

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
 * @brief Initialize slow network parameters
 */
void initializeSlowNet (void);

/**
 * @brief Reset digital sampling rate parametes
 */
void rstMessRate (void);
	
/**
 * @brief Initialize pwm input params
 */
void initializePwmInput (void);

/**
 * @brief Get pwm input
 */
pwmInput_t* GetPwmInputByNumber(uint8_t number);

/**
 * @brief Halt pwm input
 */
void PwmInputHalt(void);

/**
 * @brief Sample pwm input
 */
void readPwmInput(void);

/**
 * @brief Set up Pwm input for sampling
 */
void startPwmInput(uint64_t samples);

/**
 * @brief List all Pwm inputs
 */
Tekdaqc_Function_Error_t ListPwmInputs(void);

/**
 * @brief Write pwm input sample data to telnet
 */
void WriteToTelnet_PwmInput (void);

/**
 * @brief Read digital pin value
 */
DigitalLevel_t ReadGPI_Pin(GPI_TypeDef gpi);
	
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
