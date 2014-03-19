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
 * @file boolean.h
 * @brief Header file for the custom boolean data type.
 *
 * This class exists to satisfy MISRA-C coding standards.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BOOLEAN_H_
#define BOOLEAN_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "stm32f4xx.h"

/** @addtogroup tekdaqc_firmware_libraries Tekdaqc Firmware Libraries
 * @{
 */

/** @addtogroup data_types Data Types
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

#ifndef BOOL
#ifndef FALSE

/**
* @def FALSE
* @brief Logical False
*/
#define FALSE 0U

/**
* @def false
* @brief Logical False
*/
#define false 0U

/**
* @def TRUE
* @brief Logical True
*/
#define TRUE  1U

/**
* @def true
* @brief Logical True
*/
#define true  1U
#endif

/**
* @brief Boolean data type.
* Defines the two possible logic states.
*/
typedef uint8_t BOOL;

/**
* @brief Boolean data type.
* Defines the two possible logic states.
*/
typedef uint8_t bool;
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

#endif /* BOOLEAN_H_ */
