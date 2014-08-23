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
 * @file Tekdaqc_Config.c
 * @brief Configures global settings and variables for the Tekdaqc.
 *
 * Provides methods to configure and manipulate global settings and variables for the Tekdaqc.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "Tekdaqc_Config.h"
#include "Tekdaqc_Timers.h"
#include "netconf.h"
#include "TLE7232_RelayDriver.h"
#include "Tekdaqc_RTC.h"
#include "TelnetServer.h"
#include "Tekdaqc_CalibrationTable.h"
#include "eeprom.h"
#include <string.h>
#include <inttypes.h>

#ifdef PRINTF_OUTPUT
#include <stdio.h>
#endif



/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* The LSI capture count */
volatile uint32_t LSICaptureNumber = 0;

/* The LSI Period value */
volatile uint32_t LSIPeriodValue = 0;

/* The Low Speed Internal (LSI) frequency */
static volatile uint32_t LsiFreq = 0;

/* The ADS1256 clock period in seconds */
static const float ADS1256_PERIOD = 1.0f / ADS1256_CLK_FREQ;

/* COM port configuration constants */
static const uint32_t COM_USART_CLK[COMn] = { COM1_CLK, COM2_CLK };
static const uint32_t COM_TX_PORT_CLK[COMn] = { COM1_TX_GPIO_CLK, COM2_TX_GPIO_CLK };
static const uint32_t COM_RX_PORT_CLK[COMn] = { COM1_RX_GPIO_CLK, COM2_TX_GPIO_CLK };
static const uint16_t COM_TX_PIN[COMn] = { COM1_TX_PIN, COM2_TX_PIN };
static const uint16_t COM_RX_PIN[COMn] = { COM1_RX_PIN, COM2_RX_PIN };
static const uint16_t COM_TX_PIN_SOURCE[COMn] = { COM1_TX_SOURCE, COM2_TX_SOURCE };
static const uint16_t COM_RX_PIN_SOURCE[COMn] = { COM1_RX_SOURCE, COM2_RX_SOURCE };
static const uint16_t COM_TX_AF[COMn] = { COM1_TX_AF, COM2_TX_AF };
static const uint16_t COM_RX_AF[COMn] = { COM1_RX_AF, COM2_RX_AF };
static USART_TypeDef* COM_USART[COMn] = { COM1_USART, COM2_USART };
static GPIO_TypeDef* COM_TX_PORT[COMn] = { COM1_TX_GPIO_PORT, COM2_TX_GPIO_PORT };
static GPIO_TypeDef* COM_RX_PORT[COMn] = { COM1_RX_GPIO_PORT, COM2_RX_GPIO_PORT };

/* Timer Time base initialization structure */
/*static TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;*/

/* Timer output compare initialization structure */
/*static TIM_OCInitTypeDef TIM_OCInitStructure;*/



/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/* Definition of the global TOSTRING_BUFFER */
char TOSTRING_BUFFER[SIZE_TOSTRING_BUFFER];

/* Definition of the Tekdaqc's serial number */
unsigned char TEKDAQC_BOARD_SERIAL_NUM[BOARD_SERIAL_NUM_LENGTH + 1]; /* 32 chars plus NULL termination */

/* Definition of the FLASH disk EEPROM addresses */
uint16_t EEPROM_ADDRESSES[NUM_EEPROM_ADDRESSES];



/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Retrieve the LSI frequency.
 */
static uint32_t GetLSIFrequency(void);



/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Configures timer 5 (TIM5) to accurately measure the LSI oscillator frequency so that the watchdog
 * interval is minimally affected by the variations in the oscillator.
 *
 * @param none
 * @return uint32_t The LSI frequency
 */
