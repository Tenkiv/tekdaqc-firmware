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
 * @file Tekdaqc_CommandInterpreter.c
 * @brief Source file for the Tekdaqc_CommandInterpreter.
 *
 * This is an interpreter for the commands sent from the controlling device. It executes the commands and responds
 * by calling the write function (pointer), which is specified when initialized. A command string is parsed in by
 * calling the parse function. The command string can consists of multiple commands, delimited by '\n', and are
 * executed in FIFO order. When any error occurs, it will give up execution and generate error message and responds,
 * may corrupt the result strings received by controlling device.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com) 
 * @since v1.0.0.0
 *
 * @modified by Pachia Cha (pcha@tenkiv.com)
 * @since v1.2.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "Tekdaqc_CommandInterpreter.h"
#include "ADC_StateMachine.h"
#include "DI_StateMachine.h"
#include "DO_StateMachine.h"
#include "Analog_Input.h"
#include "Digital_Input.h"
#include "Digital_Output.h"
#include "TelnetServer.h"
#include "ADS1256_Driver.h"
#include "Tekdaqc_Calibration.h"
#include "Tekdaqc_CalibrationTable.h"
#include "CommandState.h"
#include "Tekdaqc_BSP.h"
#include "Tekdaqc_Error.h"
#include "boolean.h"
#include "eeprom.h"
#include "Tekdaqc_Locator.h"
#include "Tekdaqc_Timers.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <inttypes.h>

#ifdef PRINTF_OUTPUT
#include <stdio.h>
#endif

extern volatile int currentAnHandlerState;
extern volatile int readColdJunction;
extern CalibrationState_t calibrationState;
extern void SelectCalibrationInput(void);
extern void ADC_Machine_Service_CalibratingVer2(void);
/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @def ALL_CHANNELS_STRING
 * @brief The keyword string to specify all added channels should be used.
 */
#define ALL_CHANNELS_STRING				"ALL"

/**
 * @internal
 * @def KEY_VALUE_PAIR_FLAG
 * @brief The character sequence which prefixes a key/value pair.
 */
#define KEY_VALUE_PAIR_FLAG				"--"

/**
 * @internal
 * @def RANGE_DELIMETER
 * @brief The character which indicates that a range of channels should be used, delimiting the low and high values of the range.
 */
#define RANGE_DELIMETER					'-'

/**
 * @internal
 * @def SET_DELIMETER
 * @brief The character which indicates that a set of characters are being used, delimiting them.
 */
#define SET_DELIMETER					','

/**
 * @internal
 * @def KEY_VALUE_PAIR_DELIMETER
 * @brief The character which separates the key and value in a key/value pair.
 */
#define KEY_VALUE_PAIR_DELIMETER		"="

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE TYPEDEFS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief A common function pointer for all of the command execution functions.
 */
typedef Tekdaqc_Command_Error_t (* const Ex_Command_Function)(char[][MAX_COMMANDPART_LENGTH],
		char[][MAX_COMMANDPART_LENGTH], uint8_t);

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Characters which signal the end of a command.
 */
static const char COMMAND_DELIMETER[] = {0x20, 0x00};

/**
 * List of all command strings the Tekdaqc recognizes.
 */
static const char* COMMAND_STRINGS[NUM_COMMANDS] = {"LIST_ANALOG_INPUTS", "READ_ADC_REGISTERS", "READ_ANALOG_INPUT",
		"ADD_ANALOG_INPUT", "REMOVE_ANALOG_INPUT", "CHECK_ANALOG_INPUT", "SET_ANALOG_INPUT_SCALE",
		"GET_ANALOG_INPUT_SCALE", "SYSTEM_CAL", "SYSTEM_GCAL", "READ_SELF_GCAL", "READ_SYSTEM_GCAL",
		"LIST_DIGITAL_INPUTS", "READ_DIGITAL_INPUT", "ADD_DIGITAL_INPUT", "REMOVE_DIGITAL_INPUT", "ADD_PWM_INPUT",
		"REMOVE_PWM_INPUT", "READ_PWM_INPUT", "LIST_PWM_INPUTS", "LIST_DIGITAL_OUTPUTS", "SET_DIGITAL_OUTPUT",
		"READ_DIGITAL_OUTPUT", "READ_DO_DIAGS", "REMOVE_DIGITAL_OUTPUT", "CLEAR_DIG_OUTPUT_FAULT", "SET_PWM_OUTPUT",
		"SET_PWM_OUTPUT_TIMER", "DISCONNECT", "REBOOT", "UPGRADE", "IDENTIFY", "SAMPLE", "HALT",
		"SET_USER_MAC", "CLEAR_USER_MAC", "SET_STATIC_IP", "GET_CALIBRATION_STATUS",
		"ENTER_CALIBRATION_MODE", "WRITE_GAIN_CALIBRATION_VALUE", "WRITE_CALIBRATION_TEMP", "WRITE_CALIBRATION_VALID",
		"EXIT_CALIBRATION_MODE", "SET_FACTORY_MAC_ADDR", "SET_BOARD_SERIAL_NUM", "UPDATE_FIRMWARE", "NONE"};

/**
 * List of all parameters for the LIST_ANALOG_INPUTS command.
 */
const char* LIST_ANALOG_INPUTS_PARAMS[NUM_LIST_ANALOG_INPUTS_PARAMS] = {};

/**
 * List of all parameters for the READ_ADC_REGISTERS command.
 */
const char* READ_ADC_REGISTERS_PARAMS[NUM_READ_ADC_REGISTERS_PARAMS] = {};

/**
 * List of all parameters for the READ_ANALOG_INPUT command.
 */
const char* READ_ANALOG_INPUT_PARAMS[NUM_READ_ANALOG_INPUT_PARAMS] = {PARAMETER_INPUT, PARAMETER_NUMBER};

/**
 * List of all parameters for the ADD_ANALOG_INPUT command.
 */
const char* ADD_ANALOG_INPUT_PARAMS[NUM_ADD_ANALOG_INPUT_PARAMS] = {PARAMETER_INPUT, PARAMETER_BUFFER, PARAMETER_RATE,
PARAMETER_GAIN,
PARAMETER_NAME};

/**
 * List of all parameters for the REMOVE_ANALOG_INPUT command.
 */
const char* REMOVE_ANALOG_INPUT_PARAMS[NUM_REMOVE_ANALOG_INPUT_PARAMS] = {PARAMETER_INPUT};

/**
 * List of all parameters for the CHECK_ANALOG_INPUT command.
 */
const char* CHECK_ANALOG_INPUT_PARAMS[NUM_CHECK_ANALOG_INPUT_PARAMS] = {PARAMETER_INPUT};

/**
 * List of all parameters for the SET_ANALOG_INPUT_SCALE command.
 */
const char* SET_ANALOG_INPUT_SCALE_PARAMS[NUM_SET_ANALOG_INPUT_SCALE_PARAMS] = {PARAMETER_SCALE};

/**
 * List of all parameters for the GET_ANALOG_INPUT_SCALE command.
 */
const char* GET_ANALOG_INPUT_SCALE_PARAMS[NUM_GET_ANALOG_INPUT_SCALE_PARAMS] = {};

/**
 * List of all parameters for the SYSTEM_CAL command.
 */
const char* SYSTEM_CAL_PARAMS[NUM_SYSTEM_CAL_PARAMS] = {};

/**
 * List of all parameters for the SYSTEM_GCAL command.
 */
const char* SYSTEM_GCAL_PARAMS[NUM_SYSTEM_GCAL_PARAMS] = {PARAMETER_INPUT};

/**
 * List of all parameters for the READ_SELF_GCAL command.
 */
const char* READ_SELF_GCAL_PARAMS[NUM_READ_SELF_GCAL_PARAMS] = {PARAMETER_BUFFER, PARAMETER_RATE, PARAMETER_GAIN};

/**
 * List of all parameters for the READ_SYSTEM_GCAL command.
 */
const char* READ_SYSTEM_GCAL_PARAMS[NUM_READ_SYSTEM_GCAL_PARAMS] = {};

/**
 * List of all the parameters for the LIST_DIGITAL_INPUT command.
 */
const char* LIST_DIGITAL_INPUTS_PARAMS[NUM_LIST_DIGITAL_INPUTS_PARAMS] = {};

/**
 * List of all parameters for the READ_DIGITAL_INPUT command.
 */
const char* READ_DIGITAL_INPUT_PARAMS[NUM_READ_DIGITAL_INPUT_PARAMS] = {PARAMETER_INPUT, PARAMETER_NUMBER};

/**
 * List of all parameters for the ADD_DIGITAL_INPUT command.
 */
const char* ADD_DIGITAL_INPUT_PARAMS[NUM_ADD_DIGITAL_INPUT_PARAMS] = {PARAMETER_INPUT, PARAMETER_NAME};

/**
 * List of all parameters for the REMOVE_DIGITAL_INPUT command.
 */
const char* REMOVE_DIGITAL_INPUT_PARAMS[NUM_REMOVE_DIGITAL_INPUT_PARAMS] = {PARAMETER_INPUT};

/* List of all parameters for the ADD_PWM_INPUT_PARAMS command.
*/
const char* ADD_PWM_INPUT_PARAMS[NUM_ADD_PWM_INPUT_PARAMS] = {PARAMETER_INPUT, PARAMETER_AVERAGE, PARAMETER_NAME};
/**
 *  List of all parameters for the REMOVE_PWM_INPUT_PARAMS command.
*/
const char* REMOVE_PWM_INPUT_PARAMS[NUM_REMOVE_PWM_INPUT_PARAMS] = {PARAMETER_INPUT};
/**
 *  List of all parameters for the READ_PWM_INPUT_PARAMS command.
*/
const char* READ_PWM_INPUT_PARAMS[NUM_READ_PWM_INPUT_PARAMS] = {PARAMETER_INPUT, PARAMETER_NUMBER};
/**
 *  List of all parameters for the LIST_PWM_INPUTS_PARAMS command.
*/
const char* LIST_PWM_INPUTS_PARAMS[NUM_LIST_PWM_INPUTS_PARAMS];

/**
 * List of all parameters for the LIST_DIGITAL_OUTPUTS command.
 */
const char* LIST_DIGITAL_OUTPUTS_PARAMS[NUM_LIST_DIGITAL_OUTPUTS_PARAMS] = {};

/**
 * List of all parameters for the SET_DIGITAL_OUTPUT command.
 */
const char* SET_DIGITAL_OUTPUT_PARAMS[NUM_SET_DIGITAL_OUTPUT_PARAMS] = {PARAMETER_OUTPUT};

/**
 * List of all parameters for the READ_DIGITAL_OUTPUT command.
 */
const char* READ_DIGITAL_OUTPUT_PARAMS[NUM_READ_DIGITAL_OUTPUT_PARAMS] = {};

/**
 * List of all parameters for the READ_DO_DIAGS command.
 */
const char* DO_DIAGS_PARAMS[NUM_DO_DIAGS_PARAMS] = {};

/**
 * List of all parameters for the REMOVE_DIGITAL_OUTPUT command.
 */
const char* REMOVE_DIGITAL_OUTPUT_PARAMS[NUM_REMOVE_DIGITAL_OUTPUT_PARAMS] = {PARAMETER_OUTPUT};

/**
 * List of all parameters for the CLEAR_DIGITAL_OUTPUT command.
 */
const char* CLEAR_DIG_OUTPUT_FAULT_PARAMS[NUM_CLEAR_DIG_OUTPUT_FAULT_PARAMS] = {PARAMETER_OUTPUT};

/**
 * List of all parameters for the SET_PWM_PARAMS command.
 */
const char* SET_PWM_PARAMS[NUM_SET_PWM_PARAMS] = {PARAMETER_OUTPUT, PARAMETER_DUTYCYCLE};

/**
 * List of all parameters for the SET_PWM_OUT_TIME_PARAMS command.
 */
const char* SET_PWM_OUT_TIMER_PARAMS[NUM_SET_PWM_OUT_TIMER_PARAMS] = {PARAMETER_TIME};

/**
 * List of all parameters for the DISCONNECT command.
 */
const char* DISCONNECT_PARAMS[NUM_DISCONNECT_PARAMS] = {};

