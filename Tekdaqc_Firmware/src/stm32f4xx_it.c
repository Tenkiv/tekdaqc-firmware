/**
 ******************************************************************************
 * @file    stm32f4xx_it.c
 * @author  MCD Application Team
 * @version V1.1.0
 * @date    31-July-2013
 * @brief   Main Interrupt Service Routines.
 *          This file provides template for all exceptions handler and
 *          peripherals interrupt service routine.
 ******************************************************************************
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/**
 * This file modified by Tenkiv, Inc.
 * @since v1.0.0.0
 */

/* Includes ------------------------------------------------------------------*/
#include "Tekdaqc_Timers.h"
#include "Tekdaqc_Config.h"
#include "netconf.h"
#include "Tekdaqc_CAN.h"
#include "Analog_Input.h"
#include "Digital_Input.h"
#include <stdio.h>
#include <inttypes.h>

//lfao-these variables are used by the DRDY interrupt handler for it to know which channel is sampling and
//the number of samples to take...
extern volatile int viCurrentChannel;
extern volatile int viSamplesToTake;
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint16_t tmpCC4[2] = {0, 0};
__IO bool isInitialized = false;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

void Tekdaqc_Initialized(bool status) {
	isInitialized = status;
}

/**
 * @brief   This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void) {
}

/**
 * @brief  This function handles Hard Fault exception.
 * @param  None
 * @retval None
 */
void HardFault_Handler(void) {
	volatile unsigned long stacked_r0;
	volatile unsigned long stacked_r1;
	volatile unsigned long stacked_r2;
	volatile unsigned long stacked_r3;
	volatile unsigned long stacked_r12;
	volatile unsigned long stacked_lr;
	volatile unsigned long stacked_pc;
	volatile unsigned long stacked_psr;
	/*volatile unsigned long _CFSR;
	volatile unsigned long _HFSR;
	volatile unsigned long _DFSR;
	volatile unsigned long _AFSR;
	volatile unsigned long _BFAR;
	volatile unsigned long _MMAR;*/
	uint32_t* hardfault_args = (uint32_t*) 0x20000400;

	asm("TST LR, #4 \n"
			"ITE EQ \n"
			"MRSEQ R0, MSP \n"
			"MRSNE R0, PSP \n");

	stacked_r0 = ((unsigned long) hardfault_args[0]);
	stacked_r1 = ((unsigned long) hardfault_args[1]);
	stacked_r2 = ((unsigned long) hardfault_args[2]);
	stacked_r3 = ((unsigned long) hardfault_args[3]);

	stacked_r12 = ((unsigned long) hardfault_args[4]);
	stacked_lr = ((unsigned long) hardfault_args[5]);
	stacked_pc = ((unsigned long) hardfault_args[6]);
	stacked_psr = ((unsigned long) hardfault_args[7]);

	printf("[Hard fault handler]\n\r");
	printf("R0 = %" PRIx32 "\n\r", stacked_r0);
	printf("R1 = %" PRIx32 "\n\r", stacked_r1);
	printf("R2 = %" PRIx32 "\n\r", stacked_r2);
	printf("R3 = %" PRIx32 "\n\r", stacked_r3);
	printf("R12 = %" PRIx32 "\n\r", stacked_r12);
	printf("LR = %" PRIx32 "\n\r", stacked_lr);
	printf("PC = %" PRIx32 "\n\r", stacked_pc);
	printf("PSR = %" PRIx32 "\n\r", stacked_psr);
	printf("BFAR = %" PRIx32 "\n\r", (*((volatile unsigned long *) (0xE000ED38))));
	printf("CFSR = %" PRIx32 "\n\r", (*((volatile unsigned long *) (0xE000ED28))));
	printf("HFSR = %" PRIx32 "\n\r", (*((volatile unsigned long *) (0xE000ED2C))));
	printf("SCB->HFSR = 0x%08" PRIx32 "\n\r", SCB ->HFSR);
	printf("DFSR = %" PRIx32 "\n\r", (*((volatile unsigned long *) (0xE000ED30))));
	printf("AFSR = %" PRIx32 "\n\r", (*((volatile unsigned long *) (0xE000ED3C))));

	if ((SCB ->HFSR & (1 << 30)) != 0) {
		printf("Forced Hard Fault\n\r");
		printf("SCB->CFSR = 0x%08" PRIx32 "\n\r", SCB ->CFSR);
	}

	/* Go to infinite loop when Hard Fault exception occurs */
	while (1) {
		__asm("BKPT #0\n"); //Break into the debugger
	}
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void) {
	/* Go to infinite loop when Memory Manage exception occurs */
	while (1) {
	}
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void) {
	/* Go to infinite loop when Bus Fault exception occurs */
	while (1) {
	}
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void) {
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1) {
	}
}

/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void) {
}

/**
 * @brief  This function handles SysTick Handler.
 * @param  None
 * @retval None
 */

