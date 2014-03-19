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
 * @file TLE7232_RelayDriver.h
 * @brief Header file for the TLE7232 Output driver.
 *
 * Contains public definitions and data types for the TLE7232 Output driver.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TLE7232_RELAYDRIVER_H_
#define TLE7232_RELAYDRIVER_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Config.h"
#include "stm32f4xx_gpio.h"

/** @addtogroup tekdaqc_firmware_libraries Tekdaqc Firmware Libraries
 * @{
 */

/** @addtogroup tle7232_driver TLE7232 Driver
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @def TLE7232_MAP_NOT_CONTROLLED
 * @brief Defines the value for specifying that an input pin does not control outputs.
 */
#define TLE7232_MAP_NOT_CONTROLLED		0

/**
 * @def TLE7232_MAP_CONTROLLED
 * @brief Defines the value for specifying that an input pin does control outputs.
 */
#define TLE7232_MAP_CONTROLLED			1

/**
 * @def TLE7232_CTL_BOOLEAN_OR
 * @brief Defines the value for specifying that outputs are controlled by the the pin mapping logically OR'ed with the control register.
 */
#define TLE7232_CTL_BOOLEAN_OR			0

/**
 * @def TLE7232_CTL_BOOLENA_AND
 * @brief Defines the value for specifying that outputs are controlled by the the pin mapping logically AND'ed with the control register.
 */
#define TLE7232_CTL_BOOLENA_AND			1

/**
 * @def TLE7232_SLEW_FAST
 * @brief Defines the value for specifying that an output should be toggled with a high slew rate.
 */
#define TLE7232_SLEW_FAST				0

/**
 * @def TLE7232_SLEW_SLOW
 * @brief Defines the value for specifying that an output should be toggled with a low slew rate.
 */
#define TLE7232_SLEW_SLOW				1

/**
 * @def TLE7232_OUTPUT_OFF
 * @brief Defines the value for specifying that an output should be turned off.
 */
#define TLE7232_OUTPUT_OFF				0

/**
 * @def TLE7232_OUTPUT_ON
 * @brief Defines the value for specifying that an output should be turned on.
 */
#define TLE7232_OUTPUT_ON				1

/**
 * @def TLE7232_CURRENT_FAULT_LIMIT
 * @brief Defines the value for specifying that an output current fault should enter current limiting.
 */
#define TLE7232_CURRENT_FAULT_LIMIT		0

/**
 * @def TLE7232_CURRENT_FAULT_OFF
 * @brief Defines the value for specifying that an output current fault should disable the channel until cleared.
 */
#define TLE7232_CURRENT_FAULT_OFF		1

/**
 * @def TLE7232_TEMP_FAULT_RESTART
 * @brief Defines the value for specifying that an output over temperature fault should restart after cooldown.
 */
#define TLE7232_TEMP_FAULT_RESTART		0

/**
 * @def TLE7232_TEMP_FAULT_OFF
 * @brief Defines the value for specifying that an output over temperature fault should disable the channel until cleared.
 */
#define TLE7232_TEMP_FAULT_OFF			1

/**
 * @def TLE7232_CS_LOW()
 * @brief Function like MACRO to select TLE7232: Chip Select pin low
 */
#define TLE7232_CS_LOW()       (GPIO_ResetBits(TLE7232_CS_GPIO_PORT, TLE7232_CS_PIN))
/**
 * @def TLE7232_CS_HIGH()
 * @brief Function like MACRO to de-select TLE7232: Chip Select pin high
 */
#define TLE7232_CS_HIGH()      (GPIO_SetBits(TLE7232_CS_GPIO_PORT, TLE7232_CS_PIN))

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED TYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Command enumeration.
 * Defines the possible commands which can be sent to the TLE7232 chip over SPI. They are defined
 * such that you can bitwise OR valid register addresses and data to compose a valid SPI word.
 */
typedef enum {
	TLE7232_CMD_DIAGNOSIS = ((uint16_t) 0x0000), /**< Read diagnosis only.*/
	TLE7232_CMD_READ_REGISTER = ((uint16_t) 0x4000), /**< Read the contents of a register.*/
	TLE7232_CMD_RESET_REGISTERS = ((uint16_t) 0x8000), /**< Reset the registers to their default value.*/
	TLE7232_CMD_WRITE_REGISTER = ((uint16_t) 0xC000) /**< Write the contents of a register.*/
} TLE7232_Command_t;

