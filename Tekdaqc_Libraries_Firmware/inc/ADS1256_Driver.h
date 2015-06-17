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
 * @file ADS1256_Driver.h
 * @brief Header file for the ADS1256 ADC driver.
 *
 * Contains public definitions and data types for the ADS1256 ADC driver.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef ADS1256_DRIVER_H_
#define ADS1256_DRIVER_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "stm32f4xx.h"
#include "Tekdaqc_Debug.h"
#include "ADS1256_SPI_Controller.h"
#include "boolean.h"

/** @addtogroup tekdaqc_firmware_libraries Tekdaqc Firmware Libraries
 * @{
 */

/** @addtogroup ads1256_driver ADS1256 Driver
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/**
* @def ADS1256_DUMMY_BYTE
* @brief The padding value to send over SPI when reading data from the ADS1256.
*/
#define ADS1256_DUMMY_BYTE ((uint8_t) 0x00)

/** @defgroup status_register STATUS Register
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* STATUS REGISTER BITS */
/*--------------------------------------------------------------------------------------------------------*/

/**
* @def ADS1256_DRDY_BIT
* @brief The data ready bit starting position in the STATUS register.
*/
#define ADS1256_DRDY_BIT        0

/**
* @def ADS1256_DRDY_SPAN
* @brief The data ready bit count in the STATUS register.
*/
#define ADS1256_DRDY_SPAN       1
   
/**
* @def ADS1256_BUFFEN_BIT
* @brief The analog buffer enable bit starting position in the STATUS register.
*/
#define ADS1256_BUFFEN_BIT      1
   
/**
* @def ADS1256_BUFFEN_SPAN
* @brief The analog buffer enable bit count in the STATUS register.
*/
#define ADS1256_BUFFEN_SPAN     1
   
/**
* @def ADS1256_ACAL_BIT
* @brief The automatic calibration enable bit starting position in the STATUS register.
*/
#define ADS1256_ACAL_BIT        2
 
/**
* @def ADS1256_ACAL_SPAN
* @brief The automatic calibration enable bit count in the STATUS register.
*/
#define ADS1256_ACAL_SPAN       1
   
/**
* @def ADS1256_ORDER_BIT
* @brief The data output order bit starting position in the STATUS register.
*/
#define ADS1256_ORDER_BIT       3
   
/**
* @def ADS1256_ORDER_SPAN
* @brief The data output order bit count in the STATUS register.
*/
#define ADS1256_ORDER_SPAN      1
   
/**
* @def ADS1256_ID_BIT
* @brief The factory programmed ID bit starting position in the STATUS register.
*/
#define ADS1256_ID_BIT          4
   
/**
* @def ADS1256_ID_SPAN
* @brief The factory programmed ID bit count in the STATUS register.
*/
#define ADS1256_ID_SPAN         4

/**
 * @}
 */

/**
 * @defgroup adcon_register ADCON Register
 * @{
 */

/*--------------------------------------------------------------------------------------------------------*/
/* ADCON REGISTER BITS */
/*--------------------------------------------------------------------------------------------------------*/

/**
* @def ADS1256_CO_BIT
* @brief The clock out setting bit starting position in the ADCON register.
*/
#define ADS1256_CO_BIT          ((uint8_t) 5)

/**
* @def ADS1256_CO_SPAN
* @brief The clock out setting bit count in the ADCON register.
*/
#define ADS1256_CO_SPAN         ((uint8_t) 2)

/**
* @def ADS1256_SD_BIT
* @brief The sensor detect current setting bit starting position in the ADCON register.
*/
#define ADS1256_SD_BIT          ((uint8_t) 3)

/**
* @def ADS1256_SD_SPAN
* @brief The sensor detect current setting bit count in the ADCON register.
*/
#define ADS1256_SD_SPAN         ((uint8_t) 2)

/**
* @def ADS1256_PGA_BIT
* @brief The programmable gain amplifier setting bit starting position in the ADCON register.
*/
#define ADS1256_PGA_BIT         ((uint8_t) 0)

/**
* @def ADS1256_PGA_SPAN
* @brief The programmable gain amplifier setting bit count in the ADCON register.
*/
#define ADS1256_PGA_SPAN        ((uint8_t) 3)

