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

	/* Enable the PWR APB1 Clock Interface */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	/* Allow access to BKP Domain */
	PWR_BackupAccessCmd(ENABLE);

#ifdef USE_LSE
	/* Enable the LSE OSC */
	RCC_LSEConfig(RCC_LSE_ON);

	/* Wait till LSE is ready */
	while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {
		/* Do nothing */
	}

	/* Select the RTC Clock Source */
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
#else
	/* Enable the LSI OSC */
	RCC_LSICmd(ENABLE);

	/* Wait till LSI is ready */
	while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) {
	}

	/* Select the RTC Clock Source */
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
#endif

	/* Configure the RTC data register and RTC prescaler */
	RTC_InitStructure.RTC_AsynchPrediv = asynch_prediv;
	RTC_InitStructure.RTC_SynchPrediv = synch_prediv;
	RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;

	/* Check on RTC init */
	if (RTC_Init(&RTC_InitStructure) == ERROR) {
#ifdef DEBUG
		printf("[Tekdaqc RTC] RTC Prescaler Config failed.\n\r");
#endif
	}

	/* Enable the RTC Clock */
	RCC_RTCCLKCmd(ENABLE);

	/* Wait for RTC APB registers synchronisation */
	RTC_WaitForSynchro();

	/* Zero the time value */
	RTC_ZeroTime();

	uint32_t reg = RTC_ReadBackupRegister(RTC_CONFIGURED_REG);
	reg |= RTC_CONFIGURED;
	RTC_WriteBackupRegister(RTC_CONFIGURED_REG, reg);
}

void RTC_ZeroTime(void) {

	RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;

	/* Set Time hh:mm:ss */
	RTC_TimeStructure.RTC_H12 = RTC_H12_AM;
	RTC_TimeStructure.RTC_Hours = 0x00;
	RTC_TimeStructure.RTC_Minutes = 0x00;
	RTC_TimeStructure.RTC_Seconds = 0x00;
	RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure);

	/* Set Date Week/Date/Month/Year */
	RTC_DateStructure.RTC_WeekDay = RTC_Weekday_Monday;
	RTC_DateStructure.RTC_Date = 0x01;
	RTC_DateStructure.RTC_Month = RTC_Month_January;
	RTC_DateStructure.RTC_Year = 0x00;
	RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure);
}
