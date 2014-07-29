/*
 * main.c
 *
 *  Created on: Dec 3, 2013
 *      Author: web_000
 */

#include "main.h"
#include "TekDAQC_Config.h"
#include "stm32f4x7_eth.h"
#include "netconf.h"
#include "TFTP_Server.h"
#include "TekDAQC_Locator.h"
#include "TekDAQC_Timers.h"
#include <stdbool.h>
#include <stdio.h>
#include "core_cm4.h"

/* Private typedef -----------------------------------------------------------*/
typedef void (*pFunction)(void);

/* Private define ------------------------------------------------------------*/
typedef enum {
	LED1 = 0, LED2 = 1, LED3 = 2, LED4 = 3
} Led_TypeDef;

#define LEDn                             4

#define LED1_PIN                         GPIO_Pin_2
#define LED1_GPIO_PORT                   GPIOG
#define LED1_GPIO_CLK                    RCC_AHB1Periph_GPIOG

#define LED2_PIN                         GPIO_Pin_4
#define LED2_GPIO_PORT                   GPIOG
#define LED2_GPIO_CLK                    RCC_AHB1Periph_GPIOG

#define LED3_PIN                         GPIO_Pin_6
#define LED3_GPIO_PORT                   GPIOG
#define LED3_GPIO_CLK                    RCC_AHB1Periph_GPIOG

#define LED4_PIN                         GPIO_Pin_8
#define LED4_GPIO_PORT                   GPIOG
#define LED4_GPIO_CLK                    RCC_AHB1Periph_GPIOG

void STM_EVAL_LEDInit(Led_TypeDef Led);
void STM_EVAL_LEDOn(Led_TypeDef Led);
void STM_EVAL_LEDOff(Led_TypeDef Led);
void STM_EVAL_LEDToggle(Led_TypeDef Led);

GPIO_TypeDef* LED_GPIO_PORT[LEDn] = { LED1_GPIO_PORT, LED2_GPIO_PORT,
		LED3_GPIO_PORT,
		LED4_GPIO_PORT };
const uint16_t LED_GPIO_PIN[LEDn] = { LED1_PIN, LED2_PIN, LED3_PIN,
LED4_PIN };
const uint32_t LED_GPIO_CLK[LEDn] = { LED1_GPIO_CLK, LED2_GPIO_CLK,
		LED3_GPIO_CLK,
		LED4_GPIO_CLK };

void STM_EVAL_LEDInit(Led_TypeDef Led) {
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable the GPIO_LED Clock */
	RCC_AHB1PeriphClockCmd(LED_GPIO_CLK[Led], ENABLE);

	/* Configure the GPIO_LED pin */
	GPIO_InitStructure.GPIO_Pin = LED_GPIO_PIN[Led];
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LED_GPIO_PORT[Led], &GPIO_InitStructure);
}

void STM_EVAL_LEDOn(Led_TypeDef Led) {
	LED_GPIO_PORT[Led]->BSRRL = LED_GPIO_PIN[Led];
}

void STM_EVAL_LEDOff(Led_TypeDef Led) {
	LED_GPIO_PORT[Led]->BSRRH = LED_GPIO_PIN[Led];
}

void STM_EVAL_LEDToggle(Led_TypeDef Led) {
	LED_GPIO_PORT[Led]->ODR ^= LED_GPIO_PIN[Led];
}

/* Private variables ---------------------------------------------------------*/
pFunction Jump_To_Application;
uint32_t JumpAddress;

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

	RCC_ClocksTypeDef RCC_Clocks;

	/* Configure Systick clock source as HCLK */
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

	/* SystTick configuration: an interrupt every 10ms */
	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.HCLK_Frequency / SYSTEMTICK_DIVIDER);

	/* Set Systick interrupt priority to 0*/
	NVIC_SetPriority(SysTick_IRQn, 0);

	STM_EVAL_LEDInit(LED1);
	STM_EVAL_LEDInit(LED2);
	STM_EVAL_LEDInit(LED3);
	STM_EVAL_LEDInit(LED4);

	STM_EVAL_LEDOn(LED1);

	// Enable the PWR APB1 Clock Interface
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	// Allow access to BKP Domain
	PWR_BackupAccessCmd(ENABLE);

#ifdef SERIAL_DEBUG /* Configure serial debugging. */
	DebugComPort_Init();
#endif
#ifdef DEBUG
	printf("\n\rSerial Port Initialized.\n\r");
#endif

	uint32_t reg = RTC_ReadBackupRegister(UPDATE_FLAG_REGISTER);
	bool should_enter = ((reg & UPDATE_FLAG_ENABLED) != 0) ? false : true;
	if (should_enter == false) {
		/* Update flag not set: jump to user application */
		STM_EVAL_LEDOn(LED2);

		/* Check if valid stack address (RAM address) then jump to user application */
		if (((*(__IO uint32_t*) USER_FLASH_FIRST_PAGE_ADDRESS) & 0x2FFE0000)
				== 0x20000000) {
			/* Jump to user application */
			JumpAddress = *(__IO uint32_t*) (USER_FLASH_FIRST_PAGE_ADDRESS + 4);
			Jump_To_Application = (pFunction) JumpAddress;
			/* Initialize user application's Stack Pointer */
			__set_MSP(*(__IO uint32_t*) USER_FLASH_FIRST_PAGE_ADDRESS);
			Jump_To_Application();
		} else {
			/* Otherwise, do nothing */
			/* LED3 (RED) ON to indicate bad software (when not valid stack address) */
			//TODO: Error
			STM_EVAL_LEDOn(LED3);
			/* do nothing */
			while (1)
				;
		}
	} else {
		// Enter IAP mode
		STM_EVAL_LEDOn(LED4);

		/* Initialize the Tekdaqc's communication methods */
		Communication_Init();

		/* Start the TekDAQC Locator Service */
		Tekdaqc_LocatorInit();

		/* Initialize the TFTP server */
		TFTP_Init();

		/* Infinite loop */
		while (1) {
			/* check if any packet received */
			if (ETH_CheckFrameReceived()) {
				/* process received ethernet packet */
				LwIP_Pkt_Handle();
			}
			/* handle periodic timers for LwIP */
			LwIP_Periodic_Handle(GetLocalTime());
		}
	}
	return 0;
}

#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{
	}
}
#endif
