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
 * @file Analog_Input.c
 * @brief Source file for the Analog_Input_t data structure.
 *
 * Contains methods for manipulating and interfacing to the Analog_Input_t data structure.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "Analog_Input.h"
#include "AnalogInput_Multiplexer.h"
#include "Tekdaqc_CommandInterpreter.h"
#include "ADS1256_Driver.h"
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
 * @def ANALOG_INPUT_HEADER
 * @brief The header format string for printing an analog input to a human readable string.
 */
#define ANALOG_INPUT_HEADER "\n\r--------------------\n\rAnalog Input\n\r\tName: %s\n\r\tPhysical Input: %i\n\r\tPGA: %s\n\r\tRate: %s\n\r\tBuffer Status: %s\n\r--------------------\n\r"

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* The function pointer used for writing strings to the data connection */
static WriteFunction writer = 0;

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE EXTERNAL VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* List of external analog inputs */
Analog_Input_t Ext_AInputs[NUM_EXT_ANALOG_INPUTS];

/* The offset calibration input */
Analog_Input_t Offset_Cal_AInput;

/* List of internal analog inputs */
Analog_Input_t Int_AInputs[NUM_INT_ANALOG_INPUTS];

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @brief Initializes the members of the specified input structure.
 */
static void InitializeInput(Analog_Input_t* input);

/**
 * @internal
 * @brief Removes an analog input from the board's list by physical channel.
 */
static void RemoveAnalogInputByID(uint8_t id);

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Initializes the members of the specified input structure.
 *
 * @param input Analog_Input_t* Pointer/Reference to an analog input structure.
 * @retval none
 */
static void InitializeInput(Analog_Input_t* input) {
	input->buffer = ADS1256_BUFFER_ENABLED;
	input->gain = ADS1256_PGAx1;
	input->rate = ADS1256_SPS_10;
	input->bufferReadIdx = 0U;
	input->bufferWriteIdx = 0U;
	input->min = 0;
	input->max = 0;
	uint_fast8_t i = 0U;
	for (; i < ANALOG_INPUT_BUFFER_SIZE; ++i) {
		input->values[i] = 0;
		input->timestamps[i] = 0U;
	}
	input->added = CHANNEL_NOTADDED;
}

/**
 * Removes an analog input structure from the board's list by specifying its physical input channel.
 *
 * @param id uint8_t The physical channel to remove, if present.
 * @retval none
 */
static void RemoveAnalogInputByID(uint8_t id) {
	if (isExternalInput(id)) {
		InitializeInput(&Ext_AInputs[id]);
	} else if (isInternalInput(id)) {
		if (id == IN_COLD_JUNCTION) {
			/* We don't want to allow removal of this, so we return immediately. */
			return;
		}
		InitializeInput(&Int_AInputs[id - (NUM_EXT_ANALOG_INPUTS + NUM_CAL_ANALOG_INPUTS)]);
	} else if (id == EXTERNAL_OFFSET_CAL) {
		/* Do nothing, we don't want to remove this input. */
	} else {
		/* This is out of range */
#ifdef ANALOGINPUT_DEBUG
		printf("[Analog Input] Cannot find the requested analog input. Input does not exist on the board.\n\r");
#endif
	}
}

/*--------------------------------------------------------------------------------------------------------*/
/* INITIALIZATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Initializes all requisite sub-modules to properly operate analog inputs. This includes any multiplexing and
 * default inputs.
 *
 * @param none
 * @retval none
 */
void AnalogInputsInit(void) {
	/* Initialize the input multiplexer */
	InputMultiplexerInit();
	uint_fast8_t i = 0;
	for (; i < NUM_EXT_ANALOG_INPUTS; ++i) {
		InitializeInput(&Ext_AInputs[i]);
	}
	for (i = 0; i < NUM_INT_ANALOG_INPUTS; ++i) {
		InitializeInput(&Int_AInputs[i]);
	}
	/* Configure the offset cal input */
	InitializeInput(&Offset_Cal_AInput);

	/* Configure the cold junction input */
	Analog_Input_t* cold = GetAnalogInputByNumber(IN_COLD_JUNCTION);
	cold->physicalInput = IN_COLD_JUNCTION;
	cold->buffer = ADS1256_BUFFER_ENABLED;
	cold->rate = ADS1256_SPS_30000;
	cold->gain = ADS1256_PGAx8;
	strcpy(cold->name, "COLD JUNCTION");
	cold->bufferReadIdx = 0U;
	cold->bufferWriteIdx = 0U;
	cold->min = 0;
	cold->max = 0;
	AddAnalogInput(cold);
}

