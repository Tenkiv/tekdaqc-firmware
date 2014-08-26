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
 * @file Tekdaqc_BSP.h
 * @brief Header file for the Tekdaqc.
 *
 * Contains public definitions and data types for the Tekdaqc. These include
 * peripheral pin connections, register addresses and various constants used
 * for the Tekdaqc board.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TEKDAQC_BSP_H
#define TEKDAQC_BSP_H

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "stm32f4xx_adc.h"
#include "stm32f4xx_can.h"
#include "stm32f4xx_crc.h"
#include "stm32f4xx_cryp.h"
#include "stm32f4xx_dac.h"
#include "stm32f4xx_dbgmcu.h"
#include "stm32f4xx_dcmi.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_flash.h"
#include "stm32f4xx_hash.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_iwdg.h"
#include "stm32f4xx_pwr.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_rng.h"
#include "stm32f4xx_rtc.h"
#include "stm32f4xx_sdio.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_syscfg.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_wwdg.h"
#include "misc.h"

/** @addtogroup tekdaqc_firmware_libraries Tekdaqc Firmware Libraries
 * @{
 */

/* #define TEKDAQC_BOARD_TYPE ((char) 'C') */
#define TEKDAQC_BOARD_TYPE ((char) 'D')

/** @addtogroup command_parser Command Parser
  * @{
  */

#define SIZE_TOSTRING_BUFFER 512U

/**
 * The maximum length of a command line
 */
#define MAX_COMMANDLINE_LENGTH 512U
#define MAX_NUM_ARGUMENTS		6U

/**
 * The maximum length of a command parameter key/value pair.
 */
#define MAX_COMMANDPART_LENGTH 36U

/**
 * @}
 */

/** @addtogroup tekdaqc_communication Tekdaqc Communication
  * @{
  */

/**
 * @def LOCATOR_PORT
 * @brief The port the locator service is listening on.
 */
#define LOCATOR_PORT            9800U

/**
 * @def TELNET_PORT
 * @brief The port to use for the Telnet server.
 */
#define TELNET_PORT 9801U

/**
 * @}
 */

/** @addtogroup update_rtc_driver Update/RTC Driver
  * @{
  */

#define UPDATE_FLAG_REGISTER		(RTC_BKP_DR19)
#define UPDATE_FLAG_ENABLED			0x00000001U

#define RTC_CONFIGURED_REG			(RTC_BKP_DR19)
#define RTC_CONFIGURED				0x00000002U

/**
 * @}
 */

/** @addtogroup analog_input_constants Analog Input Constants
  * @{
  */

#define EXTERNAL_MUX_DELAY 2000

#define V_REFERENCE	((float) 2.5f)
#define MAX_CODE 8388607U

#define NUM_ANALOG_INPUTS 37U
#define NUM_EXT_ANALOG_INPUTS 32U
#define NUM_CAL_ANALOG_INPUTS 1U
#define NUM_INT_ANALOG_INPUTS 4U

#define NUM_SAMPLE_RATES			16U
#define NUM_PGA_SETTINGS			7U
#define NUM_BUFFER_SETTINGS			2U
#define NUM_INPUT_RANGES			2U

#define CALIBRATION_VALID_MAX_TEMP	(50.0f)
#define CALIBRATION_VALID_MIN_TEMP  (0.0f)

#define ANALOG_SCALE_5V_STRING		"ANALOG_SCALE_5V"
#define ANALOG_SCALE_400V_STRING	"ANAlOG_SCALE_400V"

typedef enum {
	ANALOG_SCALE_5V, ANALOG_SCALE_400V, INVALID_SCALE
} ANALOG_INPUT_SCALE_t;

/**
 * @def NULL_CHANNEL
 * @brief Value to represent that an channel is not being used.
 */
#define NULL_CHANNEL 				((uint8_t) 255U)

#define SINGLE_ANALOG_WRITE_COUNT	10U

/**
 * @}
 */

/** @addtogroup analog_input_multiplexer Analog Input Multiplexer
  * @{
  */

/* The external input settings for the internal MUX */
#define EXTERNAL_ANALOG_IN_AINP (ADS1256_AIN0)
#define EXTERNAL_ANALOG_IN_AINN (ADS1256_AIN1)

