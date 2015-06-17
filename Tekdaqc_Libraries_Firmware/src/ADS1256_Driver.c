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
 * @file ADS1256_Driver.c
 * @brief Controls the ADS1256 ADC via SPI.
 *
 * Controls the ADS1256 ADC via SPI. This driver retains a local copy of all of the devices registers which
 * can be used selectively. 
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "ADS1256_Driver.h"
#include "Tekdaqc_Config.h"
#include "Tekdaqc_Timers.h"

#include <string.h>
#include <stdlib.h>

#ifdef PRINTF_OUTPUT
#include <stdio.h>
#endif



/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @def ADS1256_NEGATIVE_FLAG
 * @brief Used to identify if a 24 bit unsigned int stored in a 32 bit unsigned int represents 2's compliment negative.
 */
#define ADS1256_NEGATIVE_FLAG ((uint32_t) 0x00800000U)

/**
 * @internal
 * @def ADS1256_NEGATIVE_PADDING
 * @brief Used to pad a 24 bit unsigned int which represents a 2's compliment negative number to a 32 bit int.
 */
#define ADS1256_NEGATIVE_PADDING ((uint32_t) 0xFF000000U)

/**
 * @internal
 * @def ADS1256BUFFER_OFF_STR
 * @brief String constant used for comparing to command line inputs for the state of the input buffer.
 */
#define ADS1256_BUFFER_OFF_STR		"OFF"

/**
 * @internal
 * @def ADS1256_BUFFER_DISABLED_STR
 * @brief String constant used for comparing to command line inputs for the state of the input buffer.
 */
#define ADS1256_BUFFER_DISABLED_STR "DISABLED"

/**
 * @internal
 * @def ADS1256_REGISTERS_DEBUG_FORMATTER
 * @brief Used as the format string for printing the ADS1256 registers to a string.
 */
#define ADS1256_REGISTERS_DEBUG_FORMATTER "[ADS1256] Register %2d %6s: 0x%02X\n\r"

/**
 * @internal
 * @def ADS1256_REGISTERS_TOSTRING_HEADER
 * @brief Used as a header for each block of analog measurement printing to string.
 */
#define ADS1256_REGISTERS_TOSTRING_HEADER "[ADS1256] Register Contents:\n\r"

extern void InitAnalogSamplesBuffer(void);


/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* Device ID. */
static uint8_t ID = (uint8_t) 0xFFU;

/* TODO: These need to be updated every time they are written out to the ADC */
/* Data Order. */
static ADS1256_ORDER_t ORDER = ADS1256_MSB_FIRST;
/* Auto-Calibration. */
static ADS1256_ACAL_t ACAL = ADS1256_ACAL_DISABLED;
/* Analog Buffer. */
static ADS1256_BUFFER_t BUFFER = ADS1256_BUFFER_DISABLED;

/* Analog inputs. */
static ADS1256_AIN_t AIN_POS = ADS1256_AIN0;
static ADS1256_AIN_t AIN_NEG = ADS1256_AIN1;

/* ADC Clock out setting. */
static ADS1256_CLOCK_OUT_t CLOCK_OUT = ADS1256_CLOCK_OUT_OFF;
/* ADC Sensor detect current output. */
static ADS1256_SENSOR_DETECT_t SENSOR_CURRENT = ADS1256_SD_OFF;
/* Programmable gain amplifier setting. */
static ADS1256_PGA_t PGA = ADS1256_PGAx1;

/* Samples per second setting. */
static ADS1256_SPS_t SPS = ADS1256_SPS_30000;

/* GPIO Port Directions. */
static ADS1256_GPIO_DIRECTION_t GPIO_DIRECTIONS[] = { ADS1256_GPIO_OUTPUT,
		ADS1256_GPIO_INPUT, ADS1256_GPIO_INPUT, ADS1256_GPIO_INPUT };
/* GPIO Port Status. */
static ADS1256_GPIO_STATUS_t GPIO_STATUS[] = { ADS1256_GPIO_LOW,
		ADS1256_GPIO_LOW, ADS1256_GPIO_LOW, ADS1256_GPIO_LOW };

/* Flag to indicate if command or pin should be used for SYNC of ADC. */
static bool SYNC_USE_COMMAND = false;

/* The last retrieved measurement from the ADC. */
static uint32_t ADS1256_Measurement = 0U;

/* The last retrieved state of the ADC register. */
static uint8_t ADS1256_Registers[11]; /* A local copy of all the ADC registers. Note that the indexing here only works because the addresses start at 0 and count up. */

/* Flag for if we should always read the register from the ADS1256 for most up to date values. */
static bool ADS1256_AlwaysReadReg = false;

/* Scratch string used for various string print operations. */
static char SCRATCH_STR[150];



/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @brief Returns a human readable string name for the requested register.
 */
static inline const char* ADS1256_StringFromRegister(ADS1256_Register_t reg);



/*--------------------------------------------------------------------------------------------------------*/
/* COMMAND METHODS */
/*--------------------------------------------------------------------------------------------------------*/
/**
 * @internal
 * @brief Sends a command which takes a register as a parameter.
 */
static void ADS1256_Reg_Command(ADS1256_Command_t cmd, ADS1256_Register_t reg, uint8_t count);

/**
 * @internal
 * @brief Sends a plain command.
 */
static void ADS1256_Send_Command(ADS1256_Command_t cmd);



/*--------------------------------------------------------------------------------------------------------*/
/* REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/
/**
 * @internal
 * @brief Retrieves the specified bits from a local register.
 */
static uint8_t ADS1256_GetRegisterBits(ADS1256_Register_t reg, uint8_t index, uint8_t count);

/**
 * @internal
 * @brief Sets the specified bits in a register (both local and remote).
 */
static void ADS1256_SetRegisterBits(ADS1256_Register_t reg, uint8_t index, uint8_t count, uint8_t value);

/**
 * @internal
 * @brief Retrieves the local copy of a register.
 */
static uint8_t ADS1256_GetRegister(ADS1256_Register_t reg);

/**
 * @internal
 * @brief Sets the contents of a register (both local and remote).
 */
static void ADS1256_SetRegister(ADS1256_Register_t reg, uint8_t value);

/**
 * @internal
 * @brief Sets the contents of one or more registers (both local and remote).
 */
static void ADS1256_SetRegisters(ADS1256_Register_t reg, uint8_t count, uint8_t* values);

/**
 * @internal
 * @brief Fetches a remote register and updates the local copy.
 */
static void ADS1256_ReadRegister(ADS1256_Register_t reg);

/**
 * @internal
 * @brief Fetches one or more remote registers and updates the local copies.
 */
static void ADS1256_ReadRegisters(ADS1256_Register_t reg, uint8_t count);

/**
 * @internal
 * @brief Pushes local register to remote register.
 */
static void ADS1256_WriteRegister(ADS1256_Register_t reg);

/**
 * @internal
 * @brief Pushes one or more local registers to remote registers.
 */
static void ADS1256_WriteRegisters(ADS1256_Register_t reg, uint8_t count);



/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* INITIALIZATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Initializes the ADS1256 driver and establishes proper SPI serial communication with the device. A full
 * reset of the device which consists of a reset via clock sequence, followed by a reset of the SPI module.
 * Following this, all registers of the device are read and the ADC is left waiting in the SYNC state. All
 * peripherals of the STM32 involved are enabled and configured.
 *
 * @param none
 * @retval none
 */
void ADS1256_Init(void) {
	ADS1256_SPI_Init(); /* Initialize the SPI lines */
	ADS1256_StatePins_Init(); /* Initialize the control pins */
	ADS1256_Full_Reset(); /* Perform a full reset on the ADC */
	ADS1256_ReadRegisters(ADS1256_STATUS, ADS1256_NREGS); /* Read out all the registers */
	ADS1256_Sync(true); /* SYNC the ADC so it isn't free running by sending the SPI command */
#ifdef ADS1256_DEBUG
	ADS1256_PrintRegs(); /* Print out the registers */
#endif
}




