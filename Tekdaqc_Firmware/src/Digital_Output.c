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
 * @file Digital_Output.c
 * @brief Source file for the Digital_Output_t data structure.
 *
 * Contains methods for manipulating and interfacing to the Digital_Output_t data structure.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "Digital_Output.h"
#include "Tekdaqc_Timers.h"
#include "Tekdaqc_CommandInterpreter.h"
#include "TLE7232_RelayDriver.h"
#include "boolean.h"
#include <stdlib.h>
#include <inttypes.h>

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @def DIGITAL_OUTPUT_FORMATTER
 * @brief The header format string for printing an analog input to a human readable string.
 */
#define DIGITAL_OUTPUT_FORMATTER "\n\r--------------------\n\rDigital Output\n\r\tName: %s\n\r\tPhysical Channel: %i\n\r\tTimestamp: %" PRIu64 "\n\r\tLevel: %s\n\r--------------------\n\r\x1E"

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* Pointer to the function to use when writing data to the output stream. */
static WriteFunction writer = NULL;

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE EXTERNAL VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* List of external digital outputs */
static Digital_Output_t Ext_DOutputs[NUM_DIGITAL_OUTPUTS];

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @brief Removes a digital output its absolute index.
 */
static void RemoveDigitalOutputByID(uint8_t id);

/**
 * @internal
 * @brief Determines if the absolute inded represents an external output or not.
 */
static bool isExternalOutput(uint8_t id);

/**
 * @internal
 * @brief Initializes a digital output structure to its default values.
 */
static void InitializeOutput(Digital_Output_t* output);

/**
 * @internal
 * @brief Sets the state of the specified Digital_Output_t.
 */
static void SetDigitalOutputState(Digital_Output_t* output, DigitalLevel_t level);

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Removes a digital output by specifying its absolute index. If the index is out of range,
 * and error message will be logged depending on the debug level compiled with.
 *
 * @param id uint8_t The absolute index of the output.
 * @retval none
 */
static void RemoveDigitalOutputByID(uint8_t id) {
	if (isExternalOutput(id)) {
		InitializeOutput(&Ext_DOutputs[id]);
	} else {
		/* This is out of range */
#ifdef DIGITALOUTPUT_DEBUG
		printf("[Digital Output] Cannot find the requested digital output. Input does not exist on the board.\n\r");
#endif
	}
}

/**
 * Determines if the output represented by the specified index is an external output or an internal one.
 *
 * @param id uint8_t The absolute index of the output.
 * @retval none
 */
static bool isExternalOutput(uint8_t id) {
	if (id >= 0 && id < NUM_DIGITAL_OUTPUTS) {
		return true;
	} else {
		return false;
	}
}

/**
 * Initializes the members of the specified output structure.
 *
 * @param output Digital_Output_t* Pointer/Reference to an digital output structure.
 * @retval none
 */
static void InitializeOutput(Digital_Output_t* output) {
	output->added = CHANNEL_NOTADDED;
	output->level = LOGIC_LOW;
	output->timestamp = 0U;
	strcpy(output->name, "NONE");
	output->output = NULL_CHANNEL;
	output->fault_status = TLE7232_Normal_Operation;
	output->fault_timestamp = 0U;
}

/**
 * Sets the state of the specified Digital_Output_t. This will first read the existing
 * states and construct its set request such that all other output states are unchanged.
 *
 * @param output Digital_Output_t* The output to set the state of.
 * @param level DigitalLevel_t The state to set the output to.
 * @retval none
 */
