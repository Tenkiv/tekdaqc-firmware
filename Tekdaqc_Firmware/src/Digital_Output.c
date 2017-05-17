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
#define DIGITAL_OUTPUT_FORMATTER "\n\r--------------------\n\rDigital Output\n\r\tValue: %04x\n\r--------------------\n\r%c\n\r"
#define DIGITAL_DIAGS_FORMATTER "\n\r--------------------\n\rDiagnostics\n\r\tValue: %08x\n\r--------------------\n\r"

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* Pointer to the function to use when writing data to the output stream. */
static WriteFunction writer = NULL;

//static uint8_t DigitalOutputMap[] = {6, 1, 2, 4, 15, 8, 10,	12, 7, 0, 3, 5, 14, 9, 11, 13};
//static uint8_t channelMap[16] = {14,9,10,12,7,0,2,4,15,8,11,13,6,1,3,5};
static uint8_t channelMap[16] = {5,3,1,6,13,11,8,15,4,2,0,7,12,10,9,14};
extern volatile uint8_t pwmCounter;
volatile uint16_t digiOutput = 0;  	//current active digital output
volatile uint16_t pwmOutput = 0;		//current output on
volatile uint16_t currentPwm = 0;	//current active pwm output
volatile uint8_t pwmDutyCycle[16] = {0};	//saved dutycycle
volatile bool isPwm = FALSE;			//flag to indicate output is pwm
/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE EXTERNAL VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* List of external digital outputs */
//static Digital_Output_t Ext_DOutputs[NUM_DIGITAL_OUTPUTS];

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/
#if 0
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
	output->physicalChannel = NULL_CHANNEL;
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
	TLE7232_ReadAllDiagnosis();
	uint8_t control = TLE7232_ReadRegister(TLE7232_REG_CTL, output->physicalChannel / 8U);
	uint8_t channel = output->physicalChannel - (8U * (output->physicalChannel / 8U)); /* Convert the channel to a single chip index */
	uint8_t mask = 0x01 << channel; /* Move the bit flag to the correct position */
	if (level == OUTPUT_ON) {
		/* Turn an output on */
		control |= mask; /* Set the appropriate bit */
	} else {
		/* Turn an output off */
		control &= ~mask;	/* Clear the appropriate bit */
	}
	TLE7232_WriteRegister(TLE7232_REG_CTL, control, output->physicalChannel / 8U); /* Update the register */
	TLE7232_ReadAllDiagnosis();
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
#endif
void DigitalOutputsInit(void) {
	/*for (uint_fast8_t i = 0; i < NUM_DIGITAL_OUTPUTS; ++i) {
		InitializeOutput(&Ext_DOutputs[i]);
	}*/

	uint8_t data[NUMBER_TLE7232_CHIPS] = {0x00, 0x00};
	uint8_t read[NUMBER_TLE7232_CHIPS];
	TLE7232_ReadRegisterAll(TLE7232_REG_IMCR, read);
	TLE7232_WriteRegisterAll(TLE7232_REG_IMCR, data);

	/*uint8_t data[NUMBER_TLE7232_CHIPS] = {0xFF, 0xFF};
	TLE7232_WriteRegisterAll(TLE7232_REG_CTL, data);*/
}
#if 0
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
					dig_output->physicalChannel = DigitalOutputMap[output];
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
#endif
/* Set the 16-bit digital output */
Tekdaqc_Function_Error_t SetDigitalOutput(char keys[][MAX_COMMANDPART_LENGTH], char values[][MAX_COMMANDPART_LENGTH], uint8_t count)
{
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	char* param;
	int8_t index = -1, i;
	uint16_t uiOrigOutput, uiRemappedOutput=0;
	uint8_t uiHighByte;



	index = GetIndexOfArgument(keys, SET_DIGITAL_OUTPUT_PARAMS[0], count);
	param = values[index];

	//placed some error checking here...
	if (strlen(param) != 4)
	{
		return ERR_COMMAND_BAD_PARAM;
	}
	for (i = 0; i< strlen(param); i++)
	{
		if (param[i]<0x30 || param[i]>0x66)
		{
			return ERR_COMMAND_BAD_PARAM;
		}
		if (param[i]>0x39 && param[i]<0x41)
		{
			return ERR_COMMAND_BAD_PARAM;
		}
		if (param[i]>0x46 && param[i]<0x61)
		{
			return ERR_COMMAND_BAD_PARAM;
		}
	}

	sscanf(param, "%x", &uiOrigOutput);
	
	if (!isPwm) { //command from digital output
		if (uiOrigOutput & currentPwm) { //err - setting pwm as output
			return ERR_DOUT_OUTPUT_EXISTS;
		}
		digiOutput = uiOrigOutput; //valid output command, save
		uiOrigOutput |= pwmOutput; //keep pwm on, on
	}
	else { //command from pwm output
		isPwm = FALSE;
		uiOrigOutput |= digiOutput; //keep digital on, on
	}
	
    //remap channel labels data to actual output pins...
	for (i = 0; i < 16; i++)
	{
		if ((uiOrigOutput >> i) & 0x0001)
		{
			uiRemappedOutput = uiRemappedOutput | ((unsigned short int)(0x0001) << channelMap[i]);
		}
	}


	uiHighByte = (uint8_t)((uiRemappedOutput>>8) & 0xFF);

	//write to SPIDER
	//TLE7232_WriteRegister
	TLE7232_CS_LOW();
	//put delay here...refer to datasheet
	Delay_us(2);
	uiRemappedOutput = uiRemappedOutput & 0xFF;
	uint16_t command = TLE7232_CMD_WRITE_REGISTER | TLE7232_REG_CTL;
	command = command | uiRemappedOutput;
	SPI_I2S_SendData(TLE7232_SPI, command);
	while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
		/* The SPI transmit buffer has data, so wait patiently */
	}
	command = TLE7232_CMD_WRITE_REGISTER | TLE7232_REG_CTL;
	command = command | uiHighByte;
	SPI_I2S_SendData(TLE7232_SPI, command);
	while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
		/* The SPI transmit buffer has data, so wait patiently */
	}
	//put delay here...refer to datasheet
	Delay_us(2);
	TLE7232_CS_HIGH();
	Delay_us(2);
	//put delay here???