/**
 * @}
 */

/**
 * @defgroup  io_register IO Register
 * @{
 */

/*--------------------------------------------------------------------------------------------------------*/
/* IO REGISTER BITS */
/*--------------------------------------------------------------------------------------------------------*/

/**
* @def ADS1256_GPIO_DIR_OFFSET
* @brief Offset in the IO register to the direction bits.
*/
#define ADS1256_GPIO_DIR_OFFSET 4

/**
* @def ADS1256_GPIO_BIT_SPAN
* @brief The bit count for one GPIO pin int the IO register.
*/
#define ADS1256_GPIO_BIT_SPAN 1

/**
 * @}
 */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED TYPES */
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/
/* STATUS REGISTER SETTINGS */
/*--------------------------------------------------------------------------------------------------------*/
/**
* @brief Data ready flag enumeration.
* Defines the two possible states of the ADS1256 data ready flag.
*/
typedef enum {
  ADS1256_DATA_READY = ((uint8_t) 0x00), /**< Valid data is ready.*/
  ADS1256_DATA_NOTREADY = ((uint8_t) 0x01) /**< Valid data is not ready.*/
} ADS1256_DATA_READY_t;

/**
* @brief Analog input buffer flag enumeration.
* Defines the two possible states of the ADS1256 analog input buffer flag.
*/
typedef enum {
  ADS1256_BUFFER_DISABLED = ((uint8_t) 0x00), /**< Analog input buffer disabled. (Default)*/
  ADS1256_BUFFER_ENABLED = ((uint8_t) 0x01) /**< Analog input buffer enabled.*/
} ADS1256_BUFFER_t;

/**
* @brief Automatic calibration flag enumeration.
* Defines the two possible states of the ADS1256 automatic calibration flag.
*/
typedef enum {
  ADS1256_ACAL_DISABLED = ((uint8_t) 0x00), /**< Automatic calibration is disabled (Default).*/
  ADS1256_ACAL_ENABLED = ((uint8_t) 0x01) /**< Automatic calibration is enabled.*/
} ADS1256_ACAL_t;

/**
* @brief Data output order flag enumeration.
* Defines the two possible states of the ADS1256 data output order flag.
*/
typedef enum {
  ADS1256_MSB_FIRST = ((uint8_t) 0x00), /**< Data is output most significant bit first (Default).*/
  ADS1256_LSB_FIRST = ((uint8_t) 0x01) /**< Data is output least significant bit first.*/
} ADS1256_ORDER_t;

/*--------------------------------------------------------------------------------------------------------*/
/* MUX REGISTER SETTINGS */
/*--------------------------------------------------------------------------------------------------------*/

/**
* @brief Analog input channel enumeration.
* Defines the register values for each analog input selectable by the ADS1256's multiplexer.
*/
typedef enum {
  ADS1256_AIN0 = ((uint8_t) 0x00), /**< Analog input 0 (Default Positive).*/
  ADS1256_AIN1 = ((uint8_t) 0x01), /**< Analog input 1 (Default Negative).*/
  ADS1256_AIN2 = ((uint8_t) 0x02), /**< Analog input 2.*/
  ADS1256_AIN3 = ((uint8_t) 0x03), /**< Analog input 3.*/
  ADS1256_AIN4 = ((uint8_t) 0x04), /**< Analog input 4.*/
  ADS1256_AIN5 = ((uint8_t) 0x05), /**< Analog input 5.*/
  ADS1256_AIN6 = ((uint8_t) 0x06), /**< Analog input 6.*/
  ADS1256_AIN7 = ((uint8_t) 0x07), /**< Analog input 7.*/
  ADS1256_AIN_COM = ((uint8_t) 0x08) /**< Common analog input.*/
} ADS1256_AIN_t;

/**
* @def IS_ADS1256_AIN_SETTING(AIN)
* @brief Checks that the specified value is a valid analog input channel.
*/
#define IS_ADS1256_AIN_SETTING(AIN) (((AIN) == ADS1256_AIN0) || \
  ((AIN) == ADS1256_AIN1) || \
  ((AIN) == ADS1256_AIN2) || \
  ((AIN) == ADS1256_AIN3) || \
  ((AIN) == ADS1256_AIN4) || \
  ((AIN) == ADS1256_AIN5) || \
  ((AIN) == ADS1256_AIN6) || \
  ((AIN) == ADS1256_AIN7) || \
  ((AIN) == ADS1256_AIN_COM))

