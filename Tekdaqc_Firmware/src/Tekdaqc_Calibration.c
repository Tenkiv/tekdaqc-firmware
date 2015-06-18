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
 * @file Tekdaqc_Calibration.c
 * @brief Implements the run time calibration processes.
 *
 * Implements the runtime calibration processes of the Tekdaqc board. This includes things like system
 * offset calibration and self gain calibration.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "Tekdaqc_Calibration.h"
#include "Tekdaqc_CalibrationTable.h"
#include "Tekdaqc_CommandInterpreter.h"
#include "AnalogInput_Multiplexer.h"
#include "ADC_StateMachine.h"
#include "ADS1256_Driver.h"
#include "BoardTemperature.h"
#include "Tekdaqc_BSP.h"
#include "Tekdaqc_CommandInterpreter.h"
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Sets the ADC parameters for the specified calibration.
 */
static Tekdaqc_Function_Error_t SetADCParameters(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Sets the ADC parameters for the upcoming calibration to the specified values.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Function_Error_t The function error status.
 */
static Tekdaqc_Function_Error_t SetADCParameters(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	int8_t index = -1;
	ADS1256_PGA_t pga = ADS1256_PGAx1;
	ADS1256_SPS_t rate = ADS1256_SPS_60;
	ADS1256_BUFFER_t buffer = ADS1256_BUFFER_DISABLED;
	for (uint_fast8_t i = 0; i < NUM_SYSTEM_CAL_PARAMS; ++i) {
		index = GetIndexOfArgument(keys, SYSTEM_CAL_PARAMS[i], count);
		if (index >= 0) { /* We found the key in the list */
			switch (i) { /* Switch on the key not position in arguments list */
				case 0: /* BUFFER key */
					buffer = ADS1256_StringToBuffer(values[index]);
					break;
				case 1: /* RATE key */
					rate = ADS1256_StringToDataRate(values[index]); /* We use the discovered index for this key */
					break;
				case 2: /* GAIN key */
					pga = ADS1256_StringToPGA(values[index]); /* We use the discovered index for this key */
					break;
				default:
					retval = ERR_AIN_PARSE_ERROR;
			}
		} else if (i == 0 || i == 1 || i == 2 || i == 3) {
			/* The GAIN, RATE and BUFFER keys are not strictly required */
			continue;
		} else {
			/* Somehow an error happened */
#ifdef CALIBRATION_DEBUG
			printf("[Calibration Process] Unable to locate key: %s\n\r", READ_ANALOG_KEYS[i]);
#endif
			retval = ERR_AIN_PARSE_MISSING_KEY; /* Failed to locate a key */
		}
	}
	ADS1256_SetInputBufferSetting(buffer);
	ADS1256_SetDataRate(rate);
	ADS1256_SetPGASetting(pga);
	return retval;
}

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Performs a system auto calibration. This will consist of a full ADC self calibration followed by a system
 * offset calibration for each combination of sampling parameters (gain, rate, buffer state).
 *
 * @param none
 * @retval Tekdaqc_Function_Error_t The function error status.
 */
Tekdaqc_Function_Error_t PerformSystemCalibration(void) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	ADC_Calibrate();
	return retval;
}

/**
 * Performs a gain calibration with specified parameters. It is important that this is not executed while
 * the ADC is performing anything other than it's idle task.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Function_Error_t The function error status.
 */
Tekdaqc_Function_Error_t PerformSystemGainCalibration(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	retval = SetADCParameters(keys, values, count);
	PhysicalAnalogInput_t input = EXTERNAL_0;
	int8_t index = -1;
	for (uint_fast8_t i = 0; i < NUM_SYSTEM_GCAL_PARAMS; ++i) {
		index = GetIndexOfArgument(keys, SYSTEM_GCAL_PARAMS[i], count);
		if (index >= 0) { /* We found the key in the list */
			switch (i) { /* Switch on the key not position in arguments list */
				case 0: /* INPUT key */
					input = ADS1256_StringToBuffer(values[index]);
					break;
				default:
					retval = ERR_CALIBRATION_PARSE_ERROR;
			}
		} else {
			/* Somehow an error happened */
#ifdef CALIBRATION_DEBUG
			printf("[Command Interpreter] Unable to locate key: %s\n\r", READ_ANALOG_KEYS[i]);
#endif
			retval = ERR_CALIBRATION_MISSING_KEY; /* Failed to locate a key */
		}
	}
	ADC_GainCalibrate(input);
	return retval;
}

/**
 * Checks the recorded temperature history to determine if the board has ever fallen out of its
 * specified temperature range.
 *
 * @param none
 * @retval bool True if the board has never exceeded the specified calibration limit temperatures.
 */
bool isTekdaqc_CalibrationValid(void) {
	bool valid = true;
	float max = getMaximumBoardTemperature();
	float min = getMinimumBoardTemperature();
	if ((max > CALIBRATION_VALID_MAX_TEMP) || (min < CALIBRATION_VALID_MIN_TEMP)) {
		valid = false;
	}
	return valid;
}

