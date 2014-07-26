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
 * @file ADC_StateMachine.c
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
#include "ADC_StateMachine.h"
#include "AnalogInput_Multiplexer.h"
#include "CommandState.h"
#include "BoardTemperature.h"
#include "Tekdaqc_Calibration.h"
#include "Tekdaqc_CalibrationTable.h"
#include "ADS1256_Driver.h"
#include "Tekdaqc_Config.h"
#include "Tekdaqc_Timers.h"
#include "TelnetServer.h"
#include "boolean.h"
#include <inttypes.h>
#include <stdio.h>

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE TYPE DEFINITIONS */
/*--------------------------------------------------------------------------------------------------------*/

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
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* The current state of the machine. */
static ADC_State_t CurrentState;

/* The previous state of the adc machine. Used for some intermediate states */
static ADC_State_t PreviousState;

/* The total number of samples to take */
static volatile uint32_t SampleTotal;

/* The current sample number. */
static volatile uint32_t SampleCurrent;

/* List of inputs to sample. */
static Analog_Input_t** samplingInputs;

/* Length of list. */
static uint8_t numberSamplingInputs = 0U;

/* The current input to sample */
static uint8_t currentSamplingInput = 0U;

/* Used to indicate if we are waiting on a temperature sample */
static bool waitingOnTemp = false;

static uint8_t scratch_bytes[3];

static CalibrationState_t calibrationState;

static bool isFirstIdle = true;

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @brief Returns a human readable string of the input state.
 */
static inline const char* ADCMachine_StringFromState(ADC_State_t machine_state);

/**
 * @internal
 * @brief Begins a new conversion for the specified input.
 */
static void BeginNextConversion(Analog_Input_t* input);

/**
 * @internal
 * @brief Performs the necessary calibration steps for the ADC.
 */
static void ADC_Machine_Service_Calibrating(void);

/**
 * @internal
 * @brief Performs the necessary system gain calibration steps for the ADC.
 */
static void ADC_Machine_Service_GainCalibrating(void);

/**
 * @internal
 * @brief Performs the necessary service functions when the ADC is idling.
 */
static void ADC_Machine_Service_Idle(void);

/**
 * @internal
 * @brief Performs the necessary service functions when the ADC is sampling.
 */
static void ADC_Machine_Service_Sampling(void);

/**
 * @internal
 * @brief Performs the necessary service functions when the ADC is switching external channels.
 */
static void ADC_Machine_Service_Muxing(void);

/**
 * @internal
 * @brief Performs a conversion from a 32 bit word to a 3 byte array.
 */
static void ConvertCalibrationToBytes(uint8_t bytes[], uint32_t cal);

/**
 * @internal
 * @brief Applies any calibration parameters to the ADC.
 */
static void ApplyCalibrationParameters(Analog_Input_t* input);

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Retrieve a human readable string for the provided state.
 *
 * @param state ADC_State_t The state to retrieve a string for.
 * @retval const char* The human readable string representation.
 */
static inline const char* ADCMachine_StringFromState(ADC_State_t machine_state) {
	static const char* strings[] = { "ADC_UNINITIALIZED", "ADC_INITIALIZED", "ADC_CALIBRATING", "ADC_GAIN_CALIBRATING", "ADC_IDLE",
			"ADC_CHANNEL_SAMPLING", "ADC_RESET", "ADC_EXTERNAL_MUXING" };
	return strings[machine_state];
}

/**
 * Begins a new conversion for the provided input. This method is primarily used for multi input conversions.
 *
 * @param input Analog_Input to begin conversion for. It is assumed the switch has been made.
 */
static void BeginNextConversion(Analog_Input_t* input) {
	/* Set sampling parameters */
	ADS1256_SetDataRate(input->rate);
	ADS1256_SetPGASetting(input->gain);
	ADS1256_SetInputBufferSetting(input->buffer);
	ApplyCalibrationParameters(input);
	ADS1256_Wakeup();
	input->timestamps[input->bufferWriteIdx] = GetLocalTime();
}