/**
 * List of all parameters for the REBOOT command.
 */
const char* REBOOT_PARAMS[NUM_REBOOT_PARAMS] = {};

/**
 * List of all parameters for the UPGRADE command.
 */
const char* UPGRADE_PARAMS[NUM_UPGRADE_PARAMS] = {};

/**
 * List of all parameters for the IDENTIFY command.
 */
const char* IDENTIFY_PARAMS[NUM_IDENTIFY_PARAMS] = {};

/**
 * List of all parameters for the SAMPLE command.
 */
const char* SAMPLE_PARAMS[NUM_SAMPLE_PARAMS] = {PARAMETER_NUMBER};

/**
 * List of all parameters for the HALT command.
 */
const char* HALT_PARAMS[NUM_HALT_PARAMS] = {};

/**
 * List of all parameters for the SET_USER_MAC command.
 */
const char* SET_USER_MAC_PARAMS[NUM_SET_USER_MAC_PARAMS] = {PARAMETER_VALUE};

/**
 * List of all parameters for the CLEAR_USER_MAC command.
 */
const char* CLEAR_USER_MAC_PARAMS[NUM_CLEAR_USER_MAC_PARAMS] = {};

/**
 * List of all parameters for the SET_STATIC_IP command.
 */
const char* SET_STATIC_IP_PARAMS[NUM_SET_STATIC_IP_PARAMS] = {PARAMETER_VALUE};

/**
 * List of all parameters for the GET_CALIBRATION_STATUS command.
 */
const char* GET_CALIBRATION_STATUS_PARAMS[NUM_GET_CALIBRATION_STATUS_PARAMS] = {};

/**
 * List of all parameters for the ENTER_CALIBRATION_MODE command.
 */
const char* ENTER_CALIBRATION_MODE_PARAMS[NUM_ENTER_CALIBRATION_MODE_PARAMS] = {};

/**
 * List of all parameters for the WRITE_GAIN_CALIBRATION_VALUE command.
 */
const char* WRITE_GAIN_CALIBRATION_VALUE_PARAMS[NUM_WRITE_GAIN_CALIBRATION_VALUE_PARAMS] = {PARAMETER_VALUE,
PARAMETER_GAIN, PARAMETER_RATE, PARAMETER_BUFFER, PARAMETER_SCALE, PARAMETER_INDEX};

/**
 * List of all parameters for the WRITE_CALIBRATION_TEMP command.
 */
const char* WRITE_CALIBRATION_TEMP_PARAMS[NUM_WRITE_CALIBRATION_TEMP_PARAMS] = {PARAMETER_TEMPERATURE, PARAMETER_INDEX};

/**
 * List of all parameters for the WRITE_CAL_VALID command.
 */
const char* WRITE_CAL_VALID_PARAMS[NUM_WRITE_CAL_VALID_PARAMS] = {};

/**
 * List of all parameters for the EXIT_CALIBRATION_MODE command.
 */
const char* EXIT_CALIBRATION_MODE_PARAMS[NUM_EXIT_CALIBRATION_MODE_PARAMS] = {};

/**
 * List of all parameters for the SET_FACTORY_MAC_ADDR command.
 */
const char* SET_FACTORY_MAC_ADDR_PARAMS[NUM_SET_FACTORY_MAC_ADDR_PARAMS] = {PARAMETER_VALUE};

/**
 * List of all parameters for the SET_BOARD_SERIAL_NUM command.
 */
const char* SET_BOARD_SERIAL_NUM_PARAMS[NUM_SET_BOARD_SERIAL_NUM_PARAMS] = {PARAMETER_VALUE};

/**
 * List of all parameters for the UPDATE_FIRMWARE command.
 */
const char* UPDATE_FIRMWARE_PARAMS[NUM_UPDATE_FIRMWARE_PARAMS] = {};

/**
 * List of all parameters for the NONE command.
 */
const char* NONE_PARAMS[NUM_NONE_PARAMS] = {};

/**
 * The command interpreter data structure instance.
 */
static Tekdaqc_CommandInterpreter_t interpreter;
/**
 * List of analog inputs referenced for use by a command.
 */
Analog_Input_t* aInputs[NUM_ANALOG_INPUTS];

volatile uint64_t numAnalogSamples, numDigitalSamples;
extern volatile uint64_t numSamplesTaken;
volatile int numOfDigitalInputs=0;
volatile int infiniteSampling=0;

/**
 * List of digital inputs referenced for use by a command.
 */
Digital_Input_t* dInputs[NUM_DIGITAL_INPUTS];

/**
 * List of pwm inputs referenced for use by a command.
 */
pwmInput_t* pInputs[NUM_DIGITAL_INPUTS];

/**
 * List of digital outputs referenced for use by a command.
 */
//Digital_Output_t* dOutputs[NUM_DIGITAL_OUTPUTS];

/**
 * The last function error which occurred.
 */
static Tekdaqc_Function_Error_t lastFunctionError = ERR_FUNCTION_OK;

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE TYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @brief Enumeration of the different channel selection types.
 * Defines the possible channel selection set types.
 */
typedef enum {
	SINGLE_CHANNEL, /**< A single channel is selected. */
	CHANNEL_RANGE, /**< A range of channels is selected. */
	CHANNEL_SET, /**< A set of channels is selected. */
	ALL_CHANNELS /**< All added channels are selected. */
} Channel_List_t;

/**
 * @internal
 * @brief Enumeration of the different multi-channel sampling types.
 * Enumeration of the different multi-channel sampling types.
 */
typedef enum {
	MULTI_ANALOG_INPUT, /**< Multi-sampling analog inputs. */
	MULTI_DIGITAL_INPUT, /**< Multi-sampling digital inputs. */
	MULTI_DIGITAL_OUTPUT /**< Multi-sampling digital outputs. */
} Multisampling_t;

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @brief Processes a command and any arguments it has.
 */
static void ProcessCommand(char* command, char raw_args[][MAX_COMMANDPART_LENGTH], uint8_t arg_count);

/**
 * @internal
 * @brief Parses a line from the command buffer.
 */
static void Command_ParseLine(void);

/**
 * @internal
 * @brief Parses the command portion of a command line.
 */
static Command_t ParseCommand(const char* command);

/**
 * @internal
 * @brief Process a command error.
 */
static void ProcessCommandError(const Tekdaqc_Command_Error_t error);

/**
 * @internal
 * @brief Process a function error.
 */
static void ProcessFunctionError(void);

/**
 * @internal
 * @brief Process the raw key/value pair string to extract the key/values.
 */
static bool ParseKeyValuePairs(char raw_args[][MAX_COMMANDPART_LENGTH], char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Checks the input arguments to ensure that they are appropriate for the specified command.
 */
static bool InputArgsCheck(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], uint8_t count,
		uint8_t num_params, const char** params);

/**
 * @internal
 * @brief Determine the channel list type.
 */
static Channel_List_t GetChannelListType(char* arg);

/**
 * @internal
 * @brief Build the list of analog inputs to sample.
 */
static void BuildAnalogInputList(Channel_List_t list_type, char* param);

/**
 * @internal
 * @brief Build the list of digital inputs to sample.
 */
static void BuildDigitalInputList(Channel_List_t list_type, char* param);

/**
 * @internal
 * @brief Build the list of digital outputs to sample.
 */
//static void BuildDigitalOutputList(Channel_List_t list_type, char* param);

/**
 * @internal
 * @brief Determine if the provided character is a valid text character.
 */
static bool isValidTextCharacter(const char character);

/**
 * @internal
 * @brief Converts the provided string to be entirely upper case.
 */
static void ToUpperCase(char* string);

/**
 * @internal
 * @brief Execute the specified command with the provided parameters.
 */
