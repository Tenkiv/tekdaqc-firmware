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
 * @file Digital_Input.c
 * @brief Source file for the Digital_Input_t data structure.
 *
 * Contains methods for manipulating and interfacing to the Digital_Input)t data structure.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "Digital_Input.h"
#include "Tekdaqc_BSP.h"
#include "Tekdaqc_CommandInterpreter.h"
#include "Tekdaqc_Timers.h"
#include "boolean.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifdef PRINTF_OUTPUT
#include <stdio.h>
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @def DIGITAL_INPUT_FORMATTER
 * @brief The message format string for printing a digital input to a human readable string.
 */
#define DIGITAL_INPUT_FORMATTER "\n\r--------------------\n\rDigital Input\n\r\tName: %s\n\r\tPhysical Channel: %i\n\r\tTimestamp: %" PRIu64 "\n\r\tLevel: %s\n\r--------------------\n\r\x1E"

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* The function pointer used for writing strings to the data connection */
static WriteFunction writer = NULL;

/* List of external digital inputs */
static Digital_Input_t Ext_DInputs[NUM_DIGITAL_INPUTS];

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @brief Converts a specified GPI input into a human readable string.
 */
static inline const char* GPIToString(GPI_TypeDef gpi);

/**
 * @internal
 * @brief Read the current logic level of the specified GPI pin
 */
static DigitalLevel_t ReadGPI_Pin(GPI_TypeDef gpi);

/**
 * @internal
 * @brief Checks if the specified input is an external input.
 */
static bool isExternalInput(uint8_t id);

/**
 * @internal
 * @brief Initializes the members of the specified input structure.
 */
static void InitializeInput(Digital_Input_t* input);

/**
 * @internal
 * @brief Removes a digital input by specifying the input number.
 */
static void RemoveDigitalInputByID(uint8_t id);

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Converts the specified GPI input into a human readable string.
 *
 * @param gpi GPI_TypeDef The GPI input to convert.
 * @retval const char* The human readable string.
 */
static inline const char* GPIToString(GPI_TypeDef gpi) {
	static const char* strings[] = { "GPI0", "GPI1", "GPI2", "GPI3", "GPI4", "GPI5", "GPI6", "GPI7", "GPI8", "GPI9", "GPI10", "GPI11",
			"GPI12", "GPI13", "GPI14", "GPI15", "GPI16", "GPI17", "GPI18", "GPI19", "GPI20", "GPI21", "GPI22", "GPI23" };
	return strings[gpi];
}

/**
 * Reads the specified GPI and retrieves its digital logic level.
 *
 * @param gpi GPI_TypeDef The GPI input to read.
 * @retval DigitalLevel_t The digital logic level of the input pin.
 */