/**
 * Prints a human readable representation of all the added analog inputs via the current write function.
 *
 * @param none
 * @return Tekdaqc_Function_Error_t The error status of this function.
 */
Tekdaqc_Function_Error_t ListAnalogInputs(void) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	ClearToStringBuffer();
	uint8_t n = snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER,
			"\n\r--------------------\n\rAdded Analog Inputs\n\r\tExternal Inputs:\n\r");
	if (n <= 0) {
#ifdef ANALOGINPUT_DEBUG
		printf("Failed to write header of analog input list.\n\r");
#endif
		retval = ERR_AIN_FAILED_WRITE;
	} else {
		if (writer != 0) {
			writer(TOSTRING_BUFFER);
		}
		Analog_Input_t input;
		for (uint_fast8_t i = 0U; i < NUM_EXT_ANALOG_INPUTS; ++i) {
			input = Ext_AInputs[i];
			if (input.added == CHANNEL_ADDED) {
				/* This input has been added */
				n = snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "\t\tPhysical Input %" PRIi8
				":\n\r\t\t\tExternal Input: %s\n\r\t\t\tName: %s\n\r\t\t\tGain: %s\n\r\t\t\tRate: %s\n\r\t\t\tBuffer: %s\n\r",
						input.physicalInput, ExtAnalogInputToString(input.externalInput), input.name, ADS1256_StringFromPGA(input.gain),
						ADS1256_StringFromSPS(input.rate), ADS1256_StringFromBuffer(input.buffer));
				if (n <= 0) {
#ifdef ANALOGINPUT_DEBUG
					printf("Failed to write an analog input to the list.\n\r");
#endif
					retval = ERR_AIN_FAILED_WRITE;
					break;
				} else {
					if (writer != 0) {
						writer(TOSTRING_BUFFER);
					}
				}
			}
		}
		if (retval == ERR_FUNCTION_OK) {
			n = snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "\n\r\tInternal Inputs:\n\r");
			if (n <= 0) {
#ifdef ANALOGINPUT_DEBUG
				printf("Failed to write header of internal analog input list.\n\r");
#endif
				retval = ERR_AIN_FAILED_WRITE;
			} else {
				if (writer != 0) {
					writer(TOSTRING_BUFFER);
				}
				for (uint_fast8_t i = 0U; i < NUM_INT_ANALOG_INPUTS; ++i) {
					input = Int_AInputs[i];
					if (input.added == CHANNEL_ADDED) {
						/* This input has been added */
						n = snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "\t\tPhysical Input %" PRIi8
						":\n\r\t\t\tInternal Input: %s\n\r\t\t\tName: %s\n\r\t\t\tGain: %s\n\r\t\t\tRate: %s\n\r\t\t\tBuffer: %s\n\r",
								input.physicalInput, IntAnalogInputToString(input.internalInput), input.name,
								ADS1256_StringFromPGA(input.gain), ADS1256_StringFromSPS(input.rate),
								ADS1256_StringFromBuffer(input.buffer));
						if (n <= 0) {
#ifdef ANALOGINPUT_DEBUG
							printf("Failed to write an analog input to the list.\n\r");
#endif
							retval = ERR_AIN_FAILED_WRITE;
							break;
						} else {
							if (writer != 0) {
								writer(TOSTRING_BUFFER);
							}
						}
					}
				}
			}
		}
	}
	return retval;
}

/**
 * Creates a new analog input data structure from the supplied parameters and adds it to the board's relevant
 * input list.
 *
 * @param keys char** Array of strings containing the command line keys. Indexed with values.
 * @param values char** Array of strings containing the command line values. Indexed with keys.
 * @param count uint8_t The number of parameters passed on the command line.
 * @retval uint8_t The error status code.
 */