static Tekdaqc_Command_Error_t ExecuteCommand(Command_t command, char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the LIST_ANALOG_INPUTS command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ListAnalogInputs(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the READ_ADC_REGISTERS command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ReadADCRegisters(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the READ_ANALOG_INPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ReadAnalogInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);
//lfao
static Tekdaqc_Command_Error_t Ex_ReadAnalogInputVer2(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the ADD_ANALOG_INPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_AddAnalogInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the REMOVE_ANALOG_INPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_RemoveAnalogInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the CHECK_ANALOG_INPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_CheckAnalogInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the SET_ANALOG_INPUT_SCALE command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_SetAnalogInputScale(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the gET_ANALOG_INPUT_SCALE command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_GetAnalogInputScale(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the SYSTEM_GCAL command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_SystemGainCal(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the SYSTEM_CAL command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_SystemCalVer2(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count);

/**
 * @internal
 * @brief Execute the READ_SELF_GCAL command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ReadSelfGCal(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the READ_SYSTEM_GCAL command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ReadSystemGCal(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the LIST_DIGITAL_INPUTS command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ListDigitalInputs(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the READ_DIGITAL_INPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ReadDigitalInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the ADD_DIGITAL_INPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_AddDigitalInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the REMOVE_DIGITAL_INPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_RemoveDigitalInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the ADD_PWM_INPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_AddPwmInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);
/**
 * @internal
 * @brief Execute the REMOVE_PWM_INPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_RemovePwmInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the READ_PWM_INPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ReadPwmInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the LIST_PWM_INPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ListPwmInputs(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the LIST_DIGITAL_OUTPUTS command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ListDigitalOutputs(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the SET_DIGITAL_OUTPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_SetDigitalOutput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the READ_DIGITAL_OUTPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ReadDigitalOutput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the ADD_DIGITAL_OUTPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ReadDigitalOutputDiags(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the REMOVE_DIGITAL_OUTPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_RemoveDigitalOutput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the CLEAR_DIGITAL_OUTPUT_FAULT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ClearDigitalOutputFault(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the SET_PWM_OUTPUT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_SetPwmOutput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the SET_PWM_OUTPUT_TIME command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_SetPwmOutputTimer(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the DISCONNECT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_Disconnect(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count);

/**
 * @internal
 * @brief Execute the REBOOT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_Reboot(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count);

/**
 * @internal
 * @brief Execute the UPGRADE command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_Upgrade(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count);

/**
 * @internal
 * @brief Execute the IDENTIFY command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_Identify(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count);

/**
 * @internal
 * @brief Execute the SAMPLE command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_Sample(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count);

/**
 * @internal
 * @brief Execute the HALT command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_Halt(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count);

/**
 * @internal
 * @brief Execute the SET_USER_MAC command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_SetUserMac(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count);

/**
 * @internal
 * @brief Execute the CLEAR_USER_MAC command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ClearUserMac(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the SET_STATIC_IP command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_SetStaticIP(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the GET_CALIBRATION_STATUS command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_GetCalibrationStatus(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the ENTER_CALIBRATION_MODE command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_EnterCalibrationMode(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the WRITE_GAIN_CALIBRATION_VALUE command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_WriteGainCalibrationValue(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the WRITE_CALIBRATION_TEMP command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_WriteCalibrationTemp(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the WRITE_CALIBRATION_VALID command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_WriteCalibrationValid(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the EXIT_CALIBRATION_MODE command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_ExitCalibrationMode(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the SET_FACTORY_MAC_ADDR command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_SetFactoryMACAddr(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the SET_BOARD_SERIAL_NUM command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_SetBoardSerialNum(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the UPDATE_FIRMWARE command with the provided parameters.
 */

static Tekdaqc_Command_Error_t Ex_UpdateFirmware(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count);

/**
 * @internal
 * @brief Execute the NONE command with the provided parameters.
 */
static Tekdaqc_Command_Error_t Ex_None(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count);

/**
 * @internal
 * @brief Function lookup table for command execution.
 */
static Ex_Command_Function ExecutionFunctions[NUM_COMMANDS] = {Ex_ListAnalogInputs, Ex_ReadADCRegisters,
		Ex_ReadAnalogInputVer2, Ex_AddAnalogInput, Ex_RemoveAnalogInput, Ex_CheckAnalogInput, Ex_SetAnalogInputScale,
		Ex_GetAnalogInputScale, Ex_SystemCalVer2, Ex_SystemGainCal, Ex_ReadSelfGCal, Ex_ReadSystemGCal,
		Ex_ListDigitalInputs, Ex_ReadDigitalInput, Ex_AddDigitalInput, Ex_RemoveDigitalInput, Ex_AddPwmInput,
		Ex_RemovePwmInput, Ex_ReadPwmInput, Ex_ListPwmInputs, Ex_ListDigitalOutputs, Ex_SetDigitalOutput,
		Ex_ReadDigitalOutput, Ex_ReadDigitalOutputDiags, Ex_RemoveDigitalOutput, Ex_ClearDigitalOutputFault,
		Ex_SetPwmOutput, Ex_SetPwmOutputTimer, Ex_Disconnect, Ex_Reboot, Ex_Upgrade, Ex_Identify, Ex_Sample, Ex_Halt,
		Ex_SetUserMac, Ex_ClearUserMac, Ex_SetStaticIP, Ex_GetCalibrationStatus,
		Ex_EnterCalibrationMode, Ex_WriteGainCalibrationValue, Ex_WriteCalibrationTemp, Ex_WriteCalibrationValid,
		Ex_ExitCalibrationMode, Ex_SetFactoryMACAddr, Ex_SetBoardSerialNum, Ex_UpdateFirmware, Ex_None};

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Process a command and its arguments.
 *
 * @param command char* Pointer to the C-String holding the command.
 * @param raw_args char[][] Array of C-String's holding the unprocessed command arguments.
 * @param arg_count uint8_t The number of arguments provided.
 * @retval none
 */
static void ProcessCommand(char* command, char raw_args[][MAX_COMMANDPART_LENGTH], uint8_t arg_count) {
#ifdef COMMAND_DEBUG
	printf("[Command Interpreter] Processing command.\n\r");
#endif
	ToUpperCase(command); /* Convert the command string to upper case */
	Command_t command_type = ParseCommand(command); /* Determine which command this is */
	char keys[MAX_NUM_ARGUMENTS][MAX_COMMANDPART_LENGTH];
	char values[MAX_NUM_ARGUMENTS][MAX_COMMANDPART_LENGTH];
	Tekdaqc_Command_Error_t error = ERR_COMMAND_OK;
	if (command_type == COMMAND_ERROR) {
		error = ERR_COMMAND_BAD_COMMAND;
	} else {
		bool success = ParseKeyValuePairs(raw_args, keys, values, arg_count); /* Parse the argument key/value pairs. */
		if (success == TRUE) {
			error = ExecuteCommand(command_type, keys, values, arg_count); /* Execute the command */
		} else {
			error = ERR_COMMAND_PARSE_ERROR;
		}
	}
#ifdef COMMAND_DEBUG
	printf("[Command Interpreter] Processing command error.\n\r");
#endif
	ProcessCommandError(error); /* Handle any errors. */
#ifdef COMMAND_DEBUG
	printf("[Command Interpreter] Command processing returning.\n\r");
#endif
}

/**
 * Parse a command line from the command buffer.
 *
 * @param none
 * @retval none
 */
static void Command_ParseLine(void) {
#ifdef COMMAND_DEBUG
	printf("Parsing command: %s\n\r", interpreter.command_buffer);
#endif
	char raw_args[MAX_NUM_ARGUMENTS][MAX_COMMANDPART_LENGTH];
	char* pch;
	uint8_t count = 0U;
	char command[MAX_COMMANDPART_LENGTH];
	pch = strpbrk(interpreter.command_buffer, COMMAND_DELIMETER);
	while (pch != NULL) {
		++count;
		pch = strpbrk(pch + 1, COMMAND_DELIMETER);
	}
#ifdef COMMAND_DEBUG
	printf("[Command Parser] Parts count: %i\n\r", count);
#endif
	if (count > 0) { /* Command with arguments */
		/* Extract the command first */
		int command_end = strcspn(interpreter.command_buffer, COMMAND_DELIMETER);
		if (command_end == MAX_COMMANDPART_LENGTH) {
			/* This is not a valid command */
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Command was too long, ignoring.\n\r");
#endif
			ClearCommandBuffer();
			return;
		} else {
			/* This is a valid command */
			pch = strtok(interpreter.command_buffer, COMMAND_DELIMETER);
			strcpy(command, pch);
			for (int i = 0; i < count; ++i) {
				pch = strtok(NULL, COMMAND_DELIMETER);
				if (pch == NULL) {
					break;
				}
				strcpy(*(raw_args + i), pch);
			}
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Command: %s\n\r\tParts:\n\r", command);
			for (int i = 0; i < count; ++i) {
				printf("\t\tArg[%i]: %s\n\r", i, raw_args[i]);
			}
#endif 
		}
	} else {
		/* Command without arguments */
		int length = strlen(interpreter.command_buffer);
		if (length > (MAX_COMMANDPART_LENGTH - 1)) {
			/* This is not a valid command */
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Command was too long, ignoring.\n\r");
#endif
			/* Cleanup */
			ClearCommandBuffer();
			return;
		} else {
			/* Its a valid command */
			/* Copy the command */
			strcpy(command, interpreter.command_buffer);
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Command: %s\n\r", command);
#endif
		}
	}
	/* Process the command */
	ProcessCommand(command, raw_args, count);
}

/**
 * Parse a command string to determine which command it is.
 *
 * @param command const char* C-String containing the command text.
 * @retval Command_t The determined command.
 */
static Command_t ParseCommand(const char* command) {
#ifdef COMMAND_DEBUG
	printf("[Command Interpreter] Parsing command.\n\r");
#endif
	Command_t ret_command = COMMAND_LIST_ANALOG_INPUTS;
	while (ret_command <= COMMAND_NONE) {
		if (strcmp(command, COMMAND_STRINGS[ret_command]) == 0) {
			break;
		} else {
			++ret_command;
		}
	}
#ifdef COMMAND_DEBUG
	if (ret_command <= COMMAND_NONE) {
		printf("[Command Interpreter] Determined command to be: %s\n\r", COMMAND_STRINGS[ret_command]);
	} else {
		printf("[Command Interpreter] Unable to determine command.\n\r");
		ret_command = COMMAND_NONE;
	}
#endif
	return ret_command;
}

/**
 * Evaluate the specified Tekdaqc_Command_Error_t and handle any errors appropriately.
 *
 * @param error Tekdaqc_Command_Error_t The command error.
 * @retval none
 */
static void ProcessCommandError(const Tekdaqc_Command_Error_t error) {
	const char* error_string = Tekdaqc_CommandError_ToString(error);
	switch (error) {
		case ERR_COMMAND_OK:
			snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "SUCCESS - %s", error_string);
			break;
		case ERR_COMMAND_BAD_PARAM:
			snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "FAIL - %s.", error_string);
			break;
		case ERR_COMMAND_BAD_COMMAND:
			snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "FAIL - %s", error_string);
			break;
		case ERR_COMMAND_PARSE_ERROR:
			snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "FAIL - %s", error_string);
			break;
		case ERR_COMMAND_FUNCTION_ERROR:
			snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "FAIL - %s:\n\r\t", error_string);
			ProcessFunctionError();
			break;
		case ERR_COMMAND_UNKNOWN_ERROR:
			snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "FAIL - %s", error_string);
			break;
		case ERR_COMMAND_ADC_INVALID_OPERATION:
			snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "FAIL - %s", error_string);
			break;
		case ERR_COMMAND_DI_INVALID_OPERATION:
			snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "FAIL - %s", error_string);
			break;
		case ERR_COMMAND_DO_INVALID_OPERATION:
			snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "FAIL - %s", error_string);
			break;
		default:
			snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "ERROR - %s", error_string);
			break;
	}
	if (error == ERR_COMMAND_OK) {
		TelnetWriteStatusMessage(TOSTRING_BUFFER);
	} else {
		TelnetWriteErrorMessage(TOSTRING_BUFFER);
	}
}

/**
 * Evaluate the specified Tekdaqc_Function_Error_t and handle any errors appropriately.
 *
 * @param error Tekdaqc_Function_Error_t The command error.
 * @retval none
 */
static void ProcessFunctionError(void) {
	snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "Function Error: %s",
			Tekdaqc_FunctionError_ToString(lastFunctionError));
}

/**
 * Parse the raw key/value strings to determine the specific key/value parameters.
 *
 * @param raw_args char[][] Array of C-Strings containing the raw key/value pairs.
 * @param keys char[][] Array of C-Strings to store the parsed keys in.
 * @param values char[][] Array of C-Strings to store the parsed values in.
 * @param count uint8_t The number of arguments passed.
 * @retval bool TRUE if the parsing was successful.
 */
static bool ParseKeyValuePairs(char raw_args[][MAX_COMMANDPART_LENGTH], char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	char* raw;
	char temp[MAX_COMMANDPART_LENGTH];
	bool retval = TRUE;
	for (int i = 0; i < count; ++i) {
		raw = raw_args[i];
		if (strncmp(raw, KEY_VALUE_PAIR_FLAG, 2) == 0) {
			/* Properly formatted command */
			strcpy(temp, (raw + 2)); /* Offset for the first two characters */
			raw = strtok(temp, KEY_VALUE_PAIR_DELIMETER);
			ToUpperCase(raw);
			strcpy(keys[i], raw);
			raw = strtok(NULL, KEY_VALUE_PAIR_DELIMETER);
			if (raw != NULL) {
				ToUpperCase(raw);
				strcpy(values[i], raw);
			} else {
				strcpy(values[i], "");
			}
		} else {
			/* Not a properly formatted command */
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Key/value pair %i (%s) was not properly formatted.\n\r", i, raw);
#endif
			retval = FALSE;
			break;
		}
	}
	return retval;
}

/**
 * Evaluate the input arguments to determine if they are properly formatted.
 *
 * @param keys char[][] Array of C-Strings containing the parameter keys.
 * @param values[][] Array of C-Strings containing the parameter values.
 * @param count uint8_t The number of parameters provided.
 * @param num_params uint8_t The total number of parameters associated with the command.
 * @retval bool True if the check passes.
 */
static bool InputArgsCheck(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], uint8_t count,
		uint8_t num_params, const char** params) {
	if (count > num_params) {
		return false;
	}
	int idx = 0;
	bool valid = false;
	for (int i = 0; i < count; ++i) {
		idx = 0;
		valid = false;
		while (idx < num_params) {
			if (strcmp(keys[i], params[idx++]) == 0) {
				valid = true;
				break;
			}
		}
		if (valid == false) {
			/* This is a bad key */
			return false;
		}
	}
	return true;
}

/**
 * Determine which type of channel list was provided.
 *
 * @param arg char* C-String containing the channel list.
 * @retval Channel_List_t The type of channel list specified.
 */
static Channel_List_t GetChannelListType(char* arg) {
	if (strcmp(arg, ALL_CHANNELS_STRING) == 0) {
		return ALL_CHANNELS;
	} else if (strchr(arg, SET_DELIMETER) != NULL) {
		return CHANNEL_SET;
	} else if (strchr(arg, RANGE_DELIMETER) != NULL) {
		return CHANNEL_RANGE;
	} else {
		return SINGLE_CHANNEL;
	}
}

/**
 * Build the list of analog inputs which are to be sampled.
 *
 * @param list_type Channel_List_t The type of list provided in the parameter.
 * @param param char* C-String containing the inputs to use.
 * @retval none
 */