/**
 * Performs the necessary functions to calibrate the ADC and populate its calibration tables.
 *
 * @param none
 * @retval none
 */
static void ADC_Machine_Service_Calibrating(void) {
	static ADS1256_PGA_t gains[] = { ADS1256_PGAx1, ADS1256_PGAx2, ADS1256_PGAx4, ADS1256_PGAx8, ADS1256_PGAx16, ADS1256_PGAx32,
			ADS1256_PGAx64 };
	static ADS1256_SPS_t rates[] = { ADS1256_SPS_30000, ADS1256_SPS_15000, ADS1256_SPS_7500, ADS1256_SPS_3750, ADS1256_SPS_2000,
			ADS1256_SPS_1000, ADS1256_SPS_500, ADS1256_SPS_100, ADS1256_SPS_60, ADS1256_SPS_50, ADS1256_SPS_30, ADS1256_SPS_25,
			ADS1256_SPS_15, ADS1256_SPS_10, ADS1256_SPS_5, ADS1256_SPS_2_5 };
	static ADS1256_BUFFER_t buffers[] = { ADS1256_BUFFER_ENABLED, ADS1256_BUFFER_DISABLED };

	if (calibrationState.finished == false) {
		if (calibrationState.gain_index < NUM_PGA_SETTINGS) {
			if (calibrationState.rate_index < NUM_SAMPLE_RATES) {
				if (calibrationState.buffer_index < NUM_BUFFER_SETTINGS) {
					ADS1256_SetInputBufferSetting(buffers[calibrationState.buffer_index]);
					ADS1256_SetDataRate(rates[calibrationState.rate_index]);
					ADS1256_SetPGASetting(gains[calibrationState.gain_index]);
					ADS1256_CalibrateSelf();
					Tekdaqc_SetBaseGainCalibration(ADS1256_GetGainCalSetting(), rates[calibrationState.rate_index],
							gains[calibrationState.gain_index], buffers[calibrationState.buffer_index]);
					ADS1256_CalibrateSystem_Offset();
					Tekdaqc_SetOffsetCalibration(ADS1256_GetOffsetCalSetting(), rates[calibrationState.rate_index],
							gains[calibrationState.gain_index], buffers[calibrationState.buffer_index++]);
					calibrationState.finished_count++;
				} else {
					calibrationState.buffer_index = 0;
					calibrationState.rate_index++;
				}
			} else {
				calibrationState.rate_index = 0;
				calibrationState.gain_index++;
			}
		} else {
			calibrationState.finished = true;
		}
#ifdef ADC_STATE_MACHINE_DEBUG
		static uint32_t total = NUM_PGA_SETTINGS * NUM_SAMPLE_RATES * NUM_BUFFER_SETTINGS;
		snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "[ADC STATE MACHINE] Calibration progress: %f\%.\n\r",
				((float) calibrationState.finished_count / total) * 100.0f);
		printf("%s", TOSTRING_BUFFER);
		TelnetWriteStatusMessage(TOSTRING_BUFFER);
#endif
	} else {
		/* The calibration is complete */
		printf("[ADC STATE MACHINE] Calibration Completed.\n\r");
		TelnetWriteStatusMessage("[ADC STATE MACHINE] Calibration Completed.\n\r");
		ADC_Machine_Idle();
	}
}

/**
 * Performs the necessary functions to system gain calibrate the ADC and populate its calibration tables.
 *
 * @param none
 * @retval none
 */
static void ADC_Machine_Service_GainCalibrating(void) {
	uint32_t calibration = ADS1256_GetGainCalSetting();
	ADS1256_SPS_t rate = ADS1256_GetDataRate();
	ADS1256_PGA_t gain = ADS1256_GetPGASetting();
	ADS1256_BUFFER_t buffer = ADS1256_GetInputBufferSetting();
	Tekdaqc_SetBaseGainCalibration(calibration, rate, gain, buffer);
}

