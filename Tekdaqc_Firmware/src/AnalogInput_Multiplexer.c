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
 * @file AnalogInput_Multiplexer.c
 * @brief Analog input multiplexer, handling internal and external analog inputs.
 *
 * The analog sampling circuitry of the Tekdaqc can only sample a single channel at a time and so it needs to multiplex
 * the various analog input channels to the ADC. The Tekdaqc incorporates both an external analog input multiplexer and
 * the ADC's internal input multiplexer, and the methods in this file will ensure that both the external multiplexer and
 * the ADC's internal multiplexer are properly configured.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "AnalogInput_Multiplexer.h"
#include "ADC_StateMachine.h"
#include "ADS1256_Driver.h"
#include "Tekdaqc_Timers.h"
#include "TelnetServer.h"
#include <inttypes.h>

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* The system time at which the multiplexing process will be completed. */
static uint64_t MuxCompleteTime = 0U;

/* The analog input which is currently selected. Used for reverting state when handling cold junction sampling. */
static Analog_Input_t* CurrentInput = NULL;

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @brief Selects the specified external input.
 */
static void SelectExternalInput(ExternalMuxedInput_t input,  bool doMuxDelay);

/**
 * @internal
 * @brief Selects the specified internal input.
 */
static void SelectInternalInput(InternalAnalogInput_t input);

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Selects the specified external input. This will cause the board to temporarily enter the ADC_EXTERNAL_MUXING
 * state (which will also sample the boards temperature sensor). This function is non-blocking but any analog
 * sampling will be held up by the state machine until the muxing process is completed.
 *
 * @param input ExternalMuxedInput_t The external input to select.
 * @param doMuxDelay bool If true, the ADC_MUXING state should be used while waiting for the external multiplexers.
 * @retval none
 */
static void SelectExternalInput(ExternalMuxedInput_t input, bool doMuxDelay) {
	/* Verify that the internal input parameter is set */
	if (input == NULL_CHANNEL ) {
#ifdef INPUT_MULTIPLEXER_DEBUG
		printf("[Analog Input Multiplexer] Attempted to select an input as external when it is not configured to be.\n\r");
#endif
		return;
	}
	/* Make the switch */
	SelectInternalInput(EXTERNAL_ANALOG_IN);
	GPIO_WriteBit(OCAL_CONTROL_GPIO_PORT, OCAL_CONTROL_PIN, EXT_ANALOG_SELECT);

	printf("[Analog Input Multiplexer] Writing %" PRIX16 " to the external multiplexer.\n\r", input);

	GPIO_Write(EXT_ANALOG_IN_MUX_PORT, (input | (GPIO_ReadOutputData(EXT_ANALOG_IN_MUX_PORT) & EXT_ANALOG_IN_BITMASK)));

	if (doMuxDelay == true) {
		/* Wait for the external multiplexing relays to conduct */
		MuxCompleteTime = GetLocalTime() + EXTERNAL_MUX_DELAY;
		/* Change ADC state machine to ADC_MUXING state */
		ADC_External_Muxing();
	}
}

/**
 * Selects the specified internal input. NOTE: A SYNC command should be issued after calling this function to ensure the ADC
 * provides settled data on the next conversion.
 *
 * @param input InternalAnalogInput_t The internal input to select.
 * @retval none
 */
static void SelectInternalInput(InternalAnalogInput_t input) {
	/* Verify that the internal input parameter is set */
	if (input == NULL_CHANNEL ) {
#ifdef INPUT_MULTIPLEXER_DEBUG
		printf("[Analog Input Multiplexer] Attempted to select an input as internal when it is not configured to be.\n\r");
#endif
		return;
	}
	ADS1256_AIN_t pos;
	ADS1256_AIN_t neg;
	/* Make the switch */
	switch (input) {
	case SUPPLY_9V:
		pos = SUPPLY_9V_AINP;
		neg = SUPPLY_9V_AINN;
		break;
	case SUPPLY_5V:
		pos = SUPPLY_5V_AINP;
		neg = SUPPLY_5V_AINN;
		break;
	case SUPPLY_3_3V:
		pos = SUPPLY_3_3V_AINP;
		neg = SUPPLY_3_3V_AINN;
		break;
	case COLD_JUNCTION:
		pos = COLD_JUNCTION_AINP;
		neg = COLD_JUNCTION_AINN;
		break;
	case EXTERNAL_ANALOG_IN:
		pos = EXTERNAL_ANALOG_IN_AINP;
		neg = EXTERNAL_ANALOG_IN_AINN;
		break;
	default:
#ifdef INPUT_MULTIPLEXER_DEBUG
		printf("[Analog Input Multiplexer] The requested internal input is invalid.\n\r");
#endif
		return;
	}
	ADS1256_SetInputChannels(pos, neg);
}

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Checks if the specified analog input is an external input.
 *
 * @param input PhysicalAnalogInput_t The physical analog input to check.
 * @retval bool TRUE if the input is an external input.
 */