volatile uint8_t timerCounter = 0; 	//flag 100us
volatile uint64_t pwmTimer = 0; 	//50us increment for pwm input
extern volatile pwmInput_t* pInputs[NUM_DIGITAL_INPUTS];
extern volatile uint64_t currentDTime;
extern volatile uint8_t update_DTime;

void SysTick_Handler(void) {
#if 0
	/* Update the LocalTime by adding SYSTEMTICK_PERIOD_MS each SysTick interrupt */
	//printf("SysTick_Handler...\n");
	//TestPin_On(PIN2);
#endif
	if (timerCounter == 2) { //100us
		Time_Update();
		if (update_DTime) {
			currentDTime += SYSTEMTICK_PERIOD;
		}
		timerCounter = 0;
	}
	pwmTimer += 50; //50us

	timerCounter++;
#if 0
	//TestPin_Off(PIN2);

#endif
}

/******************************************************************************/
/*            STM32F4xx Peripherals Interrupt Handlers                        */
/******************************************************************************/

/**
 * @brief  This function handles External line 10 interrupt request.
 * @param  None
 * @retval None
 */
#if 0
void EXTI15_10_IRQHandler(void) {
	if (EXTI_GetITStatus(ETH_LINK_EXTI_LINE) != RESET) {
		Eth_Link_ITHandler(DP83848_PHY_ADDRESS);
		// Clear interrupt pending bit
		EXTI_ClearITPendingBit(ETH_LINK_EXTI_LINE);
	}
}
#endif

void EXTI15_10_IRQHandler(void)
{
	uint8_t ads1256data[3];
	Analog_Samples_t newAnalogSample;

	if (EXTI_GetITStatus(EXTI_Line10) != RESET)
	{
		//viCurrentChannel
		//viSamplesToTake

		// Clear interrupt pending bit
		EXTI_ClearITPendingBit(EXTI_Line10);

		/****Get ADS1256 reading routine******/
		ADS1256_CS_LOW(); /* Enable SPI communication */
		ADS1256_SendByte(ADS1256_RDATA); /* Send RDATA command byte */
		ShortDelayUS(11);
		//Delay_us((uint64_t) (50U * ADS1256_CLK_PERIOD_US)); /*  timing characteristic t6 */
		ADS1256_ReceiveBytes(ads1256data, 3U);
		ShortDelayUS(3);
		//Delay_us(8 * ADS1256_CLK_PERIOD_US); /* timing characteristic t10 */
		ADS1256_CS_HIGH(); /* Latch SPI communication */
		ShortDelayUS(2);
		//Delay_us((uint64_t) (4U * ADS1256_CLK_PERIOD_US)); /*  timing characteristic t11 */
		newAnalogSample.iChannel = viCurrentChannel;
		newAnalogSample.iReading = (uint32_t)(ads1256data[0]<<16 | ads1256data[1]<<8 | ads1256data[2]);
		newAnalogSample.ui64TimeStamp = currentDTime;
		WriteSampleToBuffer(&newAnalogSample);
		//lfao - infinite sampling, do nothing, just let it run, else disable this interrupt...
		if(viSamplesToTake!=-1)
		{
			viSamplesToTake--;
			if(viSamplesToTake==0)
			{
				ADS1256_EXTI_Disable();
			}
		}
		/*************************************************/

	}
}



void TIM5_IRQHandler(void) {
	if (TIM_GetITStatus(TIM5, TIM_IT_CC4) != RESET) {
		// Get the Input Capture value
		tmpCC4[LSICaptureNumber++] = TIM_GetCapture4(TIM5);

		// Clear CC4 Interrupt pending bit
		TIM_ClearITPendingBit(TIM5, TIM_IT_CC4);

		if (LSICaptureNumber >= 2) {
			// Compute the period length
			LSIPeriodValue = (uint16_t) (0xFFFF - tmpCC4[0] + tmpCC4[1] + 1);
		}
	}
}
void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
        AnalogChannelHandler();
    }
}

volatile uint8_t pwmCounter = 0;  //duty cycle for pwm output
void TIM3_IRQHandler(void) {  //pwm output timer - interrupt every 1ms
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		pwmCounter++;
		pwmCounter%=100; //update counter, each increment = 1%
	}
}

/**
  * @brief  This function handles CAN1 RX0 request.
  * @param  None
  * @retval None
  */
void CAN1_RX0_IRQHandler(void) {
  CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
  if ((RxMessage.StdId == 0x321)&&(RxMessage.IDE == CAN_ID_STD) && (RxMessage.DLC == 1)) {
    printf("[CAN Handler] Data: %" PRIu8 "\n\r", RxMessage.Data[0]);
  }
}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
 * @brief  This function handles PPP interrupt request.
 * @param  None
 * @retval None
 */
/*void PPP_IRQHandler(void)
 {
 }*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