Tekdaqc_Function_Error_t CreateAnalogInput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	char* param;
	int8_t index = -1;
	/* Set the default parameters */
	uint8_t input = NULL_CHANNEL; /* The physical input */
	ADS1256_BUFFER_t buffer = ADS1256_BUFFER_ENABLED; /* The default buffer setting */
	ADS1256_SPS_t rate = ADS1256_SPS_10; /* The default sample rate setting */
	ADS1256_PGA_t gain = ADS1256_PGAx1; /* The default gain setting */
	char name[MAX_ANALOG_INPUT_NAME_LENGTH]; /* The name */
	strcpy(name, "NONE");
	uint_fast8_t i = 0U;
	for (; i < NUM_ADD_ANALOG_INPUT_PARAMS; ++i) {
		index = GetIndexOfArgument(keys, ADD_ANALOG_INPUT_PARAMS[i], count);
		if (index >= 0) { /* We found the key in the list */
			param = values[index]; /* We use the discovered index for this key */
			switch (i) { /* Switch on the key not position in arguments list */
			case 0U: { /* INPUT key */
				char* testPtr = NULL;
				uint8_t in = (uint8_t) strtol(param, &testPtr, 10);
				if (testPtr == param) {
					retval = ERR_AIN_PARSE_ERROR;
				} else {
					if (in >= 0U && in <= NUM_ANALOG_INPUTS) {
						/* A valid input number */
						input = in;
					} else {
						/* Input number out of range */
#ifdef ANALOGINPUT_DEBUG
						printf("[Analog Input] The requested input number is invalid.\n\r");
#endif
						retval = ERR_AIN_INPUT_OUTOFRANGE;
					}
				}
				break;
			}
			case 1U: /* BUFFER key */
				buffer = ADS1256_StringToBuffer(param);
				break;
			case 2U: /* RATE key */
				rate = ADS1256_StringToDataRate(param);
				break;
			case 3U: /* GAIN key */
				gain = ADS1256_StringToPGA(param);
				break;
			case 4U: /* NAME key */
				strcpy(name, param);
				break;
			default:
				retval = ERR_AIN_PARSE_ERROR;
			}
		} else if (i == 1U || i == 2U || i == 3U || i == 4U || i == 5U) {
			/* The BUFFER, RATE, GAIN and NAME keys are not strictly required, leave the defaults */
			continue;
		} else {
			/* Somehow an error happened */
#ifdef ANALOGINPUT_DEBUG
			printf("[Analog Input] Unable to locate required key: %s\n\r", ADD_ANALOG_INPUT_PARAMS[i]);
#endif
			retval = ERR_AIN_PARSE_MISSING_KEY; /* Failed to locate a key */
		}
		if (retval != ERR_FUNCTION_OK) {
			break;
		}
	}
	if (retval == ERR_FUNCTION_OK) {
		if (input != NULL_CHANNEL ) {
			Analog_Input_t* an_input = GetAnalogInputByNumber(input);
			if (an_input != NULL ) {
				if (an_input->added == CHANNEL_NOTADDED) {
					an_input->physicalInput = input;
					an_input->buffer = buffer;
					an_input->rate = rate;
					an_input->gain = gain;
					strcpy(an_input->name, name);
					an_input->bufferReadIdx = 0U;
					an_input->bufferWriteIdx = 0U;
					an_input->min = 0;
					an_input->max = 0;
					retval = AddAnalogInput(an_input);
				} else {
					retval = ERR_AIN_INPUT_EXISTS;
				}
			} else {
				/* The input could not be found */
				retval = ERR_AIN_INPUT_NOT_FOUND;
			}
		} else {
			/* A valid input was never specified */
			retval = ERR_AIN_INPUT_UNSPECIFIED;
		}
	}
	return retval; /* Return the status */
}

/**
 * Adds an analog input structure to the board's appropriate list of inputs.
 *
 * @param input Analog_Input_t* The analog input structure to add.
 * @retval none
 */
Tekdaqc_Function_Error_t AddAnalogInput(Analog_Input_t* input) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	PhysicalAnalogInput_t index = input->physicalInput;
	if (isExternalInput(index)) {
		/* This is an external analog input */
		input->externalInput = GetExternalMuxedInputByNumber(index);
		input->internalInput = NULL_CHANNEL;
		input->added = CHANNEL_ADDED;
#ifdef ANALOGINPUT_DEBUG
		printf("[Analog Input] Added input to external inputs list.\n\r");
#endif
	} else if (isInternalInput(index)) {
		/* This is an internal analog input */
		input->externalInput = NULL_CHANNEL;
		input->internalInput = index - (NUM_EXT_ANALOG_INPUTS + NUM_CAL_ANALOG_INPUTS);
		input->added = CHANNEL_ADDED;
#ifdef ANALOGINPUT_DEBUG
		printf("[Analog Input] Added input to internal inputs list.\n\r");
#endif
	} else if (index == EXTERNAL_OFFSET_CAL) {
		/* This is the offset calibration input */
		input->externalInput = NULL_CHANNEL;
		input->internalInput = NULL_CHANNEL;
		input->added = CHANNEL_ADDED;
#ifdef ANALOGINPUT_DEBUG
		printf("[Analog Input] Added external offset input.\n\r");
#endif
	} else {
		/* This is out of range */
#ifdef ANALOGINPUT_DEBUG
		printf("[Analog Input] Cannot add the provided analog input. Input does not exist on the board.\n\r");
#endif
		retval = ERR_AIN_INPUT_OUTOFRANGE;
	}
	return retval;
}

