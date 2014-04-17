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
 * @file DI_StateMachine.c
 * @brief State machine for the digital input system of the board.
 *
 * Implements a state machine used for time slicing tasks on the processor. In this way this file
 * encapsulates a very basic thread emulation.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "DI_StateMachine.h"
#include "CommandState.h"
#include "TelnetServer.h"

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* The current state of the machine. */
static DI_State_t CurrentState;

/* The total number of samples to take */
static uint32_t SampleTotal;

/* The current sample number. */
static uint32_t SampleCurrent;

/* List of inputs to sample. */
static Digital_Input_t** samplingInputs;

/* Length of list. */
static uint8_t numberSamplingInputs = 0U;

/* The current input to sample */
static uint8_t currentSamplingInput = 0U;

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/*
 * @brief Returns a human readable string of the input state.
 */
static inline const char* DIMachine_StringFromState(DI_State_t state);

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Retrieve a human readable string for the provided state.
 *
 * @param state DI_State_t The state to retrieve a string for.
 * @retval const char* The human readable string representation.
 */
static inline const char* DIMachine_StringFromState(DI_State_t state) {
	static const char* strings[] = { "DI_UNINITIALIZED", "DI_INITIALIZED", "DI_IDLE", "DI_CHANNEL_SAMPLING", "DI_RESET" };
	return strings[state];
}

/*--------------------------------------------------------------------------------------------------------*/
/* INITIALIZATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Creates the DI state machine. Note that no initialization is done here.
 *
 * @param none
 * @retval none
 */
void DI_Machine_Create(void) {
#ifdef DI_STATE_MACHINE_DEBUG
	printf("[DI STATE MACHINE] Creating DI state machine.\n\r");
#endif
	CurrentState = DI_UNINITIALIZED;
}

/**
 * Initializes the DI state machine to a valid state and performs all necessary setup of the DI.
 *
 * @param none
 * @retval none
 */
void DI_Machine_Init(void) {
	/* TODO: Can this be merged into DI_Machine_Create()? */
	if ((CurrentState != DI_UNINITIALIZED)) {
#ifdef DI_STATE_MACHINE_DEBUG
		printf("[DI STATE MACHINE] Attempted to enter DI_INITIALIZED state from %s\n\r", DIMachine_StringFromState(CurrentState));
#endif
		return;
	}
#ifdef DI_STATE_MACHINE_DEBUG
	printf("[DI STATE MACHINE] Moving to state DI_INITIALIZED.\n\r");
#endif

	/* Initialize the count variables */
	SampleTotal = 0U;
	SampleCurrent = 0U;
	samplingInputs = NULL;
	numberSamplingInputs = 0U;

	/* Update the state */
	CurrentState = DI_INITIALIZED;
}

/*--------------------------------------------------------------------------------------------------------*/
/* SERVICE METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Perform a periodic service of the state machine. This will subsequently call the relevant state handler.
 *
 * @param none
 * @retval none
 */
void DI_Machine_Service(void) {
	/* Determine the state */
	switch (CurrentState) {
	case DI_UNINITIALIZED:
		DI_Machine_Init();
		break;
	case DI_INITIALIZED:
		DI_Machine_Idle();
		break;
	case DI_IDLE:
		/* We don't need to do anything, just let it run */
		break;
	case DI_CHANNEL_SAMPLING: {
		Digital_Input_t* input = samplingInputs[currentSamplingInput];
		if ((SampleCurrent < SampleTotal) || (SampleTotal == 0)) {
			if (numberSamplingInputs == 1) {
				SampleDigitalInput(samplingInputs[0]);
				WriteDigitalInput(samplingInputs[0]);
				++SampleCurrent;
			} else {
				for (uint_fast8_t i = 0; i < NUM_DIGITAL_INPUTS; ++i) {
					input = samplingInputs[i];
					if ((input != NULL) & (input->added == CHANNEL_ADDED)) {
						SampleDigitalInput(input);
						WriteDigitalInput(input);
					}
				}
				++SampleCurrent;
			}
		} else {
			/* Sampling has completed. */
#ifdef DI_STATE_MACHINE_DEBUG
			printf("[DI STATE MACHINE] Channel sampling is complete.\n\r");
#endif
			TelnetWriteStatusMessage("DI Channel sampling completed.");
			DI_Machine_Idle();
			CompletedDISampling();
		}
	}
	break;
	case DI_RESET:
		SampleCurrent = 0U;
		SampleTotal = 0U;
		samplingInputs = NULL;
		numberSamplingInputs = 0U;
		DI_Machine_Idle();
		break;
	default:
		/* TODO: Throw an error */
		break;
	}
}

/**
 * Halt the current sampling activity of the DI state machine and return to the idle state. This can be used
 * to interrupt long term or continuous sampling.
 *
 * @param none
 * @retval none
 */
void DI_Machine_Halt(void) {
#ifdef DI_STATE_MACHINE_DEBUG
	printf("[DI STATE MACHINE] Halting DI sampling.\n\r");
#endif
	DI_Machine_Idle();
	CompletedDISampling();
}