/* 9V analog supply input */
#define SUPPLY_9V_AINP (ADS1256_AIN3)
#define SUPPLY_9V_AINN (ADS1256_AIN_COM)
/* 5V analog supply input */
#define SUPPLY_5V_AINP (ADS1256_AIN4)
#define SUPPLY_5V_AINN (ADS1256_AIN_COM)
/* 3.3V analog supply input */
#define SUPPLY_3_3V_AINP (ADS1256_AIN7)
#define SUPPLY_3_3V_AINN (ADS1256_AIN_COM)
/* Cold junction input */
#define COLD_JUNCTION_AINP (ADS1256_AIN6)
#define COLD_JUNCTION_AINN (ADS1256_AIN_COM)

typedef enum { /* We explicitly count here because the telnet server will rely on these numbers */
	EXTERNAL_0 = 0U,
	EXTERNAL_1 = 1U,
	EXTERNAL_2 = 2U,
	EXTERNAL_3 = 3U,
	EXTERNAL_4 = 4U,
	EXTERNAL_5 = 5U,
	EXTERNAL_6 = 6U,
	EXTERNAL_7 = 7U,
	EXTERNAL_8 = 8U,
	EXTERNAL_9 = 9U,
	EXTERNAL_10 = 10U,
	EXTERNAL_11 = 11U,
	EXTERNAL_12 = 12U,
	EXTERNAL_13 = 13U,
	EXTERNAL_14 = 14U,
	EXTERNAL_15 = 15U,
	EXTERNAL_16 = 16U,
	EXTERNAL_17 = 17U,
	EXTERNAL_18 = 18U,
	EXTERNAL_19 = 19U,
	EXTERNAL_20 = 20U,
	EXTERNAL_21 = 21U,
	EXTERNAL_22 = 22U,
	EXTERNAL_23 = 23U,
	EXTERNAL_24 = 24U,
	EXTERNAL_25 = 25U,
	EXTERNAL_26 = 26U,
	EXTERNAL_27 = 27U,
	EXTERNAL_28 = 28U,
	EXTERNAL_29 = 29U,
	EXTERNAL_30 = 30U,
	EXTERNAL_31 = 31U,
	EXTERNAL_OFFSET_CAL = 32U,
	IN_SUPPLY_9V = 33U,
	IN_SUPPLY_5V = 34U,
	IN_SUPPLY_3_3V = 35U,
	IN_COLD_JUNCTION = 36U
} PhysicalAnalogInput_t;

typedef enum {
	SUPPLY_9V = 0U, SUPPLY_5V = 1U, SUPPLY_3_3V = 2U, COLD_JUNCTION = 3U, EXTERNAL_ANALOG_IN = 4U
} InternalAnalogInput_t;

typedef enum {
	EXTERN_0 = 0x1800U,
	EXTERN_1 = 0x1000U,
	EXTERN_2 = 0x3000U,
	EXTERN_3 = 0x3800U,
	EXTERN_4 = 0x7800U,
	EXTERN_5 = 0x5000U,
	EXTERN_6 = 0x6000U,
	EXTERN_7 = 0x5800U,
	EXTERN_8 = 0x9800U,
	EXTERN_9 = 0x9000U,
	EXTERN_10 = 0xB000U,
	EXTERN_11 = 0xB800U,
	EXTERN_12 = 0xF800U,
	EXTERN_13 = 0xD000U,
	EXTERN_14 = 0xE000U,
	EXTERN_15 = 0xD800U,
	EXTERN_16 = 0x0000U,
	EXTERN_17 = 0x0800U,
	EXTERN_18 = 0x2800U,
	EXTERN_19 = 0x2000U,
	EXTERN_20 = 0x4000U,
	EXTERN_21 = 0x4800U,
	EXTERN_22 = 0x6800U,
	EXTERN_23 = 0x7000U,
	EXTERN_24 = 0x8000U,
	EXTERN_25 = 0x8800U,
	EXTERN_26 = 0xA800U,
	EXTERN_27 = 0xA000U,
	EXTERN_28 = 0xC000U,
	EXTERN_29 = 0xC800U,
	EXTERN_30 = 0xE800U,
	EXTERN_31 = 0xF000U,

} ExternalMuxedInput_t;

/**
 * @}
 */

/** @addtogroup board_channel_constants General Board IO Channel Constants
  * @{
  */