#if 0


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
#endif
	return retval;
}

/* Reads the digital outputs current output data */
Tekdaqc_Function_Error_t ReadDigitalOutput(void)
{
	uint16_t uOutputDataRead = 0;
	uint16_t uiOrigOutput = 0, i, j;

	//write to SPIDER
	//TLE7232_WriteRegister
	TLE7232_CS_LOW();
	//put delay here...refer to datasheet
	Delay_us(2);
	uint16_t command = TLE7232_CMD_READ_REGISTER | TLE7232_REG_CTL;
	SPI_I2S_SendData(TLE7232_SPI, command);
	while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
		/* The SPI transmit buffer has data, so wait patiently */
	}
	SPI_I2S_SendData(TLE7232_SPI, command);
	while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
		/* The SPI transmit buffer has data, so wait patiently */
	}
	//put delay here...refer to datasheet
	Delay_us(2);
	TLE7232_CS_HIGH();
	//put delay here???
	Delay_us(10);
	//clear whatever is in the received data before?
	uOutputDataRead =  SPI_I2S_ReceiveData(TLE7232_SPI);
	TLE7232_CS_LOW();
	//put delay here...refer to datasheet
	Delay_us(2);
	//send command to shift out data that needs to be read...

	SPI_I2S_SendData(TLE7232_SPI, command);
	while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
		/* The SPI transmit buffer has data, so wait patiently */
	}
	Delay_us(2);
	uOutputDataRead =  SPI_I2S_ReceiveData(TLE7232_SPI);
	uOutputDataRead = uOutputDataRead & 0xFF;
	Delay_us(2);
	SPI_I2S_SendData(TLE7232_SPI, command);
	while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
		/* The SPI transmit buffer has data, so wait patiently */
	}
	Delay_us(2);
	uOutputDataRead =  ((SPI_I2S_ReceiveData(TLE7232_SPI))<<8) | uOutputDataRead;
	TLE7232_CS_HIGH();
	Delay_us(2);


    //remap read data to actual channel labels...
	for (i = 0; i < 16; i++)
	{
		if((uOutputDataRead >> i) & 0x0001)
		{
			for(j=0; j<16; j++)
			{
				if(channelMap[j]==i)
				{
					uiOrigOutput = uiOrigOutput | ((uint16_t)(0x0001) << j);
					break;
				}

			}

		}
	}



	sprintf(TOSTRING_BUFFER, DIGITAL_OUTPUT_FORMATTER, uiOrigOutput, 0x1e);
	writer(TOSTRING_BUFFER);
	//print all active digital output
	sprintf(TOSTRING_BUFFER, "Digital Output: %04x\n\r"
			"%c\n\r", digiOutput, 0x1e);
	writer(TOSTRING_BUFFER);

	//print all active pwm output
	uint8_t index[16] = {0};
	if (currentPwm) {
		for (uint8_t i = 0; i < 16; i++) {
			if (!index[i]) {
				if (pwmDutyCycle[i]) {
					uint8_t tempDutyCycle = pwmDutyCycle[i];
					uint16_t tempPwm = (0x0001 << i);
					for (uint8_t j = (i+1); j < 16; j++) {
						if (tempDutyCycle == pwmDutyCycle[j]) {
							index[j] = 1;
							tempPwm |= (0x0001 << j);
						}
					}

					sprintf(TOSTRING_BUFFER, "\n\rPwm Output: %04x\n\r\tDutyCycle: %" PRIu8
							"%c\n\r", tempPwm, pwmDutyCycle[i], 0x1e);
					writer(TOSTRING_BUFFER);
				}
			}
		}
	}
	else {
		sprintf(TOSTRING_BUFFER, "\n\rPwm Output: 0000\n\r\tDutyCycle: 0"
				"%c\n\r", 0x1e);
		writer(TOSTRING_BUFFER);
	}
	return ERR_FUNCTION_OK;
}