static void SetDigitalOutputState(Digital_Output_t* output, DigitalLevel_t level) {
	/* Set the state */
	uint8_t control = TLE7232_ReadRegister(TLE7232_REG_CTL, output->output % 8U);
	uint8_t channel = output->output;
	if (channel >= 8) { /* Convert the channel to a single chip index */
		channel -= 8;
	}
	uint8_t mask = 0x01 << channel; /* Move the bit flag to the correct position */
	if (level == OUTPUT_ON) {
		control |= mask; /* Set the appropriate bit */
	} else {
		control ^= mask;	/* Clear the appropriate bit */
	}
	TLE7232_WriteRegister(TLE7232_REG_CTL, control, output->output % 8U); /* Update the register */
}

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Initializes all requisite sub-modules to properly operate digital outputs. This includes any multiplexing and
 * default inputs.
 *
 * @param none
 * @retval none
 */
void DigitalOutputsInit(void) {
	for (uint_fast8_t i = 0; i < NUM_DIGITAL_OUTPUTS; ++i) {
		InitializeOutput(&Ext_DOutputs[i]);
	}

	/*uint8_t data[NUMBER_TLE7232_CHIPS] = {0xFF, 0xFF};
	TLE7232_WriteRegisterAll(TLE7232_REG_CTL, data);*/
}

/**
 * Prints a human readable representation of all the added digital outputs via the current write function.
 *
 * @param none
 * @return Tekdaqc_Function_Error_t The error status of this function.
 */
Tekdaqc_Function_Error_t ListDigitalOutputs(void) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	ClearToStringBuffer();
	uint8_t n = snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "\n\r----------\n\rAdded Digital Outputs\n\r----------\n\r");
	if (n <= 0) {
#ifdef DIGITALOUTPUT_DEBUG
		printf("Failed to write header of digital output list.\n\r");
#endif
		retval = ERR_DOUT_FAILED_WRITE;
	} else {
		if (writer != 0) {
			writer(TOSTRING_BUFFER);
		}
		Digital_Output_t output;
		for(uint_fast8_t i = 0U; i < NUM_DIGITAL_OUTPUTS; ++i) {
			output = Ext_DOutputs[i];
			if (output.added == CHANNEL_ADDED) {
				/* This input has been added */
				n = snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "\tPhysical Output %" PRIi8 ":\n\r\t\tName: %s\n\r", output.output, output.name);
				if (n <= 0) {
#ifdef DIGITALOUTPUT_DEBUG
					printf("Failed to write an digital output to the list.\n\r");
#endif
					retval = ERR_DOUT_FAILED_WRITE;
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
 * Creates a new digital output data structure from the supplied parameters and adds it to the board's relevant
 * output list.
 *
 * @param keys char** Array of strings containing the command line keys. Indexed with values.
 * @param values char** Array of strings containing the command line values. Indexed with keys.
 * @param count uint8_t The number of parameters passed on the command line.
 * @retval uint8_t The error status code.
 */
Tekdaqc_Function_Error_t CreateDigitalOutput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], int count) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	char* param;
	char* testPtr = NULL;
	int8_t index = -1;
	uint8_t output = NULL_CHANNEL; /* The physical input */
	uint8_t out = NULL_CHANNEL;
	char name[MAX_DIGITAL_OUTPUT_NAME_LENGTH]; /* The name */
	strcpy(name, "NONE");
	for (int i = 0; i < NUM_ADD_DIGITAL_OUTPUT_PARAMS; ++i) {
		index = GetIndexOfArgument(keys, ADD_DIGITAL_OUTPUT_PARAMS[i], count);
		if (index >= 0) { /* We found the key in the list */
			param = values[index]; /* We use the discovered index for this key */
			switch (i) { /* Switch on the key not position in arguments list */
			case 0U: /* OUTPUT key */
				out = (uint8_t) strtol(param, &testPtr, 10);
				if (testPtr == param) {
					retval = ERR_DOUT_PARSE_ERROR;
				} else {
					if (out >= 0U && out <= NUM_DIGITAL_OUTPUTS) {
						/* A valid output number */
						output = out;
					} else {
						/* Input number out of range */
#ifdef DIGITALOUTPUT_DEBUG
						printf("[Digital Output] The requested output number is invalid.\n\r");
#endif
						retval = ERR_DOUT_OUTPUT_OUTOFRANGE;
					}
				}
				break;
			case 1U: /* NAME key */
				strcpy(name, param);
				break;
			default:
				/* Return an error */
				retval = ERR_DOUT_PARSE_ERROR;
			}
		} else if (i == 1U) {
			/* The NAME key is not strictly required, apply the defaults */
			continue;
		} else {
			/* Somehow an error happened */
#ifdef DIGITALOUTPUT_DEBUG
			printf("[Digital Output] Unable to locate required key: %s\n\r", ADD_DIGITAL_OUTPUT_PARAMS[i]);
#endif
			retval = ERR_DOUT_PARSE_MISSING_KEY; /* Failed to locate a key */
		}
		if (retval != ERR_FUNCTION_OK) {
			break;
		}
	}
	if (retval == ERR_FUNCTION_OK) {
		if (output != NULL_CHANNEL) {
			Digital_Output_t* dig_output = GetDigitalOutputByNumber(output);
			if (dig_output != NULL) {
				if (dig_output->added != CHANNEL_ADDED) {
					dig_output->output = output;
					strcpy(dig_output->name, name);
					dig_output->level = LOGIC_LOW;
					dig_output->timestamp = 0U;
					retval = AddDigitalOutput(dig_output);
				} else {
					retval = ERR_DOUT_OUTPUT_EXISTS;
				}
			} else {
				/* The output could not be found */
				retval = ERR_DOUT_OUTPUT_NOT_FOUND;
			}
		} else {
			/* A valid input was never specified */
			retval = ERR_DOUT_OUTPUT_UNSPECIFIED;
		}
	}
	return retval;
}