/**
 * Performs all the necessary functions to keep the ADC warm when it is in an idle state.
 *
 * @param none
 * @retval none
 */
static void ADC_Machine_Service_Idle(void) {
	/* If a temperature reading is ready, update the board temperature */
	if (ADS1256_IsDataReady(false)) {
		Analog_Input_t* input = GetAnalogInputByNumber(IN_COLD_JUNCTION);
		ADS1256_Sync(true);
		waitingOnTemp = false;
		/* We need to read it */
		input->values[input->bufferWriteIdx] = ADS1256_GetMeasurement();
		/* Update temperature */
		updateBoardTemperature(input, input->values[input->bufferWriteIdx]);
		++(input->bufferWriteIdx);
		input->timestamps[input->bufferWriteIdx] = GetLocalTime();
		ADS1256_Wakeup();
#ifdef BOARD_TEMPERATURE_DEBUG
		printf("[ADC STATE MACHINE] Cold junction temperature sample is complete.\n\r");
#endif
		/* Check the buffer positions for errors/roll over */
		if (input->bufferWriteIdx == input->bufferReadIdx) {
			/* Check the buffer positions for errors/roll over */
#ifdef BOARD_TEMPERATURE_DEBUG
			printf("[ADC STATE MACHINE] Cold junction sampling overwrote data before it could be read.\n\r");
			TelnetWriteErrorMessage("[ADC STATE MACHINE] Cold junction sampling overwrote data before it could be read.\n\r");
#endif
			/* Buffer is full, overwrite */
			input->bufferReadIdx = (input->bufferReadIdx + 1) % ANALOG_INPUT_BUFFER_SIZE;
		}
		if (((input->bufferWriteIdx - 1U) == input->bufferReadIdx) && ((input->bufferWriteIdx - 1U) > 0U)) {
			/* This means we wrote over some data */
		}
		if (input->bufferWriteIdx == ANALOG_INPUT_BUFFER_SIZE) {
			/* We have reached the end of the buffer and need to start writing to the beginning */
			input->bufferWriteIdx = 0U;
		}
	}
}

/**
 * Performs all necessary functions to keep the ADC sampling, either single channel or multi channel, for the specified number of samples.
 *
 * @param none
 * @retval none
 */