void ADS1256_EXTI_Enable(void) {
	// Clear interrupt pending bit
	EXTI_ClearITPendingBit(EXTI_Line10);
	NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void ADS1256_EXTI_Disable(void) {
	NVIC_DisableIRQ(EXTI15_10_IRQn);
}

//lfao- this timer is used for very short delay purposes...this does not trigger an interrupt...
//lfao- this is currently configured at 1 count=1us...
//lfao- NOTE: when doing short delays(i.e., reading the counter), user must handle the special
//lfao- case where the counter is near the reload value of 0xFFFFFFF0...
void InitializeShortDelayTimer(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitTypeDef ShortDelayTimer;
    ShortDelayTimer.TIM_Prescaler = 83;
    ShortDelayTimer.TIM_CounterMode = TIM_CounterMode_Up;
    ShortDelayTimer.TIM_Period = 0xFFFFFFFF;
    ShortDelayTimer.TIM_ClockDivision = TIM_CKD_DIV1;
    ShortDelayTimer.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &ShortDelayTimer);
    TIM_Cmd(TIM2, ENABLE);
}
//lfao-new timer used for the channel switching...
//currently configured to trigger an interrupt every 3ms...
void InitializeChannelSwitchTimer(void)
{

    NVIC_InitTypeDef nVIC_InitStructure;
    nVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
    nVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    nVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    nVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nVIC_InitStructure);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    TIM_TimeBaseInitTypeDef ChnSwitchTimer;
    ChnSwitchTimer.TIM_Prescaler = 41999;
    ChnSwitchTimer.TIM_CounterMode = TIM_CounterMode_Up;
    ChnSwitchTimer.TIM_Period = 5;
    ChnSwitchTimer.TIM_ClockDivision = TIM_CKD_DIV1;
    ChnSwitchTimer.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM4, &ChnSwitchTimer);


    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
}
volatile int totalDelay = 0;
void ShortDelayUS(uint32_t Delay)
{
	totalDelay = totalDelay+Delay;
	//lfao- set counter to 0
	TIM_SetCounter(TIM2,0);
	while(TIM_GetCounter(TIM2) < Delay)
	{

	}
}
/**
 * Initializes the STM32 pins connected to the ADS1256 state pins such as data ready (DRDY) and reset.
 */
void ADS1256_StatePins_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure; /* Structure used to initialize the GPIO pins */
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Confgure the DRDY Pin */
	/* Enable the GPIO Clock */
	RCC_AHB1PeriphClockCmd(ADS1256_DRDY_GPIO_CLK, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	/* Configure the GPIO pin */
	GPIO_InitStructure.GPIO_Pin = ADS1256_DRDY_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(ADS1256_DRDY_GPIO_PORT, &GPIO_InitStructure);

	/* Connect EXTI Line to INT Pin */
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource10);
	/* Configure EXTI line */
	EXTI_InitStructure.EXTI_Line = EXTI_Line10;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	/* Enable and set the EXTI interrupt to priority 1*/
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Confgure the SYNC Pin */
	/* Enable the GPIO Clock */
	RCC_AHB1PeriphClockCmd(ADS1256_SYNC_GPIO_CLK, ENABLE);

	/* Configure the GPIO pin */
	GPIO_InitStructure.GPIO_Pin = ADS1256_SYNC_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(ADS1256_SYNC_GPIO_PORT, &GPIO_InitStructure);

	/* Bring the SYNC pin high */
	GPIO_SetBits(ADS1256_SYNC_GPIO_PORT, ADS1256_SYNC_PIN);

	/* Confgure the RESET Pin */
	/* Enable the GPIO Clock */
	RCC_AHB1PeriphClockCmd(ADS1256_RESET_GPIO_CLK, ENABLE);

	/* Configure the GPIO pin */
	GPIO_InitStructure.GPIO_Pin = ADS1256_RESET_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(ADS1256_RESET_GPIO_PORT, &GPIO_InitStructure);

	/* Bring the RESET pin high */
	GPIO_SetBits(ADS1256_RESET_GPIO_PORT, ADS1256_RESET_PIN);

	//lfao- during init, disable the interrupt at first...
	ADS1256_EXTI_Disable();

	//lfao- initialize the buffer for analog samples...
	InitAnalogSamplesBuffer();
}




/*--------------------------------------------------------------------------------------------------------*/
/* STRING METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Retrieves a human readable string representation of the requester register's name.
 *
 * @param reg ADS1256_Register_t The register to retrieve the name of.
 * @retval const char* The name of the register.
 */
static inline const char* ADS1256_StringFromRegister(ADS1256_Register_t reg) {
	static const char* strings[] = { "STATUS", "MUX", "ADCON", "DRATE", "IO",
			"OFC0", "OFC1", "OFC2", "FSC0", "FSC1", "FSC2" };
	return strings[reg];
}

/**
 * Prints a neatly formated series of statements to the stdout of the program consisting of the current local
 * copy of all of the ADS1256's registers.
 *
 * @param none
 * @retval none
 */
void ADS1256_PrintRegs(void) {
	ADS1256_ReadRegisters(ADS1256_STATUS, ADS1256_NREGS); /* Read out all the registers */
	for (uint_fast8_t i = 0; i < ADS1256_NREGS; ++i) {
		printf(ADS1256_REGISTERS_DEBUG_FORMATTER, i, ADS1256_StringFromRegister(i), ADS1256_Registers[i]);
	}
}

/**
 * Prints a neatly formated version of the contents of the local version of all the ADC's registers to the global to string buffer.
 * The calling functions are expected to free this memory when they are finished.
 *
 * @param none
 * @retval none
 */
void ADS1256_RegistersToString(void) {
	ADS1256_ReadRegisters(ADS1256_STATUS, ADS1256_NREGS); /* Read out all the registers */
#ifdef ADS1256_DEBUG
	ADS1256_PrintRegs(); /* If we are debugging this module, print to stdout as well. */
#endif
	strncat(TOSTRING_BUFFER, ADS1256_REGISTERS_TOSTRING_HEADER, SIZE_TOSTRING_BUFFER); /* Append the header first */
	uint8_t n = 0;
	for (uint_fast8_t i = 0U; i < ADS1256_NREGS; ++i) { /* For each register */
		/* Print the register string to the scratch string, with overflow safety. This will truncate the string if necessary. */
		n = snprintf(SCRATCH_STR, sizeof(SCRATCH_STR), ADS1256_REGISTERS_DEBUG_FORMATTER, i, ADS1256_StringFromRegister(i), ADS1256_Registers[i]);
		if (n > 0) { /* Writing was successful */
			/* Append the scratch string to the output string */
			strncat(TOSTRING_BUFFER, SCRATCH_STR, (SIZE_TOSTRING_BUFFER - strlen(TOSTRING_BUFFER) - 1U)); /* Reserve one for the terminating NULL */
		} else {
			/* We failed to write anything to the string */
#ifdef ADS1256_DEBUG
			printf("[ADS1256] Failed to write register %s to the output string.\n\r", ADS1256_StringFromRegister(i));
#endif
		}
	}
}

/**
 * Returns a human readable string representation of the requested data rate.
 *
 * @param sps ADS1256_SPS_t The data rate.
 * @retval const char* The string representation.
 */
const char* ADS1256_StringFromSPS(ADS1256_SPS_t sps) {
	static const char* strings[] = { "30,000", "15,000", "7,500", "3,750",
			"2,000", "1,000", "500", "100", "60", "50", "30", "25", "15", "10",
			"5", "2.5", "INVALID" };
	switch (sps) {
	case ADS1256_SPS_30000:
		return strings[0];
	case ADS1256_SPS_15000:
		return strings[1];
	case ADS1256_SPS_7500:
		return strings[2];
	case ADS1256_SPS_3750:
		return strings[3];
	case ADS1256_SPS_2000:
		return strings[4];
	case ADS1256_SPS_1000:
		return strings[5];
	case ADS1256_SPS_500:
		return strings[6];
	case ADS1256_SPS_100:
		return strings[7];
	case ADS1256_SPS_60:
		return strings[8];
	case ADS1256_SPS_50:
		return strings[9];
	case ADS1256_SPS_30:
		return strings[10];
	case ADS1256_SPS_25:
		return strings[11];
	case ADS1256_SPS_15:
		return strings[12];
	case ADS1256_SPS_10:
		return strings[13];
	case ADS1256_SPS_5:
		return strings[14];
	case ADS1256_SPS_2_5:
		return strings[15];
	default:
		return strings[0];
	}
}

/**
 * Returns a human readable string representation of the requested gain setting.
 *
 * @param pga ADS1256_PGA_t The gain setting.
 * @retval const char* The string representation.
 */
const char* ADS1256_StringFromPGA(ADS1256_PGA_t pga) {
	static const char* strings[] = { "x1", "x2", "x4", "x8", "x16", "x32", "x64", "INVALID" };
	switch (pga) {
	case ADS1256_PGAx1:
		return strings[0];
	case ADS1256_PGAx2:
		return strings[1];
	case ADS1256_PGAx4:
		return strings[2];
	case ADS1256_PGAx8:
		return strings[3];
	case ADS1256_PGAx16:
		return strings[4];
	case ADS1256_PGAx32:
		return strings[5];
	case ADS1256_PGAx64:
		return strings[6];
	default:
		return strings[7];
	}
}

/**
 * Returns a human readable string of the requested buffer setting.
 *
 * @param buffer ADS1256_BUFFER_t The buffer setting.
 * @retval const char* The string representation.
 */
const char* ADS1256_StringFromBuffer(ADS1256_BUFFER_t buffer) {
	static const char* strings[] = { "DISABLED", "ENABLED", "INVALID" };
	switch (buffer) {
	case ADS1256_BUFFER_DISABLED:
		return strings[0];
	case ADS1256_BUFFER_ENABLED:
		return strings[1];
	default:
		return strings[2];
	}
}

