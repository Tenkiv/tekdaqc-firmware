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


/* Includes ------------------------------------------------------------------*/
#include "Tekdaqc_Config.h"
#include "stm32f4x7_eth.h"
#include "netconf.h"
#include "TelnetServer.h"
#include "Tekdaqc_Locator.h"
#include "Tekdaqc_CommandInterpreter.h"
#include "Tekdaqc_CalibrationTable.h"
#include "Tekdaqc_Calibration.h"
#include "Tekdaqc_RTC.h"
#include "CommandState.h"
#include "ADS1256_SPI_Controller.h"
#include "Tekdaqc_Error.h"
#include "Tekdaqc_Version.h"
#include <stdio.h>

/* Private variables ---------------------------------------------------------*/

#define CMDMaxLength 512

Tekdaqc_CommandInterpreter_t* interpreter;
char character;
TelnetStatus_t status;

/* Private functions ---------------------------------------------------------*/
static void program_loop(void);
static void Init_Locator();

/**
 * @brief Initializes all the Tekdaqc specific structures and hardware features.
 */
static void Tekdaqc_Init(void);

/**
 * @brief  Main program.
 * @param  None
 * @retval None
 */
int main(void) {
	/*!< At this stage the microcontroller clock setting is already configured to
	 168 MHz, this is done through SystemInit() function which is called from
	 startup file (startup_stm32f4xx.s) before to branch to application main.
	 To reconfigure the default setting of SystemInit() function, refer to
	 system_stm32f4xx.c file
	 */

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

#ifdef SERIAL_DEBUG /* Configure serial debugging. */
	DebugComPort_Init();
#endif
#ifdef DEBUG
	printf("\n\rSerial Port Initialized.\n\r");
#endif

	/* Enable the PWR APB1 Clock Interface */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	/* Allow access to BKP Domain */
	PWR_BackupAccessCmd(ENABLE);

	if ((RTC_ReadBackupRegister(RTC_CONFIGURED_REG ) & RTC_CONFIGURED)
			!= RTC_CONFIGURED) {
#ifdef DEBUG
		printf("[Main] Configuring the RTC domain.\n\r");
#endif
		/* Initialize the RTC and Backup registers */
		/* RTC_Config(); */

		/* Setup date and time */
		/* TODO: Set up date and time */
	} else {
		/* Wait for RTC APB registers synchronization */
		RTC_WaitForSynchro();
	}

	/* Setup Watchdog debug leds */

	/**
	 * Check if the system has resumed from a WWDG reset
	 */
	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST ) != RESET) {
		/* This is a watchdog reset */

		/* Clear the reset flags */
		RCC_ClearFlag();

		/* Do any watchdog reset specific stuff here */
		/* TODO: Watchdog reset specific handling */
	} else {
		/* This is not a watchdog reset */
		/* Do any normal reset stuff here */
		if (RCC_GetFlagStatus(RCC_FLAG_SFTRST )) {
			/* Software reset */
		} else if (RCC_GetFlagStatus(RCC_FLAG_PORRST )) {
			/* Power on Reset/Power down reset */
		} else if (RCC_GetFlagStatus(RCC_FLAG_PINRST )) {
			/* Reset pin reset */
		} else {
			/* Some other reset */
		}
	}

	/* Clear the reset flags */
	RCC_ClearFlag();

	/* Initialize the Tekdaqc's peripheral hardware */
	Tekdaqc_Init();

	Init_Locator();

	if (InitializeTelnetServer() == TELNET_OK) {
		CreateCommandInterpreter();
		program_loop();
	} else {
		/* We have a fatal error */
		/* Reset the board */
		NVIC_SystemReset();
	}
	return 0;
}

static void program_loop(void) {
	/* Infinite loop */
	while (1) {
		/* Service the inputs/outputs */
		ServiceTasks();

		/* Check if any packet received */
		if (ETH_CheckFrameReceived()) {
			/* Process received ethernet packet */
			LwIP_Pkt_Handle();
		}

		/* Handle periodic timers for LwIP */
		LwIP_Periodic_Handle(GetLocalTime());

		if (TelnetIsConnected() == true) { /* We have an active Telnet connection to service */
			/* Do server stuff */
			character = TelnetRead();
			if (character != '\0') {
				Command_AddChar(character);
			}
		}

		/* Check to see if any faults have occurred */
		Tekdaqc_CheckStatus();

		/* Reload the IWDG Counter to prevent reset */
		IWDG_ReloadCounter();
	}
}

static void Init_Locator() {
	/* Start the Tekdaqc Locator Service */
	Tekdaqc_LocatorInit();

	/* Set the firmware version */
	uint32_t version = 0;
	version |= (MAJOR_VERSION << 24);
	version |= (MINOR_VERSION << 16);
	version |= (BUILD_NUMBER << 8);
	version |= SPECIAL_BUILD;
	Tekdaqc_LocatorVersionSet(version);

	Tekdaqc_LocatorBoardIDSet(TEKDAQC_BOARD_SERIAL_NUM);
}

/**
 * Initializes Tekdaqc data structures and hardware peripherals.
 *
 * @param none
 * @retval none
 */
static void Tekdaqc_Init(void) {
	/* Initialize the Tekdaqc's communication methods */
	Communication_Init();

	/* Initialize the command state handler. This will initialize any */
	/* Input/Output state machines. */
	InitCommandStateHandler();

	/* Initialize the analog inputs */
	AnalogInputsInit();

	/* Initialize the digital inputs */
	DigitalInputsInit();

	/* Initialize the digital outputs */
	DigitalOutputsInit();

	/* Set the write functions */
	SetAnalogInputWriteFunction(&TelnetWriteString);
	SetDigitalInputWriteFunction(&TelnetWriteString);
	SetDigitalOutputWriteFunction(&TelnetWriteString);

	/* Initialize the FLASH disk */
	FlashDiskInit();

	/* Initialize the calibration table and fetch the board serial number */
	Tekdaqc_CalibrationInit();
	char data = '\0';
	uint32_t Address = BOARD_SERIAL_NUM_ADDR;
	for (int i = 0; i < BOARD_SERIAL_NUM_LENGTH; ++i) {
		if (Address >= CAL_TEMP_LOW_ADDR) {
			/* We have overflowed and there is a problem in the software */
#ifdef DEBUG
			printf("[Config] Reading board serial number overflowed.\n\r");
#endif
			break; /* Break out to prevent an invalid access */
		}
		data = *((__IO char*)Address);
		TEKDAQC_BOARD_SERIAL_NUM[i] = data;
		Address += sizeof(char);
	}
	TEKDAQC_BOARD_SERIAL_NUM[BOARD_SERIAL_NUM_LENGTH] = '\0'; /* Apply the NULL termination character */

#ifdef USE_WATCHDOG
	// Initialize the watchdog timer
	Watchdog_Init();
#endif
}

#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *   where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line) {
	/* User can add his own implementation to report the file name and line number,
 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	printf("Wrong parameters value: file %s on line %d\r\n", file, line);
	/* Infinite loop */
	while (1) {}
}
#endif