static void BuildAnalogInputList(Channel_List_t list_type, char* param) {
	uint8_t count = 0U;
	char* ptr = NULL;
	int8_t channel = -1;
	long int value = 0L;
	char* str = NULL;
	uint8_t start = 0U;
	uint8_t end = 0U;
	uint8_t value1 = 0U;
	uint8_t value2 = 0U;

	for (uint_fast8_t i = 0; i < NUM_ANALOG_INPUTS; ++i) {
		aInputs[i] = NULL; /* NULL them all initially */
	}

	switch (list_type) {
		case SINGLE_CHANNEL: /* SINGLE_CHANNEL */
			channel = (int8_t) strtoul(param, NULL, 10);
			if (channel < 0 || channel > NUM_ANALOG_INPUTS) {
				/* Input number out of range */
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] The requested input number is out of range.\n\r");
#endif
				break;
			}
			aInputs[0] = GetAnalogInputByNumber(channel);
			break; /* END SINGLE_INPUT */
		case CHANNEL_SET: /* CHANNEL_SET */
			if (param != NULL) {
				str = param;
				while (TRUE) {
					if (*ptr == SET_DELIMETER) {
						str = ptr + 1;
					}
					value = strtoul(str, &ptr, 10);
					if (value < NUM_ANALOG_INPUTS && value >= 0L) {
						aInputs[value] = GetAnalogInputByNumber(value);
						++count;
					}
					if (ptr == NULL || (count != 0U && value == 0U)) {
						break;
					}
				}
			}
			break; /* END INPUT SET */
		case CHANNEL_RANGE: /* INPUT_RANGE */
			start = 0U;
			end = 0U;
			value1 = (uint8_t) strtoul(param, &ptr, 10); /* We know these can potentially loose data...it would be invalid anyway */
			value2 = (uint8_t) strtoul((ptr + 1), NULL, 10);

			if (value1 != 0L) {
				start = value1;
			} else {
				start = 0U;
			}
			if (value2 != 0L) {
				end = value2;
			} else {
				end = NUM_ANALOG_INPUTS;
			}

			count = end - start + 1;
			for (int i = 0; i < count; ++i) {
				aInputs[i] = GetAnalogInputByNumber(start + i); /* Some of these may be NULL, this is OK. */
			}
			break; /* END INPUT RANGE */
		case ALL_CHANNELS: /* ALL_CHANNELS */
			//lfao-not including the cold junction here...
			count = NUM_ANALOG_INPUTS - 1;
			for (int i = 0; i < count; ++i) {
				aInputs[i] = GetAnalogInputByNumber(i); /* Some of these may be NULL, this is OK. */
			}
			break;
		default:
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] The specified input range is invalid.\n\r");
#endif
			break;
	}
}

/**
 * Build the list of digital inputs which are to be sampled.
 *
 * @param list_type Channel_List_t The type of list provided in the parameter.
 * @param param char* C-String containing the inputs to use.
 * @retval none
 */
static void BuildDigitalInputList(Channel_List_t list_type, char* param) {
	uint8_t count = 0U;
	char* ptr = NULL;
	int8_t channel = -1;
	long int value = 0L;
	char* str = NULL;
	uint8_t start = 0U;
	uint8_t end = 0U;
	uint8_t value1 = 0U;
	uint8_t value2 = 0U;

	for (uint_fast8_t i = 0U; i < NUM_DIGITAL_INPUTS; ++i) {
		dInputs[i] = NULL; /* NULL them all initially */
	}

	switch (list_type) {
		case SINGLE_CHANNEL: /* SINGLE_CHANNEL */
			channel = (int8_t) strtol(param, NULL, 10);
			if (channel < 0 || channel > NUM_DIGITAL_INPUTS) {
				/* Input number out of range */
#ifdef COMMAND_DEBUG
				printf("[Command Interpreter] The requested input number is out of range.\n\r");
#endif
				break;
			}
			dInputs[0] = GetDigitalInputByNumber(channel);
			break; /* END SINGLE_INPUT */
		case CHANNEL_SET: /* CHANNEL_SET */
			if (param != NULL) {
				str = param;
				while (TRUE) {
					if (*ptr == SET_DELIMETER) {
						str = ptr + 1;
					}
					value = strtol(str, &ptr, 10);
					if (value < NUM_DIGITAL_INPUTS && value >= 0L) {
						dInputs[value] = GetDigitalInputByNumber(value);
						++count;
					}
					if (ptr == NULL) {
						break;
					}
				}
			}
			break; /* END INPUT SET */
		case CHANNEL_RANGE: /* INPUT_RANGE */
			start = 0U;
			end = 0U;
			value1 = (uint8_t) strtol(param, &ptr, 10); /* We know these can potentially loose data...it would be invalid anyway */
			value2 = (uint8_t) strtol((ptr + 1), NULL, 10);

			if (value1 != 0L) {
				start = value1;
			} else {
				start = 0U;
			}
			if (value2 != 0L) {
				end = value2;
			} else {
				end = NUM_DIGITAL_INPUTS;
			}

			count = end - start + 1;
			for (int i = 0; i < count; ++i) {
				dInputs[i] = GetDigitalInputByNumber(start + i); /* Some of these may be NULL, this is OK. */
			}
			break;
		case ALL_CHANNELS: /* ALL_CHANNELS */
			count = NUM_DIGITAL_INPUTS;
			for (int i = 0; i < count; ++i) {
				dInputs[i] = GetDigitalInputByNumber(i); /* Some of these may be NULL, this is OK. */
			}
			break; /* END ALL_CHANNELS */
		default:
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] The specified input range is invalid.\n\r");
#endif
			break;
	}
}

/**
 * Build the list of pwm inputs which are to be sampled.
 *
 * @param list_type Channel_List_t The type of list provided in the parameter.
 * @param param char* C-String containing the inputs to use.
 * @retval none
 */
static void BuildPwmInputList(Channel_List_t list_type, char* param) {
	uint8_t count = 0U;
	char* ptr = NULL;
	int8_t channel = -1;
	long int value = 0L;
	char* str = NULL;
	uint8_t start = 0U;
	uint8_t end = 0U;
	uint8_t value1 = 0U;
	uint8_t value2 = 0U;

	for (uint_fast8_t i = 0U; i < NUM_DIGITAL_INPUTS; ++i) {
		pInputs[i] = NULL; /* NULL them all initially */
	}

	switch (list_type) {
		case SINGLE_CHANNEL: /* SINGLE_CHANNEL */
			channel = (int8_t) strtol(param, NULL, 10);
			if (channel < 0 || channel > NUM_DIGITAL_INPUTS) {
				/* Input number out of range */
#ifdef COMMAND_DEBUG
				printf("[Command Interpreter] The requested input number is out of range.\n\r");
#endif
				break;
			}
			pInputs[channel] = GetPwmInputByNumber(channel);
			break; /* END SINGLE_INPUT */
		case CHANNEL_SET: /* CHANNEL_SET */
			if (param != NULL) {
				str = param;
				while (TRUE) {
					if (*ptr == SET_DELIMETER) {
						str = ptr + 1;
					}
					value = strtol(str, &ptr, 10);
					if (value < NUM_DIGITAL_INPUTS && value >= 0L) {
						pInputs[value] = GetPwmInputByNumber(value);
						++count;
					}
					if (ptr == NULL) {
						break;
					}
				}
			}
			break; /* END INPUT SET */
		case CHANNEL_RANGE: /* INPUT_RANGE */
			start = 0U;
			end = 0U;
			value1 = (uint8_t) strtol(param, &ptr, 10); /* We know these can potentially loose data...it would be invalid anyway */
			value2 = (uint8_t) strtol((ptr + 1), NULL, 10);

			if (value1 != 0L) {
				start = value1;
			} else {
				start = 0U;
			}
			if (value2 != 0L) {
				end = value2;
			} else {
				end = NUM_DIGITAL_INPUTS;
			}

			count = end - start + 1;
			for (int i = 0; i < count; ++i) {
				pInputs[i] = GetPwmInputByNumber(start + i); /* Some of these may be NULL, this is OK. */
			}
			break;
		case ALL_CHANNELS: /* ALL_CHANNELS */
			count = NUM_DIGITAL_INPUTS;
			for (int i = 0; i < count; ++i) {
				pInputs[i] = GetPwmInputByNumber(i); /* Some of these may be NULL, this is OK. */
			}
			break; /* END ALL_CHANNELS */
		default:
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] The specified input range is invalid.\n\r");
#endif
			break;
	}
	for (int i = 0; i < NUM_DIGITAL_INPUTS; ++i) {
		if (pInputs[i] != NULL) {
			if (!pInputs[i]->average) {
				pInputs[i] = NULL;
			}
		}
	}
}

#if 0
/**
 * Build the list of digital outputs which are to be sampled.
 *
 * @param list_type Channel_List_t The type of list provided in the parameter.
 * @param param char* C-String containing the outputs to use.
 * @retval none
 */
static void BuildDigitalOutputList(Channel_List_t list_type, char* param) {
	uint8_t count = 0U;
	char* ptr = NULL;
	int8_t channel = -1;
	long int value = 0L;
	char* str = NULL;
	uint8_t start = 0U;
	uint8_t end = 0U;
	uint8_t value1 = 0U;
	uint8_t value2 = 0U;

	for (uint_fast8_t i = 0; i < NUM_DIGITAL_OUTPUTS; ++i) {
		dOutputs[i] = NULL; /* NULL them all initially */
	}

	switch (list_type) {
		case SINGLE_CHANNEL:
			channel = (int8_t) strtol(param, NULL, 10);
			if (channel < 0 || channel > NUM_DIGITAL_OUTPUTS) {
				/* Input number out of range */
#ifdef COMMAND_DEBUG
				printf("[Command Interpreter] The requested input number is out of range.\n\r");
#endif
				break;
			}
			dOutputs[0] = GetDigitalOutputByNumber(channel);
			break;
		case CHANNEL_SET:
			if (param != NULL) {
				str = param;
				while (TRUE) {
					if (*ptr == SET_DELIMETER) {
						str = ptr + 1;
					}
					value = strtol(str, &ptr, 10);
					if (value < NUM_DIGITAL_OUTPUTS && value >= 0L) {
						dOutputs[value] = GetDigitalOutputByNumber(value);
						++count;
					}
					if (ptr == NULL) {
						break;
					}
				}
			}
			break; /* END INPUT SET */
		case CHANNEL_RANGE:
			start = 0U;
			end = 0U;
			value1 = (uint8_t) strtol(param, &ptr, 10); /* We know these can potentially loose data...it would be invalid anyway */
			value2 = (uint8_t) strtol((ptr + 1), NULL, 10);

			if (value1 != 0L) {
				start = value1;
			} else {
				start = 0U;
			}
			if (value2 != 0L) {
				end = value2;
			} else {
				end = NUM_DIGITAL_OUTPUTS;
			}

			count = end - start + 1;
			for (int i = 0; i < count; ++i) {
				dOutputs[i] = GetDigitalOutputByNumber(start + i); /* Some of these may be NULL, this is OK. */
			}
			break; /* END CHANNEL RANGE */
		case ALL_CHANNELS: /* ALL_CHANNELS */
			count = NUM_DIGITAL_OUTPUTS;
			for (int i = 0; i < count; ++i) {
				dOutputs[i] = GetDigitalOutputByNumber(i); /* Some of these may be NULL, this is OK. */
			}
			break; /* END ALL_CHANNELS */
		default:
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] The specified input range is invalid.\n\r");
#else
			; /* Add an empty statment for the compiler */
#endif
	}
}
#endif
/**
 * Determines if the character provided is a valid text character for commands.
 *
 * @param character const char The character to check.
 * @retval bool True if the character is valid.
 */
static bool isValidTextCharacter(const char character) {
	return (character == 0x5F || (character >= 0x41 && character <= 0x5A) || (character >= 0x61 && character <= 0x7A)) ?
			true : false;
}

/**
 * Converts the provided C-String into all upper case characters, in place.
 *
 * @param string char* The C-String to convert.
 * @retval none
 */
static void ToUpperCase(char* string) {
	int i = 0;
	char character;
	while (string[i]) {
		character = string[i];
		if (isValidTextCharacter(character)) {
			if ((character >= 0x41 && character <= 0x5A) == false) {
				if (character != 0x5F) { /* Don't treat underscores */
					string[i] = (character - 0x20);
				}
			}
		}
		++i;
	}
}

