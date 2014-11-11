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
 * @file TLE7232_RelayDriver.c
 * @brief Controls the TLE7232 relay driver via SPI.
 *
 * LE7232 relay driver via SPI. This driver assumes that the chips are configured in daisy chain
 * mode and allows access to a particular chip or all chips.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdio.h>
#include <stm32f4xx.h>
#include <stm32f4xx_rcc.h>
#include <stm32f4xx_spi.h>
#include <Tekdaqc_BSP.h>
#include <Tekdaqc_Timers.h>
#include <TLE7232_RelayDriver.h>
//#include "Tekdaqc_Config.h"
//#include "Tekdaqc_Debug.h"

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* Function pointer to the function to be called when a fault is detected */
static SetOutputFaultStatus SetFaultStatus = NULL;

/* Local copy of the diagnosis registers of all TLE7232 chips */
static uint16_t DiagnosisRegisters[NUMBER_TLE7232_CHIPS];

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE METHOD PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Performs low level communication initialization for the TLE7232 driver.
 */
static void TLE7232_LowLevel_Init(void);

/**
 * @brief Initialize the TLE7232 SPI peripherals.
 */
static void TLE7232_SPI_Init(void);

/**
 * @brief Evaluates the state of the diagnosis registers, looking for faults.
 */
static void TLE7232_EvaluateDiagnosis(void);

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Performs low level communication initialization of the peripherals used by the SPI driver.
 *
 * @param  none
 * @retval none
 */