/**
 * Converts a human readable string into the appropriate ADS1256_BUFFER_t value.
 *
 * @param str char* The human readable string.
 * @retval ADS1256_BUFFER_t value correlating to the string.
 */
ADS1256_BUFFER_t ADS1256_StringToBuffer(char* str) {
	if (strcmp(str, ADS1256_BUFFER_OFF_STR) == 0 || strcmp(str, ADS1256_BUFFER_DISABLED_STR) == 0) {
		return ADS1256_BUFFER_DISABLED;
	} else {
		return ADS1256_BUFFER_ENABLED;
	}
}

/**
 * Converts a human readable string into the appropriate ADS1256_SPS_t value.
 *
 * @param str char* The human readable string.
 * @retval ADS1256_SPS_t value correlating to the string.
 */
ADS1256_SPS_t ADS1256_StringToDataRate(char* str) {
	int_least16_t i = (int_least16_t) strtol(str, NULL, 10);
	switch (i) {
	case 30000:
		return ADS1256_SPS_30000;
	case 15000:
		return ADS1256_SPS_15000;
	case 7500:
		return ADS1256_SPS_7500;
	case 3750:
		return ADS1256_SPS_3750;
	case 2000:
		return ADS1256_SPS_2000;
	case 1000:
		return ADS1256_SPS_1000;
	case 500:
		return ADS1256_SPS_500;
	case 100:
		return ADS1256_SPS_100;
	case 60:
		return ADS1256_SPS_60;
	case 50:
		return ADS1256_SPS_50;
	case 30:
		return ADS1256_SPS_30;
	case 25:
		return ADS1256_SPS_25;
	case 15:
		return ADS1256_SPS_15;
	case 10:
		return ADS1256_SPS_10;
	case 5:
		return ADS1256_SPS_5;
	case 2:
		return ADS1256_SPS_2_5;
	default:
		printf("[ERROR] ADS1256_StringToDataRate(char* str): Invalid parameter (%s), providing 30000 SPS.\n\r", str);
		return ADS1256_SPS_30000;
	}
}

/**
 * Converts a human readable string into the appropriate ADS1256_PGA_t value.
 *
 * @param str char* The human readable string.
 * @retval ADS1256_PGA_t value correlating to the string.
 */
ADS1256_PGA_t ADS1256_StringToPGA(char* str) {
	int_least16_t i = (int_least16_t) strtol(str, NULL, 10);
	switch (i) {
	case 1:
		return ADS1256_PGAx1;
	case 2:
		return ADS1256_PGAx2;
	case 4:
		return ADS1256_PGAx4;
	case 8:
		return ADS1256_PGAx8;
	case 16:
		return ADS1256_PGAx16;
	case 32:
		return ADS1256_PGAx32;
	case 64:
		return ADS1256_PGAx64;
	default:
		printf("[ERROR] ADS1256_StringToPGA(char* str): Invalid parameter (%s), providing x1.\n\r", str);
		return ADS1256_PGAx1;
	}
}



/*--------------------------------------------------------------------------------------------------------*/
/* RESET METHODS */
/*--------------------------------------------------------------------------------------------------------*/
/**
 * Performs a full reset of the ADS1256 ADC. This consists of a clock sequence reset followed by a reset of the
 * ADC's SPI port. This sequence should be called whenever there is reason to suspect that the state of the
 * ADC is unknown.
 *
 * @param none
 * @param none
 */
void ADS1256_Full_Reset(void) {
	ADS1256_Reset_By_Pin(); /* Reset the ADC */
	ADS1256_Reset_SPI(); /* Reset the SPI line */
	ADS1256_Sync(true); /* SYNC the ADC via SPI */
}

/**
 * Performs a hardware reset of the ADS1256 ADC. This is done by sending a RESET command via SPI. This method
 * requires that correct communication with the ADC exists and can not be expected to recover state or communication
 * with the ADC.
 *
 * @param none
 * @retval none
 */
void ADS1256_Reset_By_Command(void) {
	ID = 0xFFU; /* Reset the stored ID so we can properly identify the ADC after reset */
	ADS1256_CS_LOW(); /* Enable SPI communication */
	ADS1256_Send_Command(ADS1256_RESET); /* Send the reset command */
	Delay_us(8 * ADS1256_CLK_PERIOD_US); /* timing characteristic t10 */
	ADS1256_CS_HIGH(); /* Latch the SPI communication */
	ADS1256_WaitUntilDataReady(false); /* Wait until the ADC reports that data is ready via the DRDY pin. */
}

/**
 * Performs a hardware reset of the ADS1256 ADC. This is done by sending a specially timed sequence via the
 * SPI clock line. This method can be used whether valid communication with the ADC is present or not.
 *
 * @param none
 * @retval none
 */
void ADS1256_Reset_By_Clock(void) {
	ID = 0xFFU; /* Reset the stored ID so we can properly identify the ADC after reset. */
	ADS1256_CLK_To_GPIO(); /* Change the clock line from SPI mode to GPIO mode */
	ADS1256_SCLK_LOW(); /* Start the clock reset sequence. */
	Delay_us(10U); /* Start low */
	ADS1256_SCLK_HIGH(); /* First high pulse */
	Delay_us((uint64_t) (300U * ADS1256_CLK_PERIOD_US)); /* Timing characteristic t12, tuned because of timing errors */
	ADS1256_SCLK_LOW(); /* First low pulse */
	Delay_us((uint64_t) (5U * ADS1256_CLK_PERIOD_US)); /* Timing characteristic t13 */
	ADS1256_SCLK_HIGH(); /* Second high pulse */
	Delay_us((uint64_t) (550U * ADS1256_CLK_PERIOD_US)); /* Timing characteristic t14, tuned because of timing errors */
	ADS1256_SCLK_LOW(); /* Second low pulse */
	Delay_us((uint64_t) (5U * ADS1256_CLK_PERIOD_US)); /* Timing characteristic t13 */
	ADS1256_SCLK_HIGH(); /* Third high pulse */
	Delay_us((uint64_t) (1050U * ADS1256_CLK_PERIOD_US)); /* Timing characteristic t15, tuned because of timing errors */
	ADS1256_SCLK_LOW(); /* End the clock reset sequence. */
	ADS1256_GPIO_To_CLK(); /* Return the clock line to SPI mode. */
}

/**
 * Performs a hardware reset of the ADS1256 ADC. This is done by taking the reset pin low for the required amount
 * of time. This method can be used whether valid communication with the ADC is present or not and is the method of
 * choice when there is need to reset the ADC regardless of its state. This method specifies the minimum amount of
 * delay but in all likelihood the overhead of the delay will cause it to be significantly longer.
 *
 * @param none
 * @retval none
 */
void ADS1256_Reset_By_Pin(void) {
	GPIO_ResetBits(ADS1256_RESET_GPIO_PORT, ADS1256_RESET_PIN);
	Delay_ms(0.0006f); /* Timing characteristic t16 */
	GPIO_SetBits(ADS1256_RESET_GPIO_PORT, ADS1256_RESET_PIN);
}

/**
 * Performs a reset of the SPI serial port of the ADS1256 ADC. This is done by holding the SPI clock line low
 * for an extended period of time. Note that this can be used to recover lost communication with the ADC but
 * not to return it to a predictable state.
 *
 * @param none
 * @retval none
 */
void ADS1256_Reset_SPI(void) {
	ADS1256_SCLK_LOW();
	/* Delay for at least 32 /DRDY periods */
	/* For simplicity, we will simply delay for 0.5 seconds */
	for (uint_fast8_t i = 0U; i < 32U; ++i) {
		Delay_ms(ADS1256_GetSettlingTime());
	}
}

/**
 * Performs a complete reset of the ADS1256 and restores its state to the current settings such as PGA, data rate
 * and multiplexer setting.
 *
 * @param none
 * @retval none
 */
void ADS1256_ResetAndReprogram(void) {
	ADS1256_Full_Reset(); /* Perform the full reset */
	ADS1256_ReadRegisters(ADS1256_STATUS, ADS1256_NREGS); /* Read out all registers */

	/* Set PGA and data rate first */
#ifdef ADS1256_DEBUG
	printf("\n\rResetting ADC Parameters:\n\r");
	/*printf("\tRate: %s\n\r", ADS1256_StringFromSPS(SPS));
	printf("\tGain: %s\n\r", ADS1256_StringFromPGA(PGA));
	printf("\tBuffer: %s\n\r", ADS1256_StringFromBuffer(BUFFER));
	printf("\t")*/
#endif
	ADS1256_SetInputBufferSetting(BUFFER);
	ADS1256_SetDataRate(SPS);
	ADS1256_SetPGASetting(PGA);

	/* Now set other stuff */
	ADS1256_SetInputChannels(AIN_POS, AIN_NEG);
	ADS1256_SetAutoCalSetting(ACAL);
	ADS1256_SetDataOutputBitOrder(ORDER);
	ADS1256_SetClockOutRate(CLOCK_OUT);
	ADS1256_SetSensorDetectCurrent(SENSOR_CURRENT);

	/* GPIO Stuff */
	for (uint_fast8_t i = 0U; i < 4U; ++i) {
		ADS1256_SetGPIODirection(i, GPIO_DIRECTIONS[i]);
		if (GPIO_DIRECTIONS[i] == ADS1256_GPIO_OUTPUT) {
			ADS1256_SetGPIOStatus(i, GPIO_STATUS[i]);
		}
	}

#ifdef ADS1256_DEBUG
	printf("ADC Registers after reset:\n\r");
	ADS1256_PrintRegs();
#endif
}