/**
 * @brief Register enumeration.
 * Defines the registers and their addresses for the TLE7232. They are defined such that
 * you can bitwise OR valid commands and data to compose a valid SPI word.
 */
typedef enum {
	TLE7232_REG_IMCR = ((uint16_t) 0x0100), /**< Input mapping configuration register. Defaults to 0x08.*/
	TLE7232_REG_BOCR = ((uint16_t) 0x0200), /**< Boolean operator configuration register. Defaults to 0x00.*/
	TLE7232_REG_OLCR = ((uint16_t) 0x0300), /**< Overload configuration register. Defaults to 0x00.*/
	TLE7232_REG_OTCR = ((uint16_t) 0x0400), /**< Overtemperature configuration register. Defaults to 0x00.*/
	TLE7232_REG_SRCR = ((uint16_t) 0x0500), /**< Slew rate configuration register. Defaults to 0x00.*/
	TLE7232_REG_STA = ((uint16_t) 0x0600), /**< Output status monitor. Defaults to 0x00.*/
	TLE7232_REG_CTL = ((uint16_t) 0x0700) /**< Output control register. Defaults to 0x00.*/
} TLE7232_Register_t;

/**
 * @brief TLE7232 Status enumeration.
 * Defines the possible status states of the TLE7232.
 */
typedef enum {
	TLE7232_Short_To_Ground = ((uint8_t) 0x00), /**< The channel is shorted to ground. */
	TLE7232_Open_Load = ((uint8_t) 0x01), /**< The channel is open circuit/open load. */
	TLE7232_Overload_Overtemp = ((uint8_t) 0x02), /**< The channel is in overtemperature overload. */
	TLE7232_Normal_Operation = ((uint8_t) 0x03) /**< The channel is in normal operation. */
} TLE7232_Status_t;

/**
 * @brief Function pointer definition for setting the fault status of a channel.
 * Defines a function pointer which can be used to set the fault status of a particular channel on a particular driver chip.
 */
typedef bool (*SetOutputFaultStatus)(TLE7232_Status_t status, uint8_t chip_id, uint8_t channel);

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Initialize the TLE7232 Driver.
 */
void TLE7232_Init(void);

/**
 * @brief Reset the TLE7232.
 */
void TLE7232_Reset(void);

/**
 * @brief Read the diagnosis registers of the specified device.
 */
uint16_t TLE7232_ReadDiagnosis(uint8_t chip_index);

/**
 * @brief Read the diagnosis registers of all devices.
 */
void TLE7232_ReadAllDiagnosis(void);

/**
 * @breif Retrieve the diagnosis registers of the specified device.
 */
uint16_t TLE7232_GetDiagnosis(uint8_t chip_index);

/**
 * @brief Reset the diagnosis registers of the specified device.
 */
void TLE7232_ResetRegisters(uint8_t chip_index);

/**
 * @brief Reset the diagnosis registers of all devices.
 */
void TLE7232_ResetAllRegisters(void);

/**
 * @brief Write the specified data to the specified register of a device.
 */
void TLE7232_WriteRegister(TLE7232_Register_t reg, uint8_t data, uint8_t chip_index);

/**
 * @brief Write the specified data to the specified register of all devices. Data can be different for each device.
 */
void TLE7232_WriteRegisterAll(TLE7232_Register_t reg, uint8_t data[NUMBER_TLE7232_CHIPS]);

/**
 * @brief Write the specified data to the specified register of all devices. Register and data can be different for each device.
 */
void TLE7232_WriteArbitraryRegisterAll(TLE7232_Register_t reg[NUMBER_TLE7232_CHIPS], uint8_t data[NUMBER_TLE7232_CHIPS]);

/**
 * @brief Read the specified register of a specific device.
 */
uint8_t TLE7232_ReadRegister(TLE7232_Register_t reg, uint8_t chip_index);

/**
 * @brief Read the specified register of all devices.
 */
void TLE7232_ReadRegisterAll(TLE7232_Register_t reg, uint8_t data[NUMBER_TLE7232_CHIPS]);

/**
 * @brief Read arbitrary registers from each device.
 */
void TLE7232_ReadArbitraryRegisterAll(TLE7232_Register_t reg[NUMBER_TLE7232_CHIPS], uint8_t data[NUMBER_TLE7232_CHIPS]);

/**
 * @brief Sets the function to be called when an output fault occurs.
 */
void SetOutputFaultStatusFunction(SetOutputFaultStatus func);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* TLE7232_RELAYDRIVER_H_ */