/*--------------------------------------------------------------------------------------------------------*/
/* ADCON REGISTER SETTINGS */
/*--------------------------------------------------------------------------------------------------------*/

/**
* @brief Clock out frequency setting enumeration.
* Defines the different possible clock out rates of the ADS1256.
*/
typedef enum {
  ADS1256_CLOCK_OUT_OFF = ((uint8_t) 0x00), /**< Clock out disabled. (Default)*/
  ADS1256_CLOCK_OUT_F = ((uint8_t) 0x01), /**< Clock out frequency equal to the ADS1256 clock.*/
  ADS1256_CLOCK_OUT_F2 = ((uint8_t) 0x02), /**< Clock out frequency equal to one half the ADS1256 clock.*/
  ADS1256_CLOCK_OUT_F4 = ((uint8_t) 0x03) /**< Clock out frequency equal to one quarter the ADS1256 clock.*/
} ADS1256_CLOCK_OUT_t;

/**
* @def IS_ADS1256_CLOCKOUT_SETTING(CLK)
* @brief Checks that the specified value is a valid clock out rate.
*/
#define IS_ADS1256_CLOCKOUT_SETTING(CLK) (((CLK) == ADS1256_CLOCK_OUT_OFF) || \
  ((CLK) == ADS1256_CLOCK_OUT_F) || \
  ((CLK) == ADS1256_CLOCK_OUT_F2) || \
  ((CLK) == ADS1256_CLOCK_OUT_F4))

/**
* @brief Sensor detect current source setting enumeration.
* Defines the different possible current source outputs for sensor break/short circuit detection.
*/
typedef enum {
  ADS1256_SD_OFF = ((uint8_t) 0x00), /**< Sensor detect sources disabled (Default). */
  ADS1256_SDC_0_5 = ((uint8_t) 0x01), /**< Sensor detect source of 0.5 uA. */
  ADS1256_SDC_2 = ((uint8_t) 0x02),  /**< Sensor detect source of 2.0 uA. */
  ADS1256_SDC_10 = ((uint8_t) 0x03) /**< Sensor detect source of 10 uA. */
} ADS1256_SENSOR_DETECT_t;

/**
* @def IS_ADS1256_SENSOR_DETECT_SETTING(SEN)
* @brief Checks that the specified value is a valid sensor detect current output.
*/
#define IS_ADS1256_SENSOR_DETECT_SETTING(SEN) (((SEN) == ADS1256_SD_OFF) || \
  ((SEN) == ADS1256_SDC_0_5) || \
  ((SEN) == ADS1256_SDC_2) || \
  ((SEN) == ADS1256_SDC_10))

/**
* @brief Programmable gain amplifier gain setting enumeration.
* Defines the different possible gain settings of the ADS1256's input programmable gain amplifier.
*/
typedef enum {
  ADS1256_PGAx1 = ((uint8_t) 0x00), /**< 1x Gain (Default). */
  ADS1256_PGAx2 = ((uint8_t) 0x01), /**< 2x Gain. */
  ADS1256_PGAx4 = ((uint8_t) 0x02), /**< 4x Gain. */
  ADS1256_PGAx8 = ((uint8_t) 0x03), /**< 8x Gain. */
  ADS1256_PGAx16 = ((uint8_t) 0x04), /**< 16x Gain. */
  ADS1256_PGAx32 = ((uint8_t) 0x05), /**< 32x Gain. */
  ADS1256_PGAx64 = ((uint8_t) 0x06) /**< 64x Gain. */
  /*ADS1256_PGA_x64 = ((uint8_t) 0x07)*/
} ADS1256_PGA_t;

/**
* @def IS_ADS1256_PGA_SETTING(PGA)
* @brief Checks that the specified value is a valid gain setting.
*/
#define IS_ADS1256_PGA_SETTING(PGA) (((PGA) == ADS1256_PGAx1) || \
  ((PGA) == ADS1256_PGAx2) || \
  ((PGA) == ADS1256_PGAx4) || \
  ((PGA) == ADS1256_PGAx8) || \
  ((PGA) == ADS1256_PGAx16)|| \
  ((PGA) == ADS1256_PGAx32) || \
  ((PGA) == ADS1256_PGAx64))