static DigitalLevel_t ReadGPI_Pin(GPI_TypeDef gpi) {
	uint8_t state = 0;
	switch (gpi) {
	case GPI0:
		state = GPIO_ReadInputDataBit(GPI0_GPIO_PORT, GPI0_PIN );
		break;
	case GPI1:
		state = GPIO_ReadInputDataBit(GPI1_GPIO_PORT, GPI1_PIN );
		break;
	case GPI2:
		state = GPIO_ReadInputDataBit(GPI2_GPIO_PORT, GPI2_PIN );
		break;
	case GPI3:
		state = GPIO_ReadInputDataBit(GPI3_GPIO_PORT, GPI3_PIN );
		break;
	case GPI4:
		state = GPIO_ReadInputDataBit(GPI4_GPIO_PORT, GPI4_PIN );
		break;
	case GPI5:
		state = GPIO_ReadInputDataBit(GPI5_GPIO_PORT, GPI5_PIN );
		break;
	case GPI6:
		state = GPIO_ReadInputDataBit(GPI6_GPIO_PORT, GPI6_PIN );
		break;
	case GPI7:
		state = GPIO_ReadInputDataBit(GPI7_GPIO_PORT, GPI7_PIN );
		break;
	case GPI8:
		state = GPIO_ReadInputDataBit(GPI8_GPIO_PORT, GPI8_PIN );
		break;
	case GPI9:
		state = GPIO_ReadInputDataBit(GPI9_GPIO_PORT, GPI9_PIN );
		break;
	case GPI10:
		state = GPIO_ReadInputDataBit(GPI10_GPIO_PORT, GPI10_PIN );
		break;
	case GPI11:
		state = GPIO_ReadInputDataBit(GPI11_GPIO_PORT, GPI11_PIN );
		break;
	case GPI12:
		state = GPIO_ReadInputDataBit(GPI12_GPIO_PORT, GPI12_PIN );
		break;
	case GPI13:
		state = GPIO_ReadInputDataBit(GPI13_GPIO_PORT, GPI13_PIN );
		break;
	case GPI14:
		state = GPIO_ReadInputDataBit(GPI14_GPIO_PORT, GPI14_PIN );
		break;
	case GPI15:
		state = GPIO_ReadInputDataBit(GPI15_GPIO_PORT, GPI15_PIN );
		break;
	case GPI16:
		state = GPIO_ReadInputDataBit(GPI16_GPIO_PORT, GPI16_PIN );
		break;
	case GPI17:
		state = GPIO_ReadInputDataBit(GPI17_GPIO_PORT, GPI17_PIN );
		break;
	case GPI18:
		state = GPIO_ReadInputDataBit(GPI18_GPIO_PORT, GPI18_PIN );
		break;
	case GPI19:
		state = GPIO_ReadInputDataBit(GPI19_GPIO_PORT, GPI19_PIN );
		break;
	case GPI20:
		state = GPIO_ReadInputDataBit(GPI20_GPIO_PORT, GPI20_PIN );
		break;
	case GPI21:
		state = GPIO_ReadInputDataBit(GPI21_GPIO_PORT, GPI21_PIN );
		break;
	case GPI22:
		state = GPIO_ReadInputDataBit(GPI22_GPIO_PORT, GPI22_PIN );
		break;
	case GPI23:
		state = GPIO_ReadInputDataBit(GPI23_GPIO_PORT, GPI23_PIN );
		break;
	default:
		/* TODO: Throw an error */
		break;
	}
	if (state) {
		return LOGIC_HIGH;
	} else {
		return LOGIC_LOW;
	}
}

/**
 * Checks to see if an input specified by number is an external input.
 *
 * @param id uint8_t The input channel to check.
 * @retval bool TRUE if the specified input is an external input.
 */
static bool isExternalInput(uint8_t id) {
	if (id >= 0 && id < NUM_DIGITAL_INPUTS) {
		return true;
	} else {
		return false;
	}
}

/**
 * Initializes the members of the specified input structure.
 *
 * @param input Digital_Input_t* Pointer/Reference to an digital input structure.
 * @retval none
 */
static void InitializeInput(Digital_Input_t* input) {
	input->added = CHANNEL_NOTADDED;
	input->level = LOGIC_LOW;
	input->timestamp = 0U;
	strcpy(input->name, "NONE");
	input->input = NULL_CHANNEL;
}

/**
 * Removes a digital input structure from the board's list by specifying its physical input channel.
 *
 * @param id uint8_t The physical channel to remove, if present.
 * @retval none
 */
static void RemoveDigitalInputByID(uint8_t id) {
	if (isExternalInput(id)) {
		InitializeInput(&Ext_DInputs[id]);
	} else {
		/* This is out of range */
#ifdef DIGITALINPUT_DEBUG
		printf("[Digital Input] Cannot find the requested digital input. Input does not exist on the board.\n\r");
#endif
	}
}

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Initializes all requisite sub-modules to properly operate digital inputs. This includes any multiplexing and
 * default inputs.
 *
 * @param none
 * @retval none
 */