/* Reads the digital output's diagnostic data*/
Tekdaqc_Function_Error_t ReadDODiags(void)
{
	uint uDiagDataRead = 0, uiOrigOutput = 0, i, j;
	//write to SPIDER
	//TLE7232_WriteRegister
	TLE7232_CS_LOW();
	//put delay here...refer to datasheet
	Delay_us(2);
	uint16_t command = TLE7232_CMD_DIAGNOSIS;
	SPI_I2S_SendData(TLE7232_SPI, command);
	while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
		/* The SPI transmit buffer has data, so wait patiently */
	}
	SPI_I2S_SendData(TLE7232_SPI, command);
	while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
		/* The SPI transmit buffer has data, so wait patiently */
	}
	//put delay here...refer to datasheet
	Delay_us(2);
	TLE7232_CS_HIGH();
	//put delay here???
	Delay_us(10);
	//clear whatever is in the received data before?
	uDiagDataRead =  SPI_I2S_ReceiveData(TLE7232_SPI);
	TLE7232_CS_LOW();
	//put delay here...refer to datasheet
	Delay_us(2);
	//send command to shift out data that needs to be read...

	SPI_I2S_SendData(TLE7232_SPI, command);
	while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
		/* The SPI transmit buffer has data, so wait patiently */
	}
	Delay_us(2);
	uDiagDataRead =  SPI_I2S_ReceiveData(TLE7232_SPI);
	Delay_us(2);
	SPI_I2S_SendData(TLE7232_SPI, command);
	while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
		/* The SPI transmit buffer has data, so wait patiently */
	}
	Delay_us(2);
	uDiagDataRead =  (uint)(SPI_I2S_ReceiveData(TLE7232_SPI)<<16) | uDiagDataRead;
	TLE7232_CS_HIGH();
	Delay_us(2);


    //remap read data to actual channel labels...
	for (i = 0; i < 16; i++)
	{
		for(j=0; j<16; j++)
		{
			if(channelMap[j]==i)
			{

				uiOrigOutput = uiOrigOutput | ((uint)((uDiagDataRead >> i*2) & 0x00000003) << j*2);

				break;
			}

		}
	}

	sprintf(TOSTRING_BUFFER, DIGITAL_DIAGS_FORMATTER, uiOrigOutput);
	writer(TOSTRING_BUFFER);
	return ERR_FUNCTION_OK;
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