static void TLE7232_LowLevel_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable the SPI clock */
	TLE7232_SPI_CLK_INIT(TLE7232_SPI_CLK, ENABLE);

	/* Enable GPIO clocks */
	RCC_AHB1PeriphClockCmd(TLE7232_SPI_SCK_GPIO_CLK | TLE7232_SPI_MISO_GPIO_CLK |
	TLE7232_SPI_MOSI_GPIO_CLK | TLE7232_CS_GPIO_CLK, ENABLE);

	/* SPI pins configuration */

	/* Connect SPI pins to AF5 */
	GPIO_PinAFConfig(TLE7232_SPI_SCK_GPIO_PORT, TLE7232_SPI_SCK_SOURCE, TLE7232_SPI_SCK_AF);
	GPIO_PinAFConfig(TLE7232_SPI_MISO_GPIO_PORT, TLE7232_SPI_MISO_SOURCE, TLE7232_SPI_MISO_AF);
	GPIO_PinAFConfig(TLE7232_SPI_MOSI_GPIO_PORT, TLE7232_SPI_MOSI_SOURCE, TLE7232_SPI_MOSI_AF);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

	/* SPI SCK pin configuration */
	GPIO_InitStructure.GPIO_Pin = TLE7232_SPI_SCK_PIN;
	GPIO_Init(TLE7232_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

	/* SPI MOSI pin configuration */
	GPIO_InitStructure.GPIO_Pin = TLE7232_SPI_MOSI_PIN;
	GPIO_Init(TLE7232_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

	/* SPI MISO pin configuration */
	GPIO_InitStructure.GPIO_Pin = TLE7232_SPI_MISO_PIN;
	GPIO_Init(TLE7232_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

	/* Configure TLE7232 CS pin in output pushpull mode */
	GPIO_InitStructure.GPIO_Pin = TLE7232_CS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(TLE7232_CS_GPIO_PORT, &GPIO_InitStructure);
}

/**
 * Initialize the TLE7232 SPI peripherals.
 *
 * @param none
 * @retval none
 */
static void TLE7232_SPI_Init(void) {
#ifdef TLE7232_SPI_DEBUG
	printf("[TLE7232] Initializing SPI peripherals for the TLE7232.\n\r");
#endif
	SPI_InitTypeDef SPI_InitStructure;
	TLE7232_LowLevel_Init();

	/* Deselect the TLE7232: Chip Select high */
	TLE7232_CS_HIGH();

	/* SPI configuration */
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;

	SPI_Init(TLE7232_SPI, &SPI_InitStructure);

	/* Enable the TLE7232 SPI  */
	SPI_Cmd(TLE7232_SPI, ENABLE);
}

/**
 * Evaluates the contents of the local copy of the diagnosis registers, looking for any possible fault conditions.
 * If a fault is discovered, the function pointed to by SetFaultStatus will notified of the fault.
 *
 * @param none
 * @retval none
 */
static void TLE7232_EvaluateDiagnosis(void) {
	if (SetFaultStatus != NULL) { /* Don't bother evaluating the faults if we can't do anything */
		for (uint_fast8_t i = 0; i < NUMBER_TLE7232_CHIPS; ++i) {
			/* Check the status of this chip */
			uint16_t mask = 0x0003;
			uint16_t reg = 0x0000;
			TLE7232_Status_t status;
			for (uint16_t shift = 0; shift < 16; shift += 2) { /* The 16bit index is to match the data it operates on */
				reg = DiagnosisRegisters[i] & (mask << shift); /* Isolate the channel's bits */
				reg >>= shift; /* Move the bits to the least significant bits */
				status = (TLE7232_Status_t) reg;
				if (SetFaultStatus != NULL) {
					SetFaultStatus(status, i, (shift / 2)); /* Set the status on the output */
				}
			}
		}
	}
}

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Initializes the TLE7232 relay drivers and prepares them to receive commands.
 *
 * @param none
 * @retval none
 */
void TLE7232_Init(void) {
	TLE7232_SPI_Init();

	GPIO_InitTypeDef GPIO_InitStructure;

	/* Confgure the RESET Pin */
	/* Enable the GPIO Clock */
	RCC_AHB1PeriphClockCmd(TLE7232_RESET_GPIO_CLK, ENABLE);

	/* Configure the GPIO pin */
	GPIO_InitStructure.GPIO_Pin = TLE7232_RESET_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(TLE7232_RESET_GPIO_PORT, &GPIO_InitStructure);

	/* Bring the RESET pin high */
	GPIO_SetBits(TLE7232_RESET_GPIO_PORT, TLE7232_RESET_PIN);

	/* Reset the drivers */
	TLE7232_Reset();
}

/**
 * Resets the TLE7232 drivers, returning them to their default state. This method
 * does not require SPI communication and can be used to re-establish lost communication.
 *
 * @param none
 * @retval none
 */
void TLE7232_Reset(void) {
	GPIO_ResetBits(TLE7232_RESET_GPIO_PORT, TLE7232_RESET_PIN);
	Delay_ms(1000); /* Parameter 5.1.19 */
	GPIO_SetBits(TLE7232_RESET_GPIO_PORT, TLE7232_RESET_PIN);
}

/**
 * Reads the diagnosis register from the specified TLE7232 driver chip.
 *
 * @param chip_index uint8_t The index of the chip to read the diagnosis register from.
 * @retval uint16_t The read-in diagnosis register.
 */
uint16_t TLE7232_ReadDiagnosis(uint8_t chip_index) {
	if ((chip_index >= NUMBER_TLE7232_CHIPS) || (chip_index < 0)) {
		/* This chip does not exist on the board */
#ifdef TLE7232_SPI_DEBUG
		printf("[TLE7232 SPI] Attempted to write data to a device which does not exist.\n\r");
#endif
		/* TODO: Set an error. */
		return 0;
	}
	/* Select the TLE7232: Chip Select Low */
	TLE7232_CS_LOW();

	for (int i = NUMBER_TLE7232_CHIPS - 1; i >= 0; --i) {
		SPI_I2S_SendData(TLE7232_SPI, TLE7232_CMD_DIAGNOSIS);
		while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
			/* The SPI transmit buffer has data, so wait patiently */
		}
		DiagnosisRegisters[i] = SPI_I2S_ReceiveData(TLE7232_SPI);
	}


	/* De-select the TLE7232: Chip Select High */
	TLE7232_CS_HIGH();
	TLE7232_EvaluateDiagnosis();
	return DiagnosisRegisters[chip_index];
}

/**
 * Reads the diagnosis register from all TLE7232 driver chips in the daisy chain, storing them in the local
 * DiagnosisRegisters copy.
 *
 * @param none
 * @retval none
 */
void TLE7232_ReadAllDiagnosis(void) {
	/* Select the TLE7232: Chip Select Low */
	TLE7232_CS_LOW();

	for (int i = NUMBER_TLE7232_CHIPS - 1; i >= 0; --i) {
		SPI_I2S_SendData(TLE7232_SPI, TLE7232_CMD_DIAGNOSIS);
		while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
			/* The SPI transmit buffer has data, so wait patiently */
		}
		/* Fetch the specific data */
		DiagnosisRegisters[i] = SPI_I2S_ReceiveData(TLE7232_SPI);
	}

	/* Deselect the TLE7232: Chip Select High */
	TLE7232_CS_HIGH();
	TLE7232_EvaluateDiagnosis();
}