/**
 * @brief Tekdaqc logic level enumeration.
 * Defines the possible status states of an digital input/output to the Tekdaqc.
 */
typedef enum {
	LOGIC_HIGH, /**< Indicates a logic high value. */
	LOGIC_LOW /**< Indicates a logic low value. */
} DigitalLevel_t;

/**
 * @brief Enumeration to indicate if a channel has been added to the Tekdaqc or not.
 * Defines the two possible states of an input or output channel, both analog and digital.
 */
typedef enum {
	CHANNEL_ADDED = 0U, /**< The channel has been added. */
	CHANNEL_NOTADDED = 1U /**< The channel has not been added. */
} ChannelAdded_t;

/**
 * @}
 */

/** @addtogroup ads1256_driver ADS1256 Driver
  * @{
  */

/**
 * @def ADS1256_CLK_FREQ
 * @brief The operating frequency of the ADS1256.
 */
#define ADS1256_CLK_FREQ ((uint32_t) 7680000)
#define ADS1256_CLK_PERIOD_US 0.13020833333333f

/* ADS1256 SPI Interface pins  */
#define ADS1256_SPI                         (SPI2)
#define ADS1256_SPI_CLK                     (RCC_APB1Periph_SPI2)
#define ADS1256_SPI_CLK_INIT                (RCC_APB1PeriphClockCmd)

#define ADS1256_SPI_SCK_PIN                 (GPIO_Pin_10)
#define ADS1256_SPI_SCK_GPIO_PORT           (GPIOB)
#define ADS1256_SPI_SCK_GPIO_CLK            (RCC_AHB1Periph_GPIOB)
#define ADS1256_SPI_SCK_SOURCE              (GPIO_PinSource10)
#define ADS1256_SPI_SCK_AF                  (GPIO_AF_SPI2)

#define ADS1256_SPI_MISO_PIN                (GPIO_Pin_14)
#define ADS1256_SPI_MISO_GPIO_PORT          (GPIOB)
#define ADS1256_SPI_MISO_GPIO_CLK           (RCC_AHB1Periph_GPIOB)
#define ADS1256_SPI_MISO_SOURCE             (GPIO_PinSource14)
#define ADS1256_SPI_MISO_AF                 (GPIO_AF_SPI2)

#define ADS1256_SPI_MOSI_PIN                (GPIO_Pin_3)
#define ADS1256_SPI_MOSI_GPIO_PORT          (GPIOI)
#define ADS1256_SPI_MOSI_GPIO_CLK           (RCC_AHB1Periph_GPIOI)
#define ADS1256_SPI_MOSI_SOURCE             (GPIO_PinSource3)
#define ADS1256_SPI_MOSI_AF                 (GPIO_AF_SPI2)

#define ADS1256_CS_PIN                      (GPIO_Pin_12)
#define ADS1256_CS_GPIO_PORT                (GPIOB)
#define ADS1256_CS_GPIO_CLK                 (RCC_AHB1Periph_GPIOB)

#define ADS1256_DRDY_PIN					(GPIO_Pin_10)
#define ADS1256_DRDY_GPIO_PORT				(GPIOA)
#define ADS1256_DRDY_GPIO_CLK				(RCC_AHB1Periph_GPIOA)

#define ADS1256_SYNC_PIN					(GPIO_Pin_12)
#define ADS1256_SYNC_GPIO_PORT				(GPIOA)
#define ADS1256_SYNC_GPIO_CLK				(RCC_AHB1Periph_GPIOA)

#define ADS1256_RESET_PIN					(GPIO_Pin_14)
#define ADS1256_RESET_GPIO_PORT				(GPIOH)
#define ADS1256_RESET_GPIO_CLK				(RCC_AHB1Periph_GPIOH)

#define EXT_ANALOG_IN_MUX_PINS				(GPIO_Pin_15 | GPIO_Pin_14 | GPIO_Pin_13 | GPIO_Pin_12 | GPIO_Pin_11)
#define EXT_ANALOG_IN_MUX_PORT				(GPIOD)
#define EXT_ANALOG_IN_GPIO_CLK				(RCC_AHB1Periph_GPIOD)
#define EXT_ANALOG_IN_BITMASK				((uint16_t) 0x07FF)

