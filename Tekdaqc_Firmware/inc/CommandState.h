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
 * @file CommandState.h
 * @brief Header file for the command state machine.
 *
 * Contains public definitions and data types for the command state machine.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef COMMANDSTATE_H_
#define COMMANDSTATE_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "boolean.h"

/** @addtogroup tekdaqc_firmware Tekdaqc Firmware
 * @{
 */

/** @addtogroup command_state Command State Machine
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED TYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Command state machine state definitions.
 * Defines all the possible states for the command state machine to be in.
 */
typedef enum {
	UNINITIALIZED, /**< The state machine is in an invalid, uninitialized state. */
	STATE_IDLE, /**< The state machine is idling. */
	STATE_ANALOG_INPUT_SAMPLE, /**< The state machine is configured for sampling analog inputs only. */
	STATE_DIGITAL_INPUT_SAMPLE, /**< The state machine is configured for sampling digital inputs only. */
	STATE_DIGITAL_OUTPUT_SAMPLE, /**< The state machine is configured for sampling digital outputs only. */
	STATE_GENERAL_SAMPLE /**< The state machine is configured for sampling everything. */
} CommandState_t;

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Initialize the state machine.
 */
void InitCommandStateHandler(void);

/**
 * @brief Service any tasks which need to be serviced based on the current state.
 */
void ServiceTasks(void);

/**
 * @brief Halt all in progress tasks, returning to the idle state.
 */
void HaltTasks(void);

/**
 * @brief Move the state machine to analog input sampling.
 */
void CommandStateMoveToAnalogInputSample(void);

/**
 * @brief Move the state machine to digital input sampling.
 */
void CommandStateMoveToDigitalInputSample(void);

/**
 * @brief Move the state machine to digital output sampling.
 */
void CommandStateMoveToDigitalOutputSample(void);

/**
 * @brief Move the state machine to general sampling.
 */
void CommandStateMoveToGeneralSample(void);

/**
 * @brief Notifies the command state machine that the analog input sampling process has completed.
 */
void CompletedADCSampling(void);

/**
 * @brief Notifies the command state machine that the digital input sampling process has completed.
 */
void CompletedDISampling(void);

/**
 * @brief Notifies the command state machine that the digital output sampling process has completed.
 */
void CompletedDOSampling(void);

/**
 * @brief Accessor function to check the analog input sampling status.
 */
bool isADCSampling(void);

/**
 * @brief Accessor function to check the digital input sampling status.
 */
bool isDISampling(void);

/**
 * @brief Accessor function to check the digital output sampling status.
 */
bool isDOSampling(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

/**
 * @}
 */

#endif /* COMMANDSTATE_H_ */