/*--------------------------------------------------------------------------------------------------------*/
/* DRATE REGISTER SETTINGS */
/*--------------------------------------------------------------------------------------------------------*/

/**
* @brief Data rate (samples per second) setting enumeration.
* Defines the different possible data rate (samples per second) of the ADS1256. 
*/
typedef enum {
  ADS1256_SPS_30000 = ((uint8_t) 0xF0), /**< 30,000 samples per second. */
  ADS1256_SPS_15000 = ((uint8_t) 0xE0), /**< 15,000 samples per second. */
  ADS1256_SPS_7500 = ((uint8_t) 0xD0), /**< 7,500 samples per second. */
  ADS1256_SPS_3750 = ((uint8_t) 0xC0), /**< 3,750 samples per second. */
  ADS1256_SPS_2000 = ((uint8_t) 0xB0), /**< 2,000 samples per second. */
  ADS1256_SPS_1000 = ((uint8_t) 0xA1), /**< 1,000 samples per second. */
  ADS1256_SPS_500 = ((uint8_t) 0x92), /**< 500 samples per second. */
  ADS1256_SPS_100 = ((uint8_t) 0x82), /**< 100 samples per second. */
  ADS1256_SPS_60 = ((uint8_t) 0x72), /**< 60 samples per second. */
  ADS1256_SPS_50 = ((uint8_t) 0x63), /**< 50 samples per second. */
  ADS1256_SPS_30 = ((uint8_t) 0x53), /**< 30 samples per second. */
  ADS1256_SPS_25 = ((uint8_t) 0x43), /**< 25 samples per second. */
  ADS1256_SPS_15 = ((uint8_t) 0x33), /**< 15 samples per second. */
  ADS1256_SPS_10 = ((uint8_t) 0x23), /**< 10 samples per second. */
  ADS1256_SPS_5 = ((uint8_t) 0x13), /**< 5 samples per second. */
  ADS1256_SPS_2_5 = ((uint8_t) 0x03) /**< 2.5 samples per second. */
} ADS1256_SPS_t;

/**
* @def IS_ADS1256_SPS_SETTING(SPS)
* @brief Checks that the specified value is a valid data rate setting.
*/
#define IS_ADS1256_SPS_SETTING(SPS) (((SPS) == ADS1256_SPS_30000) || \
  ((SPS) == ADS1256_SPS_15000) || \
  ((SPS) == ADS1256_SPS_7500) || \
  ((SPS) == ADS1256_SPS_3750) || \
  ((SPS) == ADS1256_SPS_2000)|| \
  ((SPS) == ADS1256_SPS_1000) || \
  ((SPS) == ADS1256_SPS_500)) || \
  ((SPS) == ADS1256_SPS_100) || \
  ((SPS) == ADS1256_SPS_60) || \
  ((SPS) == ADS1256_SPS_50) || \
  ((SPS) == ADS1256_SPS_30)|| \
  ((SPS) == ADS1256_SPS_25) || \
  ((SPS) == ADS1256_SPS_15)) || \
  ((SPS) == ADS1256_SPS_10) || \
  ((SPS) == ADS1256_SPS_5) || \
  ((SPS) == ADS1256_SPS_2_5))

/*--------------------------------------------------------------------------------------------------------*/
/* IO REGISTER SETTINGS */
/*--------------------------------------------------------------------------------------------------------*/
/**
* @brief GPIO pin enumeration.
* Defines the different GPIO pins of the ADS1256.
*/
typedef enum {
  ADS1256_GPIO0 = 0, /**< GPIO Pin 0 (Clock Out). */
  ADS1256_GPIO1 = 1, /**< GPIO Pin 1. */
  ADS1256_GPIO2 = 2, /**< GPIO Pin 2. */
  ADS1256_GPIO3 = 3 /**< GPIO Pin 3. */
} ADS1256_GPIO_t;