void DigitalInputsInit(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable the GPIO Clock */
	RCC_AHB1PeriphClockCmd(GPI_GPIO_CLKS, ENABLE);

	/* Configure the GPIO pin */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	/* Configure the Port B Pins */
	GPIO_InitStructure.GPIO_Pin = GPI_PORTB_PINS;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Configure the Port C Pins */
	GPIO_InitStructure.GPIO_Pin = GPI_PORTC_PINS;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* Configure the Port E Pins */
	GPIO_InitStructure.GPIO_Pin = GPI_PORTE_PINS;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	/* Configure the Port F Pins */
	GPIO_InitStructure.GPIO_Pin = GPI_PORTF_PINS;
	GPIO_Init(GPIOF, &GPIO_InitStructure);

	/* Configure the Port G Pins */
	GPIO_InitStructure.GPIO_Pin = GPI_PORTG_PINS;
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	/* Configure the Port H Pins */
	GPIO_InitStructure.GPIO_Pin = GPI_PORTH_PINS;
	GPIO_Init(GPIOH, &GPIO_InitStructure);

	/* Configure the Port I Pins */
	GPIO_InitStructure.GPIO_Pin = GPI_PORTI_PINS;
	GPIO_Init(GPIOI, &GPIO_InitStructure);

	for (int i = 0; i < NUM_DIGITAL_INPUTS; ++i) {
		InitializeInput(&Ext_DInputs[i]);
	}
}

/**
 * Prints a human readable representation of all the added digital inputs via the current write function.
 *
 * @param none
 * @return Tekdaqc_Function_Error_t The error status of this function.
 */
Tekdaqc_Function_Error_t ListDigitalInputs(void) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	uint8_t n = snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "\n\r----------\n\rAdded Digital Inputs\n\r----------\n\r");
	if (n <= 0) {
#ifdef DIGITALINPUT_DEBUG
		printf("Failed to write header of digital input list.\n\r");
#endif
		retval = ERR_DIN_FAILED_WRITE;
	} else {
		if (writer != 0) {
			writer(TOSTRING_BUFFER);
		}
		Digital_Input_t input;
		for (uint_fast8_t i = 0U; i < NUM_DIGITAL_INPUTS; ++i) {
			input = Ext_DInputs[i];
			if (input.added == CHANNEL_ADDED) {
				/* This input has been added */
				n = snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "\tPhysical Input %" PRIi8 ":\n\r\t\tName: %s\n\r", input.input,
						input.name);
				if (n <= 0) {
#ifdef DIGITALINPUT_DEBUG
					printf("Failed to write an digital input to the list.\n\r");
#endif
					retval = ERR_DIN_FAILED_WRITE;
					break;
				} else {
					if (writer != 0) {
						writer(TOSTRING_BUFFER);
					}
				}
			}
		}
	}
	return retval;
}

/**
 * Creates a new digital input data structure from the supplied parameters and adds it to the board's relevant
 * input list.
 *
 * @param keys char** Array of strings containing the command line keys. Indexed with values.
 * @param values char** Array of strings containing the command line values. Indexed with keys.
 * @param count uint8_t The number of parameters passed on the command line.
 * @retval uint8_t The error status code.
 */