static void ADC_Machine_Service_Sampling(void) {
	/* Check if data is ready */
	if (ADS1256_IsDataReady(false)) {
		/* We need to read it */
		printf("Reading ADC Sample.\n\r");
		ADS1256_PrintRegs();
		uint8_t writeIndex = samplingInputs[currentSamplingInput]->bufferWriteIdx;
		samplingInputs[currentSamplingInput]->values[writeIndex] = ADS1256_GetMeasurement();
		samplingInputs[currentSamplingInput]->bufferWriteIdx = (writeIndex + 1) % ANALOG_INPUT_BUFFER_SIZE;
		if (samplingInputs[currentSamplingInput]->bufferWriteIdx == samplingInputs[currentSamplingInput]->bufferReadIdx) {
			/* Check the buffer positions for errors/roll over */
#ifdef ADC_STATE_MACHINE_DEBUG
			printf("[ADC STATE MACHINE] Analog sampling overwrote data before it could be read.\n\r");
#endif
			TelnetWriteErrorMessage("Analog sampling overwrote data before it could be read.");
			/* Buffer is full, overwrite */
			samplingInputs[currentSamplingInput]->bufferReadIdx = (samplingInputs[currentSamplingInput]->bufferReadIdx + 1)
					% ANALOG_INPUT_BUFFER_SIZE;
		}

		ADS1256_Sync(true); /*Halt conversion */
		/* Select the next input */

		if (numberSamplingInputs > 1) {
			Analog_Input_t* input = NULL;
			Analog_Input_t* current = samplingInputs[currentSamplingInput];
			while (input == NULL || input->added == CHANNEL_NOTADDED) { /* Keep searching until we find a non-null input */
				++currentSamplingInput;
				if (currentSamplingInput == NUM_ANALOG_INPUTS) {
					/* We have reached the end of a set */
					currentSamplingInput = 0U; /* Reset to the start */
#ifdef ADC_STATE_MACHINE_DEBUG
					printf("[ADC STATE MACHINE] Sample %" PRIi32 " of %" PRIi32 " is complete.\n\r", SampleCurrent + 1, SampleTotal);
#endif
					++SampleCurrent; /* Increment the sample counter */
				}
				input = samplingInputs[currentSamplingInput];
			}
			if (input != current) { /* Prevent unnecessary external muxings */
				SelectAnalogInput(input, true); /* Select the input */
				/* We are done sampling this input, write out any remaining data */
				WriteAnalogInput(current);
			} else {
#ifdef ADC_STATE_MACHINE_DEBUG
				printf("[ADC STATE MACHINE] Multi-channel sampling tried to select the currently selected input. Ignoring...\n\r");
#endif
				ADS1256_Wakeup(); /* Begin the next sample */
				/* Save the real time clock entry for the sample */
				samplingInputs[currentSamplingInput]->timestamps[samplingInputs[currentSamplingInput]->bufferWriteIdx] = GetLocalTime();
			}
		} else {
			/* We are single channel sampling */
#ifdef ADC_STATE_MACHINE_DEBUG
			snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "[ADC STATE MACHINE] Sample %" PRIi32 " of %" PRIi32 " is complete.\n\r",
					SampleCurrent + 1, SampleTotal);
			TelnetWriteStatusMessage(TOSTRING_BUFFER);
#endif
			++SampleCurrent; /* Increment the sample count */
			ADS1256_Wakeup(); /* Begin the next sample */
			/* Save the real time clock entry for the sample */
			samplingInputs[currentSamplingInput]->timestamps[samplingInputs[currentSamplingInput]->bufferWriteIdx] = GetLocalTime();
		}
		if ((SampleCurrent == SampleTotal) && !((numberSamplingInputs > 1) && SampleCurrent == 0)) {
			/* The equality check is because we incremented already */
			/* We are done sampling, write out any remaining data and return to idle state */
			WriteAnalogInput(samplingInputs[currentSamplingInput]);
			ADC_Machine_Idle();
#ifdef ADC_STATE_MACHINE_DEBUG
			printf("[ADC STATE MACHINE] Channel sampling is complete.\n\r");
#endif
			TelnetWriteStatusMessage("ADC Channel sampling completed.");
			/* We are done, return */
			CompletedADCSampling();
			return;
		}
	} else {
		/* There is nothing to read yet, write some data */
		/* TODO: This needs to handle other inputs most likely */
		WriteAnalogInput(samplingInputs[currentSamplingInput]);
	}
}

/**
 * Performs the necessary functions when the machine is switching external channels. This includes taking a board
 * temperature reading while also making sure sufficient time has passed for the muxing process to complete.
 *
 * @param none
 * @retval none
 */
