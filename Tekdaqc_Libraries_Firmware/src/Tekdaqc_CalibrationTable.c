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
 * @file Tekdaqc_CalibrationTable.c
 * @brief Implements a calibration lookup table for analog measurements.
 *
 * Implements a calibration lookup table for the Tekdaqc's analog measurements. 32 bit gain
 * calibration values are stored which can be applied to the ADC's measurement.
 * The table has the ability to store values for each gain, sample rate and buffer setting on
 * the Tekdaqc, as well as for various temperature data points. When requesting a value, a temperature
 * must be specified and the value will automatically be interpolated from the closest high and low
 * temperature data points.
 *
 * Since the board has the ability to in the field perform offset calibrations, no values are specified
 * for the offset register in the table. Instead they are determined at run time by performing a complete
 * offset calibration and storing the results in a RAM table.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "Tekdaqc_CalibrationTable.h"
#include "Tekdaqc_BSP.h"
#include "TelnetServer.h"
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* The human readable string representations of the analog input scales. */
static const char* strings[3] = {"ANALOG 0-5V", "ANALOG 0-400V", "Invalid Scale"};

/* The highest temperature that calibration data exists for */
static float calTempHigh = 0.0f;

/* The lowest temperature that calibration data exists for */
static float calTempLow = 0.0f;

/* The temperature step for the calibration data */
static float calTempStep = 0.0f;

/* Does valid calibration data exist */
static bool isCalibrationValid = false;

/* If calibration mode has been enabled */
static bool CalibrationModeEnabled = false;

/* RAM table of offset calibrations */
static uint32_t offsetCalibrations[NUM_SAMPLE_RATES][NUM_PGA_SETTINGS][NUM_BUFFER_SETTINGS];

/* RAM table of base gain calibrations */
static uint32_t baseGainCalibrations[NUM_SAMPLE_RATES][NUM_PGA_SETTINGS][NUM_BUFFER_SETTINGS];

/* The cold junction offset calibration value. Cached in RAM to prevent lookup delays */
static uint32_t calColdJunctionOffset = 0xFFFFFFFFU;

/* The cold junction gain calibration value. Cached in RAM to prevent lookup delays */
static uint32_t calColdJunctionGain = 0xFFFFFFFFU;

/* The current analog input voltage scale being used. Defaulting to 400V range since the boards will be configured that way */
static ANALOG_INPUT_SCALE_t CURRENT_ANALOG_SCALE = ANALOG_SCALE_400V;

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Computes the address offset for the offset calibration value with the specified parameters.
 */
static uint32_t ComputeOffset(ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer,
		ANALOG_INPUT_SCALE_t scale, float temperature);

/**
 * @brief Interpolates two calibration values with the specified interpolation factor.
 */
static uint32_t InterpolateValue(uint32_t low, uint32_t high, float factor);

/**
 * @brief Computes the indecies for the RAM gain and offset lookup tables based on the sampling parameters.
 */
static void ComputeTableIndices(uint8_t* rateIndex, uint8_t* gain_index, uint8_t* buffer_index, uint8_t* scale_index,
		ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer, ANALOG_INPUT_SCALE_t scale);

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * Computes the address offset for the offset calibration value corresponding to the specified parameters.
 *
 * @param rate ADS1256_SPS_t The sample rate to lookup for.
 * @param gain ADS1256_PGA_t The gain to lookup for.
 * @param bufer ADS1256_BUFFER_t The buffer setting to lookup for.
 * @param temperature float The temperature value to lookup for.
 * @retval The computed address offset.
 */