#define OCAL_CONTROL_PIN					(GPIO_Pin_12)
#define OCAL_CONTROL_GPIO_PORT				(GPIOH)
#define OCAL_CONTROL_GPIO_CLK				(RCC_AHB1Periph_GPIOH)
#define OCAL_SELECT							(Bit_RESET)
#define EXT_ANALOG_SELECT					(Bit_SET)

/**
 * @}
 */

/** @addtogroup tle7232_driver TLE7232 Driver
  * @{
  */

/* SPIDER TLE7232 SPI Interface pins  */
#define NUMBER_TLE7232_CHIPS				2
#define NUM_DIGITAL_OUTPUTS 				16U

#define TLE7232_SPI							(SPI1)
#define TLE7232_SPI_CLK                     (RCC_APB2Periph_SPI1)
#define TLE7232_SPI_CLK_INIT               	(RCC_APB2PeriphClockCmd)

#define TLE7232_SPI_SCK_PIN                 (GPIO_Pin_3)
#define TLE7232_SPI_SCK_GPIO_PORT           (GPIOB)
#define TLE7232_SPI_SCK_GPIO_CLK            (RCC_AHB1Periph_GPIOB)
#define TLE7232_SPI_SCK_SOURCE              (GPIO_PinSource3)
#define TLE7232_SPI_SCK_AF                  (GPIO_AF_SPI1)

#define TLE7232_SPI_MISO_PIN                (GPIO_Pin_4)
#define TLE7232_SPI_MISO_GPIO_PORT          (GPIOB)
#define TLE7232_SPI_MISO_GPIO_CLK           (RCC_AHB1Periph_GPIOB)
#define TLE7232_SPI_MISO_SOURCE             (GPIO_PinSource4)
#define TLE7232_SPI_MISO_AF                 (GPIO_AF_SPI1)

#define TLE7232_SPI_MOSI_PIN                (GPIO_Pin_5)
#define TLE7232_SPI_MOSI_GPIO_PORT          (GPIOB)
#define TLE7232_SPI_MOSI_GPIO_CLK           (RCC_AHB1Periph_GPIOB)
#define TLE7232_SPI_MOSI_SOURCE            	(GPIO_PinSource5)
#define TLE7232_SPI_MOSI_AF                	(GPIO_AF_SPI1)

#define TLE7232_CS_PIN                      (GPIO_Pin_15)
#define TLE7232_CS_GPIO_PORT                (GPIOA)
#define TLE7232_CS_GPIO_CLK                 (RCC_AHB1Periph_GPIOA)

#define TLE7232_RESET_PIN					(GPIO_Pin_13)
#define TLE7232_RESET_GPIO_PORT				(GPIOC)
#define TLE7232_RESET_GPIO_CLK				(RCC_AHB1Periph_GPIOC)

#define GPOn								16

typedef enum {
	GPO0 = 0,
	GPO1 = 1,
	GPO2 = 2,
	GPO3 = 3,
	GPO4 = 4,
	GPO5 = 5,
	GPO6 = 6,
	GPO7 = 7,
	GPO8 = 8,
	GPO9 = 9,
	GPO10 = 10,
	GPO11 = 11,
	GPO12 = 12,
	GPO13 = 13,
	GPO14 = 14,
	GPO15 = 15,
	GPO16 = 16,
	GPO17 = 17,
	GPO18 = 18,
	GPO19 = 19,
	GPO20 = 20,
	GPO21 = 21,
	GPO22 = 22,
	GPO23 = 23
} GPO_TypeDef;

/**
 * @}
 */

/** @addtogroup digital_input_driver Digital Input Driver
  * @{
  */

