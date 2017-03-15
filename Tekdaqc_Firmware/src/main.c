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
#include "stm32f4x7_eth.h"
#include "netconf.h"
#include "stm32f4xx_it.h"
#include "TelnetServer.h"
#include "Tekdaqc_CalibrationTable.h"
#include "Tekdaqc_Calibration.h"
#include "Tekdaqc_CommandInterpreter.h"
#include "Tekdaqc_Config.h"
#include "Tekdaqc_Error.h"
#include "Tekdaqc_Locator.h"
#include "Tekdaqc_RTC.h"
#include "Tekdaqc_Timers.h"
#include "Tekdaqc_Version.h"
#include "CommandState.h"
#include "ADS1256_SPI_Controller.h"
#include "ADS1256_Driver.h"
#include "lwip/tcp.h"
#include "Digital_Input.h"
#include "Digital_Output.h"
#include <stdio.h>
#include <inttypes.h>

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

	if ((RTC_ReadBackupRegister(RTC_CONFIGURED_REG) & RTC_CONFIGURED) != RTC_CONFIGURED) {
#ifdef DEBUG
		printf("[Main] Configuring the RTC domain.\n\r");
#endif
		/* Initialize the RTC and Backup registers */
		RTC_Config(RTC_SYNCH_PRESCALER, RTC_ASYNCH_PRESCALER);
	} else {
#ifdef DEBUG
		printf("[Main] RTC domain configured. Waiting for synchronization.\n\r");
#endif
		/* Wait for RTC APB registers synchronization */
		RTC_WaitForSynchro();
	}

	/* Setup Watchdog debug leds */

	/**
	 * Check if the system has resumed from a WWDG reset
	 */
	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET) {
		/* This is a watchdog reset */

		/* Clear the reset flags */
		RCC_ClearFlag();

		/* Do any watchdog reset specific stuff here */
		/* TODO: Watchdog reset specific handling */
	} else {
		/* This is not a watchdog reset */
		/* Do any normal reset stuff here */
		if (RCC_GetFlagStatus(RCC_FLAG_SFTRST)) {
			/* Software reset */
		} else if (RCC_GetFlagStatus(RCC_FLAG_PORRST)) {
			/* Power on Reset/Power down reset */
		} else if (RCC_GetFlagStatus(RCC_FLAG_PINRST)) {
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
		Tekdaqc_Initialized(true);
		initializeSlowNet();
		InitializePwmTimer();

		//lfao: do calibration here since no more state based loop...
		//ADC_Machine_SetState(ADC_UNINITIALIZED);
		//ADC_Machine_Init();
		//ADC_Machine_Idle();
		//PerformSystemCalibration();
		//while(isSelfCalibrated==false)
		//{
		//  ADC_Machine_Service_Calibrating();
		//}

		program_loop();
	} else {
		/* We have a fatal error */
		/* Reset the board */
		NVIC_SystemReset();
	}
	return 0;
}

extern slowNet_t slowNetwork;
uint64_t slowNetTime = 0;
extern uint16_t pwmOutput;
static void program_loop(void) {
	/* Infinite loop */
	while (1)
	{
		/* Service the inputs/outputs */
		if(isSelfCalibrated==false)
		{
			ServiceTasks();
		}
		/* Check if any packet received */
		if (ETH_CheckFrameReceived()) {
			/* Process received Ethernet packet */
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
		//lfao - write to telnet the analog samples data...
		WriteToTelnet_Analog();
		//lfao - read digital inputs
		if (slowNetwork.digiRate && slowNetwork.digiSamp) {
			if ((GetLocalTime() - slowNetTime) >= slowNetwork.digiSamp) {
				slowNetTime = GetLocalTime();
				ReadDigitalInputs();
				calcDigiRate(slowNetwork.digiRate);
			}
		}
		else {
			ReadDigitalInputs();
		}
		//lfao - write to telnet the digital inputs data...
		WriteToTelnet_Digital();
		SetPwm(pwmOutput);
	}

	/* Check to see if any faults have occurred */
	/*Tekdaqc_CheckStatus();*/ //TODO: Re-enable when digital output faults work
	/* Reload the IWDG Counter to prevent reset */
	IWDG_ReloadCounter();
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
	//lfao- initialize short delay timer...
	InitializeShortDelayTimer();
	//lfao- initialize the timer for channel switching...
	InitializeChannelSwitchTimer();

	/* Initialize the FLASH disk */
	FlashDiskInit();

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

	/* Initialize the calibration table and fetch the board serial number */
	Tekdaqc_CalibrationInit();
	char data = '\0';
	uint32_t Address = BOARD_SERIAL_NUM_ADDR;
	for (int i = 0; i < BOARD_SERIAL_NUM_LENGTH; ++i) {
		data = *((__IO char*) Address);
		TEKDAQC_BOARD_SERIAL_NUM[i] = data;
		Address += sizeof(char);
	}
	TEKDAQC_BOARD_SERIAL_NUM[BOARD_SERIAL_NUM_LENGTH] = '\0'; /* Apply the NULL termination character */
#ifdef DEBUG
	printf("Initialized board serial number: %s\n\r", TEKDAQC_BOARD_SERIAL_NUM);
#endif

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