//initialize pwm output interrupt
void InitializePwmInterrupt (void) {
	NVIC_InitTypeDef nVIC_InitStructure;
	nVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	nVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	nVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	nVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nVIC_InitStructure);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	InitializePwmTimer(1000);
}

//timer for pwm output
void InitializePwmTimer(uint32_t pwmTimerInt) { //interrupt every 1ms
	//84MHz / (84*1000) = 1kHz -> 1ms
    TIM_TimeBaseInitTypeDef PwmTimer;
    PwmTimer.TIM_Prescaler = 83;
    PwmTimer.TIM_CounterMode = TIM_CounterMode_Up;
    PwmTimer.TIM_Period = pwmTimerInt;
    PwmTimer.TIM_ClockDivision = TIM_CKD_DIV1;
    PwmTimer.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &PwmTimer);

    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

//update pwm
Tekdaqc_Function_Error_t SetPwm(uint16_t uiPwmOutput) {
	uint8_t i;
	uint8_t counter = pwmCounter;
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	char values[MAX_NUM_ARGUMENTS][MAX_COMMANDPART_LENGTH];
	char keys[MAX_NUM_ARGUMENTS][MAX_COMMANDPART_LENGTH];
	uint8_t count = 1;
	uint16_t dummy = pwmOutput;
	pwmOutput |= uiPwmOutput;

	//turn all inactive pwm off
	if (uiPwmOutput == 0) {
		pwmOutput &= currentPwm;
	}

	//turn all pwm on at '0' counter
	if (counter == 0) {
		for (i = 0; i < 16; i++) {
			if (pwmDutyCycle[i] > 0) {
				pwmOutput |= (0x0001 << i);
			}
		}
	}
	//turn pwm off
	else {
		for (i = 0; i < 16; i++) {
			if (pwmDutyCycle[i] && (counter >= pwmDutyCycle[i])) {
				pwmOutput &= ~(0x0001 << i);
			}
		}
	}
	if (pwmOutput == dummy) { //no change in output
		return retval;
	}
	strcpy(keys[0], PARAMETER_OUTPUT);
	char temp[5] = {'0'};
	for (int i = 0; i < 4; i++) {
		uint16_t dummy = (pwmOutput >> (i<<2)) & 0x000f;
		if (dummy < 10) {
			temp[3-i] = (int) dummy+ 48;
		}
		else {
			temp[3-i] = (int) dummy+ 87;
		}
	}
	strcpy(values[0], temp);
	isPwm = TRUE;
	retval = SetDigitalOutput(keys, values, count); //update digital outputs
	return retval;
}