/* Digital Input Interface pins  */
/* GPI0 */
#define GPI0_PIN							(GPIO_Pin_5)
#define GPI0_GPIO_PORT						(GPIOE)
/* GPI1 */
#define GPI1_PIN							(GPIO_Pin_4)
#define GPI1_GPIO_PORT						(GPIOE)
/* GPI2 */
#define GPI2_PIN							(GPIO_Pin_8)
#define GPI2_GPIO_PORT						(GPIOI)
/* GPI3 */
#define GPI3_PIN							(GPIO_Pin_11)
#define GPI3_GPIO_PORT						(GPIOI)
/* GPI4 */
#define GPI4_PIN							(GPIO_Pin_0)
#define GPI4_GPIO_PORT						(GPIOH)
/* GPI5 */
#define GPI5_PIN							(GPIO_Pin_4)
#define GPI5_GPIO_PORT						(GPIOH)
/* GPI6 */
#define GPI6_PIN							(GPIO_Pin_11)
#define GPI6_GPIO_PORT						(GPIOF)
/* GPI7 */
#define GPI7_PIN							(GPIO_Pin_15)
#define GPI7_GPIO_PORT						(GPIOF)
/* GPI8 */
#define GPI8_PIN							(GPIO_Pin_8)
#define GPI8_GPIO_PORT						(GPIOE)
/* GPI9 */
#define GPI9_PIN							(GPIO_Pin_12)
#define GPI9_GPIO_PORT						(GPIOE)
/* GPI10 */
#define GPI10_PIN							(GPIO_Pin_6)
#define GPI10_GPIO_PORT						(GPIOH)
/* GPI11 */
#define GPI11_PIN							(GPIO_Pin_11)
#define GPI11_GPIO_PORT						(GPIOH)
/* GPI12 */
#define GPI12_PIN							(GPIO_Pin_3)
#define GPI12_GPIO_PORT						(GPIOE)
/* GPI13 */
#define GPI13_PIN							(GPIO_Pin_2)
#define GPI13_GPIO_PORT						(GPIOE)
/* GPI14 */
#define GPI14_PIN							(GPIO_Pin_6)
#define GPI14_GPIO_PORT						(GPIOE)
/* GPI15 */
#define GPI15_PIN							(GPIO_Pin_14)
#define GPI15_GPIO_PORT						(GPIOC)
/* GPI16 */
#define GPI16_PIN							(GPIO_Pin_9)
#define GPI16_GPIO_PORT						(GPIOF)
/* GPI17 */
#define GPI17_PIN							(GPIO_Pin_2)
#define GPI17_GPIO_PORT						(GPIOH)
/* GPI18 */
#define GPI18_PIN							(GPIO_Pin_1)
#define GPI18_GPIO_PORT						(GPIOB)
/* GPI19 */
#define GPI19_PIN							(GPIO_Pin_13)
#define GPI19_GPIO_PORT						(GPIOF)
/* GPI20 */
#define GPI20_PIN							(GPIO_Pin_1)
#define GPI20_GPIO_PORT						(GPIOG)
/* GPI21 */
#define GPI21_PIN							(GPIO_Pin_10)
#define GPI21_GPIO_PORT						(GPIOE)
/* GPI22 */
#define GPI22_PIN							(GPIO_Pin_8)
#define GPI22_GPIO_PORT						(GPIOH)
/* GPI23 */
#define GPI23_PIN							(GPIO_Pin_10)
#define GPI23_GPIO_PORT						(GPIOH)

#define GPI_PORTB_PINS						(GPI18_PIN)
#define GPI_PORTC_PINS						(GPI15_PIN)
#define GPI_PORTE_PINS						(GPI0_PIN | GPI1_PIN | GPI8_PIN | GPI9_PIN | GPI12_PIN | GPI13_PIN | GPI14_PIN | GPI21_PIN)
#define GPI_PORTF_PINS						(GPI6_PIN | GPI7_PIN | GPI16_PIN | GPI19_PIN)
#define GPI_PORTG_PINS						(GPI20_PIN)
#define GPI_PORTH_PINS						(GPI4_PIN | GPI5_PIN | GPI10_PIN | GPI11_PIN | GPI17_PIN | GPI22_PIN | GPI23_PIN)
#define GPI_PORTI_PINS						(GPI2_PIN | GPI3_PIN)

#define GPI_GPIO_CLKS						(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOE \
		| RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG | RCC_AHB1Periph_GPIOH \
		| RCC_AHB1Periph_GPIOI)

#define NUM_DIGITAL_INPUTS 				24U

typedef enum {
	GPI0 = 0,
	GPI1 = 1,
	GPI2 = 2,
	GPI3 = 3,
	GPI4 = 4,
	GPI5 = 5,
	GPI6 = 6,
	GPI7 = 7,
	GPI8 = 8,
	GPI9 = 9,
	GPI10 = 10,
	GPI11 = 11,
	GPI12 = 12,
	GPI13 = 13,
	GPI14 = 14,
	GPI15 = 15,
	GPI16 = 16,
	GPI17 = 17,
	GPI18 = 18,
	GPI19 = 19,
	GPI20 = 20,
	GPI21 = 21,
	GPI22 = 22,
	GPI23 = 23
} GPI_TypeDef;

