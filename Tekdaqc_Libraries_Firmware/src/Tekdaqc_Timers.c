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

#ifdef PRINTF_OUTPUT
#include <stdio.h>
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* Keeps track of a local time reference incremented by the SYSTICK period */
volatile uint64_t LocalTime = 0;

/* Used to keep track of the ending time for a delay */
volatile uint64_t count = 0;



/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Configures the Tekdaqc's timers and any interrupts necessary for thier operation.
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

	/* SystTick configuration: an interrupt every 10ms */
	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.HCLK_Frequency / SYSTEMTICK_DIVIDER);

	/* Set Systick interrupt priority to 0*/
	NVIC_SetPriority (SysTick_IRQn, 0U);

	LocalTime = 0U;
}

/**
 * Updates the system local time, incrementing by SYSTEMTICK_PERIOD.
 *
 * @param  none
 * @retval none
 */
void Time_Update(void) {
	LocalTime += SYSTEMTICK_PERIOD;
}

/**
 * Retrieves the current system local time stamp.
 *
 * @param none
 * @retval uint64_t The current local time stamp in microseconds.
 */
uint64_t GetLocalTime(void) {
	return LocalTime;
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
	count = us + LocalTime;
	while (LocalTime < count) {
		/* Do nothing */
	}
}
