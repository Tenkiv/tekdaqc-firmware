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

/*
 * @file ADS1256_SPI_Controller.h
 * @brief Source file for ADS1256 SPI driver.
 *
 * Deals with low level communication with ADS1256 ADC via SPI.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @date Aug 18, 2013
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "ADS1256_SPI_Controller.h"
#include "ADS1256_Driver.h"
#include "Tekdaqc_Config.h"

#ifdef PRINTF_OUTPUT
#include <stdio.h>
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Perform the low level peripheral initialization for the SPI peripheral.
 */
static void ADS1256_LowLevel_Init(void);

/**
 * @brief Perform the low level peripheral de-initialization the SPI peripheral.
 */
static void ADS1256_LowLevel_DeInit(void);

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
  * @brief  Initializes the peripherals used by the SPI driver.
  * @param  None
  * @retval None
  */
static void ADS1256_LowLevel_Init(void) {
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable the SPI clock */
  ADS1256_SPI_CLK_INIT(ADS1256_SPI_CLK, ENABLE);

  /* Enable GPIO clocks */
  RCC_AHB1PeriphClockCmd(ADS1256_SPI_SCK_GPIO_CLK | ADS1256_SPI_MISO_GPIO_CLK |
                         ADS1256_SPI_MOSI_GPIO_CLK | ADS1256_CS_GPIO_CLK, ENABLE);

  /* Connect SPI pins to AF5 */
  GPIO_PinAFConfig(ADS1256_SPI_SCK_GPIO_PORT, ADS1256_SPI_SCK_SOURCE, ADS1256_SPI_SCK_AF);
  GPIO_PinAFConfig(ADS1256_SPI_MISO_GPIO_PORT, ADS1256_SPI_MISO_SOURCE, ADS1256_SPI_MISO_AF);
  GPIO_PinAFConfig(ADS1256_SPI_MOSI_GPIO_PORT, ADS1256_SPI_MOSI_SOURCE, ADS1256_SPI_MOSI_AF);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;

  /* SPI SCK pin configuration */
  GPIO_InitStructure.GPIO_Pin = ADS1256_SPI_SCK_PIN;
  GPIO_Init(ADS1256_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

  /* SPI MOSI pin configuration */
  GPIO_InitStructure.GPIO_Pin =  ADS1256_SPI_MOSI_PIN;
  GPIO_Init(ADS1256_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

  /* SPI MISO pin configuration */
  GPIO_InitStructure.GPIO_Pin =  ADS1256_SPI_MISO_PIN;
  GPIO_Init(ADS1256_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

  /* Configure ADS1256 CS pin in output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = ADS1256_CS_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(ADS1256_CS_GPIO_PORT, &GPIO_InitStructure);
}

/**
  * @brief  DeInitializes the peripherals used by the SPI driver.
  * @param  none
  * @retval none
  */
static void ADS1256_LowLevel_DeInit(void) {
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Disable the ADS1256_SPI */
  SPI_Cmd(ADS1256_SPI, DISABLE);

  /* DeInitializes the ADS1256_SPI */
  SPI_I2S_DeInit(ADS1256_SPI);

  /* ADS1256_SPI Periph clock disable */
  ADS1256_SPI_CLK_INIT(ADS1256_SPI_CLK, DISABLE);

  /* Configure all pins used by the SPI as input floating */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

  GPIO_InitStructure.GPIO_Pin = ADS1256_SPI_SCK_PIN;
  GPIO_Init(ADS1256_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = ADS1256_SPI_MISO_PIN;
  GPIO_Init(ADS1256_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = ADS1256_SPI_MOSI_PIN;
  GPIO_Init(ADS1256_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = ADS1256_CS_PIN;
  GPIO_Init(ADS1256_CS_GPIO_PORT, &GPIO_InitStructure);
}

/*--------------------------------------------------------------------------------------------------------*/
/* INITIALIZATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Initialize the ADS1256 connection over SPI and bring it to a known good, idle state.
 *
 * @param none
 * @retval none
 * */
void ADS1256_SPI_Init(void) {
#ifdef ADS1256_SPI_DEBUG
  printf("[ADS1256] Initializing SPI peripherals for the ADS1256.\n\r");
#endif
  SPI_InitTypeDef  SPI_InitStructure;
  ADS1256_LowLevel_Init();
    
  /* Deselect the ADS1256: Chip Select high */
  ADS1256_CS_HIGH();

  /* SPI configuration */
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  
  SPI_Init(ADS1256_SPI, &SPI_InitStructure);

  /* Enable the ADC1256 SPI  */
  SPI_Cmd(ADS1256_SPI, ENABLE);
}

/*
 * De-initialize the ADS1256 connection over SPI and free any resources being used to maintain it.
 *
 * @param none
 * @retval none
 */
void ADS1256_SPI_DeInit(void) {
#ifdef ADS1256_SPI_DEBUG
  printf("[ADS1256] Deinitializing peripherals for the ADS1256.\n\r");
#endif
  ADS1256_LowLevel_DeInit();
}

/*--------------------------------------------------------------------------------------------------------*/
/* PIN CONTROL METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Convert the SPI clock pin to GPIO function, enabling software control.
 *
 * @param none
 * @retval none
 */
void ADS1256_CLK_To_GPIO(void) {
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  /* Enable the GPIO Clock */
  ADS1256_SPI_CLK_INIT(ADS1256_SPI_SCK_GPIO_CLK, ENABLE);

  /* Configure the GPIO_LED pin */
  GPIO_InitStructure.GPIO_Pin = ADS1256_SPI_SCK_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(ADS1256_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);
}

/**
 * Convert the SPI clock pin from GPIO to SPI function, disabling software control.
 *
 * @param none
 * @retval none
 */
void ADS1256_GPIO_To_CLK(void) {
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable the SPI clock */
  ADS1256_SPI_CLK_INIT(ADS1256_SPI_CLK, ENABLE);

  /* Enable GPIO clocks */
  RCC_AHB1PeriphClockCmd(ADS1256_SPI_SCK_GPIO_CLK | ADS1256_SPI_MISO_GPIO_CLK |
                         ADS1256_SPI_MOSI_GPIO_CLK | ADS1256_CS_GPIO_CLK, ENABLE);
  
  /* Connect SPI pins to AF5 */
  GPIO_PinAFConfig(ADS1256_SPI_SCK_GPIO_PORT, ADS1256_SPI_SCK_SOURCE, ADS1256_SPI_SCK_AF);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
        
  /*!< SPI SCK pin configuration */
  GPIO_InitStructure.GPIO_Pin = ADS1256_SPI_SCK_PIN;
  GPIO_Init(ADS1256_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);
}

/*--------------------------------------------------------------------------------------------------------*/
/* COMMUNICATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
  * Sends a byte through the SPI interface and return the byte received from the SPI bus.
  * This method blocks until transmission and reception of the byte are complete.
  *
  * @param data uint8_t The byte to send.
  * @retval uint8_t The value of the received byte.
  */
uint8_t ADS1256_SendByte(uint8_t data) {
#ifdef ADS1256_SPI_DEBUG
  printf("[ADS1256] Sending byte: 0x%02X\n\r", data);
#endif
  /* Send byte through the SPI peripheral */
  SPI_I2S_SendData(ADS1256_SPI, data);
  
  /* Loop while DR register in not empty */
  while (SPI_I2S_GetFlagStatus(ADS1256_SPI, SPI_I2S_FLAG_TXE) == RESET);

  /* Wait to receive a byte */
  while (SPI_I2S_GetFlagStatus(ADS1256_SPI, SPI_I2S_FLAG_RXNE) == RESET);

  /* Return the byte read from the SPI bus */
  return SPI_I2S_ReceiveData(ADS1256_SPI);
}

/**
  * Sends an array of bytes through the SPI interface, ignoring the returns. This method blocks
  * until transmission is completed.
  *
  * @param data uint8_t* Array of bytes to send.
  * @param n uint8_t The number of bytes to send. Maximum is 256.
  */
void ADS1256_SendBytes(uint8_t* data, const uint8_t n) {
#ifdef ADS1256_SPI_DEBUG
  printf("[ADS1256] Sending %d bytes\n\r", n);
#endif
  for (uint_fast8_t i = 0; i < n; ++i) {
    ADS1256_SendByte(data[i]);
  }
}

/**
  * Reads a byte from the SPI interface by sending a dummy byte over the interface.
  *
  * @param  none
  * @retval uint8_t Byte read from the SPI Interface.
  */
uint8_t ADS1256_ReceiveByte(void) {
  uint8_t data = (ADS1256_SendByte(ADS1256_DUMMY_BYTE));
#ifdef ADS1256_SPI_DEBUG
  printf("[ADS1256] Received byte: 0x%02X\n\r", data);
#endif
  return data;
}

/**
 * Reads an a series of bytes from the SPI interface by sending dummy bytes over the interface.
 *
 * @param data uint8_t* Pointer to array of uint8_t to store the received data in.
 * @param n uint8_t The number of bytes to read from the interface. Maximum is 256.
 */
void ADS1256_ReceiveBytes(uint8_t* data, uint8_t n) {
	/* TODO: This should check that count is valid and that the array pointer is for a sufficiently large array */
#ifdef ADS1256_SPI_DEBUG
  printf("[ADS1256] Receiving %d bytes\n\r", n);
#endif
  for (uint_fast8_t i = 0; i < n; ++i) {
    data[i] = ADS1256_ReceiveByte();
  }
}