/**
* @brief GPIO direction enumeration.
* Defines the input and output states of the GPIO pins of the ADS1256.
*/
typedef enum {
  ADS1256_GPIO_OUTPUT = ((uint8_t) 0x00), /**< GPIO pin output. */
  ADS1256_GPIO_INPUT = ((uint8_t) 0x01) /**< GPIO pin input. */
} ADS1256_GPIO_DIRECTION_t;

/**
* @def IS_ADS1256_GPIO_DIR_SETTING(DIR)
* @brief Checks that the specified value is a valid GPIO direction state.
*/
#define IS_ADS1256_GPIO_DIR_SETTING(DIR) (((DIR) == ADS1256_GPIO_OUTPUT) || \
  ((DIR) == ADS1256_INPUT))

 /**
 * @brief GPIO logic level enumeration.
 * Defines the two logic states of the GPIO pins of the ADS1256.
*/ 
typedef enum {
  ADS1256_GPIO_LOW = ((uint8_t) 0x00), /**< Logic level low. */
  ADS1256_GPIO_HIGH = ((uint8_t) 0x01) /**< Logic level high. */
} ADS1256_GPIO_STATUS_t;

/**
* @def IS_ADS1256_GPIO_VALUE(VAL)
* @brief Checks that the specified value is a valid GPIO state.
*/
#define IS_ADS1256_GPIO_VALUE(VAL) (((VAL) == ADS1256_GPIO_LOW) || \
  ((VAL) == ADS1256_GPIO_HIGH))

/*--------------------------------------------------------------------------------------------------------*/
/* REGISTER ADDRESSES */
/*--------------------------------------------------------------------------------------------------------*/
/**
* @brief ADS1256 register enumeration.
* Defines the addresses of the 11 control registers of the ADS1256 ADC.
*/
typedef enum {
  ADS1256_STATUS        = 0x00U, /**< The status register. */
  ADS1256_MUX           = 0x01U, /**< The input multiplexer register. */
  ADS1256_ADCON         = 0x02U, /**< The converter control register. */
  ADS1256_DRATE         = 0x03U, /**< The data rate (samples per second) register. */
  ADS1256_IO            = 0x04U, /**< The GPIO control register. */
  ADS1256_OFC0          = 0x05U, /**< The least significant byte of the offset calibration. */
  ADS1256_OFC1          = 0x06U, /**< The middle byte of the offset calibration. */
  ADS1256_OFC2          = 0x07U, /**< The most significant byte of the offset calibration. */
  ADS1256_FSC0          = 0x08U, /**< The least significant byte of the gain calibration. */
  ADS1256_FSC1          = 0x09U, /**< The middle byte of the gain calibration. */
  ADS1256_FSC2          = 0x0AU, /**< The most significant byte of the gain calibration. */
  ADS1256_NREGS         = 0x0BU /**<@internal The total number of registers. */
} ADS1256_Register_t;   

/**
* @def IS_ADS1256_REGISTER(REG)
* @brief Checks that the specified value is a valid register of the ADS1256.
*/
#define IS_ADS1256_REGISTER(REG) (((REG) == ADS1256_STATUS) || \
  ((REG) == ADS1256_MUX) || \
  ((REG) == ADS1256_ADCON) || \
  ((REG) == ADS1256_DRATE) || \
  ((REG) == ADS1256_IO)|| \
  ((REG) == ADS1256_OFC0) || \
  ((REG) == ADS1256_OFC1) || \
  ((REG) == ADS1256_OFC2) || \
  ((REG) == ADS1256_FSC0)|| \
  ((REG) == ADS1256_FSC1) || \
  ((REG) == ADS1256_FSC2)) /* Intentionally omitting the count */