/**
 * Executes the specified command with the specified parameters.
 *
 * @param command Command_t The command to execute.
 * @param keys char[][] Array of C-String parameter keys for the command.
 * @param values char[][] Array of C-String parameter values for the command.
 * @param count uint8_t The number of parameters for the command.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t ExecuteCommand(Command_t command, char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	Ex_Command_Function function = ExecutionFunctions[command];
	retval = function(keys, values, count);
	return retval;
}

/**
 * Execute the LIST_ANALOG_INPUTS command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_ListAnalogInputs(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_LIST_ANALOG_INPUTS_PARAMS, LIST_ANALOG_INPUTS_PARAMS)) {
		Tekdaqc_Function_Error_t status = ListAnalogInputs();
		if (status != ERR_FUNCTION_OK) {
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Listing of analog inputs failed with error: %s.\n\r",
					Tekdaqc_FunctionError_ToString(status));
#endif
			lastFunctionError = status;
			retval = ERR_COMMAND_FUNCTION_ERROR;
		}
	} else {
		/* We can't create a new input */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for listing the analog inputs.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the READ_ADC_REGISTERS command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_ReadADCRegisters(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_READ_ADC_REGISTERS_PARAMS, READ_ADC_REGISTERS_PARAMS)) {
		ClearToStringBuffer();
		ADS1256_RegistersToString();
		if (TOSTRING_BUFFER[0] != '\0') {
			//TelnetWriteString(TOSTRING_BUFFER);
			TelnetWriteCommandDataMessage(TOSTRING_BUFFER);
			ClearToStringBuffer();
		} else {
			/* An error occurred */
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Unable to build string to print ADC registers.\n\r");
#endif
			retval = ERR_COMMAND_FUNCTION_ERROR;
		}
	} else {
		/* We can't create a new input */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for reading the ADC registers.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}
#if 0
/**
 * Execute the READ_ANALOG_INPUT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_ReadAnalogInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_READ_ANALOG_INPUT_PARAMS, READ_ANALOG_INPUT_PARAMS)) {
		int32_t numSamples = 0;
		Channel_List_t list_type = ALL_CHANNELS;
		int8_t index = -1;
		for (int i = 0; i < NUM_READ_ANALOG_INPUT_PARAMS; ++i) {
			index = GetIndexOfArgument(keys, READ_ANALOG_INPUT_PARAMS[i], count);
			if (index >= 0) { /* We found the key in the list */
				switch (i) { /* Switch on the key not position in arguments list */
					case 0: /* INPUT key */
#ifdef COMMAND_DEBUG
						printf("Processing INPUT key\n\r");
#endif
						list_type = GetChannelListType(values[index]);
						BuildAnalogInputList(list_type, values[index]);
						break;
					case 1: /* NUMBER key */
#ifdef COMMAND_DEBUG
						printf("Processing NUMBER key\n\r");
#endif
						numSamples = (int32_t) strtol(values[index], NULL, 10);
						break;
					default:
						/* Return an error */
						retval = ERR_COMMAND_PARSE_ERROR;
				}
			}
			if (retval != ERR_COMMAND_OK) {
				break; /* If an error occurred, don't bother continuing */
			}
		}
		/* If an error occurred, don't bother continuing */
		if (retval == ERR_COMMAND_OK && list_type != ALL_CHANNELS) {
			for (uint_fast8_t i = 0; i < NUM_ANALOG_INPUTS; ++i) {
				if (aInputs[i] != NULL) {
					if (aInputs[i]->added == CHANNEL_NOTADDED) {
						retval = ERR_COMMAND_FUNCTION_ERROR;
						lastFunctionError = ERR_AIN_INPUT_NOT_FOUND;
						break;
					}
				}
			}
		}
		if (retval == ERR_COMMAND_OK) { /* If an error occurred, don't bother continuing */
			switch (list_type) {
				case SINGLE_CHANNEL:
#ifdef COMMAND_DEBUG
					printf("[Command Interpreter] ADC Sampling Single Channel.\n\r");
#endif
					ADC_Machine_Input_Sample(aInputs, numSamples, true);
					break;
				case CHANNEL_SET:
#ifdef COMMAND_DEBUG
					printf("[Command Interpreter] ADC Sampling Channel Set.\n\r");
#endif
					ADC_Machine_Input_Sample(aInputs, numSamples, false);
					break;
				case CHANNEL_RANGE:
#ifdef COMMAND_DEBUG
					printf("[Command Interpreter] ADC Sampling Channel Range.\n\r");
#endif
					ADC_Machine_Input_Sample(aInputs, numSamples, false);
					break;
				case ALL_CHANNELS:
#ifdef COMMAND_DEBUG
					printf("[Command Interpreter] ADC Sampling All Channels.\n\r");
#endif
					ADC_Machine_Input_Sample(aInputs, numSamples, false);
					break;
				default:
					/* Return an error */
					retval = ERR_COMMAND_FUNCTION_ERROR;
					lastFunctionError = ERR_COMMAND_ADC_INVALID_OPERATION;
			}
			CommandStateMoveToAnalogInputSample();
		}
	} else {
		/* We can't create a new input */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for read of an analog input.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}
#endif
//lfao
static Tekdaqc_Command_Error_t Ex_ReadAnalogInputVer2(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	AnalogHalt();
	if (InputArgsCheck(keys, values, count, NUM_READ_ANALOG_INPUT_PARAMS, READ_ANALOG_INPUT_PARAMS)) {
		numAnalogSamples = 0;

		Channel_List_t list_type = ALL_CHANNELS;
		int8_t index = -1;
		for (int i = 0; i < NUM_READ_ANALOG_INPUT_PARAMS; ++i) {
			index = GetIndexOfArgument(keys, READ_ANALOG_INPUT_PARAMS[i], count);
			if (index >= 0) { /* We found the key in the list */
				switch (i) { /* Switch on the key not position in arguments list */
					case 0: /* INPUT key */
#ifdef COMMAND_DEBUG
						printf("Processing INPUT key\n\r");
#endif
						list_type = GetChannelListType(values[index]);
						BuildAnalogInputList(list_type, values[index]);
						break;
					case 1: /* NUMBER key */
#ifdef COMMAND_DEBUG
						printf("Processing NUMBER key\n\r");
#endif
						numAnalogSamples = (int32_t) strtol(values[index], NULL, 10);
						break;
					default:
						/* Return an error */
						retval = ERR_COMMAND_PARSE_ERROR;
				}
			}
			if (retval != ERR_COMMAND_OK) {
				break; /* If an error occurred, don't bother continuing */
			}
		}
		/* If an error occurred, don't bother continuing */
		if (retval == ERR_COMMAND_OK && list_type != ALL_CHANNELS) {
			for (uint_fast8_t i = 0; i < NUM_ANALOG_INPUTS; ++i) {
				if (aInputs[i] != NULL) {
					if (aInputs[i]->added == CHANNEL_NOTADDED) {
						retval = ERR_COMMAND_FUNCTION_ERROR;
						lastFunctionError = ERR_AIN_INPUT_NOT_FOUND;
						break;
					}
				}
			}
		}

	} else {
		/* We can't create a new input */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for read of an analog input.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
    currentAnHandlerState=1;
	return retval;
}


/**
 * Execute the ADD_ANALOG_INPUT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_AddAnalogInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (isADCSampling() == FALSE) {
		if (InputArgsCheck(keys, values, count, NUM_ADD_ANALOG_INPUT_PARAMS, ADD_ANALOG_INPUT_PARAMS) == true) {
			/* Create a new input */
			Tekdaqc_Function_Error_t status = CreateAnalogInput(keys, values, count);
			if (status != ERR_FUNCTION_OK) {
				/* Something went wrong with creating the input */
#ifdef COMMAND_DEBUG
				printf("[Command Interpreter] Creating a new analog input failed with error: %s.\n\r",
						Tekdaqc_FunctionError_ToString(status));
#endif
				lastFunctionError = status;
				retval = ERR_COMMAND_FUNCTION_ERROR;
			}
		} else {
			/* We can't create a new input */
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Provided arguments are not valid for creation of a new analog input.\n\r");
#endif
			retval = ERR_COMMAND_PARSE_ERROR;
		}
	} else {
		retval = ERR_COMMAND_ADC_INVALID_OPERATION;
	}
	return retval;
}

/**
 * Execute the REMOVE_ANALOG_INPUT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_RemoveAnalogInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (isADCSampling() == FALSE) {
		if (InputArgsCheck(keys, values, count, NUM_REMOVE_ANALOG_INPUT_PARAMS, REMOVE_ANALOG_INPUT_PARAMS)) {
			Tekdaqc_Function_Error_t status = RemoveAnalogInput(keys, values, count);
			if (status != ERR_FUNCTION_OK) {
				/* Something went wrong with removing the input */
#ifdef COMMAND_DEBUG
				printf("[Command Interpreter] Removing an analog input failed with error code: %s.\n\r",
						Tekdaqc_FunctionError_ToString(status));
#endif
				lastFunctionError = status;
				retval = ERR_COMMAND_FUNCTION_ERROR;
			}
		} else {
			/* We can't delete an input */
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Provided arguments are not valid for removal of an analog input.\n\r");
#endif
			retval = ERR_COMMAND_BAD_PARAM;
		}
	} else {
		retval = ERR_COMMAND_ADC_INVALID_OPERATION;
	}
	return retval;
}

/**
 * Execute the CHECK_ANALOG_INPUT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_CheckAnalogInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	/* TODO: Fill in this method */
	return retval;
}

/**
 * Execute the SET_ANALOG_INPUT_SCALE command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_SetAnalogInputScale(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_SET_ANALOG_INPUT_SCALE_PARAMS, SET_ANALOG_INPUT_SCALE_PARAMS)) {
		int8_t index = -1;
		for (int i = 0; i < NUM_SET_ANALOG_INPUT_SCALE_PARAMS; ++i) {
			index = GetIndexOfArgument(keys, SET_ANALOG_INPUT_SCALE_PARAMS[i], count);
			if (index >= 0) { /* We found the key in the list */
				switch (i) { /* Switch on the key not position in arguments list */
					case 0: /* SCALE key */
#ifdef COMMAND_DEBUG
						printf("Processing SCALE key\n\r");
#else
						; /* Add an empty statement for the compiler */
#endif
						ANALOG_INPUT_SCALE_t scale = Tekdaqc_StringToAnalogInputScale(values[index]);
						Tekdaqc_SetAnalogInputScale(scale);
						break;
					default:
						/* Return an error */
						retval = ERR_COMMAND_PARSE_ERROR;
				}
			}
			if (retval != ERR_COMMAND_OK) {
				break; /* If an error occurred, don't bother continuing */
			}
		}
	}
	return retval;
}

/**
 * Execute the GET_ANALOG_INPUT_SCALE command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_GetAnalogInputScale(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	const char* scale = Tekdaqc_AnalogInputScaleToString(Tekdaqc_GetAnalogInputScale());
	snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "Current Analog Input Voltage Scale: %s", scale);
	TelnetWriteCommandDataMessage(TOSTRING_BUFFER);
	return retval;
}

/**
 * Execute the SYSTEM_GCAL command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_SystemGainCal(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_SYSTEM_GCAL_PARAMS, SYSTEM_GCAL_PARAMS)) {
		/* Perform a system gain calibration */
		Tekdaqc_Function_Error_t status = PerformSystemGainCalibration(keys, values, count);
		if (status != ERR_FUNCTION_OK) {
			lastFunctionError = status;
			retval = ERR_COMMAND_FUNCTION_ERROR;
		}
	} else {
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for performing a system gain calibration.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the SYSTEM_CAL command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
#if 0
static Tekdaqc_Command_Error_t Ex_SystemCal(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	Tekdaqc_Function_Error_t status = PerformSystemCalibration();
	if (status != ERR_FUNCTION_OK) {
		lastFunctionError = status;
		retval = ERR_COMMAND_FUNCTION_ERROR;
	}
	return retval;
}
#endif

static Tekdaqc_Command_Error_t Ex_SystemCalVer2(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	AnalogHalt();
	//Tekdaqc_Function_Error_t status = PerformSystemCalibration();
	//---ADC_Calibrate();----------//////////////
	/* Update the finished state */
	calibrationState.finished = false;
	calibrationState.finished_count = 0U;
	calibrationState.buffer_index = 0U;
	calibrationState.rate_index = 0U;
	calibrationState.gain_index = 0U;

	/* Select the calibration input */
	SelectCalibrationInput();
	Delay_us(EXTERNAL_MUX_DELAY);
	//---------------------------------
	//---ADC_Machine_Service_Calibrating();----------//////////////
	while(1)
	{
		ADC_Machine_Service_CalibratingVer2();
		if(calibrationState.finished == true)
		{
			//lfao - run one more time to display the calibration complete message and set the isSelfCalibrated flag
			ADC_Machine_Service_CalibratingVer2();
			break;
		}
	}
	//---------------------------------
	return retval;
}

/**
 * Execute the READ_SELF_GCAL command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_ReadSelfGCal(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_READ_SELF_GCAL_PARAMS, READ_SELF_GCAL_PARAMS)) {
		/* Retrieve the self gain calibration value */
		uint32_t calibration;
		Tekdaqc_Function_Error_t status = GetSelfGainCalibration(&calibration, keys, values, count);
		if (status != ERR_FUNCTION_OK) {
			lastFunctionError = status;
			retval = ERR_COMMAND_FUNCTION_ERROR;
		} else {
			snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "Gain calibration value: 0x%" PRIX32, calibration);
			TelnetWriteCommandDataMessage(TOSTRING_BUFFER);
		}
	} else {
#ifdef COMMAND_DEBUG
		printf(
				"[Command Interpreter] Provided arguments are not valid for fetching a self gain calibration value.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the READ_SYSTEM_GCAL command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_ReadSystemGCal(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	lastFunctionError = ERR_FUNCTION_OK;
	uint32_t calibration = ADS1256_GetGainCalSetting();
	snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "Gain calibration value: 0x%" PRIX32, calibration);
	TelnetWriteCommandDataMessage(TOSTRING_BUFFER);
	return retval;
}