static uint32_t ComputeOffset(ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer,
		ANALOG_INPUT_SCALE_t scale, float temperature) {
	uint32_t offset = 0U;
	uint8_t rateIDX = 0U;
	uint8_t gainIDX = 0U;
	uint8_t bufferIDX = 0U;
	uint8_t scaleIDX = 0U;

	ComputeTableIndices(&rateIDX, &gainIDX, &bufferIDX, &scaleIDX, rate, gain, buffer, scale);
#ifdef CALIBRATION_TABLE_DEBUG
	printf("[Calibration Table] Computed table indices - Rate: %i, Gain: %i, Buffer: %i, Scale: %i\n\r", rateIDX,
			gainIDX, bufferIDX, scaleIDX);
#endif
	uint8_t num_cal_steps = (temperature - calTempLow) / calTempStep;
#ifdef CALIBRATION_TABLE_DEBUG
	printf("[Calibration Table] Computed number of temperature steps: %i\n\r", num_cal_steps);
#endif
	offset = (num_cal_steps * CALIBRATION_TEMP_OFFSET)
			+ rateIDX * NUM_PGA_SETTINGS * (NUM_INPUT_RANGES + NUM_BUFFER_SETTINGS)
			+ gainIDX * (NUM_INPUT_RANGES + NUM_BUFFER_SETTINGS) + bufferIDX * NUM_BUFFER_SETTINGS + scaleIDX;
	offset *= 4; /* Multiply by 4 because these are word values */
#ifdef CALIBRATION_TABLE_DEBUG
	printf("[Calibration Table] Computed table offset: %" PRIu32 "\n\r", offset);
#endif
	return offset;
}

/**
 * @internal
 * Interpolates two calibration values based on the specified factor.
 *
 * @param low uint32_t The lower calibration data point.
 * @param high uint32_t The higher calibration data point.
 * @param factor float The interpolation factor. A value of 0 corresponds to low, a value of 1 corresponds to high.
 * @retval uint32_t The interpolated value.
 */
static uint32_t InterpolateValue(uint32_t low, uint32_t high, float factor) {
	uint32_t value = low + (high - low) * factor;
	return value;
}

static void ComputeTableIndices(uint8_t* rate_index, uint8_t* gain_index, uint8_t* buffer_index, uint8_t* scale_index,
		ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer, ANALOG_INPUT_SCALE_t scale) {
	*buffer_index = (buffer == ADS1256_BUFFER_ENABLED) ? 0U : 1U;
	*scale_index = (scale == ANALOG_SCALE_5V) ? 0U : 1U;

	switch (gain) {
		case ADS1256_PGAx1:
			*gain_index = 0U;
			break;
		case ADS1256_PGAx2:
			*gain_index = 1U;
			break;
		case ADS1256_PGAx4:
			*gain_index = 2U;
			break;
		case ADS1256_PGAx8:
			*gain_index = 3U;
			break;
		case ADS1256_PGAx16:
			*gain_index = 4U;
			break;
		case ADS1256_PGAx32:
			*gain_index = 5U;
			break;
		case ADS1256_PGAx64:
			*gain_index = 6U;
			break;
		default:
			*gain_index = 0U;
#ifdef CALIBRATION_TABLE_DEBUG
			printf("[Calibration Table] The requested gain was out of range, defaulting to x1.\n\r");
#endif
			break;
	}

	switch (rate) {
		case ADS1256_SPS_30000:
			*rate_index = 0U;
			break;
		case ADS1256_SPS_15000:
			*rate_index = 1U;
			break;
		case ADS1256_SPS_7500:
			*rate_index = 2U;
			break;
		case ADS1256_SPS_3750:
			*rate_index = 3U;
			break;
		case ADS1256_SPS_2000:
			*rate_index = 4U;
			break;
		case ADS1256_SPS_1000:
			*rate_index = 5U;
			break;
		case ADS1256_SPS_500:
			*rate_index = 6U;
			break;
		case ADS1256_SPS_100:
			*rate_index = 7U;
			break;
		case ADS1256_SPS_60:
			*rate_index = 8U;
			break;
		case ADS1256_SPS_50:
			*rate_index = 9U;
			break;
		case ADS1256_SPS_30:
			*rate_index = 10U;
			break;
		case ADS1256_SPS_25:
			*rate_index = 11U;
			break;
		case ADS1256_SPS_15:
			*rate_index = 12U;
			break;
		case ADS1256_SPS_10:
			*rate_index = 13U;
			break;
		case ADS1256_SPS_5:
			*rate_index = 14U;
			break;
		case ADS1256_SPS_2_5:
			*rate_index = 15U;
			break;
		default:
			rate_index = 0U;
#ifdef CALIBRATION_TABLE_DEBUG
			printf("[Calibration Table] The requested rate was out of range, defaulting to 30000.\n\r");
#endif
			break;
	}
}

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Initializes the calibration table for read operations.
 *
 * @param none
 * @retval bool Always TRUE.
 */