/**
 * Retrieve the current local copy of the diagnosis register for the specified driver chip.
 *
 * @param chip_index uint8_t The index of the chip to read the diagnosis register from.
 * @retval uint16_t The retrieved diagnosis register value.
 */
uint16_t TLE7232_GetDiagnosis(uint8_t chip_index) {
	if ((chip_index >= NUMBER_TLE7232_CHIPS) || (chip_index < 0)) {
		/* This chip does not exist on the board */
#ifdef TLE7232_SPI_DEBUG
		printf("[TLE7232 SPI] Attempted to write data to a device which does not exist.\n\r");
#endif
		/*TODO: Set an error. */
		return 0;
	}
	return DiagnosisRegisters[chip_index];
}

/**
 * Resets (clears errors) from the diagnosis register of the specified driver chip.
 *
 * @param chip_index uint8_t The index of the chip to read the diagnosis register from.
 * @retval none
 */
void TLE7232_ResetRegisters(uint8_t chip_index) {
	if ((chip_index >= NUMBER_TLE7232_CHIPS) || (chip_index < 0)) {
		/* This chip does not exist on the board */
#ifdef TLE7232_SPI_DEBUG
		printf("[TLE7232 SPI] Attempted to write data to a device which does not exist.\n\r");
#endif
		/*TODO: Set an error. */
		return;
	}
	/* Select the TLE7232: Chip Select Low */
	TLE7232_CS_LOW();

	for (int i = NUMBER_TLE7232_CHIPS - 1; i >= 0; --i) {
		if (i == chip_index) {
			/* Reset the specific chip */
			SPI_I2S_SendData(TLE7232_SPI, TLE7232_CMD_RESET_REGISTERS);
		} else {
			SPI_I2S_SendData(TLE7232_SPI, TLE7232_CMD_DIAGNOSIS);
		}
		while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
			/* The SPI transmit buffer has data, so wait patiently */
		}
		DiagnosisRegisters[i] = SPI_I2S_ReceiveData(TLE7232_SPI);
	}

	/* De-select the TLE7232: Chip Select High */
	TLE7232_CS_HIGH();
	TLE7232_EvaluateDiagnosis();
}

/**
 * Resets (clears errors) from the diagnosis register of all TLE7232 drivers in the daisy chain.
 *
 * @param none
 * @retval none
 */
void TLE7232_ResetAllRegisters(void) {
	/* Select the TLE7232: Chip Select Low */
	TLE7232_CS_LOW();

	for (int i = NUMBER_TLE7232_CHIPS - 1; i >= 0; --i) {
		printf("Resetting TLE7232 %i\n", i);
		SPI_I2S_SendData(TLE7232_SPI, TLE7232_CMD_RESET_REGISTERS);
		while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
			/* The SPI transmit buffer has data, so wait patiently */
		}
		/* Fetch the specific data */
		DiagnosisRegisters[i] = SPI_I2S_ReceiveData(TLE7232_SPI);
	}

	/* De-select the TLE7232: Chip Select High */
	TLE7232_CS_HIGH();
	TLE7232_EvaluateDiagnosis();
}

/**
 * Writes the provided data to the specified register of the specified TLE7232 driver in the chain.
 *
 * @param reg TLE7232_Register_t The register to write data to.
 * @param data uint8_t The data to write
 * @param chip_index The specific TLE7232 to write data to.
 * @retval none
 */