/**
 * Execute the LIST_DIGITAL_INPUTS command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_ListDigitalInputs(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_LIST_DIGITAL_INPUTS_PARAMS, LIST_DIGITAL_INPUTS_PARAMS)) {
		Tekdaqc_Function_Error_t status = ListDigitalInputs();
		if (status != ERR_FUNCTION_OK) {
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Listing of digital inputs failed with error: %s.\n\r",
					Tekdaqc_FunctionError_ToString(status));
#endif
			lastFunctionError = status;
			retval = ERR_COMMAND_FUNCTION_ERROR;
		}
	} else {
		/* We can't create a new input */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for listing the digital inputs.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the READ_DIGITAL_INPUT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_ReadDigitalInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	DigitalInputHalt();
	if (InputArgsCheck(keys, values, count, NUM_READ_DIGITAL_INPUT_PARAMS, READ_DIGITAL_INPUT_PARAMS)) {
		Channel_List_t list_type = 0;
		int8_t index = -1;
		for (int i = 0; i < NUM_READ_DIGITAL_INPUT_PARAMS; ++i) {
			index = GetIndexOfArgument(keys, READ_DIGITAL_INPUT_PARAMS[i], count);
			if (index >= 0) { /* We found the key in the list */
				switch (i) { /*Switch on the key not position in arguments list */
					case 0: /* INPUT key */
#ifdef COMMAND_DEBUG
						printf("Processing INPUT key\n\r");
#endif
						list_type = GetChannelListType(values[index]);
						BuildDigitalInputList(list_type, values[index]);
						//get count of all channels to sample...
						for (uint_fast8_t i = 0; i < NUM_DIGITAL_INPUTS; ++i)
						{
							if (dInputs[i] != NULL)
							{
								if(dInputs[i]->added == CHANNEL_ADDED)
								{
									numOfDigitalInputs++;
								}
							}
						}
						break;
					case 1: /* NUMBER key */
#ifdef COMMAND_DEBUG
						printf("Processing NUMBER key\n\r");
#endif
						numDigitalSamples = strtol(values[index], NULL, 10);
						break;
					default:
						/* Return error */
						retval = ERR_COMMAND_PARSE_ERROR;
				}
			}
			if (retval != ERR_COMMAND_OK) { /* If there was an error, don't both continuing. */
				break;
			}
		}
		/* If an error occurred, don't bother continuing */
		if (retval == ERR_COMMAND_OK && list_type != ALL_CHANNELS) {
			for (uint_fast8_t i = 0; i < NUM_DIGITAL_INPUTS; ++i) {
				if (dInputs[i] != NULL) {
					if (dInputs[i]->added == CHANNEL_NOTADDED) {
						retval = ERR_COMMAND_FUNCTION_ERROR;
						lastFunctionError = ERR_DIN_INPUT_NOT_FOUND;
						break;
					}
				}
			}
		}
	}
	return retval;
}

/**
 * Execute the ADD_DIGITAL_INPUT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_AddDigitalInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (isDISampling() == FALSE) {
		if (InputArgsCheck(keys, values, count, NUM_ADD_DIGITAL_INPUT_PARAMS, ADD_DIGITAL_INPUT_PARAMS) == true) {
			/* Create a new input */
			Tekdaqc_Function_Error_t status = CreateDigitalInput(keys, values, count);
			if (status != ERR_FUNCTION_OK) {
				/* Something went wrong with creating the input */
#ifdef COMMAND_DEBUG
				printf("[Command Interpreter] Creating a new digital input failed with error code: %s.\n\r",
						Tekdaqc_FunctionError_ToString(status));
#endif
				lastFunctionError = status;
				retval = ERR_COMMAND_FUNCTION_ERROR;
			}
		} else {
			/* We can't create a new input */
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Provided arguments are not valid for creation of a new digital input.\n\r");
#endif
			retval = ERR_COMMAND_BAD_PARAM;
		}
	} else {
		retval = ERR_COMMAND_DI_INVALID_OPERATION;
	}
	return retval;
}

/**
 * Execute the REMOVE_DIGITAL_INPUT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_RemoveDigitalInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (isDISampling() == FALSE) {
		if (InputArgsCheck(keys, values, count, NUM_REMOVE_DIGITAL_INPUT_PARAMS, REMOVE_DIGITAL_INPUT_PARAMS)) {
			Tekdaqc_Function_Error_t status = RemoveDigitalInput(keys, values, count);
			if (status != ERR_FUNCTION_OK) {
				/* Something went wrong with removing the input */
#ifdef COMMAND_DEBUG
				printf("[Command Interpreter] Removing a digital input failed with error code: %s.\n\r",
						Tekdaqc_FunctionError_ToString(status));
#endif
				lastFunctionError = status;
				retval = ERR_COMMAND_FUNCTION_ERROR;
			}
		} else {
			/* We can't delete an input */
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Provided arguments are not valid for removal of a digital input.\n\r");
#endif
			retval = ERR_COMMAND_BAD_PARAM;
		}
	} else {
		retval = ERR_COMMAND_DI_INVALID_OPERATION;
	}
	return retval;
}

/**
 * Execute the ADD_PWM_INPUT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_AddPwmInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;

	//keys are valid
	if (InputArgsCheck(keys, values, count, NUM_ADD_PWM_INPUT_PARAMS, ADD_PWM_INPUT_PARAMS)) {
		//check for valid values - input, average
		Tekdaqc_Function_Error_t status = CreatePwmInput(keys, values, count);
		if (status != ERR_FUNCTION_OK) {
			lastFunctionError = status;
			retval = ERR_COMMAND_FUNCTION_ERROR;
		}
	}
	else {
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the REMOVE_PWM_INPUT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_RemovePwmInput(char keys[][MAX_COMMANDPART_LENGTH], //add to command
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;

	if (InputArgsCheck(keys, values, count, NUM_REMOVE_PWM_INPUT_PARAMS, REMOVE_PWM_INPUT_PARAMS)) { //create variable
		Tekdaqc_Function_Error_t status = removePwmInput(keys, values, count);
		if (status != ERR_FUNCTION_OK) {
			lastFunctionError = status;
			retval = ERR_COMMAND_FUNCTION_ERROR;
		}
	}
	else {
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the READ_PWM_INPUT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_ReadPwmInput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	uint64_t numPwmSamples = 0;

	PwmInputHalt();
	if (InputArgsCheck(keys, values, count, NUM_READ_PWM_INPUT_PARAMS, READ_PWM_INPUT_PARAMS)) {
		Channel_List_t list_type = 0;
		int8_t index = -1;
		for (int i = 0; i < NUM_READ_PWM_INPUT_PARAMS; ++i) {
			index = GetIndexOfArgument(keys, READ_PWM_INPUT_PARAMS[i], count);
			if (index >= 0) { /* We found the key in the list */
				switch (index) {
					case 0:	//input
						list_type = GetChannelListType(values[index]);
						BuildPwmInputList(list_type, values[index]);
						break;
					case 1: //number
						numPwmSamples = strtol(values[index], NULL, 10);
						break;
					default:
						/* Return error */
						return ERR_COMMAND_PARSE_ERROR;
				}
			}
		}
		startPwmInput(numPwmSamples);
	}
	else {
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the LIST_DIGITAL_OUTPUTS command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_ListPwmInputs(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_LIST_PWM_INPUTS_PARAMS, LIST_PWM_INPUTS_PARAMS)) {
		Tekdaqc_Function_Error_t status = ListPwmInputs();
		if (status != ERR_FUNCTION_OK) {
			lastFunctionError = status;
			retval = ERR_COMMAND_FUNCTION_ERROR;
		}
	} else {
		/* We can't create a new input */
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the LIST_DIGITAL_OUTPUTS command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_ListDigitalOutputs(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
#if 0
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_LIST_DIGITAL_OUTPUTS_PARAMS, LIST_DIGITAL_OUTPUTS_PARAMS)) {
		Tekdaqc_Function_Error_t status = ListDigitalOutputs();
		if (status != ERR_FUNCTION_OK) {
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Listing of digital outputs failed with error: %s.\n\r",
					Tekdaqc_FunctionError_ToString(status));
#endif
			lastFunctionError = status;
			retval = ERR_COMMAND_FUNCTION_ERROR;
		}
	} else {
		/* We can't create a new input */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for listing the digital outputs.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
#endif
	return ERR_COMMAND_OK;
}

/**
 * Execute the SET_DIGITAL_OUTPUT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_SetDigitalOutput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count)
{

	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_SET_DIGITAL_OUTPUT_PARAMS, SET_DIGITAL_OUTPUT_PARAMS))
	{
		/* Set digital output */
		Tekdaqc_Function_Error_t status = SetDigitalOutput(keys, values, count);
		if (status != ERR_FUNCTION_OK) {
			lastFunctionError = status;
			retval = ERR_COMMAND_FUNCTION_ERROR;
		}
	}
	else
	{
		/* We can't set an output */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for setting a digital output.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;

}

/**
 * Execute the READ_DIGITAL_OUTPUT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_ReadDigitalOutput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count)
{

	//snprintf(TOSTRING_BUFFER, sizeof(TOSTRING_BUFFER), "Digital Output\n\r\tValue: %04x", ReadDigitalOutput());
	//TelnetWriteStatusMessage(TOSTRING_BUFFER);
	return ReadDigitalOutput();

}


static Tekdaqc_Command_Error_t Ex_ReadDigitalOutputDiags(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {

	//snprintf(TOSTRING_BUFFER, sizeof(TOSTRING_BUFFER), "Diagnostics\n\r\tValue: %08x", ReadDODiags());
	//TelnetWriteStatusMessage(TOSTRING_BUFFER);
	return ReadDODiags();

}

/**
 * Execute the REMOVE_DIGITAL_OUTPUT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_RemoveDigitalOutput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
#if 0
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (isDOSampling() == FALSE) {
		if (InputArgsCheck(keys, values, count,
		NUM_REMOVE_DIGITAL_OUTPUT_PARAMS, REMOVE_DIGITAL_OUTPUT_PARAMS)) {
			Tekdaqc_Function_Error_t status = RemoveDigitalOutput(keys, values, count);
			if (status != ERR_FUNCTION_OK) {
				/* Something went wrong with removing the input */
#ifdef COMMAND_DEBUG
				printf("[Command Interpreter] Removing an digital output failed with error code: %s.\n\r",
						Tekdaqc_FunctionError_ToString(status));
#endif
				lastFunctionError = status;
				retval = ERR_COMMAND_FUNCTION_ERROR;
			}
		} else {
			/* We can't delete an output */
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Provided arguments are not valid for removal of an digital output.\n\r");
#endif
			retval = ERR_COMMAND_BAD_PARAM;
		}
	} else {
		retval = ERR_COMMAND_DO_INVALID_OPERATION;
	}
	return retval;
#endif
	return ERR_COMMAND_OK;
}

/**
 * Execute the CLEAR_DIGITAL_OUTPUT_FAULT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_ClearDigitalOutputFault(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;

	/* TODO: Clear digital faults */

	return retval;
}

