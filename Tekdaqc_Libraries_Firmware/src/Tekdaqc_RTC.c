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
 * @file Tekdaqc_RTC.c
 * @brief Interfaces to the Tekdaqc's real time clock (RTC).
 *
 * Interfaces to the Tekdaqc's real time clock (RTC) peripheral allowing for the creation of alarm
 * interrupts as well as accurate timing/timestamps. 
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "Tekdaqc_RTC.h"
#include "Tekdaqc_BSP.h"

#ifdef PRINTF_OUTPUT
#include <stdio.h>
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Configure the RTC peripheral by selecting the clock source.
 * ck_spre(1Hz) = RTCCLK(LSE) /(asynchPrediv + 1)*(synchPrediv + 1)
 *
 * @param none
 * @retval none
 */
void RTC_Config(uint32_t synch_prediv, uint32_t asynch_prediv) {
#ifdef DEBUG
	printf("Configuring real time clock.\n\r");
#endif
	/* Allocate the init structure */
	RTC_InitTypeDef RTC_InitStructure;

	/* Enable the PWR clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	/* Allow access to RTC */
	PWR_BackupAccessCmd(ENABLE);

	/* Enable the LSE OSC */
	RCC_LSEConfig(RCC_LSE_ON );

	/* Wait till LSE is ready */
	while (RCC_GetFlagStatus(RCC_FLAG_LSERDY ) == RESET) {
		/* Do nothing */
	}

	/* Select the RTC Clock Source */
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE );

	/* Configure the RTC data register and RTC prescaler */
	RTC_InitStructure.RTC_AsynchPrediv = synch_prediv;
	RTC_InitStructure.RTC_SynchPrediv = asynch_prediv;
	RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;

	/* Check on RTC init */
	if (RTC_Init(&RTC_InitStructure) == ERROR) {
#ifdef DEBUG
		printf("[Tekdaqc RTC] RTC Prescaler Config failed.\n\r");
#endif
	}

	/* Enable the RTC Clock */
	RCC_RTCCLKCmd(ENABLE);

	/* Wait for RTC APB registers synchronization */
	RTC_WaitForSynchro();

	uint32_t reg = RTC_ReadBackupRegister(RTC_CONFIGURED_REG );
	reg |= RTC_CONFIGURED;
	RTC_WriteBackupRegister(RTC_CONFIGURED_REG, reg);
}