static void ADC_Machine_Service_Muxing(void) {
	Analog_Input_t* input = GetAnalogInputByNumber(IN_COLD_JUNCTION);
	if (waitingOnTemp == false) {
		/* We need to begin a temperature sample */
		ADS1256_Sync(true);
		SelectColdJunctionInput();
		Analog_Input_t* cold = GetAnalogInputByNumber(IN_COLD_JUNCTION);
		BeginNextConversion(cold);
		waitingOnTemp = true;
	} else {
		/* We are waiting for a temperature sample to complete */
		if (ADS1256_IsDataReady(false)) {
			/* We need to read it */
			uint8_t writeIndex = input->bufferWriteIdx;
			input->values[writeIndex] = ADS1256_GetMeasurement();
			/* Update temperature */
			updateBoardTemperature(input, input->values[writeIndex]);
			input->bufferWriteIdx = (writeIndex + 1) % ANALOG_INPUT_BUFFER_SIZE;
			if (input->bufferWriteIdx == input->bufferReadIdx) {
				/* Check the buffer positions for errors/roll over */
				/*#ifdef ADC_STATE_MACHINE_DEBUG
				 printf("[ADC STATE MACHINE] Analog sampling overwrote data before it could be read.\n\r");
				 #endif
				 TelnetWriteErrorMessage("Analog sampling overwrote data before it could be read.");*/
				/* Buffer is full, overwrite */
				input->bufferReadIdx = (input->bufferReadIdx + 1) % ANALOG_INPUT_BUFFER_SIZE;
			}

#ifdef BOARD_TEMPERATURE_DEBUG
			printf("[ADC STATE MACHINE] Cold junction temperature sample is complete.\n\r");
#endif
			ADS1256_Sync(false); /*Halt conversion */
		}
		if ((PreviousState == ADC_CHANNEL_SAMPLING) && (isExternalMuxingComplete() == true)) {
			/* We need to select external inputs on the internal multiplexer */
			ResetSelectedInput();
			/* We need to begin the next conversion */
			BeginNextConversion(samplingInputs[currentSamplingInput]);
			CurrentState = PreviousState; /* Return the state to its previous value */
		} else if (((PreviousState == ADC_IDLE) || (PreviousState == ADC_CALIBRATING) || (PreviousState == ADC_GAIN_CALIBRATING))
				&& (isExternalMuxingComplete() == true)) {
			ResetSelectedInput();
			CurrentState = PreviousState;
		}
	}
}

static void ConvertCalibrationToBytes(uint8_t bytes[], uint32_t cal) {
	bytes[0] = cal & 0xFF;
	bytes[1] = (cal & 0xFF00) >> 8;
	bytes[2] = (cal & 0xFF0000) >> 16;
}

static void ApplyCalibrationParameters(Analog_Input_t* input) {
	uint32_t offset_cal;
	uint32_t gain_cal;
	printf("Applying calibration parameters for input: %s\n\r", input->name);
	if (input->physicalInput == IN_COLD_JUNCTION) {
		offset_cal = Tekdaqc_GetColdJunctionOffsetCalibration();
		gain_cal = Tekdaqc_GetColdJunctionGainCalibration();
	} else {
		offset_cal = Tekdaqc_GetOffsetCalibration(input->rate, input->gain, input->buffer);
		gain_cal = Tekdaqc_GetGainCalibration(input->rate, input->gain, input->buffer, getBoardTemperature());
	}
	printf("Calibration params: Offset: 0x%" PRIx32 " Gain: 0x%" PRIx32 "\n\r", offset_cal, gain_cal);
	ConvertCalibrationToBytes(scratch_bytes, offset_cal);
	ADS1256_SetOffsetCalSetting(scratch_bytes);
	ConvertCalibrationToBytes(scratch_bytes, gain_cal);
	ADS1256_SetGainCalSetting(scratch_bytes);
}

/*--------------------------------------------------------------------------------------------------------*/
/* INITIALIZATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Creates the ADC state machine. Note that no initialization is done here.
 *
 * @param none
 * @retval none
 */
void ADC_Machine_Create(void) {
#ifdef ADC_STATE_MACHINE_DEBUG
	printf("[ADC STATE MACHINE] Creating ADC state machine.\n\r");
#endif
	CurrentState = ADC_UNINITIALIZED;
}

/**
 * Initializes the ADC state machine to a valid state and performs all necessary setup of the ADC.
 *
 * @param none
 * @retval none
 */