Tekdaqc_Function_Error_t CreateDigitalInput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], int count) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	char* param;
	char* testPtr = NULL;
	int8_t index = -1;
	uint8_t input = NULL_CHANNEL; /* The physical input */
	uint8_t in = 255U;
	char name[MAX_DIGITAL_INPUT_NAME_LENGTH]; /* The name */
	strcpy(name, "NONE");
	for (int i = 0; i < NUM_ADD_DIGITAL_INPUT_PARAMS; ++i) {
		index = GetIndexOfArgument(keys, ADD_DIGITAL_INPUT_PARAMS[i], count);
		if (index >= 0) { /* We found the key in the list */
			param = values[index]; /* We use the discovered index for this key */
			switch (i) { /* Switch on the key not position in arguments list */
			case 0U: /* INPUT key */
				in = (uint8_t) strtol(param, &testPtr, 10);
				if (testPtr == param) {
					retval = ERR_DIN_PARSE_ERROR;
				} else {
					if (in >= 0U && in <= NUM_DIGITAL_INPUTS) {
						/* A valid input number */
						input = in;
					} else {
						/* Input number out of range */
#ifdef DIGITALINPUT_DEBUG
						printf("[Digital Input] The requested input number is invalid.\n\r");
#endif
						retval = ERR_DIN_INPUT_OUTOFRANGE;
					}
				}
				break;
			case 1U: /* NAME key */
				strcpy(name, param);
				break;
			default:
				retval = ERR_DIN_PARSE_ERROR;
			}
		} else if (i == 1U) {
			/* The NAME key is not strictly required, apply the defaults */
			continue;
		} else {
			/* Somehow an error happened */
#ifdef DIGITALINPUT_DEBUG
			printf("[Digital Input] Unable to locate required key: %s\n\r", ADD_DIGITAL_INPUT_PARAMS[i]);
#endif
			retval = ERR_DIN_PARSE_MISSING_KEY; /* Failed to locate a key */
		}
		if (retval != ERR_FUNCTION_OK) {
			break;
		}
	}
	if (retval == ERR_FUNCTION_OK) {
		if (input != NULL_CHANNEL ) {
			Digital_Input_t* dig_input = GetDigitalInputByNumber(input);
			if (dig_input != NULL ) {
				if (dig_input->added == CHANNEL_NOTADDED) {
					dig_input->input = input;
					strcpy(dig_input->name, name);
					dig_input->level = LOGIC_LOW;
					dig_input->timestamp = 0U;
					AddDigitalInput(dig_input);
				} else {
					retval = ERR_DIN_INPUT_EXISTS;
				}
			} else {
				/* The input could not be found */
				return ERR_DIN_INPUT_NOT_FOUND;
			}
		} else {
			/* A valid input was never specified */
			retval = ERR_DIN_INPUT_UNSPECIFIED;
		}
	}
	return retval;
}

/**
 * Adds a digital input structure to the board's appropriate list of inputs.
 *
 * @param input Digital_Input_t* The digital input structure to add.
 * @retval none
 */
void AddDigitalInput(Digital_Input_t* input) {
	uint8_t index = input->input;
	if (index < NUM_DIGITAL_INPUTS) {
		/* This is a valid digital input */
		input->added = CHANNEL_ADDED;
#ifdef DIGITALINPUT_DEBUG
		printf("[Digital Input] Added input to external inputs list.\n\r");
#endif
	} else {
		/* This is out of range */
#ifdef DIGITALINPUT_DEBUG
		printf("[Digital Input] Cannot add the provided digital input structure. Input field is out of range.\n\r");
#endif
	}
}

/**
 * Removes a digital input from the board's list based on the supplied parameters.
 *
 * @param keys char** Array of strings containing the command line keys. Indexed with values.
 * @param values char** Array of strings containing the command line values. Indexed with keys.
 * @param count uint8_t The number of parameters passed on the command line.
 * @retval int The error status code.
 */
