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
 * @file BoardTemperature.c
 * @brief Source file for the board temperature monitoring system.
 *
 * Contains methods for updating the board's temperature as well as reading current and historical temperatures.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "BoardTemperature.h"
#include "ADS1256_Driver.h"
#include "AnalogInput_Multiplexer.h"
#include "Tekdaqc_BSP.h"
#include "Tekdaqc_Debug.h"
#include "eeprom.h"

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @def LM35_SLOPE
 * @brief The slope of the voltage-temperature conversion function for the LM35 temperature sensor.
 */
#define LM35_SLOPE	100

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* The current board temperature in degrees C */
static float temperature;

/* The maximum board temperature ever recorded. */
static float max_temp = 0.0f;

/* The minimum board temperature ever recorded. */
static float min_temp = 0.0f;

/* The lower 16 bits of the maximum temperature reading. */
static uint16_t low_max_word;

/* The upper 16 bits of the maximum temperature reading. */
static uint16_t high_max_word;

/* The lower 16 bits of the minimum temperature reading. */
static uint16_t low_min_word;

/* The upper 16 bits of the minimum temperature reading. */
static uint16_t high_min_word;

/* Temporary scratch variable */
static uint32_t scratch;

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Updates the boards internal temperature. This will also check to see if a new minimum or maximum temperature
 * has been reached.
 *
 * @param input Analog_Input_t* The analog input which took the measurement.
 * @param code int32_t The integer code from the ADC.
 * @retval none
 */
void updateBoardTemperature(Analog_Input_t* input, int32_t code) {
	int32_t max = MAX_CODE;
	if (code < 0)
		++max; /* Add one for negative range. */
	temperature = LM35_SLOPE * ((2.0f * V_REFERENCE )/ADS1256_GetGainMultiplier(input->gain))* (((float) code)/max);
	return;
#ifdef BOARD_TEMPERATURE_DEBUG
	printf("[Board Temperature] New board temperature: %f Deg C.\n\r", temperature);
#endif
	if (max_temp == 0.0f) {
		EE_ReadVariable(ADDR_BOARD_MAX_TEMP_HIGH, &high_max_word);
		EE_ReadVariable(ADDR_BOARD_MAX_TEMP_LOW, &low_max_word);
		scratch = (((uint32_t) high_max_word) << 16) | low_max_word;
		max_temp = *(float*) (void*)(&scratch);
	}
	if (min_temp == 0.0f) {
		EE_ReadVariable(ADDR_BOARD_MIN_TEMP_HIGH, &high_min_word);
		EE_ReadVariable(ADDR_BOARD_MIN_TEMP_LOW, &low_min_word);
		scratch = (((uint32_t) high_min_word) << 16) | low_min_word;
		min_temp = *(float*) (void*)(&scratch);
	}

	if (temperature > max_temp) {
		printf("[Board Temperature] New board max temperature: %f Deg C.\n\r", temperature);
		max_temp = temperature;
		scratch = *(uint32_t*) (void*)(&temperature);
		EE_WriteVariable(ADDR_BOARD_MAX_TEMP_LOW, (uint16_t) (scratch & 0xFFFF));
		EE_WriteVariable(ADDR_BOARD_MAX_TEMP_HIGH, (uint16_t) (scratch >> 16));
	}
	if (temperature < min_temp) {
		printf("[Board Temperature] New board min temperature: %f Deg C.\n\r", temperature);
		min_temp = temperature;
		scratch = *(uint32_t*) (void*)(&temperature);
		EE_WriteVariable(ADDR_BOARD_MIN_TEMP_LOW, (uint16_t) (scratch & 0xFFFF));
		EE_WriteVariable(ADDR_BOARD_MIN_TEMP_HIGH, (uint16_t) (scratch >> 16));
	}
}

/**
 * Retrieves the board's most current temperature reading.
 *
 * @param none
 * @retval float The board temperature in degrees C.
 */
float getBoardTemperature(void) {
	return temperature;
}

/**
 * Retrieves the maximum temperature ever observed by this board.
 *
 * @param none
 * @retval float The maximum temperature in degrees C.
 */
float getMaximumBoardTemperature(void) {
	if (max_temp == 0.0f) {
		EE_ReadVariable(ADDR_BOARD_MAX_TEMP_HIGH, &high_max_word);
		EE_ReadVariable(ADDR_BOARD_MAX_TEMP_LOW, &low_max_word);
		scratch = (((uint32_t) high_max_word) << 16) | low_max_word;
		max_temp = *(float*) (void*)(&scratch);
	}
	return max_temp;
}

/**
 * Retrieves the minimum temperature ever observed by this board.
 *
 * @param none
 * @retval float The minimum temperature in degrees C.
 */
float getMinimumBoardTemperature(void) {
	if (min_temp == 0.0f) {
		EE_ReadVariable(ADDR_BOARD_MIN_TEMP_HIGH, &high_min_word);
		EE_ReadVariable(ADDR_BOARD_MIN_TEMP_LOW, &low_min_word);
		scratch = (((uint32_t) high_min_word) << 16) | low_min_word;
		min_temp = *(float*) (void*)(&scratch);
	}
	return min_temp;
}