void ADC_Machine_Init(void) {
	/* TODO: Can this be merged into ADC_Machine_Create()? */
	if ((CurrentState != ADC_UNINITIALIZED)) {
#ifdef ADC_STATE_MACHINE_DEBUG
		printf("[ADC STATE MACHINE] Attempted to enter ADC_INITIALIZED state from %s\n\r", ADCMachine_StringFromState(CurrentState));
#endif
	} else {
#ifdef ADC_STATE_MACHINE_DEBUG
		printf("[ADC STATE MACHINE] Moving to state ADC_INITIALIZED.\n\r");
#endif
		/* Initialize the ADS1256 */
		ADS1256_Init();

		/* Initialize the count variables */
		SampleTotal = 0U;
		SampleCurrent = 0U;
		samplingInputs = NULL;
		numberSamplingInputs = 0U;

		/* Initialize the calibration state */
		calibrationState.buffer_index = 0U;
		calibrationState.gain_index = 0U;
		calibrationState.rate_index = 0U;
		calibrationState.finished = false;

		/* Update the state */
		CurrentState = ADC_INITIALIZED;
	}
}

/**
 * Calibrates the ADC, calculating the correct values to make the calibration table valid. This can not be performed
 * while sampling is active (the state machine must be in the state ADC_IDLE).
 *
 * @param none
 * @retval none
 */
void ADC_Calibrate(void) {
	if ((CurrentState != ADC_IDLE)) {
#ifdef ADC_STATE_MACHINE_DEBUG
		printf("[ADC STATE MACHINE] Attempted to enter ADC_CALIBRATING state from %s\n\r", ADCMachine_StringFromState(CurrentState));
#endif
	} else {
#ifdef ADC_STATE_MACHINE_DEBUG
		printf("[ADC STATE MACHINE] Moving to state ADC_CALIBRATING.\n\r");
#endif
		/* Update the state */
		CurrentState = ADC_CALIBRATING;

		/* Update the finished state */
		calibrationState.finished = false;
		calibrationState.finished_count = 0U;
		calibrationState.buffer_index = 0U;
		calibrationState.rate_index = 0U;
		calibrationState.gain_index = 0U;

		/* Select the calibration input */
		SelectCalibrationInput();
		Delay_ms(EXTERNAL_MUX_DELAY);
	}
}

/**
 * Performs a system gain calibration on the specified physical input. This process can not be performed while sampling
 * is active (the state machine must be in the state ADC_IDLE). Additionally, the parameters for the calibration must have
 * already been set.
 *
 * @param input PhysicalAnalogInput_t The physical analog input to switch to for calibration.
 * @retval none
 */