/**
 * Execute the SET_PWM_OUTPUT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_SetPwmOutput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;

	//check for valid keys
	if (InputArgsCheck(keys, values, count, NUM_SET_PWM_PARAMS, SET_PWM_PARAMS)) {
		Tekdaqc_Command_Error_t status = SetPwmOutput(keys, values, count);
		if (status != ERR_COMMAND_OK) {
			lastFunctionError = status;
			retval = ERR_COMMAND_FUNCTION_ERROR;
		}
	}
	else {
		retval = ERR_COMMAND_PARSE_ERROR;
	}
	return retval;
}


/**
 * Execute the SET_PWM_OUTPUT_TIMER command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_SetPwmOutputTimer(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;

	//check for valid keys
	if (InputArgsCheck(keys, values, count, NUM_SET_PWM_OUT_TIMER_PARAMS, SET_PWM_OUT_TIMER_PARAMS)) {
		Tekdaqc_Command_Error_t status = SetPwmOutputInterrupt(keys, values, count);
		if (status != ERR_COMMAND_OK) {
			lastFunctionError = status;
			retval = ERR_COMMAND_FUNCTION_ERROR;
		}
	}
	else {
		retval = ERR_COMMAND_PARSE_ERROR;
	}
	return retval;
}

/**
 * Execute the DISCONNECT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_Disconnect(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_DISCONNECT_PARAMS, DISCONNECT_PARAMS)) {
		/* Close the telnet connection */
		TelnetClose();
	} else {
		/* We received some params we weren't expecting */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for disconnecting the Telnet connection.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the REBOOT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_Reboot(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_REBOOT_PARAMS, REBOOT_PARAMS)) {
		/* Close the telnet connection */
		TelnetClose();
		/* Reset the processor */
		NVIC_SystemReset();
	} else {
		/* We received some params we weren't expecting */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for rebooting the Tekdaqc.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the UPGRADE command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_Upgrade(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_UPGRADE_PARAMS, UPGRADE_PARAMS)) {
		/* Write the update flag to the backup register */
		EE_WriteVariable(ADDR_USE_USER_MAC, UPDATE_FLAG_ENABLED);
		/* Close the telnet connection */
		TelnetClose();
		/* Reset the processor */
		NVIC_SystemReset();
	} else {
		/* We received some params we weren't expecting */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for rebooting the Tekdaqc.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the IDENTIFY command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_Identify(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_IDENTIFY_PARAMS, IDENTIFY_PARAMS)) {
		unsigned char* serial = Tekdaqc_GetLocatorBoardID();
		if (serial == '\0') {
			serial = ((unsigned char*) "None");
		}
		uint8_t type = Tekdaqc_GetLocatorBoardType();
		uint32_t ip = Tekdaqc_GetLocatorIp();
		unsigned char* MAC = Tekdaqc_GetLocatorMAC();
		uint32_t version = Tekdaqc_GetLocatorVersion();
		uint32_t count = 0;
		count = snprintf(TOSTRING_BUFFER, sizeof(TOSTRING_BUFFER), "Board Identity");
		count += snprintf(TOSTRING_BUFFER + count, 51 * sizeof(char), "\n\r\tSerial Number: %s", serial);
		//count -= 2;
		count = strlen(TOSTRING_BUFFER);
		count += snprintf(TOSTRING_BUFFER + count, sizeof(TOSTRING_BUFFER),
				"\n\r\tBoard Revision: %c\n\r\tFirmware Version: %lu.%lu.%lu.%lu", (char) type, (version & 0xff),
				((version >> 8) & 0xff), ((version >> 16) & 0xff), ((version >> 24) & 0xff));
		count += snprintf(TOSTRING_BUFFER + count, sizeof(TOSTRING_BUFFER), "\n\r\tIP Address: %lu.%lu.%lu.%lu",
				ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff);
		count += snprintf(TOSTRING_BUFFER + count, sizeof(TOSTRING_BUFFER),
				"\n\r\tMAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n\r", *MAC, *(MAC + 1), *(MAC + 2), *(MAC + 3),
				*(MAC + 4), *(MAC + 5));
		TelnetWriteStatusMessage(TOSTRING_BUFFER);
	} else {
		/* We can't sample */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for identification.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the SAMPLE command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
//lfao
static Tekdaqc_Command_Error_t Ex_Sample(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	uint64_t numPwmSamples = 0;
	numAnalogSamples = 0;

	//halt analog...
	AnalogHalt();
	DigitalInputHalt();
	PwmInputHalt();

	if (InputArgsCheck(keys, values, count, NUM_SAMPLE_PARAMS, SAMPLE_PARAMS)) {

		int8_t index = -1;
		for (int i = 0; i < NUM_SAMPLE_PARAMS; ++i) {
			index = GetIndexOfArgument(keys, SAMPLE_PARAMS[i], count);
			if (index >= 0) { /* We found the key in the list */
				switch (i) { /* Switch on the key not position in arguments list */
					case 0: /* NUMBER key */
#ifdef COMMAND_DEBUG
						printf("Processing NUMBER key\n\r");
#endif
						numAnalogSamples = (int32_t) strtol(values[index], NULL, 10);
						numDigitalSamples= numAnalogSamples;
						numPwmSamples = numAnalogSamples;
						break;
					default:
						/* Return an error */
						retval = ERR_COMMAND_PARSE_ERROR;
				}
			}
			if (retval != ERR_COMMAND_OK) {
				break; /* If an error occurred, don't bother continuing */
			}
		}
		if (retval == ERR_COMMAND_OK) { /* If an error occurred, don't bother continuing */
			BuildAnalogInputList(ALL_CHANNELS, NULL);
			BuildDigitalInputList(ALL_CHANNELS, NULL);
			//get count of all channels to sample...
			for (uint_fast8_t i = 0; i < NUM_DIGITAL_INPUTS; ++i)
			{
				if (dInputs[i] != NULL)
				{
					if(dInputs[i]->added == CHANNEL_ADDED)
					{
						numOfDigitalInputs++;
					}
				}
			}
			BuildPwmInputList(ALL_CHANNELS, NULL);
			startPwmInput(numPwmSamples);
		}
	} else {
		/* We can't sample */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for sampling.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	//enable analog sampling...
	currentAnHandlerState = 1;
	return retval;
}

/**
 * Execute the HALT command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
//lfao
static Tekdaqc_Command_Error_t Ex_Halt(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_HALT_PARAMS, HALT_PARAMS)) {
		/* Instruct the command state machine to halt all tasks */
		//disable analog sampling...
		AnalogHalt();
		DigitalInputHalt();
		PwmInputHalt();
	} else {
		/* We received some params we weren't expecting */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for halting task on the Tekdaqc.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the SET_USER_MAC command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_SetUserMac(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_SET_USER_MAC_PARAMS, SET_USER_MAC_PARAMS)) {
		int8_t index = -1;
		for (int i = 0; i < NUM_SET_USER_MAC_PARAMS; ++i) {
			index = GetIndexOfArgument(keys, SET_USER_MAC_PARAMS[i], count);
			if (index >= 0) { /* We found the key in the list */
				switch (i) { /* Switch on the key not position in arguments list */
					case 0: /* VALUE key */
#ifdef COMMAND_DEBUG
						printf("Processing VALUE key\n\r");
#else
						; /* Add an empty statement for the compiler */
#endif
						uint64_t mac = strtoull(values[i], NULL, 16);
#ifdef COMMAND_DEBUG
						printf("Decoded MAC address: 0x%012" PRIX64 "\n\r", mac);
#else
						; /* Add an empty statement for the compiler */
#endif
						uint16_t low = mac & 0xFFFF;
						uint16_t mid = (mac >> 16) & 0xFFFF;
						uint16_t high = (mac >> 32) & 0xFFFF;
#ifdef COMMAND_DEBUG
						printf("MAC Address:\n\r\tHIGH: %" PRIX16 "\n\r\tMID: %" PRIX16 "\n\r\tLOW: %" PRIX16 "\n\r",
								high, mid, low);
#else
						; /* Add an empty statement for the compiler */
#endif
						EE_WriteVariable(ADDR_USER_MAC_LOW, low);
						EE_WriteVariable(ADDR_USER_MAC_MID, mid);
						EE_WriteVariable(ADDR_USER_MAC_HIGH, high);
						EE_WriteVariable(ADDR_USE_USER_MAC, USE_USER_MAC);
						break;
					default:
						/* Return an error */
						retval = ERR_COMMAND_PARSE_ERROR;
				}
			}
			if (retval != ERR_COMMAND_OK) {
				break; /* If an error occurred, don't bother continuing */
			}
		}
	} else {
		/* We can't create a new input */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for setting the user MAC Address.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the CLEAR_USER_MAC command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_ClearUserMac(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_CLEAR_USER_MAC_PARAMS, CLEAR_USER_MAC_PARAMS)) {
		EE_WriteVariable(ADDR_USE_USER_MAC, USE_DEFAULT_MAC);
	} else {
		/* We can't create a new input */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for clearing the user MAC Address.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the SET_STATIC_IP command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_SetStaticIP(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;

	/* TODO: Set static IP */

	return retval;
}

/**
 * Execute the GET_CALIBRATION_STATUS command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_GetCalibrationStatus(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	bool valid = isTekdaqc_CalibrationValid();
	count = snprintf(TOSTRING_BUFFER, sizeof(TOSTRING_BUFFER), "Calibration Status: %s",
			(valid == true) ? "VALID" : "INVALID");
	TelnetWriteStatusMessage(TOSTRING_BUFFER);
	return retval;
}

/**
 * Execute the ENTER_CALIBRATION_MODE command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_EnterCalibrationMode(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	bool valid = (Tekdaqc_SetCalibrationMode() == FLASH_COMPLETE);
	if (valid != true) {
		retval = ERR_COMMAND_FUNCTION_ERROR;
		lastFunctionError = ERR_CALIBRATION_MODE_FAILED;
	}
	return retval;
}

/**
 * Execute the WRITE_GAIN_CALIBRATION_VALUE command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_WriteGainCalibrationValue(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_WRITE_GAIN_CALIBRATION_VALUE_PARAMS,
			WRITE_GAIN_CALIBRATION_VALUE_PARAMS) == true) {
		/* Create a new input */
		Tekdaqc_Function_Error_t status = Tekdaqc_WriteGainCalibrationValue(keys, values, count);
		if (status != ERR_FUNCTION_OK) {
			/* Something went wrong with creating the input */
#ifdef COMMAND_DEBUG
			printf("[Command Interpreter] Writing gain calibration value failed with error: %s.\n\r",
					Tekdaqc_FunctionError_ToString(status));
#endif
			lastFunctionError = status;
			retval = ERR_COMMAND_FUNCTION_ERROR;
		}
	} else {
		/* We can't create a new input */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for writing a gain calibration value.\n\r");
#endif
		retval = ERR_COMMAND_PARSE_ERROR;
	}
	return retval;
}

/**
 * Execute the WRITE_CALIBRATION_TEMP command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_WriteCalibrationTemp(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	if (InputArgsCheck(keys, values, count, NUM_WRITE_CALIBRATION_TEMP_PARAMS, WRITE_CALIBRATION_TEMP_PARAMS)) {
		int8_t index = -1;
		float temperature;
		uint8_t temp_idx;
		for (int i = 0; i < NUM_WRITE_CALIBRATION_TEMP_PARAMS; ++i) {
			index = GetIndexOfArgument(keys, WRITE_CALIBRATION_TEMP_PARAMS[i], count);
			if (index >= 0) { /* We found the key in the list */
				switch (i) { /* Switch on the key not position in arguments list */
					case 0: /* TEMPERATURE key */
#ifdef COMMAND_DEBUG
						printf("Processing TEMPERATURE key\n\r");
#endif
						errno = 0; /* Set the global error number to 0 so we get a valid check */
						temperature = strtof(values[index], NULL);
						if (errno != 0) {
							retval = ERR_COMMAND_PARSE_ERROR;
							break;
						}
						break;
					case 1: /* INDEX key */
#ifdef COMMAND_DEBUG
						printf("Processing INDEX key\n\r");
#endif
						temp_idx = strtol(values[index], NULL, 10);
						break;
					default:
						/* Return an error */
						retval = ERR_COMMAND_PARSE_ERROR;
				}
			}
			if (retval != ERR_COMMAND_OK) {
				break; /* If an error occurred, don't bother continuing */
			}
		}
		if (retval == ERR_COMMAND_OK) {
			/* If an error occured, don't bother executing */
			const bool valid = (Tekdaqc_SetCalibrationTemperature(temperature, temp_idx) == FLASH_COMPLETE);
			if (valid != true) {
				retval = ERR_COMMAND_FUNCTION_ERROR;
				lastFunctionError = ERR_CALIBRATION_WRITE_FAILED;
			}
		}
	}
	return retval;
}