/**
 * Retrieves the self gain calibration value stored in the base gain calibration table for the specified parameters.
 *
 * @param cal uint32_t* Pointer to the variable in which to store the calibration value.
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Function_Error_t The function error status.
 */
Tekdaqc_Function_Error_t GetSelfGainCalibration(uint32_t* cal, char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	char* param;
	int8_t index = -1;
	/* Set the default parameters */
	ADS1256_BUFFER_t buffer;
	ADS1256_SPS_t rate;
	ADS1256_PGA_t gain;
	uint_fast8_t i = 0U;
	for (; i < NUM_READ_SELF_GCAL_PARAMS; ++i) {
		index = GetIndexOfArgument(keys, READ_SELF_GCAL_PARAMS[i], count);
		if (index >= 0) { /* We found the key in the list */
			param = values[index]; /* We use the discovered index for this key */
			switch (i) { /* Switch on the key not position in arguments list */
				case 0U: /* BUFFER key */
					buffer = ADS1256_StringToBuffer(param);
					break;
				case 1U: /* RATE key */
					rate = ADS1256_StringToDataRate(param);
					break;
				case 2U: /* GAIN key */
					gain = ADS1256_StringToPGA(param);
					break;
				default:
					retval = ERR_CALIBRATION_PARSE_ERROR;
			}
		} else {
			/* Somehow an error happened */
#ifdef CALIBRATION_DEBUG
			printf("[Calibration Process] Unable to locate required key: %s\n\r", GET_SELF_GCAL_PARAMS[i]);
#endif
			retval = ERR_CALIBRATION_MISSING_KEY; /* Failed to locate a key */
		}
		if (retval != ERR_FUNCTION_OK) {
			break;
		}
	}
	if (retval == ERR_FUNCTION_OK) {
		*cal = Tekdaqc_GetBaseGainCalibration(rate, gain, buffer);
	}
	return retval; /* Return the status */
}

/**
 * Writes a gain calibration value into the FLASH calibration table.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Function_Error_t The function error status.
 */
Tekdaqc_Function_Error_t Tekdaqc_WriteGainCalibrationValue(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	char* param;
	int8_t index = -1;
	/* Set the default parameters */
	float value;
	ADS1256_BUFFER_t buffer;
	ADS1256_SPS_t rate;
	ADS1256_PGA_t gain;
	ANALOG_INPUT_SCALE_t scale;
	uint8_t temperature;
	uint_fast8_t i = 0U;
	for (; i < NUM_WRITE_GAIN_CALIBRATION_VALUE_PARAMS; ++i) {
		index = GetIndexOfArgument(keys, WRITE_GAIN_CALIBRATION_VALUE_PARAMS[i], count);
		if (index >= 0) { /* We found the key in the list */
			param = values[index]; /* We use the discovered index for this key */
			switch (i) { /* Switch on the key not position in arguments list */
				case 0U: { /* VALUE key */
					char* testPtr = NULL;
					value = strtof(param, &testPtr);
					if (testPtr == param) {
						retval = ERR_CALIBRATION_PARSE_ERROR;
					}
					break;
				}
				case 1U: /* GAIN key */
					gain = ADS1256_StringToPGA(param);
					break;
				case 2U: /* RATE key */
					rate = ADS1256_StringToDataRate(param);
					break;
				case 3U: /* BUFFER key */
					buffer = ADS1256_StringToBuffer(param);
					break;
				case 4U: /* SCALE key */
					scale = Tekdaqc_StringToAnalogInputScale(param);
					break;
				case 5U: /* INDEX key */
					errno = 0; /* Set the global error number to 0 so we get a valid check */
					temperature = (uint8_t) strtol(values[index], NULL, 0);
					if (errno != 0) {
						retval = ERR_CALIBRATION_PARSE_ERROR;
						break;
					}
					break;
				default:
					retval = ERR_CALIBRATION_PARSE_ERROR;
			}
		} else {
			/* Somehow an error happened */
#ifdef CALIBRATION_DEBUG
			printf("[Calibration Process] Unable to locate required key: %s\n\r", WRITE_GAIN_CALIBRATION_VALUE_PARAMS_PARAMS[i]);
#endif
			retval = ERR_CALIBRATION_MISSING_KEY; /* Failed to locate a key */
		}
		if (retval != ERR_FUNCTION_OK) {
			break;
		}
	}
	if (retval == ERR_FUNCTION_OK) {
		printf("Writing gain calibration value 0x%0.4f for Rate: %s, Gain %s, Buffer: %s\n\r", value, ADS1256_StringFromSPS(rate),
				ADS1256_StringFromPGA(gain), ADS1256_StringFromBuffer(buffer));
		if (Tekdaqc_SetGainCalibration(value, rate, gain, buffer, scale, temperature) != FLASH_COMPLETE) {
			retval = ERR_CALIBRATION_WRITE_FAILED;
		}
	}
	return retval; /* Return the status */
}