/*--------------------------------------------------------------------------------------------------------*/
/* COMMANDS */
/*--------------------------------------------------------------------------------------------------------*/
/**
* @brief ADS1256 command enumeration.
* Defines the commands which the ADS1256 ADC understands. 
*/
typedef enum {
  ADS1256_WAKEUP    = 0x00, /**< Wakes up the ADS1256 from sync/standby and performs a settled conversion. */
  ADS1256_RDATA     = 0x01, /**< Reads the data stored from the last conversion cycle. */
  ADS1256_RDATAC    = 0x03, /**< Reads the data stored from the last conversion and puts the ADS1256 in continuous conversion mode. */
  ADS1256_SDATAC    = 0x0F, /**< Takes the ADS1256 out of continuous conversion mode. */
  ADS1256_RREG      = 0x10, /**< Reads 1 or more registers from the ADS1256. */
  ADS1256_WREG      = 0x50, /**< Writes 1 or more registers of the ADS1256. */
  ADS1256_SELFCAL   = 0xF0, /**< Performs a self offset and gain calibration. */
  ADS1256_SELFOCAL  = 0xF1, /**< Performs a self offset calibration. */
  ADS1256_SELFGCAL  = 0xF2, /**< Performs a self gain calibration. */
  ADS1256_SYSOCAL   = 0xF3, /**< Performs a system offset calibration. */
  ADS1256_SYSGCAL   = 0xF4, /**< Performs a system gain calibration. */
  ADS1256_SYNC      = 0xFC, /**< Syncs the ADS1256 and halts all operations, waiting for a WAKEUP command. */
  ADS1256_STANDBY   = 0xFD, /**< Puts the ADS1256 in standby mode, halting all operations. */
  ADS1256_RESET     = 0xFE /**< Performs a reset cycle on the ADS1256, restoring registers to their default values. */
  /* ADS1256_WAKEUP = 0xFF; alternative */
} ADS1256_Command_t;

/**
* @def IS_ADS1256_COMMAN(CMD)
* @brief Checks that the specified value is a valid command of the ADS1256.
*/
#define IS_ADS1256_COMMAND(CMD) (((CMD) == ADS1256_WAKEUP) || \
  ((REG) == ADS1256_RDATA) || \
  ((REG) == ADS1256_RDATAC) || \
  ((REG) == ADS1256_SDATAC) || \
  ((REG) == ADS1256_RREG)|| \
  ((REG) == ADS1256_WREG) || \
  ((REG) == ADS1256_SELFCAL) || \
  ((REG) == ADS1256_SELFOCAL) || \
  ((REG) == ADS1256_SELFGCAL)|| \
  ((REG) == ADS1256_SYSOCAL) || \
  ((REG) == ADS1256_SYSGCAL) || \
  ((REG) == ADS1256_SYNC)|| \
  ((REG) == ADS1256_STANDBY) || \
  ((REG) == ADS1256_RESET))
  
 /**
 * @def IS_ADS1256_REGISTER_COMMAND(CMD)
 * @brief Checks that the specified value is a special register command of the ADS1256.
 */
#define IS_ADS1256_REGISTER_COMMAND(CMD) (((CMD) == ADS1256_RREG)|| \
  ((CMD) == ADS1256_WREG))

/*--------------------------------------------------------------------------------------------------------*/
/* INITIALIZATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
* @brief Initialize the ADS1256 ADC Driver.
*/
void ADS1256_Init(void);

/**
 * @brief Initialize the state pins of the ADC Driver.
 */
void ADS1256_StatePins_Init(void);
void ADS1256_EXTI_Disable(void);
void ADS1256_EXTI_Enable(void);
void InitializeShortDelayTimer(void);
void InitializeChannelSwitchTimer(void);
void ShortDelayUS(uint32_t Delay);
/*--------------------------------------------------------------------------------------------------------*/
/* STRING METHODS */
/*--------------------------------------------------------------------------------------------------------*/
/**
* @brief Print all the local registers.
*/
void ADS1256_PrintRegs(void);

/**
 * @brief Print all the local registers to a string.
 */
void ADS1256_RegistersToString(void);

/**
 * @brief Return the human readable string representation of the provided data rate.
 */
const char* ADS1256_StringFromSPS(ADS1256_SPS_t sps);

/**
 * @brief Return the human readable string representation of the provided gain setting.
 */
const char* ADS1256_StringFromPGA(ADS1256_PGA_t pga);

/**
 * @brief Return the human readable string representation of the provided buffer setting.
 */
const char* ADS1256_StringFromBuffer(ADS1256_BUFFER_t buffer);

/**
 * @brief Convert a human readable string into the relevant ADS1256_BUFFER_t value.
 */
ADS1256_BUFFER_t ADS1256_StringToBuffer(char* str);

/**
 * @brief Convert a human readable string into the relevant ADS1256_SPS_t value.
 */
ADS1256_SPS_t ADS1256_StringToDataRate(char* str);