/**
 * Execute the WRITE_CALIBRATION_VALID command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_WriteCalibrationValid(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	bool valid = (Tekdaqc_SetCalibrationValid() == FLASH_COMPLETE);
	if (valid != true) {
		retval = ERR_COMMAND_FUNCTION_ERROR;
		lastFunctionError = ERR_CALIBRATION_WRITE_FAILED;
	}
	return retval;
}

/**
 * Execute the EXIT_CALIBRATION_MODE command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_ExitCalibrationMode(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	Tekdaqc_EndCalibrationMode();
	return retval;
}

/**
 * Execute the SET_FACTORY_MAC_ADDR command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_SetFactoryMACAddr(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	uint16_t low, mid, high;
	if (InputArgsCheck(keys, values, count, NUM_SET_FACTORY_MAC_ADDR_PARAMS, SET_FACTORY_MAC_ADDR_PARAMS)) {
		int8_t index = -1;
		for (int i = 0; i < NUM_SET_FACTORY_MAC_ADDR_PARAMS; ++i) {
			index = GetIndexOfArgument(keys, SET_FACTORY_MAC_ADDR_PARAMS[i], count);
			if (index >= 0) { /* We found the key in the list */
				switch (i) { /* Switch on the key not position in arguments list */
					case 0: /* VALUE key */
#ifdef COMMAND_DEBUG
						printf("Processing VALUE key\n\r");
#else
						; /* Add an empty statement for the compiler */
#endif
						uint64_t mac = strtoull(values[i], NULL, 16);
#ifdef COMMAND_DEBUG
						printf("Decoded MAC address: 0x%012" PRIX64 "\n\r", mac);
#endif
						low = mac & 0xFFFF;
						mid = (mac >> 16) & 0xFFFF;
						high = (mac >> 32) & 0xFFFF;
#ifdef COMMAND_DEBUG
						printf(
								"MAC Address:\n\r\tHIGH: 0x%04" PRIX16 "\n\r\tMID: 0x%04" PRIX16 "\n\r\tLOW: 0x%04" PRIX16 "\n\r",
								high, mid, low);
#endif
						break;
					default:
						/* Return an error */
						retval = ERR_COMMAND_PARSE_ERROR;
				}
			}
			if (retval != ERR_COMMAND_OK) {
				break; /* If an error occurred, don't bother continuing */
			}
		}
		/* If an error occurred, don't bother continuing */
		if (retval == ERR_COMMAND_OK) {

			FLASH_Unlock();

			/* Clear pending flags (if any), observed post flashing */
			FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
			FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

			/* Wait for last operation to be completed */
			FLASH_Status status = FLASH_WaitForLastOperation();
			if (status != FLASH_COMPLETE) {
#ifdef COMMAND_DEBUG
				printf("Failed to unlock OTP region.\n\r");
#endif
				lastFunctionError = ERR_CALIBRATION_WRITE_FAILED;
				retval = ERR_COMMAND_FUNCTION_ERROR;
			} else {
				/* The high bytes are the first bytes in the MAC address from the perspective of the user */
				status = FLASH_ProgramByte(FACTORY_MAC_ADDR0, (high >> 8) & 0xFF);
				if (status == FLASH_COMPLETE)
					status = FLASH_ProgramByte(FACTORY_MAC_ADDR1, high & 0xFF);
				if (status == FLASH_COMPLETE)
					status = FLASH_ProgramByte(FACTORY_MAC_ADDR2, (mid >> 8) & 0xFF);
				if (status == FLASH_COMPLETE)
					status = FLASH_ProgramByte(FACTORY_MAC_ADDR3, mid & 0xFF);
				if (status == FLASH_COMPLETE)
					status = FLASH_ProgramByte(FACTORY_MAC_ADDR4, (low >> 8) & 0xFF);
				if (status == FLASH_COMPLETE)
					status = FLASH_ProgramByte(FACTORY_MAC_ADDR5, low & 0xFF);
				if (status == FLASH_COMPLETE)
					/* Lock the Factory MAC Address Block */
					status = FLASH_ProgramByte(FACTORY_MAC_LOCK_ADDR, 0x00);
				if (status != FLASH_COMPLETE) {
					/* An error occurred in writing the bytes */
					lastFunctionError = ERR_CALIBRATION_WRITE_FAILED;
					retval = ERR_COMMAND_FUNCTION_ERROR;
				}
			}

			FLASH_Lock();
		}
	} else {
		/* We can't create a new input */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for setting the factory MAC Address.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the SET_BOARD_SERIAL_NUM command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_SetBoardSerialNum(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Command_Error_t retval = ERR_COMMAND_OK;
	char * serial;
	uint8_t char_count = 1;
	if (InputArgsCheck(keys, values, count, NUM_SET_BOARD_SERIAL_NUM_PARAMS, SET_BOARD_SERIAL_NUM_PARAMS)) {
		int8_t index = -1;
		for (int i = 0; i < NUM_SET_BOARD_SERIAL_NUM_PARAMS; ++i) {
			index = GetIndexOfArgument(keys, SET_BOARD_SERIAL_NUM_PARAMS[i], count);
			if (index >= 0) { /* We found the key in the list */
				switch (i) { /* Switch on the key not position in arguments list */
					case 0: /* VALUE key */
#ifdef COMMAND_DEBUG
						printf("Processing VALUE key\n\r");
#endif
						serial = values[i];
						printf("Received serial number: %s.\n\r", serial);
						while (serial[char_count] != '\0') {
							++char_count;
						}
						if (char_count < BOARD_SERIAL_NUM_LENGTH) {
							retval = ERR_COMMAND_PARSE_ERROR;
						}
						break;
					default:
						/* Return an error */
						retval = ERR_COMMAND_PARSE_ERROR;
				}
			}
			if (retval != ERR_COMMAND_OK) {
				break; /* If an error occurred, don't bother continuing */
			}
		}
		/* If an error occurred, don't bother continuing */
		if (retval == ERR_COMMAND_OK) {

			FLASH_Unlock();

			/* Clear pending flags (if any), observed post flashing */
			FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
			FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

			/* Wait for last operation to be completed */
			FLASH_Status status = FLASH_WaitForLastOperation();
			if (status != FLASH_COMPLETE) {
#ifdef COMMAND_DEBUG
				printf("Failed to unlock OTP region.\n\r");
#endif
				lastFunctionError = ERR_CALIBRATION_WRITE_FAILED;
				retval = ERR_COMMAND_FUNCTION_ERROR;
			} else {
				uint8_t idx = 0;
				/* Program each byte of the serial number sequentially */
				while (status == FLASH_COMPLETE && idx < char_count) {
					status = FLASH_ProgramByte(BOARD_SERIAL_NUM_ADDR + idx, serial[idx]);
					++idx;
				}
				/* Lock the Serial Number Address Block */
				status = FLASH_ProgramByte(BOARD_SERIAL_LOCK_ADDR, 0x00);
				if (status != FLASH_COMPLETE) {
					/* An error occurred in writing the bytes */
					lastFunctionError = ERR_CALIBRATION_WRITE_FAILED;
					retval = ERR_COMMAND_FUNCTION_ERROR;
				}
			}

			FLASH_Lock();
		}
	} else {
		/* We can't create a new input */
#ifdef COMMAND_DEBUG
		printf("[Command Interpreter] Provided arguments are not valid for setting the factory MAC Address.\n\r");
#endif
		retval = ERR_COMMAND_BAD_PARAM;
	}
	return retval;
}

/**
 * Execute the UPDATE_FIRMWARE command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
__attribute__((section(".Boot_Check"))) uint32_t updateFirmware = 0;
static Tekdaqc_Command_Error_t Ex_UpdateFirmware(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count) {
	updateFirmware = 1;
	TelnetClose();
	NVIC_SystemReset();
	return ERR_COMMAND_OK;
}

/**
 * Execute the NONE command.
 *
 * @param keys char[][] C-String of the command parameter keys.
 * @param values char[][] C-String of the command parameter values.
 * @param count uint8_t The number of command parameters.
 * @retval Tekdaqc_Command_Error_t The command error status.
 */
static Tekdaqc_Command_Error_t Ex_None(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH],
		uint8_t count) {
	//ADS1256_SetDataRate(ADS1256_SPS_7500);
	//ADS1256_SetDataRate(ADS1256_SPS_3750);
	//lfao-settling time
	//Delay_us(1000000);
	//ADS1256_EXTI_Enable();
	return ERR_COMMAND_OK;
}

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Create the command interpreter, initializing its internal data structure to default values.
 *
 * @param none
 * @retval none
 */
void CreateCommandInterpreter(void) {
	ClearCommandBuffer();
}

/**
 * Clear all characters from the command buffer.
 *
 * @param none
 * @retval none
 */
void ClearCommandBuffer(void) {
	for (uint_fast8_t i = 0; i < MAX_COMMANDLINE_LENGTH; ++i) {
		interpreter.command_buffer[i] = '\0';
	}
	interpreter.buffer_position = 0U;
}

/**
 * Adds a character to the end of the command buffer.
 *
 * @param character const char The character to add.
 * @retval none
 */
void Command_AddChar(const char character) {
	if (character != 0x00 && interpreter.buffer_position < MAX_COMMANDLINE_LENGTH) {
		if (character == 0x0A || character == 0x0D) {
			/* We have reached the end of a command, parse it */
			Command_ParseLine();
			/* Clear the command buffer */
			ClearCommandBuffer();
		} else if (character == 0x08 || character == 0x7F) {
			/* Remove the last character from the buffer */
			if (interpreter.buffer_position > 0) {
				--interpreter.buffer_position; /* Move the list back one */
				interpreter.command_buffer[interpreter.buffer_position] = '\0'; /* Set the current position to a null character */
			}
		} else if (character == 0x20) {
			if (interpreter.buffer_position > 0
					&& interpreter.command_buffer[interpreter.buffer_position - 1] != 0x20) {
				/* Add another character to the buffer */
				interpreter.command_buffer[interpreter.buffer_position] = character; /* Add the character at the current position */
				++interpreter.buffer_position; /* Move the buffer pointer to the next position */
			}
			/* If the previous character is a space, ignore further spaces */
		} else {
			/* Add another character to the buffer */
			interpreter.command_buffer[interpreter.buffer_position] = character; /* Add the character at the current position */
			++interpreter.buffer_position; /* Move the buffer pointer to the next position */
		}
	}
}

/**
 * Retrieves the last set value for a function error and resets it to ERR_FUNCTION_OK.
 *
 * @param none
 * @return Tekdaqc_Function_Error_t The last set value for a function error.
 */
Tekdaqc_Function_Error_t GetLastFunctionError(void) {
	Tekdaqc_Function_Error_t retval = lastFunctionError;
	lastFunctionError = ERR_FUNCTION_OK;
	return retval;
}

/**
 * Retrieves the index of the desired argument in the list of keys.
 *
 * @param keys char[][] Array of C-Strings containing the list of keys to search.
 * @param target const char* The parameter key C-String to search for.
 * @param total uint8_t The total number of keys in the list.
 * @retval int8_t The index of the parameter in the keys array, or -1 if it was not found.
 */
int8_t GetIndexOfArgument(char keys[][MAX_COMMANDPART_LENGTH], const char* target, uint8_t total) {
	int8_t index = -1;
	for (uint_fast8_t i = 0U; i < total; ++i) {
		if (strcmp(keys[i], target) == 0) {
			/* Match found */
			index = i;
			break;
		}
	}
	return index;
}
