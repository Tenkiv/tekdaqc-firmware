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
 * @file DI_StateMachine.h
 * @brief Header file for the digital input state machine.
 *
 * Contains public constants, typedefs and function prototypes for the digital input state machine.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef DI_STATEMACHINE_H_
#define DI_STATEMACHINE_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Digital_Input.h"
#include "boolean.h"

/** @addtogroup tekdaqc_firmware Tekdaqc Firmware
 * @{
 */

/** @addtogroup di_statemachine Digital Input State Machine
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED TYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Digital input state machine state definitions.
 * Defines all the possible states for the digital input state machine to be in.
 */
typedef enum {
	DI_UNINITIALIZED, /**< The state machine is in an invalid, uninitialized state. */
	DI_INITIALIZED, /**< The state machine is in a valid, initialized state. */
	DI_IDLE, /**< The state machine is idling. */
	DI_CHANNEL_SAMPLING, /**< The state machine is configured for sampling digital inputs. */
	DI_RESET /**< The state machine is resetting. It will return to IDLE after reset completes. */
} DI_State_t;

/*--------------------------------------------------------------------------------------------------------*/
/* INITIALIZATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Create the digital input state machine.
 */
void DI_Machine_Create(void);

/**
 * @brief Initialize the digital input state machine.
 */
void DI_Machine_Init(void);

/*--------------------------------------------------------------------------------------------------------*/
/* SERVICE METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Perform a periodic service of the state machine.
 */
void DI_Machine_Service(void);

/**
 * @brief Halt any current operations and return the idle state.
 */
void DI_Machine_Halt(void);

/*--------------------------------------------------------------------------------------------------------*/
/* STATE METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Idle state handler.
 */
void DI_Machine_Idle(void);

/**
 * @brief Input sampling state handler.
 */
void DI_Machine_Input_Sample(Digital_Input_t** inputs, uint32_t count, bool singleChannel);

/**
 * @brief Reset state handler.
 */
void DI_Machine_Reset(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

/**
 * @}
 */

#endif /* DI_STATEMACHINE_H_ */