/*--------------------------------------------------------------------------------------------------------*/
/* STATE METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Enter the idle state. In this state the DI will be made to do nothing.
 *
 * @param none
 * @retval none
 */
void DI_Machine_Idle(void) {
	/* We use explicit checking here in case other state are added later */
	if ((CurrentState != DI_INITIALIZED) && (CurrentState != DI_CHANNEL_SAMPLING)) {
		/* We can only enter this state from these states */
#ifdef DI_STATE_MACHINE_DEBUG
		printf("[DI STATE MACHINE] Attempted to enter DI_IDLE state from %s\n\r", DIMachine_StringFromState(CurrentState));
#endif
		return;
	}
#ifdef DI_STATE_MACHINE_DEBUG
	printf("[DI STATE MACHINE] Moving to state DI_IDLE.\n\r");
#endif
	CurrentState = DI_IDLE;
}

/**
 * Enter the input sampling state. In this state the DI will be made to sample a set of inputs each the specified number of times.
 *
 * @param inputs Digital_Input_t** Pointer to array of input data structures holding the configurations of the sampling.
 * @param count uint32_t The number of samples of each input to take. 0 results in continuous sampling.
 * @param singleChannel bool True if the sampling is for a single channel only.
 * @retval none
 */
void DI_Machine_Input_Sample(Digital_Input_t** inputs, uint32_t count, bool singleChannel) {
	/* We use explicit checking here in case other state are added later */
	if ((CurrentState != DI_IDLE)) {
#ifdef DI_STATE_MACHINE_DEBUG
		printf("[DI STATE MACHINE] Attempted to enter DI_CHANNEL_SAMPLING state from %s\n\r", DIMachine_StringFromState(CurrentState));
#endif
		return;
	}
	/* Validate the inputs list */
	if (inputs == NULL ) {
#ifdef DI_STATE_MACHINE_DEBUG
		printf("[DI STATE MACHINE] Attempted to enter DI_CHANNEL_SAMPLING state with a NULL digital input list. Ignoring...\n\r");
#endif
		return;
	}

	/* Save sample count */
	SampleCurrent = 0U;
	SampleTotal = count;

	if (singleChannel == true) {
		/* Validate the input(s) */
		if (inputs[0] == NULL ) {
#ifdef DI_STATE_MACHINE_DEBUG
			printf("[DI STATE MACHINE] Attempted to enter DI_CHANNEL_SAMPLING state with a NULL digital input. Ignoring...\n\r");
#endif
			return;
		}
		if (inputs[0]->added == CHANNEL_NOTADDED) {
#ifdef ADC_STATE_MACHINE_DEBUG
			printf("[DI STATE MACHINE] Attempted to enter DI_CHANNEL_SAMPLING state with an un-added digital input. Ignoring...\n\r");
#endif
			return;
		}
		/* Select input */
		currentSamplingInput = 0;
		numberSamplingInputs = 1;
	} else {
		/* Validate the input(s) */
		uint8_t i = 0U;
		for (; i < NUM_DIGITAL_INPUTS; ++i) {
			if (inputs[i] != NULL && inputs[i]->added == CHANNEL_ADDED) {
				break;
			}
		}
		if (i >= NUM_DIGITAL_INPUTS) {
#ifdef DI_STATE_MACHINE_DEBUG
			printf("[DI STATE MACHINE] Attempted to enter DI_CHANNEL_SAMPLING state with NULL digital inputs. Ignoring...\n\r");
#endif
			return;
		}
		/* Select input */
		currentSamplingInput = i; /* Use i here so we don't loose any work from the search earlier */
		numberSamplingInputs = NUM_DIGITAL_INPUTS;
	}

	samplingInputs = inputs;
	Digital_Input_t* input = samplingInputs[currentSamplingInput];

	if (input == NULL) {
		uint8_t i = 0U;
		while (input == NULL ) { /* Keep searching until we find a non-null input */
			input = samplingInputs[++i];
			if (i == NUM_DIGITAL_INPUTS) {
				/* We have reached the end of a set */
				/* There is nothing to sample */
				return;
			}
		}
		currentSamplingInput = i;
	}
	/* Save current time and sample count */
	SampleCurrent = 0U;
	SampleTotal = count;
	CurrentState = DI_CHANNEL_SAMPLING;
}

/**
 * Enter the reset state. In this state the DI will be reset and returned to the idle state.
 *
 * @param none
 * @retval none
 */
void DI_Machine_Reset(void) {
	/* We use explicit checking here in case other state are added later */
	if ((CurrentState != DI_IDLE) && (CurrentState != DI_INITIALIZED) && (CurrentState != DI_CHANNEL_SAMPLING) && (CurrentState != DI_RESET)) {
#ifdef DI_STATE_MACHINE_DEBUG
		printf("[DI STATE MACHINE] Attempted to enter DI_RESET state from %s\n\r", DIMachine_StringFromState(CurrentState));
#endif
		return;
	}
	CurrentState = DI_RESET;
}