bool isExternalInput(PhysicalAnalogInput_t input) {
	return ((input == EXTERNAL_0) || (input == EXTERNAL_1) || (input == EXTERNAL_2) || (input == EXTERNAL_3) || (input == EXTERNAL_4)
			|| (input == EXTERNAL_5) || (input == EXTERNAL_6) || (input == EXTERNAL_7) || (input == EXTERNAL_8) || (input == EXTERNAL_9)
			|| (input == EXTERNAL_10) || (input == EXTERNAL_11) || (input == EXTERNAL_12) || (input == EXTERNAL_13)
			|| (input == EXTERNAL_14) || (input == EXTERNAL_15) || (input == EXTERNAL_16) || (input == EXTERNAL_17)
			|| (input == EXTERNAL_18) || (input == EXTERNAL_19) || (input == EXTERNAL_20) || (input == EXTERNAL_21)
			|| (input == EXTERNAL_22) || (input == EXTERNAL_23) || (input == EXTERNAL_24) || (input == EXTERNAL_25)
			|| (input == EXTERNAL_26) || (input == EXTERNAL_27) || (input == EXTERNAL_28) || (input == EXTERNAL_29)
			|| (input == EXTERNAL_30) || (input == EXTERNAL_31));
}

/**
 * Checks if the specified analog input is an internal input.
 *
 * @param input PhysicalAnalogInput_t The physical analog input to check.
 * @retval bool TRUE if the input is an internal input.
 */
bool isInternalInput(PhysicalAnalogInput_t input) {
	return ((input == IN_SUPPLY_9V) || (input == IN_SUPPLY_5V) || (input == IN_SUPPLY_3_3V) || (input == IN_COLD_JUNCTION));
}

/**
 * Checks if the external multiplexing process has completed.
 *
 * @param none
 * @retval bool TRUE if the external multiplexing process is completed.
 */
bool isExternalMuxingComplete(void) {
	return (GetLocalTime() >= MuxCompleteTime) ? true : false;
}

/**
 * Initializes the analog input multiplexer, configuring the processor's peripherals and setting
 * the multiplexer to its default state.
 *
 * @param none
 * @retval none
 */