/**
 * @}
 */

/** @addtogroup ethernet_driver Ethernet Driver
  * @{
  */

/* Ethernet defines */
#define USE_DHCP       					/* enable DHCP, if disabled static address is used */

#define DP83848_PHY_ADDRESS       		0x01

#define ETHERNET_GPIO_CLKS				(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOG)

#define ETH_MDIO_PIN					(GPIO_Pin_2)
#define ETH_MDIO_GPIO_PORT				(GPIOA)
#define ETH_MDIO_SOURCE             	(GPIO_PinSource2)

#define ETH_MDC_PIN						(GPIO_Pin_1)
#define ETH_MDC_GPIO_PORT				(GPIOC)
#define ETH_MDC_SOURCE             		(GPIO_PinSource1)

#define ETH_RMII_REF_CLK_PIN			(GPIO_Pin_1)
#define ETH_RMII_REF_CLK_GPIO_PORT		(GPIOA)
#define ETH_RMII_REF_CLK_SOURCE         (GPIO_PinSource1)

#define ETH_RMII_CRS_DV_PIN				(GPIO_Pin_7)
#define ETH_RMII_CRS_DV_GPIO_PORT		(GPIOA)
#define ETH_RMII_CRS_DV_SOURCE          (GPIO_PinSource7)

#define ETH_RMII_RXD0_PIN				(GPIO_Pin_4)
#define ETH_RMII_RXD0_GPIO_PORT			(GPIOC)
#define ETH_RMII_RXD0_SOURCE         	(GPIO_PinSource4)

#define ETH_RMII_RXD1_PIN				(GPIO_Pin_5)
#define ETH_RMII_RXD1_GPIO_PORT			(GPIOC)
#define ETH_RMII_RXD1_SOURCE         	(GPIO_PinSource5)

#define ETH_RMII_TX_EN_PIN				(GPIO_Pin_11)
#define ETH_RMII_TX_EN_GPIO_PORT		(GPIOB)
#define ETH_RMII_TX_EN_SOURCE         	(GPIO_PinSource11)

#define ETH_RMII_TXD0_PIN				(GPIO_Pin_13)
#define ETH_RMII_TXD0_GPIO_PORT			(GPIOG)
#define ETH_RMII_TXD0_SOURCE         	(GPIO_PinSource13)

#define ETH_RMII_TXD1_PIN				(GPIO_Pin_13)
#define ETH_RMII_TXD1_GPIO_PORT			(GPIOB)
#define ETH_RMII_TXD1_SOURCE         	(GPIO_PinSource13)

/* Specific defines for EXTI line, used to manage Ethernet link status */
#define ETH_LINK_EXTI_LINE             (EXTI_Line14)
#define ETH_LINK_EXTI_PORT_SOURCE      (EXTI_PortSourceGPIOB)
#define ETH_LINK_EXTI_PIN_SOURCE       (EXTI_PinSource14)
#define ETH_LINK_EXTI_IRQn             (EXTI15_10_IRQn)
/* PB14 */
#define ETH_LINK_PIN                   (GPIO_Pin_14)
#define ETH_LINK_GPIO_PORT             (GPIOB)
#define ETH_LINK_GPIO_CLK              (RCC_AHB1Periph_GPIOB)

/* MAC ADDRESS: MAC_ADDR0:MAC_ADDR1:MAC_ADDR2:MAC_ADDR3:MAC_ADDR4:MAC_ADDR5 */
#define MAC_ADDR0   0U
#define MAC_ADDR1   25U
#define MAC_ADDR2   13U
#define MAC_ADDR3   8U
#define MAC_ADDR4   0U
#define MAC_ADDR5   0U

/* Static IP ADDRESS: IP_ADDR0.IP_ADDR1.IP_ADDR2.IP_ADDR3 */
#define IP_ADDR0   192U
#define IP_ADDR1   168U
#define IP_ADDR2   1U
#define IP_ADDR3   27U

/* NETMASK */
#define NETMASK_ADDR0   255U
#define NETMASK_ADDR1   255U
#define NETMASK_ADDR2   255U
#define NETMASK_ADDR3   0U