void ADC_GainCalibrate(PhysicalAnalogInput_t input) {
	if ((CurrentState != ADC_IDLE)) {
#ifdef ADC_STATE_MACHINE_DEBUG
		printf("[ADC STATE MACHINE] Attempted to enter ADC_CALIBRATING state from %s\n\r", ADCMachine_StringFromState(CurrentState));
#endif
	} else {
#ifdef ADC_STATE_MACHINE_DEBUG
		printf("[ADC STATE MACHINE] Moving to state ADC_CALIBRATING.\n\r");
#endif
		/* Update the state */
		CurrentState = ADC_GAIN_CALIBRATING;

		/* Update the finished state */
		calibrationState.finished = false;
		calibrationState.finished_count = 0U;

		/* Select the calibration input */
		SelectPhysicalInput(input, true);
	}
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
void ADC_Machine_Service(void) {
	/* Determine the state */
	switch (CurrentState) {
	case ADC_UNINITIALIZED:
		ADC_Machine_Init();
		break;
	case ADC_INITIALIZED:
		ADC_Machine_Idle();
		break;
	case ADC_CALIBRATING:
		ADC_Machine_Service_Calibrating();
		break;
	case ADC_GAIN_CALIBRATING:
		ADC_Machine_Service_GainCalibrating();
		break;
	case ADC_IDLE:
		if (isFirstIdle == true) {
			isFirstIdle = false;
			PerformSystemCalibration(); /* Load the offset calibration table */
		} else {
			ADC_Machine_Service_Idle();
		}
		break;
	case ADC_CHANNEL_SAMPLING:
		ADC_Machine_Service_Sampling();
		break;
	case ADC_RESET:
		ADS1256_Full_Reset();
		SampleCurrent = 0U;
		SampleTotal = 0U;
		samplingInputs = NULL;
		numberSamplingInputs = 0U;
		currentSamplingInput = NULL_CHANNEL;
		ADC_Machine_Idle();
		break;
	case ADC_EXTERNAL_MUXING:
		ADC_Machine_Service_Muxing();
		break;
	default:
		/* TODO: Throw an error */
		break;
	}
}

/**
 * Halt the current sampling activity of the ADC state machine and return to the idle state. This can be used
 * to interrupt long term or continuous sampling.
 *
 * @param none
 * @retval none
 */
void ADC_Machine_Halt(void) {
#ifdef ADC_STATE_MACHINE_DEBUG
	printf("[ADC STATE MACHINE] Halting ADC sampling.\n\r");
#endif
	ADC_Machine_Idle();
	CompletedADCSampling();
}

/*--------------------------------------------------------------------------------------------------------*/
/* STATE METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Enter the idle state. In this state the ADC will be made to sample arbitrarily to keep the input path warmed up.
 *
 * @param none
 * @retval none
 */
void ADC_Machine_Idle(void) {
	/* We use explicit checking here in case other state are added later */
	if ((CurrentState != ADC_INITIALIZED) && (CurrentState != ADC_CHANNEL_SAMPLING) && (CurrentState != ADC_EXTERNAL_MUXING)
			&& (CurrentState != ADC_CALIBRATING) && (CurrentState != ADC_GAIN_CALIBRATING)) {
		/* We can only enter this state from these states */
#ifdef ADC_STATE_MACHINE_DEBUG
		printf("[ADC STATE MACHINE] Attempted to enter ADC_IDLE state from %s\n\r", ADCMachine_StringFromState(CurrentState));
#endif
	} else {
#ifdef ADC_STATE_MACHINE_DEBUG
		printf("[ADC STATE MACHINE] Moving to state ADC_IDLE.\n\r");
#endif
		CurrentState = ADC_IDLE;
		ADS1256_Sync(true);
		Analog_Input_t* cold = GetAnalogInputByNumber(IN_COLD_JUNCTION);
		SelectAnalogInput(cold, false);
		BeginNextConversion(cold);
	}
}

/**
 * Enter the input sampling state. In this state the ADC will be made to sample a set of inputs each the specified number of times.
 *
 * @param inputs Analog_Input_t** Pointer to array of input data structures holding the configurations of the sampling.
 * @param count uint32_t The number of samples of each input to take. 0 results in continuous sampling.
 * @param singleChannel bool True if the sampling is for a single channel only.
 * @retval none
 */
void ADC_Machine_Input_Sample(Analog_Input_t** inputs, uint32_t count, bool singleChannel) {
	/* We use explicit checking here in case other state are added later */
	if ((CurrentState != ADC_IDLE)) {
#ifdef ADC_STATE_MACHINE_DEBUG
		printf("[ADC STATE MACHINE] Attempted to enter ADC_CHANNEL_SAMPLING state from %s\n\r", ADCMachine_StringFromState(CurrentState));
#endif
		return;
	}
	/* Validate the inputs list */
	if (inputs == NULL) {
#ifdef ADC_STATE_MACHINE_DEBUG
		printf("[ADC STATE MACHINE] Attempted to enter ADC_CHANNEL_SAMPLING state with a NULL analog input list. Ignoring...\n\r");
#endif
		return;
	}

	/* Save sample count */
	SampleCurrent = 0U;
	SampleTotal = count;

	if (singleChannel == true) {
		/* Validate the input(s) */
		if (inputs[0] == NULL) {
#ifdef ADC_STATE_MACHINE_DEBUG
			printf("[ADC STATE MACHINE] Attempted to enter ADC_CHANNEL_SAMPLING state with a NULL analog input. Ignoring...\n\r");
#endif
			return;
		}
		if (inputs[0]->added == CHANNEL_NOTADDED) {
#ifdef ADC_STATE_MACHINE_DEBUG
			printf("[ADC STATE MACHINE] Attempted to enter ADC_CHANNEL_SAMPLING state with an un-added analog input. Ignoring...\n\r");
#endif
			return;
		}
		/* Select input */
		currentSamplingInput = 0;
		numberSamplingInputs = 1;
	} else {
		/* Validate the input(s) */
		uint8_t i = 0U;
		for (; i < NUM_ANALOG_INPUTS; ++i) {
			if (inputs[i] != NULL && inputs[i]->added == CHANNEL_ADDED) {
				break;
			}
		}
		if (i >= NUM_ANALOG_INPUTS) {
#ifdef ADC_STATE_MACHINE_DEBUG
			printf("[ADC STATE MACHINE] Attempted to enter ADC_CHANNEL_SAMPLING state with a NULL analog input. Ignoring...\n\r");
#endif
			return;
		}
		/* Select input */
		currentSamplingInput = i; /* Use i here so we don't loose any work from the search earlier */
		numberSamplingInputs = NUM_ANALOG_INPUTS;
	}

	samplingInputs = inputs;
	Analog_Input_t* input = samplingInputs[currentSamplingInput];
	CurrentState = ADC_CHANNEL_SAMPLING;
	SelectAnalogInput(input, true);

	if (CurrentState == ADC_CHANNEL_SAMPLING) {
		/* Set sampling parameters */
		ADS1256_SetDataRate(input->rate);
		ADS1256_SetPGASetting(input->gain);
		ADS1256_SetInputBufferSetting(input->buffer);
		ApplyCalibrationParameters(input);
		/* Begin sampling */
		ADS1256_Sync(false);
		ADS1256_Wakeup(); /* Start Sampling */
		/* Save the real time clock entry for the sample */
		input->timestamps[input->bufferWriteIdx] = GetLocalTime();
	} else {
		/* We entered the ADC_MUXING state */
		ADS1256_Sync(false); /* TODO: Is this necessary? It may cause temperature fluctuations */
	}
}

/**
 * Enter the reset state. In this state the ADC will be reset and returned to the idle state.
 *
 * @param none
 * @retval none
 */
void ADC_Machine_Reset(void) {
	/* We use explicit checking here in case other state are added later */
	if ((CurrentState != ADC_IDLE) && (CurrentState != ADC_INITIALIZED) && (CurrentState != ADC_CHANNEL_SAMPLING)
			&& (CurrentState != ADC_RESET)) {
#ifdef ADC_STATE_MACHINE_DEBUG
		printf("[ADC STATE MACHINE] Attempted to enter ADC_RESET state from %s\n\r", ADCMachine_StringFromState(CurrentState));
#endif
	} else {
		CurrentState = ADC_RESET;
	}
}

/**
 * External multiplexer switch handler.
 *
 * @param none
 * @retval none
 */
void ADC_External_Muxing(void) {
	/* We use explicit checking here in case other state are added later */
	if ((CurrentState != ADC_RESET) && (CurrentState != ADC_IDLE) && (CurrentState != ADC_CHANNEL_SAMPLING)
			&& (CurrentState != ADC_CALIBRATING) && (CurrentState != ADC_GAIN_CALIBRATING)) {
#ifdef ADC_STATE_MACHINE_DEBUG
		printf("[ADC STATE MACHINE] Attempted to enter ADC_EXTERNAL_MUXING state from %s\n\r", ADCMachine_StringFromState(CurrentState));
#endif
	} else {
#ifdef ADC_STATE_MACHINE_DEBUG
		printf("[ADC STATE MACHINE] Entering state %s\n\r", ADCMachine_StringFromState(ADC_EXTERNAL_MUXING));
#endif
		PreviousState = CurrentState;
		CurrentState = ADC_EXTERNAL_MUXING;
		waitingOnTemp = false;
	}
}
