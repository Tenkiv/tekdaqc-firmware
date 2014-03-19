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
 * @brief Header file for the ADS1256 SPI driver.
 *
 * Deals with low level communication with ADS1256 ADC via SPI.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef ADS1256_SPI_CONTROLLER_H_
#define ADS1256_SPI_CONTROLLER_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif
   
/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "Tekdaqc_Debug.h"

/** @addtogroup tekdaqc_firmware_libraries Tekdaqc Firmware Libraries
 * @{
 */

/** @addtogroup ads1256_driver ADS1256 Driver
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/
  
/*
 * @def ADS1256_CS_LOW()
 * @brief Select ADS1256: Chip Select pin low
 * */
#define ADS1256_CS_LOW()       (GPIO_ResetBits(ADS1256_CS_GPIO_PORT, ADS1256_CS_PIN))

/**
 * @def ADS1256_CS_HIGH()
 * @brief De-select ADS1256: Chip Select pin high
 * */
#define ADS1256_CS_HIGH()      (GPIO_SetBits(ADS1256_CS_GPIO_PORT, ADS1256_CS_PIN))

/*--------------------------------------------------------------------------------------------------------*/
/* INITIALIZATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/*
 * @brief Initialize the ADS1256 connection
 */
void ADS1256_SPI_Init(void);

/*
 * @brief De-initialize the ADS1256 connection
 */
void ADS1256_SPI_DeInit(void);  

/*--------------------------------------------------------------------------------------------------------*/
/* PIN CONTROL METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/*
 * @brief Switch the SPI clock pin to GPIO control.
 */
void ADS1256_CLK_To_GPIO(void);

/**
 * @brief Switch the SPI clock pin to SPI control.
 */
void ADS1256_GPIO_To_CLK(void);

/*--------------------------------------------------------------------------------------------------------*/
/* COMMUNICATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/*
 * @brief Send a byte over the SPI line.
 */
uint8_t ADS1256_SendByte(uint8_t data);

/**
 * @brief Send an array of bytes over the SPI line.
 */
void ADS1256_SendBytes(uint8_t* data, uint8_t n);

/**
 * @brief Receive a byte over the SPI line.
 */
uint8_t ADS1256_ReceiveByte(void);

/**
 * @brief Receive an array of bytes over the SPI line.
 */
void ADS1256_ReceiveBytes(uint8_t* data, uint8_t n);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

/**
 * @}
 */

#endif /* ADS1256_SPI_CONTROLLER_H_ */