/* Gateway Address */
#define GW_ADDR0   192U
#define GW_ADDR1   168U
#define GW_ADDR2   1U
#define GW_ADDR3   1U

/**
 * @}
 */

#define CANx                       (CAN1)
#define CAN_CLK                    (RCC_APB1Periph_CAN1)
#define CAN_RX_PIN                 (GPIO_Pin_0)
#define CAN_TX_PIN                 (GPIO_Pin_1)
#define CAN_GPIO_PORT              (GPIOD)
#define CAN_GPIO_CLK               (RCC_AHB1Periph_GPIOD)
#define CAN_AF_PORT                (GPIO_AF_CAN1)
#define CAN_RX_SOURCE              (GPIO_PinSource0)
#define CAN_TX_SOURCE              (GPIO_PinSource1)

/** @addtogroup com_port_driver COM Port Driver
  * @{
  */

typedef enum {
	COM1 = 0, COM2 = 1
} COM_TypeDef;

#define COMn                             2

/**
 * @brief Definition for COM port1, connected to USART3
 */
#define COM1_USART                  (USART2)
#define COM1_CLK                    (RCC_APB1Periph_USART2)
#define COM1_TX_PIN                 (GPIO_Pin_5)
#define COM1_TX_GPIO_PORT           (GPIOD)
#define COM1_TX_GPIO_CLK            (RCC_AHB1Periph_GPIOD)
#define COM1_TX_SOURCE              (GPIO_PinSource5)
#define COM1_TX_AF                  (GPIO_AF_USART2)
#define COM1_RX_PIN                 (GPIO_Pin_6)
#define COM1_RX_GPIO_PORT           (GPIOD)
#define COM1_RX_GPIO_CLK            (RCC_AHB1Periph_GPIOD)
#define COM1_RX_SOURCE              (GPIO_PinSource6)
#define COM1_RX_AF                  (GPIO_AF_USART2)
#define COM1_RTS_PIN                (GPIO_Pin_4)
#define COM1_RTS_GPIO_PORT          (GPIOD)
#define COM1_RTS_GPIO_CLK           (RCC_AHB1Periph_GPIOD)
#define COM1_RTS_SOURCE             (GPIO_PinSource4)
#define COM1_RTS_AF                 (GPIO_AF_USART2)
#define COM1_CTS_PIN                (GPIO_Pin_3)
#define COM1_CTS_GPIO_PORT          (GPIOD)
#define COM1_CTS_GPIO_CLK           (RCC_AHB1Periph_GPIOD)
#define COM1_CTS_SOURCE             (GPIO_PinSource3)
#define COM1_CTS_AF                 (GPIO_AF_USART2)
#define COM1_IRQn                   (USART2_IRQn)

/**
 * @brief Definition for COM port 2, connected to USART3
 */
#define COM2_USART                  (USART3)
#define COM2_CLK                    (RCC_APB1Periph_USART3)
#define COM2_TX_PIN                 (GPIO_Pin_8)
#define COM2_TX_GPIO_PORT           (GPIOD)
#define COM2_TX_GPIO_CLK            (RCC_AHB1Periph_GPIOD)
#define COM2_TX_SOURCE              (GPIO_PinSource8)
#define COM2_TX_AF                  (GPIO_AF_USART3)
#define COM2_RX_PIN                 (GPIO_Pin_9)
#define COM2_RX_GPIO_PORT           (GPIOD)
#define COM2_RX_GPIO_CLK            (RCC_AHB1Periph_GPIOD)
#define COM2_RX_SOURCE              (GPIO_PinSource9)
#define COM2_RX_AF                  (GPIO_AF_USART3)
#define COM2_IRQn                   (USART3_IRQn)

/**
 * @}
 */

/** @addtogroup calibration_table Calibration Table
  * @{
  */

#define CALIBRATION_LATENCY			(FLASH_Latency_5)
#define FLASH_VOLTAGE_RANGE			(VoltageRange_3)

#define CALIBRATION_SECTOR			(FLASH_Sector_11) /* 128KB */
#define CALIBRATION_WPSECTOR		(OB_WRP_Sector_11)
#define ADDR_CALIBRATION_BASE	    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */
#define ADDR_CALIBRATION_END		((uint32_t)0x080FFFFF)

#define CALIBRATION_TEMP_OFFSET		((NUM_INPUT_RANGES + NUM_BUFFER_SETTINGS) * NUM_PGA_SETTINGS * NUM_SAMPLE_RATES)