uint32_t GetLSIFrequency() {
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;
	RCC_ClocksTypeDef RCC_ClockFreq;

	/* Enable the LSI oscillator */
	RCC_LSICmd(ENABLE);

	/* Wait till LSI is ready */
	while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) {
		/* Do nothing */
	}

	/* TIM5 configuration */
	/* Enable TIM5 clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	/* Connect internally the TIM5_CH4 Input Capture to the LSI clock output */
	TIM_RemapConfig(TIM5, TIM5_LSI);

	/* Configure TIM5 prescaler */
	TIM_PrescalerConfig(TIM5, 0, TIM_PSCReloadMode_Immediate);

	/* TIM5 configuration: Input Capture mode
	 The LSI oscillator is connected to TIM5 CH4
	 The Rising edge is used as active edge,
	 The TIM5 CCR4 is used to compute the frequency value */
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV8;
	TIM_ICInitStructure.TIM_ICFilter = 0;
	TIM_ICInit(TIM5, &TIM_ICInitStructure);

	/* Enable TIM5 Interrupt channel */
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable TIM5 counter */
	TIM_Cmd(TIM5, ENABLE);

	/* Reset the flags */
	TIM5 ->SR = 0;

	/* Enable the CC4 Interrupt Request */
	TIM_ITConfig(TIM5, TIM_IT_CC4, ENABLE);

	/* Wait until the TIM5 get 2 LSI edges (refer to TIM5_IRQHandler() in stm32f4xx_it.c file) */
	while (LSICaptureNumber != 2) {
		/* Do Nothing */
	}
	/* Deinitialize the TIM5 peripheral registers to their default reset values */
	TIM_DeInit(TIM5);

	/* Compute the LSI frequency, depending on TIM5 input clock frequency (PCLK1) */
	/* Get SYSCLK, HCLK and PCLKx frequency */
	RCC_GetClocksFreq(&RCC_ClockFreq);

	/* Get PCLK1 prescaler */
	if ((RCC ->CFGR & RCC_CFGR_PPRE1)== 0) {
		/* PCLK1 prescaler equal to 1 => TIMCLK = PCLK1 */
		return ((RCC_ClockFreq.PCLK1_Frequency / LSIPeriodValue) * 8);
	} else {
		/* PCLK1 prescaler different from 1 => TIMCLK = 2 * PCLK1 */
		return (((2 * RCC_ClockFreq.PCLK1_Frequency) / LSIPeriodValue) * 8);
	}
}



/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Initializes the boards watchdog timer. If the behavior of the board is changed, this method may need to be
 * modified if the main control loop is changed.
 *
 * @param none
 * @retval none
 */
void Watchdog_Init(void) {
	/* Get the LSI frequency from TIM5 */
	LsiFreq = GetLSIFrequency();
	printf("LSI Frequency: %" PRIu32 "\n", LsiFreq);

	/* Enable write access to IWDG_PR and IWDG_RLR registers */
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

	/* IWDG counter clock: LSI/32 */
	IWDG_SetPrescaler(IWDG_Prescaler_64);

	/* Set counter reload value to obtain 1s IWDG TimeOut.
	     IWDG counter clock Frequency = LsiFreq/32
	     Counter Reload Value = 250ms/IWDG counter clock period
	                          = 0.25s / (32/LsiFreq)
	                          = LsiFreq/(32 * 4)
	                          = LsiFreq/128
	 */
	IWDG_SetReload(LsiFreq/16);

	/* Reload IWDG counter */
	IWDG_ReloadCounter();

	/* Enable IWDG (the LSI oscillator will be enabled by hardware) */
	IWDG_Enable();
}

/**
 * Initializes the Tekdaqc's Ethernet communications and lwIP stack. This is necessary for both
 * the main application functionality as well as the IAP update function.
 *
 * @param none
 * @retval none
 */
void Communication_Init(void) {
	for (uint_fast8_t i = 0; i < SIZE_TOSTRING_BUFFER; ++i) {
		TOSTRING_BUFFER[i] = '\0';
	}

	/* Initialize the generic delay timer */
	Timer_Config();

	/* Configure ethernet (GPIOs, clocks, MAC, DMA) */
	ETH_BSP_Config();

	/* Initilaize the LwIP stack */
	LwIP_Init();
}

/**
 * Initializes the Tekdaqc's FLASH disk.
 *
 * @param none
 * @retval none
 */
void FlashDiskInit(void) {
	/* EEPROM Init */
	EE_Init();
}

/**
 * Retrieves the boards serial number and stores in in the specified C-String.
 *
 * @param ptr char* Pointer to a C-String to store the serial number in. Must be at least
 * sizeof(TEKDAQC_BOARD_SERIAL_NUM) in size.
 * @retval none
 *
 */
void GetSerialNumber(unsigned char* ptr) {
	strncpy(ptr, TEKDAQC_BOARD_SERIAL_NUM, sizeof(TEKDAQC_BOARD_SERIAL_NUM));
}

/**
 * Returns a human readable string describing the digital logic level.
 *
 * @param level DigitalLevel_t The digital logic level to convert to a string.
 * @retval const char* The human readable C-String representation.
 */
const char* DigitalLevelToString(DigitalLevel_t level) {
	static const char* strings[] = { "Logic High", "Logic Low", "Invalid" };
	int ret_index = 2;
	switch (level) {
	case LOGIC_HIGH:
		ret_index = 0;
		break;
	case LOGIC_LOW:
		ret_index = 1;
		break;
	default:
		ret_index = 2;
		break;
	}
	return strings[ret_index];
}