/**
 * @brief Convert a human readable string into the relevant ADS1256_PGA_t value.
 */
ADS1256_PGA_t ADS1256_StringToPGA(char* str);

/*--------------------------------------------------------------------------------------------------------*/
/* RESET METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Completely reset the ADS1256 ADC.
 */
void ADS1256_Full_Reset(void);

/**
* @brief Reset the ADS1256 ADC via SPI command.
*/
void ADS1256_Reset_By_Command(void);

/**
* @brief Reset the ADS1256 ADC via clock command.
*/
void ADS1256_Reset_By_Clock(void);

/**
 * @brief Reset the ADS1256 ADC via its RESET pin.
 */
void ADS1256_Reset_By_Pin(void);

/**
* @brief Reset the ADS1256 ADC SPI port.
*/
void ADS1256_Reset_SPI(void);

/**
* @brief Performs a complete reset of the ADS1256 and restores its state.
*/
void ADS1256_ResetAndReprogram(void);

/*--------------------------------------------------------------------------------------------------------*/
/* AQUISITION METHODS */
/*--------------------------------------------------------------------------------------------------------*/
/**
 * @brief Retrieve the latest measurement code from the ADC.
 */
int32_t ADS1256_GetMeasurement(void);

/**
 * @brief Initiate a measurement.
 */
void ADS1256_RequestMeasurement(void);

/**
 * @brief Start reading the 3-byte data.
 */
void ADS1256_ReadData(uint8_t* data);

/**
 * @brief Wait in a loop until data is ready.
 */
void ADS1256_WaitUntilDataReady(bool useCommand);

/**
 * @brief Checks to see if valid data is ready.
 */
bool ADS1256_IsDataReady(bool useCommand);

/**
 * @brief Activate the SYNC feature of the ADC.
 */
void ADS1256_Sync(bool useCommand);

/**
 * @brief Wakeup the ADC from SYNC/PWDN.
 */
void ADS1256_Wakeup(void);

/**
 * @brief Put the ADC into STANDBY mode.
 */
void ADS1256_Standby(void);

/*--------------------------------------------------------------------------------------------------------*/
/* CALIBRATION METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Performs a complete self calibration of the ADC.
 */
void ADS1256_CalibrateSelf(void);

/**
 * @brief Performs a self gain calibration of the ADC.
 */
void ADS1256_CalibrateSelf_Gain(void);

/**
 * @brief Performs a self offset calibration of the ADC.
 */
void ADS1256_CalibrateSelf_Offset(void);

/**
 * @brief Performs a system gain calibration.
 */
void ADS1256_CalibrateSystem_Gain(void);

/**
 * @brief Performs a system offset calibration.
 */
void ADS1256_CalibrateSystem_Offset(void);

/*--------------------------------------------------------------------------------------------------------*/
/* UTILITY METHODS */
/*--------------------------------------------------------------------------------------------------------*/
/**
 * @brief Converts the raw output of the ADC into a signed integer type.
 */
int32_t ADS1256_ConvertRawValue(uint32_t value);

/**
 * @brief Retrieves the total self calibration time.
 */
float ADS1256_GetSelfCalTime(void);

/**
 * @brief Retrieves the self offset calibration time.
 */
float ADS1256_GetOffsetCalTime(void);

/**
 * @brief Retrieves the self gain calibration time.
 */
float ADS1256_GetSelfGainCalTime(void);

/**
 * @brief Retrieves the total system calibration time.
 */
float ADS1256_GetSystemGainCalTime(void);

/**
 * @brief Retrieves the settling time of the ADC.
 */
float ADS1256_GetSettlingTime(void);

/**
 * @brief Selects remote register auto-fetch behavior.
 */
void ADS1256_AlwayFetch(bool always);

/**
 * @brief Send the SPI Clock pin low
 */
void ADS1256_SCLK_LOW(void);

/**
 * @brief Send the SPI Clock pin high
 */
void ADS1256_SCLK_HIGH(void);

/*--------------------------------------------------------------------------------------------------------*/
/* STATUS REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Retrieves the factory programed ID of the ADC.
 */
uint8_t ADS1256_GetFactoryProgrammedID(void);

/**
 * @brief Retrieves the current data output order.
 */