#define BOARD_SERIAL_NUM_ADDR		(ADDR_CALIBRATION_BASE)
#define BOARD_SERIAL_NUM_LENGTH		32 /* Serial number is 32 bytes long (32 chars) */

#define CAL_TEMP_LOW_ADDR			(BOARD_SERIAL_NUM_ADDR + BOARD_SERIAL_NUM_LENGTH)
#define CAL_TEMP_HIGH_ADDR			(CAL_TEMP_LOW_ADDR + 4)
#define CAL_TEMP_STEP_ADDR			(CAL_TEMP_HIGH_ADDR + 4)
#define CAL_TEMP_CNT_ADDR			(CAL_TEMP_STEP_ADDR + 4)
#define CAL_VALID_ADDR				(CAL_TEMP_CNT_ADDR + 4)
#define COLD_JUNCTION_OFFSET_ADDR	(CAL_VALID_ADDR + 1)
#define COLD_JUNCTION_GAIN_ADDR		(COLD_JUNCTION_OFFSET_ADDR + 1)
#define CAL_DATA_START_ADDR			(COLD_JUNCTION_GAIN_ADDR + 2) /* This needs to be word aligned */

/**
 * @}
 */

/** @addtogroup flash_disk_driver FLASH Disk Driver
  * @{
  */

/* EEPROM start address in Flash */
#define EEPROM_START_ADDRESS  ((uint32_t)0x080A0000) /* EEPROM emulation start address:
                                                  from sector2 : after 16KByte of used
                                                  Flash memory */

/* Define the size of the sectors to be used */
#define PAGE_SIZE             ((uint32_t) 0x20000)  /* Page size = 128KByte */

/* Pages 0 and 1 base and end addresses */
#define PAGE0_BASE_ADDRESS    ((uint32_t)(EEPROM_START_ADDRESS + 0x0000))
#define PAGE0_END_ADDRESS     ((uint32_t)(EEPROM_START_ADDRESS + (PAGE_SIZE - 1)))
#define PAGE0_ID              (FLASH_Sector_9)

#define PAGE1_BASE_ADDRESS    ((uint32_t)(EEPROM_START_ADDRESS + PAGE_SIZE))
#define PAGE1_END_ADDRESS     ((uint32_t)(EEPROM_START_ADDRESS + (2 * PAGE_SIZE - 1)))
#define PAGE1_ID              (FLASH_Sector_10)

#define NUM_EEPROM_ADDRESSES			4

#define ADDR_BOARD_MAX_TEMP_HIGH		0x0000
#define ADDR_BOARD_MAX_TEMP_LOW			0x0001
#define ADDR_BOARD_MIN_TEMP_HIGH		0x0002
#define ADDR_BOARD_MIN_TEMP_LOW			0x0003

/* Virtual address defined by the user: 0xFFFF value is prohibited */
extern uint16_t EEPROM_ADDRESSES[NUM_EEPROM_ADDRESSES];

/**
 * @}
 */

/** @addtogroup debug_constants Debug Constants
  * @{
  */

#ifdef DEBUG
typedef enum {
	PIN1 = 0, PIN2 = 1, PIN3 = 2, PIN4 = 3
} TestPin_TypeDef;

#define TEST_PINn                        4

#define TEST_PIN1                        (GPIO_Pin_6)
#define TEST_PIN1_GPIO_PORT              (GPIOG)
#define TEST_PIN1_GPIO_CLK               (RCC_AHB1Periph_GPIOG)

#define TEST_PIN2                        (GPIO_Pin_8)
#define TEST_PIN2_GPIO_PORT              (GPIOG)
#define TEST_PIN2_GPIO_CLK               (RCC_AHB1Periph_GPIOG)

#define TEST_PIN3                        (GPIO_Pin_8)
#define TEST_PIN3_GPIO_PORT              (GPIOI)
#define TEST_PIN3_GPIO_CLK               (RCC_AHB1Periph_GPIOI)

#define TEST_PIN4                        (GPIO_Pin_10)
#define TEST_PIN4_GPIO_PORT              (GPIOI)
#define TEST_PIN4_GPIO_CLK               (RCC_AHB1Periph_GPIOI)
#endif

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* TEKDAQC_BSP_H */