Tekdaqc_Function_Error_t RemoveDigitalInput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], int count) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	char* param;
	int8_t index = -1;
	for (uint_fast8_t i = 0U; i < NUM_REMOVE_DIGITAL_INPUT_PARAMS; ++i) {
		index = GetIndexOfArgument(keys, REMOVE_DIGITAL_INPUT_PARAMS[i], count);
		if (index >= 0) { /* We found the key in the list */
			param = values[index]; /* We use the discovered index for this key */
			switch (i) { /* Switch on the key not position in arguments list */
			case 0U: { /* INPUT key */
				char* testPtr = NULL;
				uint8_t in = (uint8_t) strtol(param, &testPtr, 10);
				if (testPtr == param) {
					retval = ERR_DIN_PARSE_ERROR;
				} else {
					if (isExternalInput(in)) {
						/* A valid input number */
						RemoveDigitalInputByID(in);
					} else {
						/* Input number out of range */
#ifdef DIGITALINPUT_DEBUG
						printf("[Digital Input] The requested input number is invalid.\n\r");
#endif
						retval = ERR_DIN_INPUT_OUTOFRANGE;
					}
				}
				break;
			}
			default:
				retval = ERR_DIN_PARSE_ERROR;
			}
		} else {
			/* Somehow an error happened */
#ifdef DIGITALINPUT_DEBUG
			printf("[Digital Input] Unable to locate required key: %s\n\r", REMOVE_DIGITAL_INPUT_PARAMS[i]);
#endif
			retval = ERR_DIN_PARSE_MISSING_KEY; /* Failed to locate a key */
		}
	}
	return retval;
}

/**
 * Retrieve a digital input structure by specifying the physical input channel.
 *
 * @param number uint8_t The physical input channel to retrieve.
 * @retval Digital_Input_t* Pointer to the digital input structure.
 */
Digital_Input_t* GetDigitalInputByNumber(uint8_t number) {
	if (isExternalInput(number)) {
		return &Ext_DInputs[number];
	} else {
		/* This is out of range */
#ifdef DIGITALINPUT_DEBUG
		printf("[Digital Input] Cannot find the requested digital input. Input does not exist on the board.\n\r");
#endif
		return NULL ;
	}
}

/**
 * Reads the state of a digital input and stores it in the internal buffer.
 *
 * @param input Digital_Input_t The data structure of the digital input to sample.
 * @retval none
 */
void SampleDigitalInput(Digital_Input_t* input) {
	input->timestamp = GetLocalTime();
	input->level = ReadGPI_Pin(input->input);
}

/**
 * Reads the state of all digital inputs which have been added to the device and stores the result in the respective internal buffers.
 *
 * @param none
 * @retval none
 */
void SampleAllDigitalInputs(void) {
	Digital_Input_t input;
	for (int i = 0; i < NUM_DIGITAL_INPUTS; ++i) {
		input = Ext_DInputs[i];
		if (input.added == CHANNEL_ADDED) {
			SampleDigitalInput(&input);
		}
	}
}

/**
 * Set the function pointer to use when writing data from a digital input to the data connection.
 *
 * @param writeFunction WriteFunction pointer to the desired string writing function.
 * @retval none
 */
void SetDigitalInputWriteFunction(WriteFunction writeFunction) {
	writer = writeFunction;
}

/**
 * Writes the data for the provided Digital_Input_t structure to the stream controlled by the WriteFunction, if set.
 *
 * @param input Digital_Input_t* Pointer to the data structure to write out.
 * @retval none
 */
void WriteDigitalInput(Digital_Input_t* input) {
	uint8_t retval = sprintf(TOSTRING_BUFFER, DIGITAL_INPUT_FORMATTER, input->name, input->input, input->timestamp,
			DigitalLevelToString(input->level));
	if (retval >= 0) {
		if (writer != NULL ) {
			writer(TOSTRING_BUFFER);
		} else {
#ifdef DIGITALINPUT_DEBUG
			printf("[Digital Input] Cannot write digital input to string due to NULL write function.\n\r");
#endif
		}
	} else {
#ifdef DIGITALINPUT_DEBUG
		printf("[Digital Input] Error occurred while writing digital input to string.\n\r");
#endif
	}
}

/**
 * Writes out the data for all added digital inputs to the stream controlled by the WriteFunction, if set.
 *
 * @param none
 * @retval none
 */
void WriteAllDigitalInputs(void) {
	if (writer != NULL ) {
		Digital_Input_t input;
		for (int i = 0; i < NUM_DIGITAL_INPUTS; ++i) {
			input = Ext_DInputs[i];
			if (input.added == CHANNEL_ADDED) {
				WriteDigitalInput(&input);
			}
		}
	}
}