void InputMultiplexerInit(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable the GPIO Clock */
	RCC_AHB1PeriphClockCmd(OCAL_CONTROL_GPIO_CLK, ENABLE);

	/* Configure the DEMUX GPIO pins */
	GPIO_InitStructure.GPIO_Pin = OCAL_CONTROL_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; /* At most the multiplexer can handle a few hundred Hz */
	GPIO_Init(OCAL_CONTROL_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(OCAL_CONTROL_GPIO_PORT, OCAL_CONTROL_PIN, OCAL_SELECT);

	/* Enable the DEMUX GPIO Clock */
	RCC_AHB1PeriphClockCmd(EXT_ANALOG_IN_GPIO_CLK, ENABLE);

	/* Configure the DEMUX GPIO pins */
	GPIO_InitStructure.GPIO_Pin = EXT_ANALOG_IN_MUX_PINS;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; /* At most the multiplexer can handle a few hundred Hz */
	GPIO_Init(EXT_ANALOG_IN_MUX_PORT, &GPIO_InitStructure);
	GPIO_Write(EXT_ANALOG_IN_MUX_PORT, (EXTERN_0 | (GPIO_ReadOutputData(EXT_ANALOG_IN_MUX_PORT ) & EXT_ANALOG_IN_BITMASK )));
}

/**
 * Selects the specified analog input, automatically handling any multiplexing and timing needs.
 *
 * @param input Analog_Input_t* The analog input to select.
 * @param doMuxDelay bool If true, the ADC_MUXING state should be used while waiting for the external multiplexers.
 * @retval none
 */
void SelectAnalogInput(Analog_Input_t* input, bool doMuxDelay) {
	if (input != NULL ) {
		if (isExternalInput(input->physicalInput)) {
			SelectExternalInput(input->externalInput, doMuxDelay);
			CurrentInput = input;
		} else if (isInternalInput(input->physicalInput)) {
			SelectInternalInput(input->internalInput);
			CurrentInput = input;
		} else if (input->physicalInput == EXTERNAL_OFFSET_CAL) {
			SelectCalibrationInput();
			CurrentInput = input;
		} else {
			/* ERROR */
			snprintf(TOSTRING_BUFFER, sizeof(TOSTRING_BUFFER),
					"[Analog Input Multiplexer] Attempted to select an input which does not exist: %" PRIu8 ".", input->physicalInput);
			TelnetWriteErrorMessage(TOSTRING_BUFFER);
#ifdef INPUT_MULTIPLEXER_DEBUG
			printf("[Analog Input Multiplexer] Attempted to select an input which does not exist: %" PRIu8 ".\n\r", input->physicalInput);
#endif
		}
	} else {
#ifdef INPUT_MULTIPLEXER_DEBUG
		printf("[Analog Input Multiplexer] Attempted to select a NULL input.\n\r");
#endif
	}
}

/**
 * Selects the specified physical input on the board, automatically handling any multiplexing and timing needs.
 *
 * @param input PhysicalAnalogInput_t The physical input to select.
 * @param doMuxDelay bool If true, the ADC_MUXING state should be used while waiting for the external multiplexers.
 * @retval none
 */
void SelectPhysicalInput(PhysicalAnalogInput_t input, bool doMuxDelay) {
	if (isExternalInput(input)) {
		SelectExternalInput(input, doMuxDelay);
		CurrentInput = NULL;
	} else if (isInternalInput(input)) {
		SelectInternalInput(input);
		CurrentInput = NULL;
	} else if (input == EXTERNAL_OFFSET_CAL) {
		SelectCalibrationInput();
		CurrentInput = NULL;
	} else {
		/* ERROR */
		snprintf(TOSTRING_BUFFER, sizeof(TOSTRING_BUFFER),
				"[Analog Input Multiplexer] Attempted to select a physical input which does not exist.");
		TelnetWriteErrorMessage(TOSTRING_BUFFER);
#ifdef INPUT_MULTIPLEXER_DEBUG
		printf("[Analog Input Multiplexer] Attempted to select an input which does not exist.\n\r");
#endif
	}
}

/**
 * Selects the input which has been designated as the calibration input. This input is expected by the board
 * to be shorted together for performing a system offset calibration.
 *
 * @param none
 * @retval none
 */
void SelectCalibrationInput(void) {
	//SelectExternalInput(EXTERN_0, true);
	SelectInternalInput(EXTERNAL_ANALOG_IN);
	GPIO_WriteBit(OCAL_CONTROL_GPIO_PORT, OCAL_CONTROL_PIN, OCAL_SELECT);
}

/**
 * Selects the input which has been designated as the cold junction input. This input is connected to the boards
 * temperature sensor and is used for calibration and thermocouple compensations.
 *
 * @param none
 * @retval none
 */
void SelectColdJunctionInput(void) {
	SelectInternalInput(COLD_JUNCTION);
}

/**
 * Re-selects the currently selected input, returning the multiplexer to the state it was in prior to calibration
 * or temperature processes.
 *
 * @param none
 * @retval none
 */
void ResetSelectedInput(void) {
	//FIXME: What if selected input was an internal input?
	/*if (CurrentInput != NULL ) {
		if (CurrentInput->physicalInput == IN_COLD_JUNCTION) {
			SelectAnalogInput(CurrentInput, false);
		} else {
			SelectAnalogInput(CurrentInput, true);
		}
	}*/
	SelectInternalInput(EXTERNAL_ANALOG_IN);
}

/**
 * Looks up the corresponding ExternalMuxedInput_t for the specified input number.
 *
 * @param input uint8_t The input number to lookup.
 * @retval ExternalMuxInput_t The lookup value.
 */
ExternalMuxedInput_t GetExternalMuxedInputByNumber(uint8_t input) {
	ExternalMuxedInput_t in = NULL_CHANNEL;
	if (isExternalInput(input)) {
		switch (input) {
		case 0:
			in = EXTERN_0;
			break;
		case 1:
			in = EXTERN_1;
			break;
		case 2:
			in = EXTERN_2;
			break;
		case 3:
			in = EXTERN_3;
			break;
		case 4:
			in = EXTERN_4;
			break;
		case 5:
			in = EXTERN_5;
			break;
		case 6:
			in = EXTERN_6;
			break;
		case 7:
			in = EXTERN_7;
			break;
		case 8:
			in = EXTERN_8;
			break;
		case 9:
			in = EXTERN_9;
			break;
		case 10:
			in = EXTERN_10;
			break;
		case 11:
			in = EXTERN_11;
			break;
		case 12:
			in = EXTERN_12;
			break;
		case 13:
			in = EXTERN_13;
			break;
		case 14:
			in = EXTERN_14;
			break;
		case 15:
			in = EXTERN_15;
			break;
		case 16:
			in = EXTERN_16;
			break;
		case 17:
			in = EXTERN_17;
			break;
		case 18:
			in = EXTERN_18;
			break;
		case 19:
			in = EXTERN_19;
			break;
		case 20:
			in = EXTERN_20;
			break;
		case 21:
			in = EXTERN_21;
			break;
		case 22:
			in = EXTERN_22;
			break;
		case 23:
			in = EXTERN_23;
			break;
		case 24:
			in = EXTERN_24;
			break;
		case 25:
			in = EXTERN_25;
			break;
		case 26:
			in = EXTERN_26;
			break;
		case 27:
			in = EXTERN_27;
			break;
		case 28:
			in = EXTERN_28;
			break;
		case 29:
			in = EXTERN_29;
			break;
		case 30:
			in = EXTERN_30;
			break;
		case 31:
			in = EXTERN_31;
			break;
		default:
			/* Do nothing */
			break;
		}
	}
	return in;
}