/**
 * Adds an digital output structure to the board's appropriate list of outputs.
 *
 * @param input Digital_Output_t* The digital output structure to add.
 * @retval none
 */
Tekdaqc_Function_Error_t AddDigitalOutput(Digital_Output_t* output) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	uint8_t index = output->output;
	if (index < NUM_DIGITAL_OUTPUTS) {
		/* This is a valid digital input */
		output->added = CHANNEL_ADDED;
		SetDigitalOutputState(output, LOGIC_LOW);
	} else {
		/* This is out of range */
#ifdef DIGITALOUTPUT_DEBUG
		printf("[Digital Output] Cannot add the provided digital output structure. Output field is out of range.\n\r");
#endif
		retval = ERR_DOUT_OUTPUT_OUTOFRANGE;
	}
	return retval;
}

/**
 * Removes an digital output from the board's list based on the supplied parameters.
 *
 * @param keys char** Array of strings containing the command line keys. Indexed with values.
 * @param values char** Array of strings containing the command line values. Indexed with keys.
 * @param count uint8_t The number of parameters passed on the command line.
 * @retval int The error status code.
 */
Tekdaqc_Function_Error_t RemoveDigitalOutput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], int count) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	char* param;
	int8_t index = -1;
	for (uint_fast8_t i = 0U; i < NUM_REMOVE_DIGITAL_OUTPUT_PARAMS; ++i) {
		index = GetIndexOfArgument(keys, REMOVE_DIGITAL_OUTPUT_PARAMS[i], count);
		if (index >= 0) { //We found the key in the list
			param = values[index]; //We use the discovered index for this key
			switch (i) { //Switch on the key not position in arguments list
			case 0U: { //OUTPUT key
				char* testPtr = NULL;
				uint8_t out = (uint8_t) strtol(param, &testPtr, 10);
				if (testPtr == param) {
					retval = ERR_DOUT_PARSE_ERROR;
				} else {
					if (isExternalOutput(out)) {
						/* A valid input number */
						RemoveDigitalOutputByID(out);
					} else {
						/* Input number out of range */
#ifdef DIGITALOUTPUT_DEBUG
						printf("[Digital Output] The requested input number is invalid.\n\r");
#endif
						retval = ERR_DOUT_OUTPUT_OUTOFRANGE;
					}
				}
				break;
			}
			default:
				/* Return an error */
				retval = ERR_DOUT_PARSE_ERROR;
			}
		} else {
			/* Somehow an error happened */
#ifdef DIGITALINPUT_DEBUG
			printf("[Digital Output] Unable to locate required key: %s\n\r", REMOVE_DIGITAL_OUTPUT_PARAMS[i]);
#endif
			retval = ERR_DOUT_PARSE_MISSING_KEY; /* Failed to locate a key */
		}
	}
	return retval;
}