bool Tekdaqc_CalibrationInit(void) {
	FLASH_SetLatency(CALIBRATION_LATENCY);
	calTempLow = (*(__IO float*) CAL_TEMP_LOW_ADDR);
	calTempHigh = (*(__IO float*) CAL_TEMP_HIGH_ADDR);
	calTempStep = (*(__IO float*) CAL_TEMP_STEP_ADDR);
	calColdJunctionOffset = (*(__IO uint32_t*) COLD_JUNCTION_OFFSET_ADDR);
	calColdJunctionGain = (*(__IO uint32_t*) COLD_JUNCTION_GAIN_ADDR);
	isCalibrationValid = ((*(__IO uint8_t*) CAL_VALID_ADDR_LO_ADDR) == CAL_VALID_ADDR_LO_ADDR)
			&& ((*(__IO uint8_t*) CAL_VALID_ADDR_HI_ADDR) == CAL_VALID_ADDR_HI_ADDR);
	return TRUE;
}

/**
 * Set the current operating scale of the analog inputs on the board.
 *
 * @param scale {@link ANALOG_INPUT_SCALE_t} The scale setting to apply.
 * @retval none.
 */
void Tekdaqc_SetAnalogInputScale(ANALOG_INPUT_SCALE_t scale) {
	CURRENT_ANALOG_SCALE = scale;
}

/**
 * Retrieve the current operating scale of the analog inputs on the board.
 *
 * @param none.
 * @retval {@link ANALOG_INPUT_SCALE_t} The currently applied scale setting.
 */
ANALOG_INPUT_SCALE_t Tekdaqc_GetAnalogInputScale(void) {
	return CURRENT_ANALOG_SCALE;
}

/**
 * Retrieves the {@link ANALOG_INPUT_SCALE_t} value represented by the human readable string provided .
 *
 * @param str C-String to retrieve the {@link ANALOG_INPUT_SCALE_t} for.
 * @retval {@link ANALOG_INPUT_SCALE_t}.
 */
ANALOG_INPUT_SCALE_t Tekdaqc_StringToAnalogInputScale(char* str) {
	ANALOG_INPUT_SCALE_t scale = INVALID_SCALE; /* Assume invalid */
	if (strcmp(str, ANALOG_SCALE_5V_STRING) == 0) {
		/* This is ANALOG_SCALE_5V */
		scale = ANALOG_SCALE_5V;
	} else if (strcmp(str, ANALOG_SCALE_400V_STRING) == 0) {
		/* This is ANALOG_SCALE_400V */
		scale = ANALOG_SCALE_400V;
	}
	return scale;
}

/**
 * Retrieves a human readable string name for the provided {@link ANALOG_INPUT_SCALE_t}.
 *
 * @param scale {@link ANALOG_INPUT_SCALE_t} The scale value to retrieve the string for.
 * @retval const char* Human readable C-String.
 */
const char* Tekdaqc_AnalogInputScaleToString(ANALOG_INPUT_SCALE_t scale) {
	const char* str = NULL;
	switch (scale) {
		case ANALOG_SCALE_5V:
			str = strings[0];
			break;
		case ANALOG_SCALE_400V:
			str = strings[1];
			break;
		case INVALID_SCALE:
			str = strings[2];
			break;
		default:
			str = strings[2];
			break;
	}
	return str;
}

/**
 * Retrieve the base gain calibration value for the specified sampling parameters.
 *
 * @param rate ADS1256_SPS_t The sample rate to lookup for.
 * @param gain ADS1256_PGA_t The gain to lookup for.
 * @param buffer ADS1256_BUFFER_t The buffer setting to lookup for.
 * @retval The determined calibration value.
 */
uint32_t Tekdaqc_GetBaseGainCalibration(ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer) {
	uint8_t rate_index = 0U;
	uint8_t gain_index = 0U;
	uint8_t buffer_index = 0U;
	uint8_t range_index = 0U;
	ComputeTableIndices(&rate_index, &gain_index, &buffer_index, &range_index, rate, gain, buffer,
			CURRENT_ANALOG_SCALE);
	return baseGainCalibrations[rate_index][gain_index][buffer_index];
}