/**
 * Clears the global TOSTRING_BUFFER.
 *
 * @param none
 * @retval none
 */
void ClearToStringBuffer(void) {
	for (uint_fast8_t i = 0; i < SIZE_TOSTRING_BUFFER; ++i) {
		TOSTRING_BUFFER[i] = '\0';
	}
}

/**
 * Configures the specified COM port.
 *
 * @param  COM COM_TypeDef Specifies the COM port to be configured.
 * @param  USART_InitStruct USART_InitTypeDef* Pointer to a USART_InitTypeDef structure that
 *   contains the configuration information for the specified USART peripheral.
 * @retval none
 */
void COMInit(COM_TypeDef COM, USART_InitTypeDef* USART_InitStruct) {
	if (COM == COM1 || COM == COM2) {
		GPIO_InitTypeDef GPIO_InitStructure;

		/* Enable GPIO clock */
		RCC_AHB1PeriphClockCmd(COM_TX_PORT_CLK[COM] | COM_RX_PORT_CLK[COM], ENABLE);

		/* Enable UART clock */
		RCC_APB1PeriphClockCmd(COM_USART_CLK[COM], ENABLE);

		/* Connect PXx to USARTx_Tx*/
		GPIO_PinAFConfig(COM_TX_PORT[COM], COM_TX_PIN_SOURCE[COM], COM_TX_AF[COM]);

		/* Connect PXx to USARTx_Rx*/
		GPIO_PinAFConfig(COM_RX_PORT[COM], COM_RX_PIN_SOURCE[COM], COM_RX_AF[COM]);

		/* Configure USART Tx as alternate function  */
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;

		GPIO_InitStructure.GPIO_Pin = COM_TX_PIN[COM];
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(COM_TX_PORT[COM], &GPIO_InitStructure);

		/* Configure USART Rx as alternate function  */
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Pin = COM_RX_PIN[COM];
		GPIO_Init(COM_RX_PORT[COM], &GPIO_InitStructure);

		/* USART configuration */
		USART_Init(COM_USART[COM], USART_InitStruct);

		/* Enable USART */
		USART_Cmd(COM_USART[COM], ENABLE);
	}
}

#ifdef DEBUG
GPIO_TypeDef* GPIO_PORT[TEST_PINn] = { TEST_PIN1_GPIO_PORT, TEST_PIN2_GPIO_PORT,
		TEST_PIN3_GPIO_PORT, TEST_PIN4_GPIO_PORT };
const uint16_t GPIO_PIN[TEST_PINn] = { TEST_PIN1, TEST_PIN2, TEST_PIN3,
		TEST_PIN4 };
const uint32_t GPIO_CLK[TEST_PINn] = { TEST_PIN1_GPIO_CLK, TEST_PIN2_GPIO_CLK,
		TEST_PIN3_GPIO_CLK, TEST_PIN4_GPIO_CLK };

void TestPin_Init(TestPin_TypeDef pin) {
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable the GPIO Clock */
	RCC_AHB1PeriphClockCmd(GPIO_CLK[pin], ENABLE);

	/* Configure the GPIO pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_PIN[pin];
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_PORT[pin], &GPIO_InitStructure);
}

/**
 * Turns selected pin on.
 *
 * @param  pin TestPin_TypeDef Specifies the pin to be set on.
 * @retval none
 */
void TestPin_On(TestPin_TypeDef pin) {
	GPIO_PORT[pin]->BSRRL = GPIO_PIN[pin];
}

/**
 * Turns selected pin off.
 *
 * @param  pin TestPin_TypeDef Specifies the pin to be set off.
 * @retval none
 */
void TestPin_Off(TestPin_TypeDef pin) {
	GPIO_PORT[pin]->BSRRH = GPIO_PIN[pin];
}

/**
 * Toggles the selected pin.
 *
 * @param  pin TestPin_TypeDef Specifies the Led to be toggled.
 * @retval none
 */
void TestPin_Toggle(TestPin_TypeDef pin) {
	GPIO_PORT[pin]->ODR ^= GPIO_PIN[pin];
}

/**
 * Initialize COM2 interface for serial debug
 *
 * @param  none
 * @retval none
 */
void DebugComPort_Init(void) {
	USART_InitTypeDef USART_InitStructure;

	/* USARTx configured as follow:
	 - BaudRate = 115200 baud
	 - Word Length = 8 Bits
	 - One Stop Bit
	 - No parity
	 - Hardware flow control disabled (RTS and CTS signals)
	 - Receive and transmit enabled
	 */
	USART_InitStructure.USART_BaudRate = 460800;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	COMInit(COM2, &USART_InitStructure);
}

#endif /*DEBUG*/