/**
 * Retrieve a digital output structure by specifying the physical output channel.
 *
 * @param number uint8_t The physical output channel to retrieve.
 * @retval Digital_Output_t* Pointer to the analog input structure.
 */
Digital_Output_t* GetDigitalOutputByNumber(uint8_t number) {
	if (isExternalOutput(number)) {
		return &Ext_DOutputs[number];
	} else {
		/* This is out of range */
#ifdef DIGITALOUTPUT_DEBUG
		printf("[Digital Output] Cannot find the requested digital output. Output does not exist on the board.\n\r");
#endif
		return NULL;
	}
}

Tekdaqc_Function_Error_t SetDigitalOutput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	char* param;
	int8_t index = -1;
	uint8_t output = NULL_CHANNEL; /* The physical input */
	DigitalLevel_t level = OUTPUT_OFF;
	for (int i = 0; i < NUM_SET_DIGITAL_OUTPUT_PARAMS; ++i) {
		index = GetIndexOfArgument(keys, SET_DIGITAL_OUTPUT_PARAMS[i], count);
		if (index >= 0) { /* We found the key in the list */
			param = values[index]; /* We use the discovered index for this key */
			switch (i) { /* Switch on the key not position in arguments list */
			case 0: { /* OUTPUT key */
				char* testPtr = NULL;
				uint8_t out = (uint8_t) strtol(param, &testPtr, 10);
				if (testPtr == param) {
					retval = ERR_DOUT_PARSE_ERROR;
				} else {
					if (out >= 0U && out <= NUM_DIGITAL_OUTPUTS) {
						/* A valid input number */
						output = out;
					} else {
						/* Input number out of range */
#ifdef DIGITALOUTPUT_DEBUG
						printf("[Digital Output] The requested output number is invalid.\n\r");
#endif
						retval = ERR_DOUT_OUTPUT_OUTOFRANGE;
					}
				}
				break;
			}
			case 1: /* STATE key */
				if (strcmp(param, "ON") == 0) {
					level = OUTPUT_ON;
				} else {
					level = OUTPUT_OFF;
				}
				break;
			default:
				/* Return an error */
				retval = ERR_DOUT_PARSE_ERROR;
			}
		} else {
#ifdef DIGITALOUTPUT_DEBUG
			printf("[Digital Output] Unable to locate required key: %s\n\r", SET_DIGITAL_OUTPUT_PARAMS[i]);
#endif
			retval = ERR_DOUT_PARSE_MISSING_KEY; /* Failed to locate a key */
		}
		if (retval != ERR_FUNCTION_OK) {
			break;
		}
	}
	if (retval == ERR_FUNCTION_OK) {
		if (output != NULL_CHANNEL) {
			Digital_Output_t* dig_output = GetDigitalOutputByNumber(output);
			if (dig_output != NULL) {
				if (dig_output->added == CHANNEL_ADDED) {
					SetDigitalOutputState(dig_output, level);
				} else {
#ifdef DIGITALOUTPUT_DEBUG
					printf("[Digital Output] Tried to change the state of an output which has not been added.\n\r");
#endif
					retval = ERR_DOUT_DOES_NOT_EXIST;
				}
			} else {
				/* The output could not be found */
				retval = ERR_DOUT_OUTPUT_NOT_FOUND;
			}
		} else {
			/* A valid input was never specified */
			retval = ERR_DOUT_OUTPUT_UNSPECIFIED;
		}
	}
	return retval;
}