void TLE7232_WriteRegister(TLE7232_Register_t reg, uint8_t data, uint8_t chip_index) {
	uint16_t command = TLE7232_CMD_WRITE_REGISTER | reg | data;
	/* Select the TLE7232: Chip Select Low */
	TLE7232_CS_LOW();

	for (int i = NUMBER_TLE7232_CHIPS - 1; i >= 0; --i) {
		if (i == chip_index) {
			/* Write the data to the specified chip. */
			SPI_I2S_SendData(TLE7232_SPI, command);
		} else {
			SPI_I2S_SendData(TLE7232_SPI, TLE7232_CMD_DIAGNOSIS);
		}
		while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
			/* The SPI transmit buffer has data, so wait patiently */
		}
		/* Fetch the diagnosis data regardless of the command */
		DiagnosisRegisters[i] = SPI_I2S_ReceiveData(TLE7232_SPI);
	}

	/* Deselect the TLE7232: Chip Select High */
	TLE7232_CS_HIGH();
	TLE7232_EvaluateDiagnosis();
}

/**
 * Writes the provided data to the specified register of the all TLE7232 drivers in the chain.
 *
 * @param reg TLE7232_Register_t The register to write data to.
 * @param data uint8_t[] The data to write, indexed by chip index.
 * @retval none
 */
void TLE7232_WriteRegisterAll(TLE7232_Register_t reg, uint8_t data[NUMBER_TLE7232_CHIPS]) {
	uint16_t command = 0;
	/* Select the TLE7232: Chip Select Low */
	TLE7232_CS_LOW();

	for (int i = NUMBER_TLE7232_CHIPS - 1; i >= 0; --i) {
		/* Build the command word. */
		command = TLE7232_CMD_WRITE_REGISTER | reg | data[i];
		/* Write the data to the chip. */
		SPI_I2S_SendData(TLE7232_SPI, command);
		while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
			/* The SPI transmit buffer has data, so wait patiently */
		}
		/* Fetch the diagnosis data regardless of the command */
		DiagnosisRegisters[i] = SPI_I2S_ReceiveData(TLE7232_SPI);
	}

	/* De-select the TLE7232: Chip Select High */
	TLE7232_CS_HIGH();
	TLE7232_EvaluateDiagnosis();
}

/**
 * Writes the provided data to the specified register of each TLE7232 drivers in the chain.
 *
 * @param reg TLE7232_Register_t The register to write data to, indexed by chip index.
 * @param data uint8_t[] The data to write, indexed by chip index.
 * @retval none
 */
void TLE7232_WriteArbitraryRegisterAll(TLE7232_Register_t reg[NUMBER_TLE7232_CHIPS], uint8_t data[NUMBER_TLE7232_CHIPS]) {
	uint16_t command = 0;
	/* Select the TLE7232: Chip Select Low */
	TLE7232_CS_LOW();

	for (int i = NUMBER_TLE7232_CHIPS - 1; i >= 0; --i) {
		/* Build the command word. */
		command = TLE7232_CMD_WRITE_REGISTER | reg[i] | data[i];
		/* Write the data to the chip. */
		SPI_I2S_SendData(TLE7232_SPI, command);
		/* Fetch the diagnosis data regardless of the command */
		DiagnosisRegisters[i] = SPI_I2S_ReceiveData(TLE7232_SPI);
	}

	while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
		/* The SPI transmit buffer has data, so wait patiently */
	}

	/* Deselect the TLE7232: Chip Select High */
	TLE7232_CS_HIGH();
	TLE7232_EvaluateDiagnosis();
}

/**
 * Reads the specified register from the specified TLE7232 driver.
 *
 * @param reg TLE7232_Register_t The register to read data from.
 * @param chip_index The specific TLE7232 to read data from.
 * @retval uint8_t The read-in data.
 */
