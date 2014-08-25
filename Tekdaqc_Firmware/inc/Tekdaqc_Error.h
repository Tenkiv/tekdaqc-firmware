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
 * @file Tekdaqc_Error.h
 * @brief Public definitions for the Tekdaqc error type.
 *
 * Contains the public type definition and constants for Tekdaqc error types.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TEKDAQC_ERROR_H_
#define TEKDAQC_ERROR_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

/** @addtogroup tekdaqc_firmware Tekdaqc Firmware
 * @{
 */

/** @addtogroup tekdaqc_error Tekdaqc Error
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED TYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Tekdaqc Command error enumeration.
 * Defines the possible error codes returned from a Tekdaqc command handler.
 */
typedef enum {
	ERR_COMMAND_OK						=	0U, /**< Command processing completed normally. No error present. */
	ERR_COMMAND_BAD_PARAM				= 	1U, /**< Command processing failed due to a bad parameter. */
	ERR_COMMAND_BAD_COMMAND				=	2U, /**< Command processing failed due to a bad command. */
	ERR_COMMAND_PARSE_ERROR				=	3U, /**< Command processing failed due to a parsing error. */
	ERR_COMMAND_FUNCTION_ERROR			=	4U, /**< Command processing failed due to a sub function error. */
	ERR_COMMAND_ADC_INVALID_OPERATION 	= 	5U, /**< Command processing failed due to an invalid ADC operation. */
	ERR_COMMAND_DI_INVALID_OPERATION 	= 	6U, /**< Command processing failed due to an invalid digital input operation. */
	ERR_COMMAND_DO_INVALID_OPERATION 	= 	7U, /**< Command processing failed due to an invalid digital output operation. */
	ERR_COMMAND_UNKNOWN_ERROR			=	8U /**< Command processing failed due to an unknown error. */
} Tekdaqc_Command_Error_t;

/**
 * @brief Tekdaqc Command sub-function error enumeration.
 * Defines the possible error codes returned by sub-functions of a Tekdaqc command handler.
 */
typedef enum {
	ERR_FUNCTION_OK					=	0U, /**< The function completed normally. No error present. */
	ERR_AIN_INPUT_OUTOFRANGE		=	1U, /**< The function failed due to an analog input specified out of range. */
	ERR_AIN_PARSE_MISSING_KEY		=	2U, /**< The function failed due to a missing required key in the analog input command. */
	ERR_AIN_INPUT_NOT_FOUND			=	3U, /**< The function failed due to the specified analog input not being found. */
	ERR_AIN_PARSE_ERROR				=	4U, /**< The function failed due to an analog input parsing error. */
	ERR_AIN_INPUT_UNSPECIFIED		=	5U, /**< The function failed due to an unspecified analog input. */
	ERR_AIN_INPUT_EXISTS			=	6U, /**< The function failed because the specified analog input already exists. */
	ERR_AIN_FAILED_WRITE			=	7U, /**< The function failed due to a failure to write analog input data to a string buffer. */
	ERR_DIN_INPUT_OUTOFRANGE		=	8U, /**< The function failed due to a digital input specified out of range. */
	ERR_DIN_PARSE_MISSING_KEY		=	9U, /**< The function failed due to a missing required key in the digital input command. */
	ERR_DIN_INPUT_NOT_FOUND			=	10U, /**< The function failed due to the specified digital input not being found. */
	ERR_DIN_PARSE_ERROR				=	11U, /**< The function failed due to a digital input parsing error. */
	ERR_DIN_INPUT_UNSPECIFIED		=	12U, /**< The function failed due to an unspecified digital input. */
	ERR_DIN_INPUT_EXISTS			=	13U, /**< The function failed because the specified analog input already exists. */
	ERR_DIN_FAILED_WRITE			=	14U, /**< The function failed due to a failure to write digital input data to a string buffer. */
	ERR_DOUT_OUTPUT_OUTOFRANGE		=	15U, /**< The function failed due to a digital output specified out of range. */
	ERR_DOUT_PARSE_MISSING_KEY		=	16U, /**< The function failed due to a missing required key in the digital output command. */
	ERR_DOUT_OUTPUT_NOT_FOUND		=	17U, /**< The function failed due to the specified digital output not being found. */
	ERR_DOUT_PARSE_ERROR			=	18U, /**< The function failed due to a digital output parsing error. */
	ERR_DOUT_OUTPUT_UNSPECIFIED		=	19U, /**< The function failed due to an unspecified digital output. */
	ERR_DOUT_OUTPUT_EXISTS			=	20U, /**< The function failed because the specified digital output already exists. */
	ERR_DOUT_DOES_NOT_EXIST			= 	21U, /**< The function failed because the specified digital output does not exist. */
	ERR_DOUT_FAILED_WRITE			=	22U, /**< The function failed due to a failure to write digital output data to a string buffer. */
	ERR_CALIBRATION_MODE_FAILED		=	23U, /**< The function failed due to a failure to initiate the calibration mode in flash. */
	ERR_CALIBRATION_WRITE_FAILED	=	24U /**< The function failed due to a failure to write the calibration value in flash. */
} Tekdaqc_Function_Error_t;

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Check the error status of the digital outputs.
 */
void Tekdaqc_CheckStatus(void);

/**
 * @brief Converts a Tekdaqc_Command_Error_t into a human readable string.
 */
const char* Tekdaqc_CommandError_ToString(Tekdaqc_Command_Error_t error);

/**
 * @brief Converts a Tekdaqc_Function_Error_t into a human readable string.
 */
const char* Tekdaqc_FunctionError_ToString(Tekdaqc_Function_Error_t error);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

/**
 * @}
 */

#endif /* TEKDAQC_ERROR_H_ */
