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
 * @file DO_StateMachine.c
 * @brief State machine for the digital output system of the board.
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
#include "DO_StateMachine.h"
#include "TLE7232_RelayDriver.h"
#include "TelnetServer.h"
#include "CommandState.h"

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* The current state of the machine. */
static DO_State_t state;

/* The total number of samples to take */
static uint32_t SampleTotal;

/* The current sample number. */
static uint32_t SampleCurrent;

/* List of outputs to sample. */
static Digital_Output_t** samplingOutputs;

/* Length of list. */
static uint8_t numberSamplingOutputs = 0U;

/* The current input to sample */
static uint8_t currentSamplingOutput = 0U;

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/*
 * @brief Returns a human readable string of the output state.
 */
static inline const char* DOMachine_StringFromState(DO_State_t state);

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Retrieve a human readable string for the provided state.
 *
 * @param state DO_State_t The state to retrieve a string for.
 * @retval const char* The human readable string representation.
 */
static inline const char* DOMachine_StringFromState(DO_State_t state) {
	static const char* strings[] = { "DO_UNINITIALIZED", "DO_INITIALIZED", "DO_IDLE",
			"DO_CHANNEL_SAMPLING", "DO_RESET" };
	return strings[state];
}

/*--------------------------------------------------------------------------------------------------------*/
/* INITIALIZATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Creates the DO state machine. Note that no initialization is done here.
 *
 * @param none
 * @retval none
 */
void DO_Machine_Create(void) {
#ifdef DO_STATE_MACHINE_DEBUG
	printf("[DO STATE MACHINE] Creating DO state machine.\n\r");
#endif
	state = DO_UNINITIALIZED;
}

/**
 * Initializes the DO state machine to a valid state and performs all necessary setup of the DO.
 *
 * @param none
 * @retval none
 */
void DO_Machine_Init(void) {
	/* TODO: Can this be merged into DO_Machine_Create()? */
	if ((state != DO_UNINITIALIZED)) {
#ifdef DO_STATE_MACHINE_DEBUG
		printf("[DO STATE MACHINE] Attempted to enter DO_INITIALIZED state from %s\n\r", DOMachine_StringFromState(state));
#endif
		return;
	}
#ifdef DO_STATE_MACHINE_DEBUG
	printf("[DO STATE MACHINE] Moving to state DO_INITIALIZED.\n\r");
#endif
	/* Initialize the digital outputs */
	TLE7232_Init();
	//SetOutputFaultStatusFunction(&SetDigitalOutputFaultStatus);

	/* Initialize the count variables */
	SampleTotal = 0U;
	SampleCurrent = 0U;
	samplingOutputs = NULL;
	numberSamplingOutputs = 0U;

	/* Update the state */
	state = DO_INITIALIZED;
}

/*--------------------------------------------------------------------------------------------------------*/
/* SERVICE METHODS */
/*--------------------------------------------------------------------------------------------------------*/
#if 0
/**
 * Perform a periodic service of the state machine. This will subsequently call the relevant state handler.
 *
 * @param none
 * @retval none
 */
void DO_Machine_Service(void) {
	/* Determine the state */
	switch (state) {
	case DO_UNINITIALIZED:
		DO_Machine_Init();
		break;
	case DO_INITIALIZED:
		DO_Machine_Idle();
		break;
	case DO_IDLE:
		break;
	case DO_CHANNEL_SAMPLING: {
		Digital_Output_t* output = NULL;
		if (SampleCurrent < SampleTotal) {
			if (numberSamplingOutputs == 1) {
				/* TODO: Output channel sampling */
				++SampleCurrent;
			} else {
				for (uint_fast8_t i = 0; i < NUM_DIGITAL_OUTPUTS; ++i) {
					output = samplingOutputs[i];
					if (output != NULL) {
						/* TODO: Output channel sampling */
					}
				}
				++SampleCurrent;
			}
		} else {
#ifdef DO_STATE_MACHINE_DEBUG
			printf("[DO STATE MACHINE] Channel sampling is complete.\n\r");
#endif
			TelnetWriteStatusMessage("DO Channel sampling completed.");
			DO_Machine_Idle();
			CompletedDOSampling();
		}
	}
	break;
	case DO_RESET:
		SampleCurrent = 0U;
		SampleTotal = 0U;
		samplingOutputs = NULL;
		numberSamplingOutputs = 0U;
		DO_Machine_Idle();
		break;
	default:
		/* TODO: Throw an error */
		break;
	}
}

/**
 * Halt the current sampling activity of the DO state machine and return to the idle state. This can be used
 * to interrupt long term or continuous sampling.
 *
 * @param none
 * @retval none
 */
