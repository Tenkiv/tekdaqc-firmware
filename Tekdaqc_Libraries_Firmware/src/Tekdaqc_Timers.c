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
 * @file Tekdaqc_Timers.c
 * @brief Controls the Tekdaqc's timers and delays.
 *
 * Controls the Tekdaqc's various timers and provides methods for various time delays.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "Tekdaqc_Timers.h"
#include "Tekdaqc_BSP.h"
#include "ADS1256_Driver.h"
#include <inttypes.h>

#ifdef PRINTF_OUTPUT
#include <stdio.h>
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

#define EPOCH_OFFSET_SECONDS	946684800U
#define EPOCH_OFFSET_MILLI		946684800000U
#define EPOCH_OFFSET_MICRO		((uint64_t) 946684800000000U)
#define MILLISEC_PER_SEC		1000U
#define MICROSEC_PER_SEC		((uint64_t) 1000000U)
#define MILLISEC_PER_MIN		60000U
#define MICROSEC_PER_MIN		((uint64_t) 60000000U)
#define SECONDS_PER_HOUR		3600U
#define MILLISEC_PER_HOUR		3600000U
#define MICROSEC_PER_HOUR		((uint64_t) 3600000000U)
#define SECONDS_PER_DAY			86400U
#define MILLISEC_PER_DAY		86400000U
#define MICROSEC_PER_DAY		((uint64_t) 86400000000U)
#define SECONDS_PER_YEAR		31536000U
#define MILLISEC_PER_YEAR		31536000000U
#define MICROSEC_PER_YEAR		((uint64_t) 31536000000000U)

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* Keeps track of a local time reference incremented by the SYSTICK period */
static volatile uint64_t LocalTime = 0;
/* Used to keep track of the ending time for a delay */
static volatile uint64_t count = 0;

static uint64_t MonthMicroOffsets[12] = {0U, 2678400000000U, 5097600000000U, 7776000000000U, 10368000000000U,
		13046400000000U, 15638400000000U, 18316800000000U, 20995200000000U, 23587200000000U, 26265600000000U,
		28857600000000U};

/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Configures the Tekdaqc's timers and any interrupts necessary for their operation.
 *
 * @param  none
 * @retval none
 */
void Timer_Config(void) {
#ifdef DEBUG
	printf("[Config] Configuring SYSTICK timing.\n\r");
#endif
	RCC_ClocksTypeDef RCC_Clocks;

	/* Configure Systick clock source as HCLK */
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

	/* SystTick configuration: an interrupt every 1ms */
	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.HCLK_Frequency / SYSTEMTICK_DIVIDER);

	/* Set Systick interrupt priority to 0*/
	NVIC_SetPriority(SysTick_IRQn, 0U);

	LocalTime = 0U;
}

/**
 * Updates the system local time, incrementing by SYSTEMTICK_PERIOD.
 *
 * @param  none
 * @retval none
 */
void Time_Update(void) {
#ifdef RTC_TIME
	LocalTime = GetLocalTime();
#else
	LocalTime += SYSTEMTICK_PERIOD;
#endif
}

/**
 * Retrieves the current system local time stamp.
 *
 * @param none
 * @retval uint64_t The current local time stamp in microseconds.
 */
uint64_t GetLocalTime(void) {
#ifdef RTC_TIME
	RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;

	/* Get the current Time and Date */
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
	uint32_t subseconds = RTC_GetSubSecond();

	uint64_t time = EPOCH_OFFSET_MICRO + RTC_DateStructure.RTC_Year * MICROSEC_PER_YEAR
			+ MonthMicroOffsets[RTC_DateStructure.RTC_Year] + RTC_DateStructure.RTC_Date * MICROSEC_PER_DAY
			+ RTC_TimeStructure.RTC_Hours * MICROSEC_PER_HOUR + RTC_TimeStructure.RTC_Minutes * MICROSEC_PER_MIN
			+ RTC_TimeStructure.RTC_Seconds * MICROSEC_PER_SEC
			+ (1e6 * (RTC_SYNCH_PRESCALER - subseconds)) / (RTC_SYNCH_PRESCALER + 1);

	//printf("Current Time: %08" PRIu64 " ms\n\r", time / 1000);
	return time;
#else
	return LocalTime;
#endif
}

/**
 * Inserts a delay time, measured in SYSTICK periods.
 * @param  nCount uint32_t The number of SYSTICK periods to wait for.
 * @retval none
 */
void Delay_Periods(uint32_t nCount) {
	Delay_us(nCount * SYSTEMTICK_PERIOD);
}

/**
 * Inserts a delay time, measured in 10 millisecond periods.
 *
 * @param  nCount uint32_t The number of 10 millisecond periods to wait for.
 * @retval none
 */
void Delay_Periods_10MS(uint32_t nCount) {
	Delay_ms(((float) nCount) * 10.0f);
}

/**
 * Inserts a delay time, measured in fractional milliseconds.
 *
 * @param ms float The number of milliseconds to wait for.
 * @retval none
 */
void Delay_ms(float ms) {
	float us = ms * 1000.0f;
	Delay_us((uint64_t) us);
}

/**
 * Inserts a delay time, measured in integer microseconds.
 *
 * @param us uint64_t The number of microseconds to wait for.
 * @retval none
 */
void Delay_us(uint64_t us) {
#ifdef RTC_TIME
	count = us + GetLocalTime();
	while (GetLocalTime() < count) {
		/* Do nothing */
	}
#else
#if 0
	count = us + LocalTime;
	while (LocalTime < count) {
		/* Do nothing */
	}
#endif
	int delay;
	delay = us;
	//lfao, round-up, so add 1...
	ShortDelayUS(delay+1);
#endif
}
