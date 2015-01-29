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
 * @file DO_StateMachine.h
 * @brief Header file for the Tekdaqc digital output state machine.
 *
 * Contains public definitions and data types for the digital output state machine.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef DO_STATEMACHIN_H_
#define DO_STATEMACHIN_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Digital_Output.h"
#include "boolean.h"

/** @addtogroup tekdaqc_firmware Tekdaqc Firmware
 * @{
 */

/** @addtogroup do_statemachine Digital Output State Machine
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED TYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Digital output state machine state definitions.
 * Defines all the possible states for the digital output state machine to be in.
 */
typedef enum {
	DO_UNINITIALIZED, /**< The state machine is in an invalid, uninitialized state. */
	DO_INITIALIZED, /**< The state machine is in a valid, initialized state. */
	DO_IDLE, /**< The state machine is idling. */
	DO_CHANNEL_SAMPLING, /**< The state machine is configured for sampling digital outputs. */
	DO_RESET /**< The state machine is resetting. It will return to IDLE after reset completes. */
} DO_State_t;

/*--------------------------------------------------------------------------------------------------------*/
/* INITIALIZATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Create the digital output state machine.
 */
void DO_Machine_Create(void);

/**
 * @brief Initialize the digital output state machine.
 */
void DO_Machine_Init(void);

/*--------------------------------------------------------------------------------------------------------*/
/* SERVICE METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Perform a periodic service of the state machine.
 */
void DO_Machine_Service(void);

/**
 * @brief Halt any current operations and return the idle state.
 */
void DO_Machine_Halt(void);

/*--------------------------------------------------------------------------------------------------------*/
/* STATE METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Idle state handler.
 */
void DO_Machine_Idle(void);

/**
 * @brief Output sampling state handler.
 */
//void DO_Machine_Output_Sample(Digital_Output_t** outputs, uint32_t count, bool singleChannel);

/**
 * @brief Reset state handler.
 */
void DO_Machine_Reset(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

/**
 * @}
 */

#endif /* DO_STATEMACHIN_H_ */
