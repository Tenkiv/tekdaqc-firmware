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
 * @file CommandState.c
 * @brief State machine for the command handling system of the board.
 *
 * Implements a state machine used for time slicing tasks on the processor. In this way this file
 * encapsulates a very basic thread emulation. Only commands which require time to process require
 * the use of this state machine.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "CommandState.h"
#include "ADC_StateMachine.h"
#include "DI_StateMachine.h"
#include "DO_StateMachine.h"
#include "TLE7232_RelayDriver.h"

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* The current state of the machine. */
static CommandState_t CurrentState;

/* Is the ADC sampling complete */
static bool ADCSampling = FALSE;

/* Is the DI sampling complete */
static bool DISampling = FALSE;

/* Is the DO sampling complete */
static bool DOSampling = FALSE;

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Initializes the command state machine to a valid state and performs all necessary setup of the command machine.
 *
 * @param none
 * @retval none
 */
void InitCommandStateHandler(void) {
	CurrentState = UNINITIALIZED;

	/* Initialize the ADC state machine */
	ADC_Machine_Create();
	ADC_Machine_Init();
	DI_Machine_Create();
	DI_Machine_Init();
	DO_Machine_Create();
	DO_Machine_Init();
}

/**
 * Perform a periodic service of the state machine. This will subsequently call the relevant state handler.
 *
 * @param none
 * @retval none
 */
void ServiceTasks(void) {
	switch (CurrentState) {
	case UNINITIALIZED:
		ADC_Machine_Service();
		DI_Machine_Service();
		DO_Machine_Service();
		CurrentState = STATE_IDLE;
		break;
	case STATE_IDLE:
		ADC_Machine_Service();
		DI_Machine_Service();
		DO_Machine_Service();
		break;
	case STATE_ANALOG_INPUT_SAMPLE:
		if (ADCSampling == TRUE) {
			/* Service the ADC state machine */
			ADC_Machine_Service();
		} else {
			CurrentState = STATE_IDLE;
		}
		break;
	case STATE_DIGITAL_INPUT_SAMPLE:
		if (DISampling == TRUE) {
			/* Service the DI state machine */
			DI_Machine_Service();
		} else {
			CurrentState = STATE_IDLE;
		}
		break;
	case STATE_DIGITAL_OUTPUT_SAMPLE:
		if (DOSampling == TRUE) {
			/* Service the DO state machine */
			DO_Machine_Service();
		} else {
			CurrentState = STATE_IDLE;
		}
		break;
	case STATE_GENERAL_SAMPLE:
		if ((ADCSampling == TRUE) || (DISampling == TRUE) || (DOSampling == TRUE)) {
			/* Service the ADC state machine */
			ADC_Machine_Service();
			/* Service the DI state machine */
			DI_Machine_Service();
			/* Service the DO state machine */
			DO_Machine_Service();
		} else {
			CurrentState = STATE_IDLE;
		}
		break;
	default:
		/* TODO: Throw error. */
		break;
	}
}

/**
 * Halt the current activity of the command state machine and return to the idle state. This can be used
 * to interrupt long term or continuous tasks.
 *
 * @param none
 * @retval none
 */
void HaltTasks(void) {
	printf("[Command State] Halting all tasks.\n\r");
	switch (CurrentState) {
	case UNINITIALIZED:
		/* There is nothing to halt */
		break;
	case STATE_IDLE:
		/* There is nothing to halt */
		break;
	case STATE_ANALOG_INPUT_SAMPLE:
		printf("[Command State] Halting analog input sampling.\n\r");
		ADC_Machine_Halt();
		CurrentState = STATE_IDLE;
		break;
	case STATE_DIGITAL_INPUT_SAMPLE:
		printf("[Command State] Halting digital input sampling.\n\r");
		DI_Machine_Halt();
		CurrentState = STATE_IDLE;
		break;
	case STATE_DIGITAL_OUTPUT_SAMPLE:
		printf("[Command State] Halting digital output sampling.\n\r");
		DO_Machine_Halt();
		CurrentState = STATE_IDLE;
		break;
	case STATE_GENERAL_SAMPLE:
		printf("[Command State] Halting all sampling.\n\r");
		ADC_Machine_Halt();
		DI_Machine_Halt();
		DO_Machine_Halt();
		CurrentState = STATE_IDLE;
		break;
	default:
		printf("[Command State] Halt tasks called while in an unknown command state.");
		/* TODO: Throw error */
		break;
	}
}

/**
 * Enter the analog input sampling state.
 *
 * @param none
 * @retval none
 */
void CommandStateMoveToAnalogInputSample(void) {
	CurrentState = STATE_ANALOG_INPUT_SAMPLE;
	ADCSampling = TRUE;
}

/**
 * Enter the digital input sampling state.
 *
 * @param none
 * @retval none
 */
void CommandStateMoveToDigitalInputSample(void) {
	CurrentState = STATE_DIGITAL_INPUT_SAMPLE;
	DISampling = TRUE;
}

/**
 * Enter the digital output sampling state.
 *
 * @param none
 * @retval none
 */
void CommandStateMoveToDigitalOutputSample(void) {
	CurrentState = STATE_DIGITAL_OUTPUT_SAMPLE;
	DOSampling = TRUE;
}

/**
 * Enter the general (analog input, digital input and digital output) sampling state.
 *
 * @param none
 * @retval none
 */
void CommandStateMoveToGeneralSample(void) {
	CurrentState = STATE_GENERAL_SAMPLE;
	ADCSampling = TRUE;
	DISampling = TRUE;
	DOSampling = TRUE;
}

/**
 * Signal that analog input sampling process has completed.
 *
 * @param none
 * @retval none
 */
void CompletedADCSampling(void) {
	ADCSampling = FALSE;
}

/**
 * Signal that the digital input sampling process has completed.
 *
 * @param none
 * @retval none
 */
void CompletedDISampling(void) {
	DISampling = FALSE;
}

/**
 * Signal that the digital output sampling process has completed.
 *
 * @param none
 * @retval none
 */
void CompletedDOSampling(void) {
	DOSampling = FALSE;
}

/**
 * Checks if analog input sampling is in progress.
 *
 * @param none
 * @retval bool TRUE if analog input sampling is in progress.
 */
bool isADCSampling(void) {
	return ADCSampling;
}

/**
 * Checks if digital input sampling is in progress.
 *
 * @param none
 * @retval bool TRUE if digital input sampling is in progress.
 */
bool isDISampling(void) {
	return DISampling;
}

/**
 * Checks if digital output sampling is in progress.
 *
 * @param none
 * @retval bool TRUE if digital output sampling is in progress.
 */
bool isDOSampling(void) {
	return DOSampling;
}