/**
 * Removes an analog input from the board's list based on the supplied parameters.
 *
 * @param keys char** Array of strings containing the command line keys. Indexed with values.
 * @param values char** Array of strings containing the command line values. Indexed with keys.
 * @param count uint8_t The number of parameters passed on the command line.
 * @retval int The error status code.
 */
Tekdaqc_Function_Error_t RemoveAnalogInput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	char* param;
	int8_t index = -1;
	uint_fast8_t i = 0U;
	for (; i < NUM_REMOVE_ANALOG_INPUT_PARAMS; ++i) {
		index = GetIndexOfArgument(keys, REMOVE_ANALOG_INPUT_PARAMS[i], count);
		if (index >= 0) { /* We found the key in the list */
			param = values[index]; /* We use the discovered index for this key */
			switch (i) { /* Switch on the key not position in arguments list */
			case 0U: { /* INPUT key */
				printf("Processing INPUT key\n\r");
				uint8_t in = (uint8_t) strtol(param, NULL, 10);
				if (in >= 0U && in <= NUM_ANALOG_INPUTS) {
					/* A valid input number */
					RemoveAnalogInputByID(in);
				} else {
					/* Input number out of range */
#ifdef ANALOGINPUT_DEBUG
					printf("[Analog Input] The requested input number is invalid.\n\r");
#endif
					retval = ERR_AIN_INPUT_OUTOFRANGE;
				}
				break;
			}
			default:
				retval = ERR_AIN_PARSE_ERROR;
			}
		} else {
			/* Somehow an error happened */
#ifdef ANALOGINPUT_DEBUG
			printf("[Analog Input] Unable to locate required key: %s\n\r", REMOVE_ANALOG_INPUT_PARAMS[i]);
#endif
			retval = ERR_AIN_PARSE_MISSING_KEY; /* Failed to locate a key */
		}
	}
	return retval;
}

/**
 * Retrieve an analog input structure by specifying the physical input channel.
 *
 * @param number uint8_t The physical input channel to retrieve.
 * @retval Analog_Input_t* Pointer to the analog input structure.
 */
Analog_Input_t* GetAnalogInputByNumber(uint8_t number) {
	if (isExternalInput(number)) {
		return &Ext_AInputs[number];
	} else if (isInternalInput(number)) {
		return &Int_AInputs[number - (NUM_EXT_ANALOG_INPUTS + NUM_CAL_ANALOG_INPUTS)];
	} else if (number == EXTERNAL_OFFSET_CAL) {
		return &Offset_Cal_AInput;
	} else {
		/* This is out of range */
#ifdef ANALOGINPUT_DEBUG
		printf("[Analog Input] Cannot find the requested analog input. Input does not exist on the board.\n\r");
#endif
		return NULL ;
	}
}

/**
 * Writes the data for the provided Analog_Input_t structure to the stream controlled by the WriteFunction, if set.
 *
 * @param input Analog_Input_t* Pointer to the data structure to write out.
 * @retval none
 */
void WriteAnalogInput(Analog_Input_t* input) {
	uint8_t count = 0;
	uint8_t retval;
	while (count < SINGLE_ANALOG_WRITE_COUNT && input->bufferReadIdx != input->bufferWriteIdx) {
		/* We have data to print */
		if (count == 0) {
			retval = sprintf(TOSTRING_BUFFER, ANALOG_INPUT_HEADER, input->name, input->physicalInput, ADS1256_StringFromPGA(input->gain),
					ADS1256_StringFromSPS(input->rate), ADS1256_StringFromBuffer(input->buffer));
			writer(TOSTRING_BUFFER);
		}
		retval = sprintf(TOSTRING_BUFFER, "%" PRIu64 ", %" PRIi32 "\x1F\n\r", input->timestamps[input->bufferReadIdx],
				input->values[input->bufferReadIdx]);
		input->bufferReadIdx = (input->bufferReadIdx + 1) % ANALOG_INPUT_BUFFER_SIZE;
		if (retval >= 0) {
			if (writer != 0) {
				writer(TOSTRING_BUFFER);
			} else {
#ifdef ANALOGINPUT_DEBUG
				printf("[Analog Input] Cannot write analog input to string due to NULL write function.\n\r");
#endif
			}
		} else {
#ifdef ANALOGINPUT_DEBUG
			printf("[Analog Input] Error occurred while writing analog input to string.\n\r");
#endif
		}
		++count;
	}
	if (writer != 0) {
		writer("\x1E");
	}
}