Tekdaqc_Function_Error_t SetPwmOutput(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {

	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	char *param;
	int8_t index = -1, i, j;
	uint16_t uiPwmOutput = 0;
	uint32_t uiDutyCycle = 0;

	//check for valid output value(s)
	for (i = 0; i < NUM_SET_PWM_PARAMS; i++) {
		index = GetIndexOfArgument(keys, SET_PWM_PARAMS[i], count);

		if (index >= 0) {
			param = values[i];
			uint8_t len = strlen(param);
			if (len == 0) {
				return ERR_DOUT_OUTPUT_UNSPECIFIED;
			}

			switch(index) {
				case 0U: {
					if (len != 4) {
						return ERR_DOUT_PARSE_ERROR;
					}
					for (j = 0; j< len; j++)
					{
						if (param[j]<0x30 || param[j]>0x66) //<'0' / >'f'
						{
							return ERR_DOUT_PARSE_ERROR;
						}
						if (param[j]>0x39 && param[j]<0x41) //>'9' / <'A'
						{
							return ERR_DOUT_PARSE_ERROR;
						}
						if (param[j]>0x46 && param[j]<0x61) //>'F' / <'a'
						{
							return ERR_DOUT_PARSE_ERROR;
						}
					}
					sscanf(param, "%x", &uiPwmOutput); //obtain output input(s) in int
					//output already exists
					if (uiPwmOutput & digiOutput) {
						return ERR_DOUT_OUTPUT_EXISTS;
					}
					break;
				}
				case 1U: {
					//valid dutycyle value
					for (j = 0; j<len; j++) {
						if (param[j] < 48 || param[j] > 57) { //<'0', >'9'
							return ERR_DOUT_PARSE_ERROR;
						}
					}
					sscanf(param, "%lu", &uiDutyCycle); //obtain duty cycle in int
					break;
				}
				default:
					return ERR_DOUT_PARSE_ERROR;
			}
		}
		else {
			return ERR_DOUT_PARSE_MISSING_KEY;
		}
	}
	//update Duty Cycle values with user input values
	for (int j = 0; j < 16; j++) {
		if ((uiPwmOutput >> j) & 0x0001) {
			pwmDutyCycle[j] = uiDutyCycle;
		}
	}
	//track pwm output
	if (uiDutyCycle) {
		currentPwm |= uiPwmOutput;
	}
	else {
		//uiPwmOutput = !uiPwmOutput;
		currentPwm &= !uiPwmOutput;
		uiPwmOutput = 0;
	}
	//valid command, keys, values -> ok to save values
	retval = SetPwm(uiPwmOutput); //update digital outputs with new param
	return retval;
}

Tekdaqc_Function_Error_t SetPwmOutputInterrupt(char keys[][MAX_COMMANDPART_LENGTH],
		char values[][MAX_COMMANDPART_LENGTH], uint8_t count) {
	Tekdaqc_Function_Error_t retval = ERR_FUNCTION_OK;
	char *param;
	int8_t index = -1, i, j;
	uint32_t timerInterrupt = 0;

	//check for valid output value(s)
	for (i = 0; i < NUM_SET_PWM_OUT_TIMER_PARAMS; i++) {
		index = GetIndexOfArgument(keys, SET_PWM_OUT_TIMER_PARAMS[i], count);

		if (index == 0) {
			param = values[i];
			uint8_t len = strlen(param);
			if (len == 0) {
				return ERR_DOUT_PARSE_ERROR;
			}

			switch(index) {
				case 0U:  { //pwm timer interrupt in microseconds
					// 84_000_000 / 84 = 1_000_000
					// 1_000_000
					for (j = 0; j < len; j++) {
						if (param[j] < 48 || param[j] > 57) { //<'0', >'9'
							return ERR_DOUT_PARSE_ERROR;
						}
					}
					sscanf(param, "%d", &timerInterrupt); //cvt to int
					if (timerInterrupt < 1000) { // >= 1000
						return ERR_DOUT_PARSE_ERROR;
					} else {
						if ((timerInterrupt % 1000) != 0) { //multiples of 1000
							return ERR_DOUT_PARSE_ERROR;
						}
					}
					break;
				}
				default:
					return ERR_DOUT_PARSE_ERROR;
			}
		} else {
			return ERR_DOUT_PARSE_ERROR;
		}
	}

	TIM_Cmd(TIM3, DISABLE);
	InitializePwmTimer(timerInterrupt);
	return retval;
}
#if 0
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
	/* Reverse the mapping to an input number */
	for (uint8_t i = 0; i < NUM_DIGITAL_OUTPUTS; ++i) {
		if (out == DigitalOutputMap[i]) {
			out = i;
			break;
		}
	}
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

#endif