void DO_Machine_Halt(void) {
#ifdef DO_STATE_MACHINE_DEBUG
	printf("[DO STATE MACHINE] Halting DO sampling.\n\r");
#endif
	DO_Machine_Idle();
	CompletedDOSampling();
}
#endif
/*--------------------------------------------------------------------------------------------------------*/
/* STATE METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Enter the idle state. In this state the DO will be made to do nothing.
 *
 * @param none
 * @retval none
 */
void DO_Machine_Idle(void) {
	/* We use explicit checking here in case other state are added later */
	if ((state != DO_INITIALIZED) && (state != DO_CHANNEL_SAMPLING)) {
		/* We can only enter this state from these states */
#ifdef DO_STATE_MACHINE_DEBUG
		printf("[DO STATE MACHINE] Attempted to enter DO_IDLE state from %s\n\r", DOMachine_StringFromState(state));
#endif
		return;
	}
#ifdef DO_STATE_MACHINE_DEBUG
	printf("[DO STATE MACHINE] Moving to state DO_IDLE.\n\r");
#endif
	state = DO_IDLE;
}

#if 0
/**
 * Enter the output sampling state. In this state the DO will be made to sample a set of outputs each the specified number of times.
 *
 * @param outputs Digital_Output_t** Pointer to array of output data structures holding the configurations of the sampling.
 * @param count uint32_t The number of samples of each output to take. 0 results in continuous sampling.
 * @param singleChannel bool True if the sampling is for a single channel only.
 * @retval none
 */
void DO_Machine_Output_Sample(Digital_Output_t** outputs, uint32_t count, bool singleChannel) {
	/* We use explicit checking here in case other state are added later */
	if ((state != DO_IDLE)) {
#ifdef DO_STATE_MACHINE_DEBUG
		printf("[DO STATE MACHINE] Attempted to enter DO_CHANNEL_SAMPLING state from %s\n\r", DOMachine_StringFromState(state));
#endif
		return;
	}

	/* Validate the outputs list */
	if (outputs == NULL) {
#ifdef DO_STATE_MACHINE_DEBUG
		printf("[DO STATE MACHINE] Attempted to enter DO_CHANNEL_SAMPLING state with a NULL digital output list. Ignoring...\n\r");
#endif
		return;
	}
	if (singleChannel == true) {
		/* Validate the output(s) */
		if (outputs[0] == NULL) {
#ifdef DO_STATE_MACHINE_DEBUG
			printf("[DO STATE MACHINE] Attempted to enter DO_CHANNEL_SAMPLING state with a NULL digital output. Ignoring...\n\r");
#endif
			return;
		}
		/* Select output */
		currentSamplingOutput = 0; /* Use i here so we don't loose any work from the search earlier */
		numberSamplingOutputs = 1;
	} else {
		/* Validate the output(s) */
		uint8_t i = 0U;
		for (; i < NUM_DIGITAL_OUTPUTS; ++i) {
			if (outputs[i] != NULL) {
				break;
			}
		}
		if (i >= NUM_DIGITAL_OUTPUTS) {
#ifdef DO_STATE_MACHINE_DEBUG
			printf("[DO STATE MACHINE] Attempted to enter DO_CHANNEL_SAMPLING state with NULL digital outputs. Ignoring...\n\r");
#endif
			return;
		}
		/* Select output */
		currentSamplingOutput = i; /* Use i here so we don't loose any work from the search earlier */
		numberSamplingOutputs = NUM_DIGITAL_OUTPUTS;
	}

	/* Select output */
	samplingOutputs = outputs;
	uint8_t i = 0U;
	Digital_Output_t* output = samplingOutputs[currentSamplingOutput];
	while (output == NULL) { /* Keep searching until we find a non-null output */
		output = samplingOutputs[++i];
		if (i == NUM_DIGITAL_OUTPUTS) {
			/* We have reached the end of a set */
			/* There is nothing to sample */
			return;
		}
	}
	/* Save current time and sample count */
	SampleCurrent = 0U;
	SampleTotal = count;
	state = DO_CHANNEL_SAMPLING;
}
#endif
/**
 * Enter the reset state. In this state the DO will be reset and returned to the idle state.
 *
 * @param none
 * @retval none
 */
void DO_Machine_Reset(void) {
	/* We use explicit checking here in case other state are added later */
	if ((state != DO_IDLE) && (state != DO_INITIALIZED) && (state != DO_CHANNEL_SAMPLING)
			&& (state != DO_RESET)) {
#ifdef DO_STATE_MACHINE_DEBUG
		printf("[DO STATE MACHINE] Attempted to enter DO_RESET state from %s\n\r", DOMachine_StringFromState(state));
#endif
		return;
	}
	state = DO_RESET;
}