/*--------------------------------------------------------------------------------------------------------*/
/* AQUISITION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Reads out the most recent data from the ADC and converts it to a 32 bit signed integer.
 *
 * @param none
 * @return int32_t The converted ADC reading.
 */
int32_t ADS1256_GetMeasurement(void) {
	/*  Retrieve data */
	uint8_t raw_data[3];
	ADS1256_ReadData(raw_data);

	/*  Send the ADC into SYNC mode via command */
	ADS1256_Sync(true);

	/*  Combine the 3 bytes into one unsigned int */
	ADS1256_Measurement = raw_data[0] << 16U;
	ADS1256_Measurement |= raw_data[1] << 8U;
	ADS1256_Measurement |= raw_data[2];
	return ADS1256_ConvertRawValue(ADS1256_Measurement);
}

/**
 * Initiates an conversion measurement.
 *
 * @param none
 * @retval none
 */
void ADS1256_RequestMeasurement(void) {
	ADS1256_Wakeup(); /* Send WAKEUP command byte */
}

/**
 * Read the 3 raw data bytes from the ADC.
 *
 * @param none
 * @retval none
 */
void ADS1256_ReadData(uint8_t* data) {
	ADS1256_CS_LOW(); /* Enable SPI communication */
	ADS1256_SendByte(ADS1256_RDATA); /* Send RDATA command byte */
	Delay_us((uint64_t) (50U * ADS1256_CLK_PERIOD_US)); /*  timing characteristic t6 */
	ADS1256_ReceiveBytes(data, 3U);
	Delay_us(8 * ADS1256_CLK_PERIOD_US); /* timing characteristic t10 */
	ADS1256_CS_HIGH(); /* Latch SPI communication */
	Delay_us((uint64_t) (4U * ADS1256_CLK_PERIOD_US)); /*  timing characteristic t11 */
}

/**
 * Blocks processing (except interrupts) until it is seen that the ADC has valid data.
 *
 * @param useCommand bool If true, indicates that the SPI command should be used rather than the DRDY line.
 * @retval none
 */
void ADS1256_WaitUntilDataReady(bool useCommand) {
	while (!ADS1256_IsDataReady(useCommand)) { /* Wait in a loop */
		// Do nothing
	}
}

/**
 * Checks to see if the ADC has valid data ready for a read. Allows optional use of SPI command or DRDY pin.
 *
 * @param useCommand bool If true, indicates that the SPI command should be used rather than the DRDY line.
 * @retval bool True if valid data is ready, false if not.
 */
bool ADS1256_IsDataReady(bool useCommand) {
	/* TODO: This method should be smart enough to determine if the DRDY pin has been enabled and default to SPI if not. */
	if (useCommand) { /* If we are using the SPI command */
		ADS1256_ReadRegister(ADS1256_STATUS); /* Read the status register from the device */
		uint8_t byte = ADS1256_GetRegisterBits(ADS1256_STATUS, ADS1256_DRDY_BIT, ADS1256_DRDY_SPAN); /* Retrieve the DRDY bit from the register */
		bool ready = false; /* Assume data is not ready */
		if (byte == ADS1256_DATA_READY) {
			ready = true; /* Data is ready. DRDY active low */
		}
#ifdef ADS1256_DEBUG
		printf("[ADS1256] ADS1256 Data ready: 0x%02X\n\r", byte);
#endif
		return ready;
	} else {
		/* The intention is that false = 0 = data ready so we negate the logic */
		return !GPIO_ReadInputDataBit(ADS1256_DRDY_GPIO_PORT, ADS1256_DRDY_PIN);
	}
}

/**
 * Instructs the ADC to enter the SYNC state. Allows the optional use of SPI command or SYNC pin. Note
 * that if the SYNC pin is held in the SYNC state for a sufficient period of time, the ADS1256 will enter
 * the power down (PWDN) state.
 *
 * @param useCommand bool If true, indicates that the SPI command should be used rather than the DRDY line.
 * @retval none
 */
void ADS1256_Sync(bool useCommand) {
	/* TODO: This method should be smart enough to determine if the SYNC pin has been enabled and default to SPI if not. */
	SYNC_USE_COMMAND = useCommand;
	if (useCommand) {
		ADS1256_Send_Command(ADS1256_SYNC); /* Send SYNC command byte */
		/* TODO: Write SYNC pin high */
	} else {
		/* TODO: use sync pin */
	}
	Delay_us((uint64_t) (24U * ADS1256_CLK_PERIOD_US)); /* Timing characteristic t11 for SYNC */
}

/**
 * Wakes up the ADC from SYNC, PWDN or STANDBY modes. This method will use either SPI commands or
 * the relevant control pin, depending upon how the state was entered. This will initiate a conversion.
 *
 * @param none
 * @retval none
 */
void ADS1256_Wakeup(void) {
	if (SYNC_USE_COMMAND == true) { /* If we should use SPI */
		ADS1256_Send_Command(ADS1256_WAKEUP); /* Send WAKEUP command byte */
		SYNC_USE_COMMAND = false;
	} else {
		/* TODO: Handle this wakeup */
	}
}

/**
 * Puts the ADC into STANDBY mode.
 *
 * @param none
 * @retval none
 */
void ADS1256_Standby(void) {
	ADS1256_Send_Command(ADS1256_STANDBY); /* Send the STANDBY command byte */
	SYNC_USE_COMMAND = true;
}



/*--------------------------------------------------------------------------------------------------------*/
/* CALIBRATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Performs a complete self calibration of the ADC. This method blocks (except interrupts) until the calibration
 * is complete.
 *
 * @param none
 * @retval none
 */
void ADS1256_CalibrateSelf(void) {
	ADS1256_Send_Command(ADS1256_SELFCAL); /* Send the self cal command */
	ADS1256_WaitUntilDataReady(false); /* Wait until the ADC signals it is finished */
	ADS1256_Sync(true); /* Enter the SYNC state */
	ADS1256_Wakeup(); /* Wakeup the ADC */
}

/**
 * Performs a self gain calibration of the ADC. This method blocks (except interrupts) until the calibration
 * is complete.
 *
 * @param none
 * @retval none
 */
void ADS1256_CalibrateSelf_Gain(void) {
	ADS1256_Send_Command(ADS1256_SELFGCAL); /* Send the self gain cal command */
	ADS1256_WaitUntilDataReady(false); /* Wait until the ADC signals it is finished */
	ADS1256_Sync(true); /* Enter the SYNC state */
	ADS1256_Wakeup(); /* Wakeup the ADC */
}

/**
 * Performs a self offset calibration of the ADC. This method blocks (except interrupts) until the calibration
 * is complete.
 *
 * @param none
 * @retval none
 */
void ADS1256_CalibrateSelf_Offset(void) {
	ADS1256_Send_Command(ADS1256_SELFOCAL); /* Send the self offset cal command */
	ADS1256_WaitUntilDataReady(false); /* Wait until the ADC signals it is finished */
	ADS1256_Sync(true); /* Enter the SYNC state */
	ADS1256_Wakeup(); /* Wakeup the ADC */
}

/**
 * Performs a system gain calibration. This will account for the entire analog input path to the ADC. Requires the application
 * of a full scale (accounting for gain) external signal. This method blocks (except interrupts) until the calibration
 * is complete.
 *
 * @param none
 * @retval none
 */
void ADS1256_CalibrateSystem_Gain(void) {
	ADS1256_Send_Command(ADS1256_SYSGCAL); /* Send the system gain cal command */
	ADS1256_WaitUntilDataReady(false); /* Wait until the ADC signals it is finished */
	ADS1256_Sync(true); /* Enter the SYNC state */
	ADS1256_Wakeup(); /* Wakeup the ADC */
}

/**
 * Performs a system offset calibration. This will account for the entire analog input path to the ADC. Requires the application
 * of a 0 value input (shorted) external signal. This method blocks (except interrupts) until the calibration is complete.
 *
 * @param none
 * @retval none
 */
void ADS1256_CalibrateSystem_Offset(void) {
	ADS1256_Send_Command(ADS1256_SYSOCAL); /* Send the system offset cal command */
	ADS1256_WaitUntilDataReady(false); /* Wait until the ADC signals it is finished */
	ADS1256_Sync(true); /* Enter the SYNC state */
	ADS1256_Wakeup(); /* Wakeup the ADC */
}