/**
 * Set the function pointer to use when writing data from a digital output to the data connection.
 *
 * @param writeFunction WriteFunction pointer to the desired string writing function.
 * @retval none
 */
void SetDigitalOutputWriteFunction(WriteFunction writeFunction) {
	writer = writeFunction;
}

/**
 * Samples the specified digital output's level and writes out the result.
 *
 * @param output Digital_Output_t* The digital output to be sampled.
 * @retval none
 */
void SampleDigitalOutput(Digital_Output_t* output) {
	//TODO: Sample digital output
}

/**
 * Samples the digital output level of all added digital inputs, writing out the results.
 *
 * @param none
 * @retval none
 */
void SampleAllDigitalOutputs(void) {
	Digital_Output_t output;
	for (int i = 0; i < NUM_DIGITAL_OUTPUTS; ++i) {
		output = Ext_DOutputs[i];
		if (output.added == CHANNEL_ADDED) {
			SampleDigitalOutput(&output);
		}
	}
}

/**
 * Writes the data for the provided Digital_Output_t structure to the stream controlled by the WriteFunction, if set.
 *
 * @param output Digital_Output_t* Pointer to the data structure to write out.
 * @retval none
 */
void WriteDigitalOutput(Digital_Output_t* output) {
	uint8_t retval = sprintf(TOSTRING_BUFFER, DIGITAL_OUTPUT_FORMATTER, output->name, output->output, output->timestamp,
			DigitalLevelToString(output->level));
	if (retval >= 0) {
		if (writer != NULL ) {
			writer(TOSTRING_BUFFER);
		} else {
#ifdef DIGITALOUTPUT_DEBUG
			printf("[Digital Output] Cannot write digital output to string due to NULL write function.\n\r");
#endif
		}
	} else {
#ifdef DIGITALOUTPUT_DEBUG
		printf("[Digital Output] Error occurred while writing digital output to string.\n\r");
#endif
	}
}

/**
 * Writes out the data for all added digital outputs to the stream controlled by the WriteFunction, if set.
 *
 * @param none
 * @retval none
 */
void WriteAllDigitalOutputs(void) {
	if (writer != NULL ) {
		Digital_Output_t output;
		for (int i = 0; i < NUM_DIGITAL_OUTPUTS; ++i) {
			output = Ext_DOutputs[i];
			if (output.added == CHANNEL_ADDED) {
				WriteDigitalOutput(&output);
			}
		}
	}
}

/**
 * Checks the current status of all digital outputs on the board, returning if a fault has occurred or not.
 *
 * @param none
 * @retval bool TRUE if a fault was detected.
 */
bool CheckDigitalOutputStatus(void) {
	bool retval = false;
	TLE7232_ReadAllDiagnosis();
	Digital_Output_t output;
	for (uint_fast8_t i = 0U; i < NUM_DIGITAL_OUTPUTS; ++i) {
		output = Ext_DOutputs[i];
		if ((output.added == CHANNEL_ADDED) && (output.fault_status != TLE7232_Normal_Operation)) {
			/* A fault has occurred on this channel */
			retval = true;
			break;
		}
	}
	return retval;
}

/**
 * Sets the fault status for the specified chip and channel.
 *
 * @param status TLE7232_Status_t The fault status to set.
 * @param chip_id uint8_t The chip which the channel is on.
 * @param channel uint8_t The channel to set the fault status of.
 */
bool SetDigitalOutputFaultStatus(TLE7232_Status_t status, uint8_t chip_id, uint8_t channel) {
	bool retval = false;
	uint8_t out = channel * (chip_id + 1);
	if (out < NUM_DIGITAL_OUTPUTS) {
		Ext_DOutputs[out].fault_status = status;
		Ext_DOutputs[out].fault_timestamp = GetLocalTime();
		retval = true;
	} else {
#ifdef DIGITALINPUT_DEBUG
		printf("Tried to set the fault status of a digital output which is out of range.\n\r");
#endif
		retval = false;
	}
	return retval;
}