uint8_t TLE7232_ReadRegister(TLE7232_Register_t reg, uint8_t chip_index) {
	uint8_t retval;
	/* Select the TLE7232: Chip Select Low */
	TLE7232_CS_LOW();

	for (int i = NUMBER_TLE7232_CHIPS - 1; i >= 0; --i) {
		uint16_t data = (i == chip_index) ? TLE7232_CMD_READ_REGISTER | reg : TLE7232_CMD_DIAGNOSIS;
		/* Write the data to the specified chip. */
		SPI_I2S_SendData(TLE7232_SPI, data);
		while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
			/* The SPI transmit buffer has data, so wait patiently */
		}
		/* Fetch the diagnosis data */
		DiagnosisRegisters[i] = SPI_I2S_ReceiveData(TLE7232_SPI);
	}
	for (int i = NUMBER_TLE7232_CHIPS - 1; i >= 0; --i) {
		SPI_I2S_SendData(TLE7232_SPI, TLE7232_CMD_DIAGNOSIS);
		while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
			/* The SPI transmit buffer has data, so wait patiently */
		}
		if (i == chip_index) {
			/* Fetch the data */
			retval = ((uint8_t) (0xFF & SPI_I2S_ReceiveData(TLE7232_SPI)));
		} else {
			/* Fetch the diagnosis data */
			DiagnosisRegisters[i] = SPI_I2S_ReceiveData(TLE7232_SPI);
		}
	}

	/* De-select the TLE7232: Chip Select High */
	TLE7232_CS_HIGH();
	TLE7232_EvaluateDiagnosis();
	return retval;
}

/**
 * Reads the specified register from all TLE7232 drivers in the chain.
 *
 * @param reg TLE7232_Register_t The register to read data from.
 * @param data uint8_t[] Array to store the read-in data, indexed by chip index.
 * @retval none
 */
void TLE7232_ReadRegisterAll(TLE7232_Register_t reg, uint8_t data[NUMBER_TLE7232_CHIPS]) {
	uint16_t command = TLE7232_CMD_READ_REGISTER | reg;
	/* Select the TLE7232: Chip Select Low */
	TLE7232_CS_LOW();

	for (int i = NUMBER_TLE7232_CHIPS - 1; i >= 0; --i) {
		/* Write the data to the specified chip. */
		SPI_I2S_SendData(TLE7232_SPI, command);
		while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
			/* The SPI transmit buffer has data, so wait patiently */
		}
		/* Fetch the diagnosis data */
		DiagnosisRegisters[i] = SPI_I2S_ReceiveData(TLE7232_SPI);
	}
	for (int i = NUMBER_TLE7232_CHIPS - 1; i >= 0; --i) {
		SPI_I2S_SendData(TLE7232_SPI, TLE7232_CMD_DIAGNOSIS);
		while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
			/* The SPI transmit buffer has data, so wait patiently */
		}
		data[i] = SPI_I2S_ReceiveData(TLE7232_SPI);
	}

	/* De-select the TLE7232: Chip Select High */
	TLE7232_CS_HIGH();
	TLE7232_EvaluateDiagnosis();
}

/**
 * Reads all TLE7232 drivers, pulling the specified register from the specified TLE7232 drivers in the chain.
 *
 * @param reg TLE7232_Register_t The register to read data from, indexed by chip index.
 * @param data uint8_t[] Array to store the read-in data, indexed by chip index.
 * @retval none
 */
void TLE7232_ReadArbitraryRegisterAll(TLE7232_Register_t reg[NUMBER_TLE7232_CHIPS], uint8_t data[NUMBER_TLE7232_CHIPS]) {
	/* Select the TLE7232: Chip Select Low */
	TLE7232_CS_LOW();

	for (int i = NUMBER_TLE7232_CHIPS - 1; i >= 0 ; --i) {
		/* Write the data to the specified chip. */
		SPI_I2S_SendData(TLE7232_SPI, TLE7232_CMD_READ_REGISTER | reg[i]);
		/* Fetch the diagnosis data */
		DiagnosisRegisters[i] = SPI_I2S_ReceiveData(TLE7232_SPI);
		SPI_I2S_SendData(TLE7232_SPI, TLE7232_CMD_DIAGNOSIS);
		data[i] = SPI_I2S_ReceiveData(TLE7232_SPI);
	}

	while (SPI_I2S_GetFlagStatus(TLE7232_SPI, SPI_I2S_FLAG_BSY) == SET) {
		/* The SPI transmit buffer has data, so wait patiently */
	}

	/* Deselect the TLE7232: Chip Select High */
	TLE7232_CS_HIGH();
	TLE7232_EvaluateDiagnosis();
}

/**
 * Sets the function pointer for updating the output fault status when detected.
 *
 * @param func SetOutputFaultStatus The function pointer.
 * @retval none
 */
void SetOutputFaultStatusFunction(SetOutputFaultStatus func) {
	SetFaultStatus = func;
}