/*--------------------------------------------------------------------------------------------------------*/
/* UTILITY METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Converts the raw, 24 bit 2's compliment data output of the ADC into a native signed, 32 bit integer type.
 *
 * @param value uint32_t The raw output of the ADC
 * @retval int32_t The converted value of the ADC
 */
int32_t ADS1256_ConvertRawValue(uint32_t value) {
#ifdef ADS1256_DEBUG
	printf("[ADS1256] Converting raw value: 0x%08X / %i\n\r", value, value);
#endif
	/*  Convert unsigned int to signed int explicitly so the compiler doesn't do something unexpected. */
	if ((value & ADS1256_NEGATIVE_FLAG) != 0U){
		/* It is a negative number */
		value |= ADS1256_NEGATIVE_PADDING;
	}
	return ((int32_t) value);
}

/**
 * Retrieves the total self calibration time in milliseconds. See table 21.
 *
 * @param none
 * @retval float The calibration time in milliseconds.
 */
float ADS1256_GetSelfCalTime(void) {
	float retval = 0.892f;
	switch (SPS) {
	case ADS1256_SPS_30000:
		switch (PGA) {
		case ADS1256_PGAx1:
		case ADS1256_PGAx2:
			retval = 0.596f;
			break;
		case ADS1256_PGAx4:
			retval = 0.692f;
			break;
		case ADS1256_PGAx8:
			retval = 0.696f;
			break;
		case ADS1256_PGAx16:
		case ADS1256_PGAx32:
		case ADS1256_PGAx64:
			retval = 0.892f;
			break;
		default:
			retval = 0.892f;
		}
		break;
		case ADS1256_SPS_15000:
			switch (PGA) {
			case ADS1256_PGAx1:
			case ADS1256_PGAx2:
			case ADS1256_PGAx4:
				retval = 0.696f;
				break;
			case ADS1256_PGAx8:
				retval = 0.762f;
				break;
			case ADS1256_PGAx16:
			case ADS1256_PGAx32:
			case ADS1256_PGAx64:
				retval = 0.896f;
				break;
			default:
				 retval = 0.896f;
			}
			break;
			case ADS1256_SPS_7500:
				switch (PGA) {
				case ADS1256_PGAx1:
				case ADS1256_PGAx2:
				case ADS1256_PGAx4:
				case ADS1256_PGAx8:
					retval = 0.896f;
					break;
				case ADS1256_PGAx16:
				case ADS1256_PGAx32:
				case ADS1256_PGAx64:
					retval = 1.029f;
					break;
				default:
					retval = 1.029f;
					break;
				}
				break;
				case ADS1256_SPS_3750:
					retval = 1.3f;
					break;
				case ADS1256_SPS_2000:
					retval = 2.0f;
					break;
				case ADS1256_SPS_1000:
					retval = 3.6f;
					break;
				case ADS1256_SPS_500:
					retval = 6.6f;
					break;
				case ADS1256_SPS_100:
					retval = 31.2f;
					break;
				case ADS1256_SPS_60:
					retval = 50.9f;
					break;
				case ADS1256_SPS_50:
					retval = 61.8f;
					break;
				case ADS1256_SPS_30:
					retval = 101.3f;
					break;
				case ADS1256_SPS_25:
					retval = 123.2f;
					break;
				case ADS1256_SPS_15:
					retval = 202.1f;
					break;
				case ADS1256_SPS_10:
					retval = 307.2f;
					break;
				case ADS1256_SPS_5:
					retval = 613.8f;
					break;
				case ADS1256_SPS_2_5:
					retval = 1227.2f;
					break;
				default:
					retval = 1227.2f;
	}
	return retval;
}

/**
 * Retrieves the offset (self and system) calibration time in milliseconds. See table 19.
 *
 * @param none
 * @retval float The calibration time in milliseconds.
 */
float ADS1256_GetOffsetCalTime(void) {
	switch (SPS) {
	case ADS1256_SPS_30000:
		return 0.387f;
	case ADS1256_SPS_15000:
		return 0.453f;
	case ADS1256_SPS_7500:
		return 0.587f;
	case ADS1256_SPS_3750:
		return 0.853f;
	case ADS1256_SPS_2000:
		return 1.3f;
	case ADS1256_SPS_1000:
		return 2.3f;
	case ADS1256_SPS_500:
		return 4.3f;
	case ADS1256_SPS_100:
		return 20.3f;
	case ADS1256_SPS_60:
		return 33.7f;
	case ADS1256_SPS_50:
		return 40.3f;
	case ADS1256_SPS_30:
		return 67.0f;
	case ADS1256_SPS_25:
		return 80.3f;
	case ADS1256_SPS_15:
		return 133.7f;
	case ADS1256_SPS_10:
		return 200.3f;
	case ADS1256_SPS_5:
		return 400.3f;
	case ADS1256_SPS_2_5:
		return 800.3f;
	default:
		return 800.3f;
	}
}

/**
 * Retrieves the gain self calibration time in milliseconds. See table 20.
 *
 * @param none
 * @retval float The calibration time in milliseconds.
 */
float ADS1256_GetSelfGainCalTime(void) {
	switch (SPS) {
	case ADS1256_SPS_30000:
		switch (PGA) {
		case ADS1256_PGAx1:
		case ADS1256_PGAx2:
			return 0.417f;
		case ADS1256_PGAx4:
			return 0.451f;
		case ADS1256_PGAx8:
			return 0.517f;
		case ADS1256_PGAx16:
		case ADS1256_PGAx32:
		case ADS1256_PGAx64:
			return 0.651f;
		default:
			return 0.651f;
		}
		break;
		case ADS1256_SPS_15000:
			switch (PGA) {
			case ADS1256_PGAx1:
			case ADS1256_PGAx2:
			case ADS1256_PGAx4:
				return 0.484f;
			case ADS1256_PGAx8:
			case ADS1256_PGAx16:
			case ADS1256_PGAx32:
			case ADS1256_PGAx64:
				return 0.551f;
			default:
				return 0.551f;
			}
			break;
			case ADS1256_SPS_7500:
				switch (PGA) {
				case ADS1256_PGAx1:
				case ADS1256_PGAx2:
				case ADS1256_PGAx4:
				case ADS1256_PGAx8:
					return 0.617f;
				case ADS1256_PGAx16:
				case ADS1256_PGAx32:
				case ADS1256_PGAx64:
					return 0.751f;
				default:
					return 0.751f;
				}
				break;
				case ADS1256_SPS_3750:
					return 0.884f;
				case ADS1256_SPS_2000:
					return 1.4f;
				case ADS1256_SPS_1000:
					return 2.4f;
				case ADS1256_SPS_500:
					return 4.5f;
				case ADS1256_SPS_100:
					return 21.0f;
				case ADS1256_SPS_60:
					return 34.1f;
				case ADS1256_SPS_50:
					return 41.7f;
				case ADS1256_SPS_30:
					return 67.8f;
				case ADS1256_SPS_25:
					return 83.0f;
				case ADS1256_SPS_15:
					return 135.3f;
				case ADS1256_SPS_10:
					return 207.0f;
				case ADS1256_SPS_5:
					return 413.7f;
				case ADS1256_SPS_2_5:
					return 827.0f;
				default:
					return 827.0f;
	}
}

/**
 * Retrieves the system gain calibration time in milliseconds. See table 22.
 *
 * @param none
 * @retval float The calibration time in milliseconds.
 */
float ADS1256_GetSystemGainCalTime(void) {
	switch (SPS) {
	case ADS1256_SPS_30000:
		return 0.417f;
	case ADS1256_SPS_15000:
		return 0.484f;
	case ADS1256_SPS_7500:
		return 0.617f;
	case ADS1256_SPS_3750:
		return 0.884f;
	case ADS1256_SPS_2000:
		return 1.4f;
	case ADS1256_SPS_1000:
		return 2.4f;
	case ADS1256_SPS_500:
		return 4.4f;
	case ADS1256_SPS_100:
		return 20.4f;
	case ADS1256_SPS_60:
		return 33.7f;
	case ADS1256_SPS_50:
		return 40.4f;
	case ADS1256_SPS_30:
		return 67.0f;
	case ADS1256_SPS_25:
		return 80.4f;
	case ADS1256_SPS_15:
		return 133.7f;
	case ADS1256_SPS_10:
		return 200.4f;
	case ADS1256_SPS_5:
		return 400.4f;
	case ADS1256_SPS_2_5:
		return 800.4f;
	default:
		return 800.4f;
	}
}

/**
 * Retrieves the settling time of the ADC for the current sampling configuration. Determined by timing characteristics t18.
 *
 * @param none
 * @retval float The settling time of the ADC in milliseconds.
 */