/**
 * Set the function pointer to use when writing data from an analog input to the data connection.
 *
 * @param writeFunction WriteFunction pointer to the desired string writing function.
 * @retval none
 */
void SetAnalogInputWriteFunction(WriteFunction writeFunction) {
	writer = writeFunction;
}

/**
 * Returns the string representation of an externally muxed analog input.
 *
 * @param input ExternalMuxedInput_t The external input.
 * @return const char* The string representation.
 */
const char* ExtAnalogInputToString(ExternalMuxedInput_t input) {
	static const char* strings[] = { "External 0", "External 1", "External 2", "External 3", "External 4", "External 5", "External 6",
			"External 7", "External 8", "External 9", "External 10", "External 11", "External 12", "External 13", "External 14",
			"External 15", "External 16", "External 17", "External 18", "External 19", "External 20", "External 21", "External 22",
			"External 23", "External 24", "External 25", "External 26", "External 27", "External 28", "External 29", "External 30",
			"External 31" };
	const char* retval = NULL;
	switch (input) {
	case EXTERN_0:
		retval = strings[0];
		break;
	case EXTERN_1:
		retval = strings[1];
		break;
	case EXTERN_2:
		retval = strings[2];
		break;
	case EXTERN_3:
		retval = strings[3];
		break;
	case EXTERN_4:
		retval = strings[4];
		break;
	case EXTERN_5:
		retval = strings[5];
		break;
	case EXTERN_6:
		retval = strings[6];
		break;
	case EXTERN_7:
		retval = strings[7];
		break;
	case EXTERN_8:
		retval = strings[8];
		break;
	case EXTERN_9:
		retval = strings[9];
		break;
	case EXTERN_10:
		retval = strings[10];
		break;
	case EXTERN_11:
		retval = strings[11];
		break;
	case EXTERN_12:
		retval = strings[12];
		break;
	case EXTERN_13:
		retval = strings[13];
		break;
	case EXTERN_14:
		retval = strings[14];
		break;
	case EXTERN_15:
		retval = strings[15];
		break;
	case EXTERN_16:
		retval = strings[16];
		break;
	case EXTERN_17:
		retval = strings[17];
		break;
	case EXTERN_18:
		retval = strings[18];
		break;
	case EXTERN_19:
		retval = strings[19];
		break;
	case EXTERN_20:
		retval = strings[20];
		break;
	case EXTERN_21:
		retval = strings[21];
		break;
	case EXTERN_22:
		retval = strings[22];
		break;
	case EXTERN_23:
		retval = strings[23];
		break;
	case EXTERN_24:
		retval = strings[24];
		break;
	case EXTERN_25:
		retval = strings[25];
		break;
	case EXTERN_26:
		retval = strings[26];
		break;
	case EXTERN_27:
		retval = strings[27];
		break;
	case EXTERN_28:
		retval = strings[28];
		break;
	case EXTERN_29:
		retval = strings[29];
		break;
	case EXTERN_30:
		retval = strings[30];
		break;
	case EXTERN_31:
		retval = strings[31];
		break;
	default:
		retval = NULL;
	}
	return retval;
}

/**
 * Returns the string representation of an internally muxed analog input.
 *
 * @param input InternalAnalogInput_t The internal input
 * @retval const char* The string representation.
 */
const char* IntAnalogInputToString(InternalAnalogInput_t input) {
	char* str;
	switch (input) {
	case SUPPLY_9V:
		str = "9V SUPPLY";
		break;
	case SUPPLY_5V:
		str = "5V SUPPLY";
		break;
	case SUPPLY_3_3V:
		str = "3.3V SUPPLY";
		break;
	case COLD_JUNCTION:
		str = "COLD JUNCTION";
		break;
	case EXTERNAL_ANALOG_IN:
		str = "EXTERNAL ANALOG INPUT";
	default:
		str = "UNKNOWN";
	}
	return str;
}