/**
 * Retrieve the gain calibration value for the specified sampling parameters.
 *
 * @param rate ADS1256_SPS_t The sample rate to lookup for.
 * @param gain ADS1256_PGA_t The gain to lookup for.
 * @param buffer ADS1256_BUFFER_t The buffer setting to lookup for.
 * @param temperature float The temperature value to lookup for.
 * @retval The determined calibration value.
 */
uint32_t Tekdaqc_GetGainCalibration(ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer, float temperature) {
	uint8_t rate_index = 0U;
	uint8_t gain_index = 0U;
	uint8_t buffer_index = 0U;
	uint8_t range_index = 0U;
	ComputeTableIndices(&rate_index, &gain_index, &buffer_index, &range_index, rate, gain, buffer,
			CURRENT_ANALOG_SCALE);
	uint32_t baseGain = baseGainCalibrations[rate_index][gain_index][buffer_index];

	if (isCalibrationValid != TRUE) {
#ifdef CALIBRATION_TABLE_DEBUG
		printf(
				"[Calibration Table] The calibration table is not valid, returning ADC calibration only (0x%" PRIX32 ").\n\r",
				baseGain);
#endif
		return baseGain;
	}
	if (temperature < calTempLow || temperature > calTempHigh) {
		/* The temperature is out of range, we will return the closest */
#ifdef CALIBRATION_TABLE_DEBUG
		printf(
				"[Calibration Table] The requested temperature %f was out of range. Minimum is %f and maximum is %f.\n\r",
				temperature, calTempLow, calTempHigh);
#endif
		snprintf(TOSTRING_BUFFER, sizeof(TOSTRING_BUFFER),
				"Error fetching the gain calibration value for temperature: %f Deg C. Temperature out of range. Allowable range is %f to %f Deg C",
				temperature, calTempLow, calTempHigh);
		TelnetWriteErrorMessage(TOSTRING_BUFFER);
		if (temperature < calTempLow) {
			temperature = calTempLow;
		} else {
			temperature = calTempHigh;
		}
	}

	uint8_t num_temp_steps = (uint8_t) (temperature / calTempStep);
	float low_temp = calTempLow * num_temp_steps;
	float high_temp = calTempLow + calTempStep * num_temp_steps;
	float factor = (temperature - calTempLow) / calTempStep;

	uint32_t offset = ComputeOffset(rate, gain, buffer, CURRENT_ANALOG_SCALE, low_temp); /* Add one for the move to gain */
	uint32_t Address = CAL_DATA_START_ADDR + 4 * offset; /* Multiply offset by 4 because entries are 4bytes long */

	uint32_t data_low = (*(__IO uint32_t*) Address);

	offset = ComputeOffset(rate, gain, buffer, CURRENT_ANALOG_SCALE, high_temp); /* Add one for the move to gain */
	Address = CAL_DATA_START_ADDR + 4 * offset; /* Multiply offset by 4 because entries are 4bytes long */

	uint32_t data_high = (*(__IO uint32_t*) Address);
	return (baseGain + InterpolateValue(data_low, data_high, factor));
}

/**
 * Retrieve the offset calibration value for the specified sampling parameters.
 *
 * @param rate ADS1256_SPS_t The sample rate to lookup for.
 * @param gain ADS1256_PGA_t The gain to lookup for.
 * @param buffer ADS1256_BUFFER_t The buffer setting to lookup for.
 * @retval The determined calibration value.
 */
uint32_t Tekdaqc_GetOffsetCalibration(ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer) {
	uint8_t rate_index;
	uint8_t gain_index;
	uint8_t buffer_index;
	uint8_t scale_index;
	ComputeTableIndices(&rate_index, &gain_index, &buffer_index, &scale_index, rate, gain, buffer,
			CURRENT_ANALOG_SCALE);

	return offsetCalibrations[rate_index][gain_index][buffer_index];
}

/**
 * Retrieves the offset calibration value for the cold junction.
 *
 * @param none
 * @retval The calibration value.
 */
uint32_t Tekdaqc_GetColdJunctionOffsetCalibration(void) {
	uint32_t cal;
	if (calColdJunctionOffset == 0xFFFFFFFFU) {
		cal = Tekdaqc_GetOffsetCalibration(ADS1256_SPS_30000, ADS1256_PGAx8, ADS1256_BUFFER_ENABLED);
	} else {
		cal = calColdJunctionOffset;
	}

	return cal;
}