float ADS1256_GetSettlingTime(void) {
	ADS1256_GetDataRate(); /* Update the register if we need to */
	switch (SPS) {
	case ADS1256_SPS_30000:
		return 0.21f;
	case ADS1256_SPS_15000:
		return 0.25f;
	case ADS1256_SPS_7500:
		return 0.31f;
	case ADS1256_SPS_3750:
		return 0.44f;
	case ADS1256_SPS_2000:
		return 0.68f;
	case ADS1256_SPS_1000:
		return 1.18f;
	case ADS1256_SPS_500:
		return 2.18f;
	case ADS1256_SPS_100:
		return 10.18f;
	case ADS1256_SPS_60:
		return 16.84f;
	case ADS1256_SPS_50:
		return 20.18f;
	case ADS1256_SPS_30:
		return 33.51f;
	case ADS1256_SPS_25:
		return 40.18f;
	case ADS1256_SPS_15:
		return 66.84f;
	case ADS1256_SPS_10:
		return 100.18f;
	case ADS1256_SPS_5:
		return 200.18f;
	case ADS1256_SPS_2_5:
		return 400.18f;
	default:
#ifdef ADS1256_DEBUG
		printf("[ADS1256] Failed to look up settling time for 0x%02X!\n\r", SPS);
#endif
		return 0;
	}
}

/**
 * Selects the remote register auto-fetch behavior of the driver.
 *
 * @param always bool If true, any register action will automatically update the local registers to match the remote registers first.
 * @retval none
 */
void ADS1256_AlwayFetch(bool always) {
	ADS1256_AlwaysReadReg = always;
}



/*--------------------------------------------------------------------------------------------------------*/
/* STATUS REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Retrieves the factory programmed ID from the ADC.
 *
 * @param none
 * @retval uint8_t The ADC ID.
 */
uint8_t ADS1256_GetFactoryProgrammedID(void) {
	ADS1256_ReadRegister(ADS1256_STATUS);
	uint8_t byte = ADS1256_GetRegisterBits(ADS1256_STATUS, ADS1256_ID_BIT,
			ADS1256_ID_SPAN);
#ifdef ADS156_DEBUG
	printf("[ADS1256] Device ID: 0x%02X\n\r", byte);
#endif
	return byte;
}

/**
 * Retrieves the data output bit order from the ADC.
 *
 * @param none
 * @retval ADS1256_ORDER_t The data output bit order.
 */
ADS1256_ORDER_t ADS1256_GetDataOutputBitOrder(void) {
	ADS1256_ReadRegister(ADS1256_STATUS);
	ADS1256_ORDER_t order = ADS1256_GetRegisterBits(ADS1256_STATUS,
			ADS1256_ORDER_BIT, ADS1256_ORDER_SPAN);
#ifdef ADS1256_DEBUG
	printf("[ADS1256] Output bit order: 0x%02X\n\r", order);
#endif
	return order;
}

/**
 * Retrieves the ADC's auto calibration setting.
 *
 * @param none
 * @retval ADS1256_ACAL_t The auto calibration setting.
 */
ADS1256_ACAL_t ADS1256_GetAutoCalSetting(void) {
	ADS1256_ReadRegister(ADS1256_STATUS);
	ADS1256_ACAL_t acal = ADS1256_GetRegisterBits(ADS1256_STATUS,
			ADS1256_ACAL_BIT, ADS1256_ACAL_SPAN);
#ifdef ADS1256_DEBUG
	printf("[ADS1256] Auto calibration setting: 0x%02X\n\r", acal);
#endif
	return acal;
}

/**
 * Retrieves the input buffer setting.
 *
 * @param none
 * @retval ADS1256_BUFFER_t The input buffer setting.
 */
ADS1256_BUFFER_t ADS1256_GetInputBufferSetting(void) {
	ADS1256_ReadRegister(ADS1256_STATUS);
	ADS1256_BUFFER_t buffer = ADS1256_GetRegisterBits(ADS1256_STATUS,
			ADS1256_BUFFEN_BIT, ADS1256_BUFFEN_SPAN);
#ifdef ADS1256_DEBUG
	printf("[ADS1256] Input buffer setting: 0x%02X\n\r", buffer);
#endif
	return buffer;
}

/**
 * Sets the data output bit order on the ADC. This will update both the local and remote registers.
 *
 * @param order ADS1256_ORDER_t The data output bit order.
 * @retval none
 */
void ADS1256_SetDataOutputBitOrder(ADS1256_ORDER_t order) {
	ADS1256_SetRegisterBits(ADS1256_STATUS, ADS1256_ORDER_BIT,
			ADS1256_ORDER_SPAN, order);
}

/**
 * Sets the auto calibration setting on the ADC. This will update both the local and remote registers.
 *
 * @param acal ADS1256_ACAL_t The auto calibration setting.
 * @retval none
 */
void ADS1256_SetAutoCalSetting(ADS1256_ACAL_t acal) {
	ADS1256_SetRegisterBits(ADS1256_STATUS, ADS1256_ACAL_BIT, ADS1256_ACAL_SPAN, acal);
}

/**
 * Sets the input buffer setting on the ADC. This will update both the local and remote registers.
 *
 * @param buffer ADS1256_BUFFER_t The buffer setting on the ADC.
 * @retval none
 */
void ADS1256_SetInputBufferSetting(ADS1256_BUFFER_t buffer) {
	ADS1256_SetRegisterBits(ADS1256_STATUS, ADS1256_BUFFEN_BIT,
			ADS1256_BUFFEN_SPAN, buffer);
}



/*--------------------------------------------------------------------------------------------------------*/
/* MUX REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Retrieves the current multiplexer setting and determines the currently selected input channels.
 *
 * @param none
 * @retval none
 */
void ADS1256_GetInputChannels(void) {
	ADS1256_ReadRegister(ADS1256_MUX);
	uint8_t val = ADS1256_GetRegister(ADS1256_MUX);
	AIN_NEG = val & 0x0F;
	AIN_POS = val >> 4;
}

/**
 * Sets the current multiplexer setting to the specified input channels.
 *
 * @param pos ADS1256_AIN_t The high side analog input channel.
 * @param neg ADS1256_AIN_t The low side analog input channel.
 * @retval none
 */
void ADS1256_SetInputChannels(ADS1256_AIN_t pos, ADS1256_AIN_t neg) {
	assert_param(IS_ADS1256_AIN_SETTING(pos));
	assert_param(IS_ADS1256_AIN_SETTING(neg));
	uint8_t reg = (uint8_t)((pos << 4U) | neg);
	if (ADS1256_GetRegister(ADS1256_MUX) != reg) { /* If the register differs, then update it */
		ADS1256_SetRegister(ADS1256_MUX, reg);
	}
}



/*--------------------------------------------------------------------------------------------------------*/
/* ADCON REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Retrieve the ADC clock out rate setting.
 *
 * @param none
 * @retval ADS1256_CLOCK_OUT_t The clock out rate setting.
 */
ADS1256_CLOCK_OUT_t ADS1256_GetClockOutRate(void) {
	ADS1256_ReadRegister(ADS1256_ADCON);
	ADS1256_CLOCK_OUT_t clock = ADS1256_GetRegisterBits(ADS1256_ADCON, ADS1256_CO_BIT, ADS1256_CO_SPAN);
	assert_param(IS_ADS1256_CLOCKOUT_SETTING(clock));
	CLOCK_OUT = clock;
#ifdef ADS1256_DEBUG
	printf("[ADS1256] Clock out rate: 0x%02X\n\r", clock);
#endif
	return clock;
}

/**
 * Retrieve the sensor detect current output setting.
 *
 * @param none
 * @retval ADS1256_SENSOR_DETECT_t The sensor detect current output setting.
 */
ADS1256_SENSOR_DETECT_t ADS1256_GetSensorDetectCurrent(void) {
	ADS1256_ReadRegister(ADS1256_ADCON);
	ADS1256_SENSOR_DETECT_t current = ADS1256_GetRegisterBits(ADS1256_ADCON,
			ADS1256_SD_BIT, ADS1256_SD_SPAN);
	assert_param(IS_ADS1256_SENSOR_DETECT_SETTING(current));
	SENSOR_CURRENT = current;
#ifdef ADS1256_DEBUG
	printf("[ADS1256] Sensor detect current: 0x%02X\n\r", current);
#endif
	return current;
}

/**
 * Retrieve the ADC PGA gain setting.
 *
 * @param none
 * @retval ADS1256_PGA_t The PGA gain setting.
 */
ADS1256_PGA_t ADS1256_GetPGASetting(void) {
	ADS1256_ReadRegister(ADS1256_ADCON);
	ADS1256_PGA_t setting = ADS1256_GetRegisterBits(ADS1256_ADCON, ADS1256_PGA_BIT, ADS1256_PGA_SPAN);
	assert_param(IS_ADS1256_PGA_SETTING(setting));
	PGA = setting;
#ifdef ADS1256_DEBUG
	printf("[ADS1256] PGA Setting: 0x%02X\n\r", setting);
#endif
	return setting;
}

