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
 * @file ADC_StateMachine.h
 * @brief Header file for the ADC state machine.
 *
 * Contains public constants, typedefs and function prototypes for the ADC state machine.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef ADC_STATEMACHINE_H_
#define ADC_STATEMACHINE_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Analog_Input.h"
#include "boolean.h"

/** @addtogroup tekdaqc_firmware Tekdaqc Firmware
 * @{
 */

/** @addtogroup adc_statemachine ADC State Machine
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED TYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief ADC state machine state definitions.
 * Defines all the possible states for the ADC state machine to be in.
 */
typedef enum {
	ADC_UNINITIALIZED, /**< The state machine is in an invalid, uninitialized state. */
	ADC_INITIALIZED, /**< The state machine is in a valid, initialized state. */
	ADC_CALIBRATING, /**< The state machine is calibrating itself. */
	ADC_GAIN_CALIBRATING, /**< The state machine is gain calibrating. */
	ADC_IDLE, /**< The state machine is idling, keeping the ADC warm. */
	ADC_CHANNEL_SAMPLING, /**< The state machine is configured for sampling analog inputs. */
	ADC_RESET, /**< The state machine is resetting. It will return to IDLE after reset completes. */
	ADC_EXTERNAL_MUXING /**< The external multiplexer is switching inputs. Sample the cold junction temperature. */
} ADC_State_t;


/**
 * @internal
 * @brief Data structure for keeping track of the current state of the calibration process.
 *
 * The Tekdaqc uses this data structure to keep track of the current state of its calibration process, which
 * includes both offset and gain calibrations.
 */
typedef struct {
	uint8_t gain_index; /**< The index of the current gain setting being calibrated. */
	uint8_t rate_index; /**< The index of the current rate setting being calibrated. */
	uint8_t buffer_index; /**< The index of the current buffer setting being calibrated. */
	bool finished; /**< TRUE if the particular calibration set has completed. Note that this is not the entire process. */
	uint8_t finished_count; /**< The number of calibration sets which have completed. */
} CalibrationState_t;
/*--------------------------------------------------------------------------------------------------------*/
/* INITIALIZATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Create the ADC state machine.
 */
void ADC_Machine_Create(void);

/**
 * @brief Initialize the ADC state machine.
 */
void ADC_Machine_Init(void);

/**
 * @brief Perform a self zero calibration.
 */
void ADC_Calibrate(void);

/**
 * @brief Perform a system gain calibration on the specified input.
 */
void ADC_GainCalibrate(PhysicalAnalogInput_t input);

/*--------------------------------------------------------------------------------------------------------*/
/* SERVICE METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Perform a periodic service of the state machine.
 */
void ADC_Machine_Service(void);

/**
 * @brief Halt any current operations and return the idle state.
 */
void ADC_Machine_Halt(void);

/*--------------------------------------------------------------------------------------------------------*/
/* STATE METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Idle state handler.
 */
void ADC_Machine_Idle(void);

/**
 * @brief Input sampling state handler.
 */
void ADC_Machine_Input_Sample(Analog_Input_t** inputs, uint32_t count, bool singleChannel);

/**
 * @brief Reset state handler.
 */
void ADC_Machine_Reset(void);

/**
 * @brief External multiplexer switch handler.
 */
void ADC_External_Muxing(void);



#ifdef __cplusplus
}
#endif

/**
 * @}
 */

/**
 * @}
 */

#endif /* ADC_STATEMACHINE_H_ */