/**
 * Retrieves the gain calibration value for the cold junction.
 *
 * @param none
 * @retval The calibration value.
 */
uint32_t Tekdaqc_GetColdJunctionGainCalibration(void) {
	uint32_t cal;
	if (calColdJunctionGain == 0xFFFFFFFFU) {
		cal = Tekdaqc_GetGainCalibration(ADS1256_SPS_30000, ADS1256_PGAx8, ADS1256_BUFFER_ENABLED, 25.0f);
	} else {
		cal = calColdJunctionGain;
	}

	return cal;
}

/**
 * Enter calibration mode. NOTE: Calling this method will erase the calibration table.
 *
 * @param none
 * @retval FLASH_Status The status of the FLASH operations. Returns FLASH_COMPLETE on success.
 */
FLASH_Status Tekdaqc_SetCalibrationMode(void) {
	/* Enable the flash control register access */
	FLASH_Unlock();

	/* Clear pending flags (if any) */
	FLASH_ClearFlag(
	FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

	FLASH_Status status = FLASH_COMPLETE;

	/* FLASH_OB_Unlock(); */

	/* Disable write protection for this sector */
	/*FLASH_OB_WRPConfig(CALIBRATION_WPSECTOR, DISABLE);

	 FLASH_OB_Launch(); */

	/* Erase the calibration sector */
	status = FLASH_EraseSector(CALIBRATION_SECTOR, FLASH_VOLTAGE_RANGE);
	if (status != FLASH_COMPLETE) {
		return status;
	}

	/* Enable write protection for this sector */
	/*FLASH_OB_WRPConfig(CALIBRATION_WPSECTOR, ENABLE);

	 FLASH_OB_Launch();*/

	/* FLASH_OB_Lock(); */
	/* Disable the flash control register access */

	CalibrationModeEnabled = true;
	return status;
}

/**
 * Exits calibration mode, locking the FLASH sector against unintended write operations.
 *
 * @param none
 * @retval none
 */
void Tekdaqc_EndCalibrationMode(void) {
	/* Lock the Flash to disable the flash control register access (recommended
	 to protect the FLASH memory against possible unwanted operation) */
	FLASH_Lock();

	CalibrationModeEnabled = false;
}

/**
 * Writes a the provided serial number to the serial number area of the calibration table.
 * This method requires that the board be in calibration mode and will return FLASH_ERROR_WRP
 * if it is not.
 *
 * @param serial char* Pointer to a C-String containing the board serial number. Expected to
 * be BOARD_SERIAL_NUM_LENGTH in size.
 * @retval FLASH_Status FLASH_COMPLETE on success, or the error status on failure.
 */
FLASH_Status Tekdaqc_SetSerialNumber(char* serial) {
	FLASH_Status retval = FLASH_COMPLETE;
	if (CalibrationModeEnabled == false) {
		return FLASH_ERROR_WRP;
	}
	if (strlen(serial) < BOARD_SERIAL_NUM_LENGTH) {
		/* The serial number isn't long enough */
#ifdef CALIBRATION_TABLE_DEBUG
		printf("[Calibration Table] The provided serial number to flash is not long enough.\n\r");
#endif
		retval = FLASH_ERROR_PROGRAM;
	}
	if (retval == FLASH_COMPLETE) {
		for (int i = BOARD_SERIAL_NUM_ADDR; i < BOARD_SERIAL_NUM_ADDR + BOARD_SERIAL_NUM_LENGTH; i++) {
			FLASH_Status status = FLASH_ProgramByte(i, serial[i]);
			if (status != FLASH_COMPLETE) {
				retval = status;
			}
		}
	}
	return retval;
}

/**
 * Writes the low temperature for which calibration data exists. This is the lowest temperature for which
 * we have valid calibration data. This method requires that the board be in calibration mode and will return
 * FLASH_ERROR_WRP if it is not.
 *
 * @param temp float The temperature value.
 * @retval FLASH_Status FLASH_COMPLETE on success, or the error status on failure.
 */
FLASH_Status Tekdaqc_SetCalibrationLowTemperature(float temp) {
	if (CalibrationModeEnabled == false) {
		return FLASH_ERROR_WRP;
	}
#ifdef CALIBRATION_TABLE_DEBUG
	printf("[Calibration Table] Programming calibration table low temperature: %f at location 0x%08" PRIX32 ".\n\r",
			temp, CAL_TEMP_LOW_ADDR);
#endif
	/* Convert the floating point value into a byte equivilant uint32_t */
	uint32_t* value = (uint32_t*) ((void*) &temp);
	FLASH_Status status = FLASH_ProgramWord(CAL_TEMP_LOW_ADDR, *value);
	if (status == FLASH_COMPLETE)
		calTempLow = temp;
	return status;
}

/**
 * Writes the high temperature for which calibration data exists. This is the highest temperature for which
 * we have valid calibration data. This method requires that the board be in calibration mode and will return
 * FLASH_ERROR_WRP if it is not.
 *
 * @param temp float The temperature value.
 * @retval FLASH_Status FLASH_COMPLETE on success, or the error status on failure.
 */
FLASH_Status Tekdaqc_SetCalibrationHighTemperature(float temp) {
	if (CalibrationModeEnabled == false) {
		return FLASH_ERROR_WRP;
	}
#ifdef CALIBRATION_TABLE_DEBUG
	printf("[Calibration Table] Programming calibration table high temperature: %f at location 0x%08" PRIX32 ".\n\r",
			temp, CAL_TEMP_HIGH_ADDR);
#endif
	/* Convert the floating point value into a byte equivilant uint32_t */
	uint32_t* value = (uint32_t*) ((void*) &temp);
	FLASH_Status status = FLASH_ProgramWord(CAL_TEMP_HIGH_ADDR, *value);
	if (status == FLASH_COMPLETE)
		calTempHigh = temp;
	return status;
}

/**
 * Writes the temperature step for the calibration data points. This method requires that the board be in
 * calibration mode and will return FLASH_ERROR_WRP if it is not.
 *
 * @param temp float The temperature step value.
 * @retval FLASH_Status FLASH_COMPLETE on success, or the error status on failure.
 */
FLASH_Status Tekdaqc_SetCalibrationStepTemperature(float temp) {
	if (CalibrationModeEnabled == false) {
		return FLASH_ERROR_WRP;
	}
#ifdef CALIBRATION_TABLE_DEBUG
	printf("[Calibration Table] Programming calibration table temperature step: %f at location 0x%08" PRIX32 ".\n\r",
			temp, CAL_TEMP_STEP_ADDR);
#endif
	/* Convert the floating point value into a byte equivilant uint32_t */
	uint32_t* value = (uint32_t*) ((void*) &temp);
	FLASH_Status status = FLASH_ProgramWord(CAL_TEMP_STEP_ADDR, *value);
	if (status == FLASH_COMPLETE)
		calTempStep = temp;
	return status;
}

/**
 * Marks the calibration table as valid so running code can determine if the values should be used or not.
 *
 * @param None.
 * @retval FLASH_Status FLASH_COMPLETE on success, or the error status on failure.
 */
FLASH_Status Tekdaqc_SetCalibrationValid(void) {
	if (CalibrationModeEnabled == false) {
		return FLASH_ERROR_WRP;
	}
#ifdef CALIBRATION_TABLE_DEBUG
	printf("[Calibration Table] Marking calibration table as valid.\n\r");
#endif
	FLASH_Status status = FLASH_ProgramByte(CAL_VALID_ADDR_LO_ADDR, CALIBRATION_VALID_LO_BYTE);
	if (status == FLASH_COMPLETE)
		status = FLASH_ProgramByte(CAL_VALID_ADDR_HI_ADDR, CALIBRATION_VALID_HI_BYTE);
	isCalibrationValid = (status == FLASH_COMPLETE) ? true : false;
	return status;
}

/**
 * Writes the gain calibration value for the specified parameters. This method requires that the board be in
 * calibration mode and will return FLASH_ERROR_WRP if it is not.
 *
 * @param cal uint32_t The calibration value to write.
 * @param rate ADS1256_SPS_t The sample rate to write for.
 * @param gain ADS1256_PGA_t The gain to write for.
 * @param buffer ADS1256_BUFFER_t The buffer setting to write for.
 * @param scale ANALOG_INPUT_SCALE_t The input scale setting to write for.
 * @param temperature float The temperature value to write for.
 * @retval FLASH_Status FLASH_COMPLETE on success, or the error status on failure.
 */
FLASH_Status Tekdaqc_SetGainCalibration(uint32_t cal, ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer,
		ANALOG_INPUT_SCALE_t scale, float temperature) {
	if (CalibrationModeEnabled == false) {
		return FLASH_ERROR_WRP;
	}

	uint32_t addr = CAL_DATA_START_ADDR + ComputeOffset(rate, gain, buffer, scale, temperature);
#ifdef CALIBRATION_TABLE_DEBUG
	printf("[Calibration Table] Programming calibration table value: 0x%08" PRIX32 " at location 0x%08" PRIX32 ".\n\r",
			cal, addr);
#endif
	FLASH_Status status = FLASH_ProgramWord(addr, cal);
	return status;
}

/**
 * Writes the offset calibration value to be used for the cold junction sensor into the flash calibration table.
 * This method requires that the board be in calibration mode and will return FLASH_ERROR_WRP if it is not.
 *
 * @param cal uint32 The calibration value to write.
 * @retval FLASH_Status FLASH_COMPLETE on success, or the error status on failure.
 */
FLASH_Status Tekdaqc_SetColdJunctionOffsetCalibration(uint32_t cal) {
	if (CalibrationModeEnabled == false) {
		return FLASH_ERROR_WRP;
	}

	FLASH_Status status = FLASH_ProgramWord(COLD_JUNCTION_OFFSET_ADDR, cal);
	return status;
}

/**
 * Writes the gain calibration value to be used for the cold junction sensor into the flash calibration table.
 * This method requires that the board be in calibration mode and will return FLASH_ERROR_WRP if it is not.
 *
 * @param cal uint32 The calibration value to write.
 * @retval FLASH_Status FLASH_COMPLETE on success, or the error status on failure.
 */
FLASH_Status Tekdaqc_SetColdJunctionGainCalibration(uint32_t cal) {
	if (CalibrationModeEnabled == false) {
		return FLASH_ERROR_WRP;
	}

	FLASH_Status status = FLASH_ProgramWord(COLD_JUNCTION_GAIN_ADDR, cal);
	return status;
}

/**
 * Writes the offset calibration value for the specified parameters. This method does not require the board to be in calibration
 * mode and only stores the values in a RAM lookup table.
 *
 * @param cal uint32_t The calibration value to write.
 * @param rate ADS1256_SPS_t The sample rate to write for.
 * @param gain ADS1256_PGA_t The gain to write for.
 * @param buffer ADS1256_BUFFER_t The buffer setting to write for.
 * @retval none
 */
void Tekdaqc_SetOffsetCalibration(uint32_t cal, ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer) {
	uint8_t rate_index;
	uint8_t gain_index;
	uint8_t buffer_index;
	uint8_t scale_index;
	ComputeTableIndices(&rate_index, &gain_index, &buffer_index, &scale_index, rate, gain, buffer,
			CURRENT_ANALOG_SCALE);

	offsetCalibrations[rate_index][gain_index][buffer_index] = cal;
}

/**
 * Writes the base gain calibration value for the specified parameters. This method does not require the board to be in calibration
 * mode and only stores the values in a RAM lookup table.
 *
 * @param cal uint32_t The calibration value to write.
 * @param rate ADS1256_SPS_t The sample rate to write for.
 * @param gain ADS1256_PGA_t The gain to write for.
 * @param buffer ADS1256_BUFFER_t The buffer setting to write for.
 * @retval none
 */
void Tekdaqc_SetBaseGainCalibration(uint32_t cal, ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer) {
	uint8_t rate_index;
	uint8_t gain_index;
	uint8_t buffer_index;
	uint8_t scale_index;
	ComputeTableIndices(&rate_index, &gain_index, &buffer_index, &scale_index, rate, gain, buffer,
			CURRENT_ANALOG_SCALE);

	baseGainCalibrations[rate_index][gain_index][buffer_index] = cal;
}