/**
 * Set the clock out rate of the ADC.
 *
 * @param clock ADS1256_CLOCK_OUT_t The clock out setting to apply.
 * @retval none
 */
void ADS1256_SetClockOutRate(ADS1256_CLOCK_OUT_t clock) {
	assert_param(IS_ADS1256_CLOCKOUT_SETTING(clock));
	ADS1256_SetRegisterBits(ADS1256_ADCON, ADS1256_CO_BIT, ADS1256_CO_SPAN, clock);
	CLOCK_OUT = clock;
}

/**
 * Set the sensor detect source output current.
 *
 * @param current ADS1256_SENSOR_DETECT_t The source output current setting.
 */
void ADS1256_SetSensorDetectCurrent(ADS1256_SENSOR_DETECT_t current) {
	assert_param(IS_ADS1256_SENSOR_DETECT_SETTING(current));
	ADS1256_SetRegisterBits(ADS1256_ADCON, ADS1256_SD_BIT, ADS1256_SD_SPAN, current);
	SENSOR_CURRENT = current;
}

/**
 * Set the ADC PGA gain value.
 *
 * @param gain ADS1256_PGA_t The PGA gain setting.
 * @retval none
 */
void ADS1256_SetPGASetting(ADS1256_PGA_t gain) {
	/* Check the parameters */
	assert_param(IS_ADS1256_PGA_SETTING(gain));
	ADS1256_SetRegisterBits(ADS1256_ADCON, ADS1256_PGA_BIT, ADS1256_PGA_SPAN, gain);
	PGA = gain;
}

/**
 * Retrieve the correct gain multiplier for the provided gain setting.
 *
 * @param gain ADS1256_PGA_t The gain setting to fetch.
 * @return int32_t The numeric value represented by gain.
 */
int32_t ADS1256_GetGainMultiplier(ADS1256_PGA_t gain) {
	int32_t retval = 1;
	switch (gain) {
	case ADS1256_PGAx1:
		retval = 1;
		break;
	case ADS1256_PGAx2:
		retval = 2;
		break;
	case ADS1256_PGAx4:
		retval = 4;
		break;
	case ADS1256_PGAx8:
		retval = 8;
		break;
	case ADS1256_PGAx16:
		retval = 16;
		break;
	case ADS1256_PGAx32:
		retval = 32;
		break;
	case ADS1256_PGAx64:
		retval = 64;
		break;
	default:
		retval = 1;
	}
	return retval;
}



/*--------------------------------------------------------------------------------------------------------*/
/* DRATE REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Retrieve the current data rate (sample rate) setting.
 *
 * @param none
 * @retval ADS125_SPS_t The data (sample) rate setting.
 */
ADS1256_SPS_t ADS1256_GetDataRate(void) {
	ADS1256_ReadRegister(ADS1256_DRATE);
	ADS1256_SPS_t setting = ADS1256_GetRegister(ADS1256_DRATE);
	if (setting) {
		SPS = setting;
	}
#ifdef ADS1256_DEBUG
	printf("[ADS1256] Data rate: 0x%02X\n\r", setting);
#endif
	return SPS;
}

/**
 * Set the current data rate (sample rate) setting.
 *
 * @param sps ADS1256_SPS_t The data (sample) rate setting to apply.
 * @retval none
 */
void ADS1256_SetDataRate(ADS1256_SPS_t sps) {
	/* Check the parameters */
	ADS1256_SetRegister(ADS1256_DRATE, sps);
	SPS = sps;
}



/*--------------------------------------------------------------------------------------------------------*/
/* GPIO REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Retrieve the direction setting for the specified GPIO pin.
 *
 * @param pin ADS1256_GPIO_t The GPIO pin to get the direction of.
 * @retval ADS1256_GPIO_DIRECTION_t The direction setting of the specified GPIO pin.
 */
ADS1256_GPIO_DIRECTION_t ADS1256_GetGPIODirection(ADS1256_GPIO_t pin) {
	ADS1256_ReadRegister(ADS1256_IO);
	ADS1256_GPIO_DIRECTION_t dir = ADS1256_GetRegisterBits(ADS1256_IO, pin + ADS1256_GPIO_DIR_OFFSET, ADS1256_GPIO_BIT_SPAN);
	GPIO_DIRECTIONS[pin] = dir;
	return dir;
}

/**
 * Retrieve the current logic state for the specified GPIO pin.
 *
 * @param pin ADS1256_GPIO_t The GPIO pin to get the direction of.
 * @retval ADS1256_GPIO_STATUS_t The current logic state of the specified GPIO pin.
 */
ADS1256_GPIO_STATUS_t ADS1256_GetGPIOStatus(ADS1256_GPIO_t pin) {
	ADS1256_ReadRegister(ADS1256_IO);
	ADS1256_GPIO_STATUS_t status = ADS1256_GetRegisterBits(ADS1256_IO, pin, ADS1256_GPIO_BIT_SPAN);
	assert_param(IS_ADS1256_GPIO_VALUE(status));
	GPIO_STATUS[pin] = status;
	return status;
}

/**
 * Set the direction of the specified GPIO pin.
 *
 * @param pin ADS1256_GPIO_t The GPIO pin to set the direction of.
 * @param direction ADS1256_GPIO_DIRECTION_t The direction to set for the GPIO pin.
 * @retval none
 */
void ADS1256_SetGPIODirection(ADS1256_GPIO_t pin, ADS1256_GPIO_DIRECTION_t direction) {
	ADS1256_SetRegisterBits(ADS1256_IO, pin + ADS1256_GPIO_DIR_OFFSET,ADS1256_GPIO_BIT_SPAN, direction);
	GPIO_DIRECTIONS[pin] = direction;
}

/**
 * Sets the logic state for the specified GPIO pin. Note that this will only have an effect if the direction
 * of the pin is set to be ADS1256_GPIO_OUTPUT.
 *
 * @param pin ADS126_GPIO_t The GPIO pin to set the state of.
 * @param status ADS1256_GPIO_STATUS_t The logic state to apply to the pin.
 * @retval none
 */
void ADS1256_SetGPIOStatus(ADS1256_GPIO_t pin, ADS1256_GPIO_STATUS_t status) {
	assert_param(IS_ADS1256_GPIO_VALUE(status));
	ADS1256_SetRegisterBits(ADS1256_IO, pin, ADS1256_GPIO_BIT_SPAN, status);
	GPIO_STATUS[pin] = status;
}



/*--------------------------------------------------------------------------------------------------------*/
/* OFFSET CALIBRATION REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Retrieve the offset calibration register value.
 *
 * @param none
 * @retval uint32_t The offset calibration value, exactly as it is stored by the ADC but converted to a single number.
 */
uint32_t ADS1256_GetOffsetCalSetting(void) {
	uint32_t setting = 0x00000000;
	uint32_t temp = 0x00000000;
	ADS1256_ReadRegisters(ADS1256_OFC0, 3);
	temp |= ADS1256_GetRegister(ADS1256_OFC2);
	temp <<= 16;
	setting |= temp;
	temp = 0x00000000;
	temp |= ADS1256_GetRegister(ADS1256_OFC1);
	temp <<= 8;
	setting |= temp;
	setting |= ADS1256_GetRegister(ADS1256_OFC0);
	return setting;
}

/**
 * Set the offset calibration registers.
 *
 * @param value uint8_t* Pointer to 3 uint8_t values to apply to each of the 3 offset calibration registers.
 * @retval none
 */
void ADS1256_SetOffsetCalSetting(uint8_t* value) {
	ADS1256_SetRegisters(ADS1256_OFC0, 3, value);
}



/*--------------------------------------------------------------------------------------------------------*/
/* GAIN CALIBRATION REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Retrieve the gain calibration register value.
 *
 * @param none
 * @retval uint32_t The gain calibration value, exactly as it is stored by the ADC but converted to a single number.
 */
uint32_t ADS1256_GetGainCalSetting(void) {
	uint32_t setting = 0x00000000;
	uint32_t temp = 0x00000000;
	ADS1256_ReadRegisters(ADS1256_FSC0, 3);
	temp |= ADS1256_GetRegister(ADS1256_FSC2);
	temp <<= 16;
	setting |= temp;
	temp = 0x00000000;
	temp |= ADS1256_GetRegister(ADS1256_FSC1);
	temp <<= 8;
	setting |= temp;
	setting |= ADS1256_GetRegister(ADS1256_FSC0);
	return setting;
}

/**
 * Set the gain calibration registers.
 *
 * @param value uint8_t* Pointer to 3 uint8_t values to apply to each of the 3 gain calibration registers.
 * @retval none
 */
void ADS1256_SetGainCalSetting(uint8_t* value) {
	ADS1256_SetRegisters(ADS1256_FSC0, 3, value);
}



/*--------------------------------------------------------------------------------------------------------*/
/* COMMAND METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Sends a command byte to the ADC followed by the specified register parameters for the command.
 *
 * @param cmd ADS1256_Command_t The command to send the ADC.
 * @param reg ADS1256_Register_t The starting register for the command.
 * @param count uint8_t The number of registers to apply the command to.
 * @retval none
 */
