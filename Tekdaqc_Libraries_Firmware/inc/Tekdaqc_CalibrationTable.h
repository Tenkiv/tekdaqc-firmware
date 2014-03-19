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
 * @file Tekdaqc_CalibrationTable.h
 * @brief Header file for the Tekdaqc's calibration table.
 *
 * Contains public definitions and data types for the Tekdaqc's calibration table.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TEKDAQC_CALIBRATIONTABLE_H_
#define TEKDAQC_CALIBRATIONTABLE_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Config.h"
#include "ADS1256_Driver.h"
#include <boolean.h>

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Sets the base gain value to apply the calibration table to.
 */
void Tekdaqc_Calibration_SetBaseGainValue(uint32_t val, ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer);

/*
 * @brief Initializes the Tekdaqc's calibration table for read access.
 */
bool Tekdaqc_CalibrationInit(void);

/**
 * @brief Retrieves a gain calibration value for the specified parameters.
 */
uint32_t Tekdaqc_GetGainCalibration(ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer, float temperature);

/**
 * @brief Retrieves an offset calibration value for the specified parameters.
 */
uint32_t Tekdaqc_GetOffsetCalibration(ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer);

/**
 * @brief Enters calibration mode, enabling write access to the calibration table.
 */
FLASH_Status Tekdaqc_SetCalibrationMode(void);

/**
 * @brief Exits calibration mode, disabling write access to the calibration table.
 */
void Tekdaqc_EndCalibrationMode(void);

/**
 * @brief Sets the serial number of this Tekdaqc.
 */
FLASH_Status Tekdaqc_SetSerialNumber(char* serial);

/**
 * @brief Sets the temperature value for the lowest temperature entry of the table.
 */
FLASH_Status Tekdaqc_SetCalibrationLowTemperature(float temp);

/**
 * @brief Sets the temperature value for the highest temperature entry of the table.
 */
FLASH_Status Tekdaqc_SetCalibrationHighTemperature(float temp);

/**
 * @brief Sets the temperature step value for the temperature entries of the table.
 */
FLASH_Status Tekdaqc_SetCalibrationStepTemperature(float temp);

/**
 * @brief Sets the gain calibration value for the specified parameters.
 */
FLASH_Status Tekdaqc_SetGainCalibration(uint32_t cal, ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer, float temperature);

/**
 * @brief Sets the offset calibration value for the specified parameters.
 */
void Tekdaqc_SetOffsetCalibration(uint32_t cal, ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer);

/**
 * @brief Sets the base gain calibration value for the specified parameters.
 */
void Tekdaqc_SetBaseGainCalibration(uint32_t cal, ADS1256_SPS_t rate, ADS1256_PGA_t gain, ADS1256_BUFFER_t buffer);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* TEKDAQC_CALIBRATIONTABLE_H_ */