ADS1256_ORDER_t ADS1256_GetDataOutputBitOrder(void);

/**
 * @brief Retrieves the current auto calibration setting.
 */
ADS1256_ACAL_t ADS1256_GetAutoCalSetting(void);

/**
 * @brief Retrieves the current input buffer setting.
 */
ADS1256_BUFFER_t ADS1256_GetInputBufferSetting(void);

/**
 * @brief Sets the data output order.
 */
void ADS1256_SetDataOutputBitOrder(ADS1256_ORDER_t order);

/**
 * @brief Sets the auto calibration setting.
 */
void ADS1256_SetAutoCalSetting(ADS1256_ACAL_t acal);

/**
 * @brief Sets the input buffer setting.
 */
void ADS1256_SetInputBufferSetting(ADS1256_BUFFER_t buffer);

/*--------------------------------------------------------------------------------------------------------*/
/* MUX REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Retrieve the currently selected input channels.
 */
void ADS1256_GetInputChannels(void);

/**
 * @brief Set the currently selected input channels.
 */
void ADS1256_SetInputChannels(ADS1256_AIN_t pos, ADS1256_AIN_t neg);

/*--------------------------------------------------------------------------------------------------------*/
/* ADCON REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Retrieve the ADC clock out rate.
 */
ADS1256_CLOCK_OUT_t ADS1256_GetClockOutRate(void);

/*
 * @brief Retrieve the ADC sensor detect current setting.
 */
ADS1256_SENSOR_DETECT_t ADS1256_GetSensorDetectCurrent(void);

/**
 * @brief Retrieve the ADC PGA gain setting.
 */
ADS1256_PGA_t ADS1256_GetPGASetting(void);

/**
 * @brief Set the ADC clock out rate.
 */
void ADS1256_SetClockOutRate(ADS1256_CLOCK_OUT_t clock);

/**
 * @brief Set the ADC sensor detect current output.
 */
void ADS1256_SetSensorDetectCurrent(ADS1256_SENSOR_DETECT_t current);

/**
 * @brief Set the ADC PGA gain setting.
 */
void ADS1256_SetPGASetting(ADS1256_PGA_t gain);

/**
 * @brief Retrieve the multiplier for a specific PGA setting.
 */
int32_t ADS1256_GetGainMultiplier(ADS1256_PGA_t gain);

/*--------------------------------------------------------------------------------------------------------*/
/* DRATE REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Retrieve the ADC data rate (sample rate) setting.
 */
ADS1256_SPS_t ADS1256_GetDataRate(void);

/**
 * @brief Set the ADC data rate (sample rate) setting.
 */
void ADS1256_SetDataRate(ADS1256_SPS_t sps);

/*--------------------------------------------------------------------------------------------------------*/
/* GPIO REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Retrieve the GPIO direction setting of a pin.
 */
ADS1256_GPIO_DIRECTION_t ADS1256_GetGPIODirection(ADS1256_GPIO_t pin);

/**
 * @brief Retrieve the current state of a GPIO pin.
 */
ADS1256_GPIO_STATUS_t ADS1256_GetGPIOStatus(ADS1256_GPIO_t pin);

/**
 * @brief Set the GPIO direction setting of a pin.
 */
void ADS1256_SetGPIODirection(ADS1256_GPIO_t pin, ADS1256_GPIO_DIRECTION_t direction);

/**
 * @brief Set the state of a GPIO pin.
 */
void ADS1256_SetGPIOStatus(ADS1256_GPIO_t pin, ADS1256_GPIO_STATUS_t status);

/*--------------------------------------------------------------------------------------------------------*/
/* OFFSET CALIBRATION REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Retrieve the current offset calibration value.
 */
uint32_t ADS1256_GetOffsetCalSetting(void);

/**
 * @brief Set the offset calibration value.
 */
void ADS1256_SetOffsetCalSetting(uint8_t* value);

/*--------------------------------------------------------------------------------------------------------*/
/* GAIN CALIBRATION REGISTER METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Retrieve the current gain calibration value.
 */
uint32_t ADS1256_GetGainCalSetting(void);

/**
 * @brief Set the gain calibration value.
 */
void ADS1256_SetGainCalSetting(uint8_t* value);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ADS1256_DRIVER_H_ */