static void ADS1256_Reg_Command(ADS1256_Command_t cmd, ADS1256_Register_t reg, uint8_t count) {
	assert_param(IS_ADS1256_REGISTER_COMMAND(cmd));
	uint8_t cmds[] = { (cmd | reg), 0x0F & (count - 1) };
	ADS1256_SendBytes(cmds, 2);
}

/**
 * Sends a command byte to the ADC with no parameters.
 *
 * @param cmd ADS1256_Command_t The command to send the ADC.
 * @retval none
 */
static void ADS1256_Send_Command(ADS1256_Command_t cmd) {
	ADS1256_CS_LOW();
	ADS1256_SendByte(cmd);
	Delay_us(8 * ADS1256_CLK_PERIOD_US); /* timing characteristic t10 */
	ADS1256_CS_HIGH();
}



/*--------------------------------------------------------------------------------------------------------*/
/* REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Retrieves the specified bits from a local register.
 *
 * @param reg ADS1256_Register_t The register to retrieve the bits from.
 * @param index uint8_t The starting bit to retrieve.
 * @param count uint8_t The number of bits to retrieve. Must be 8 or less.
 * 	Behavior is undefined if this maximum is exceeded following ISO 9899:1999 6.5.7.
 */
static uint8_t ADS1256_GetRegisterBits(ADS1256_Register_t reg, uint8_t index, uint8_t count) {
	uint8_t byte = ADS1256_GetRegister(reg);
#ifdef ADS1256_DEBUG
	printf("[ADS1256] Get register bits (Register %6s, Index: %i, Count: %i): Original 0x%02X\n\r", ADS1256_StringFromRegister(reg), index, count, byte);
#endif
	uint8_t mask = (uint8_t) (0xFFU >> (8U - count));
	byte = (byte >> index) & mask;
#ifdef ADS1256_DEBUG
	printf("[ADS1256] Register bits after 0x%02X\n\r", byte);
#endif
	return byte;
}

/**
 * Set the specified bits of a local register. This will automatically push the changes to the remote register.
 *
 * @param reg ADS1256_Register_t The register to set the bits of.
 * @param index uint8_t The starting bit to retrieve.
 * @param count uint8_t The number of bits to retrieve. index + count must be less than 8 or the function will
 * 		clamp the range to avoid overruning the register.
 * @param value uint8_t The value to set the bits to. It is expected that this will be right justified.
 */
static void ADS1256_SetRegisterBits(ADS1256_Register_t reg, uint8_t index, uint8_t count, uint8_t value) {
	/* Ensure that the count will not overrun the register */
	if ((count - index) >= 8U) {
		count = 7U - index;
	}
	/* Ensure that there is no extraneous data in value */
	value &= ~(0xFF << count);
	/* Get the most recent value of the register */
	uint8_t byte = ADS1256_GetRegister(reg);
	/* If the value to be set is identical to the existing one, no action. */
	if (ADS1256_GetRegisterBits(reg, index, count) == value) {
		return;
	}
	uint8_t mask = 0x00;
	for (uint_fast8_t i = 0; i < count; ++i) {
		mask |= 0x01 << (index + i);
	}
	//uint8_t mask = (uint8_t)((0xFFU >> count) << index); /* Create the mask, all 1's shifted left by the starting bit index */
	byte = (byte & ~mask) | (value << index);
	ADS1256_SetRegister(reg, byte);
}

/**
 * Retrieve the local copy of a register. If an up to date value is needed then
 * a read should be requested first.
 *
 * @param reg ADS1256_Register_t The register to retrieve.
 * @retval uint8_t The retrieved register value.
 */
static uint8_t ADS1256_GetRegister(ADS1256_Register_t reg) {
	return ADS1256_Registers[reg];
}

/**
 * Set the contents of a register, both local and remote.
 *
 * @param reg ADS1256_Register_t The register to set the value of.
 * @param value uint8_t The value to apply to the register.
 * @retval none
 */
static void ADS1256_SetRegister(ADS1256_Register_t reg, uint8_t value) {
	ADS1256_SetRegisters(reg, 1, &value);
}

/**
 * Set the contents of multiple registers, both local and remote.
 *
 * @param reg ADS1256_Register_t The register to start the writing at.
 * @param count uint8_t The number of registers to write.
 * @param values uint8_t* Pointer to array of values to write to the registers.
 * @retval none
 */
static void ADS1256_SetRegisters(ADS1256_Register_t reg, uint8_t count, uint8_t* values) {
	for (uint_fast8_t i = 0; i < count; ++i) {
		ADS1256_Registers[reg + i] = values[i];
	}
	ADS1256_WriteRegisters(reg, count);
}

/**
 * Fetch a remote register and update the local copy of the register.
 *
 * @param reg ADS1256_Register_t The register to read and update.
 * @retval none
 */
static void ADS1256_ReadRegister(ADS1256_Register_t reg) {
	ADS1256_ReadRegisters(reg, 1);
}

/**
 * Fetch a set of remote registers and update the local copies.
 *
 * @param reg ADS1256_Register_t The register to start the read and update at.
 * @param count uint8_t The number of registers to update.
 * @retval none
 */
static void ADS1256_ReadRegisters(ADS1256_Register_t reg, uint8_t count) {
	ADS1256_CS_LOW();
	DisableBoardInterrupts();
	ADS1256_Reg_Command(ADS1256_RREG, reg, count);
	EnableBoardInterrupts();
	Delay_us(50 * ADS1256_CLK_PERIOD_US); /* timing characteristic t6 */
	DisableBoardInterrupts();
	ADS1256_ReceiveBytes(ADS1256_Registers + reg, count);
	EnableBoardInterrupts();
	Delay_us(8 * ADS1256_CLK_PERIOD_US); /* timing characteristic t10 */
	ADS1256_CS_HIGH();
	Delay_us(4 * ADS1256_CLK_PERIOD_US); /* timing characteristic t11 */
	if (reg == ADS1256_STATUS) {
		/* We just read the status register, lets make sure things match up */
		if (ID == 0xFF) {
			/* This is the first read */
			ID = ADS1256_GetRegisterBits(ADS1256_STATUS, ADS1256_ID_BIT, ADS1256_ID_SPAN);
#ifdef ADS1256_DEBUG
			printf("[ADS1256] Setting ADC ID: 0x%02X\n\r", ID);
#endif
		} else {
			/* This is a subsequent read */
			uint8_t tempID = ADS1256_GetRegisterBits(ADS1256_STATUS,
					ADS1256_ID_BIT, ADS1256_ID_SPAN);
			if (tempID != ID) {
				/* There was a problem, reset the ADC */
#ifdef ADS1256_DEBUG
				printf("[ADS1256] The ADC did not return the same ID, resetting and reprogramming it.\n\r");
#endif
				ADS1256_ResetAndReprogram();
			}
		}
	}
}

/**
 * Write the contents of a local register to its corresponding remote register.
 *
 * @param reg ADS1256_Register_t The register to push to the ADC.
 * @retval none
 */
static void ADS1256_WriteRegister(ADS1256_Register_t reg) {
	ADS1256_WriteRegisters(reg, 1);
}

/**
 * Write the contents of multiple local registers to their corresponding remote registers.
 *
 * @param reg ADS1256_Register_t The register to start pushing at.
 * @param count uint8_t The number of registers to push to the ADC.
 * @retval none
 */
static void ADS1256_WriteRegisters(ADS1256_Register_t reg, uint8_t count) {
	ADS1256_CS_LOW();
	DisableBoardInterrupts();
	ADS1256_Reg_Command(ADS1256_WREG, reg, count);
	EnableBoardInterrupts();
	Delay_us(50 * ADS1256_CLK_PERIOD_US); /* timing characteristic t6 */
	DisableBoardInterrupts();
	ADS1256_SendBytes(ADS1256_Registers + reg, count);
	EnableBoardInterrupts();
	Delay_us(8 * ADS1256_CLK_PERIOD_US); /* timing characteristic t10 */
	ADS1256_CS_HIGH();
	Delay_us(4 * ADS1256_CLK_PERIOD_US); /* timing characteristic t11 */
}

/**
 * Send the SPI clock line to the ADC low. The function of the pin must be set to GPIO for this to function.
 *
 * @param none
 * @retval none
 */
void ADS1256_SCLK_LOW(void) {
	GPIO_ResetBits(ADS1256_SPI_SCK_GPIO_PORT, ADS1256_SPI_SCK_PIN );
}

/**
 * Send the SPI clock line to the ADC high. The function of the pin must be set to GPIO for this to function.
 *
 * @param none
 * @retval none
 */
void ADS1256_SCLK_HIGH(void) {
	GPIO_SetBits(ADS1256_SPI_SCK_GPIO_PORT, ADS1256_SPI_SCK_PIN );
}
