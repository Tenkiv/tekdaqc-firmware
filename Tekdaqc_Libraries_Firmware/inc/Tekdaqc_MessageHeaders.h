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
 * @file Tekdaqc_MessageHeaders.h
 * @brief Header file for the Tekdaqc's message headers.
 *
 * Contains public definitions for the Tekdaqc's message headers.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TEKDAQC_MESSAGEHEADERS_H_
#define TEKDAQC_MESSAGEHEADERS_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

/** @addtogroup tekdaqc_firmware Tekdaqc Firmware
 * @{
 */

/** @addtogroup tekdaqc_communication Tekdaqc Communication
  * @{
  */

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @def DEBUG_MESSAGE_HEADER
 * @brief The printf format string for sending debug messages.
 */
#define DEBUG_MESSAGE_HEADER "\n\r--------------------\n\rDebug Message\n\r\tMessage: %s\n\r--------------------\n\r\x1E"

/**
 * @def ERROR_MESSAGE_HEADER
 * @brief The printf format string for sending error messages.
 */
#define ERROR_MESSAGE_HEADER "\n\r--------------------\n\rError Message\n\r\tMessage: %s\n\r--------------------\n\r\x1E"

/**
 * @def STATUS_MESSAGE_HEADER
 * @brief The printf format string for sending status messages.
 */
#define STATUS_MESSAGE_HEADER "\n\r--------------------\n\rStatus Message\n\r\tMessage: %s\n\r--------------------\n\r\x1E"

/**
 * @def COMMAND_DATA_MESSAGE_HEADER
 * @brief The printf format string for sending status messages.
 */
#define COMMAND_DATA_MESSAGE_HEADER "\n\r--------------------\n\rCommand Data Message\n\r\tMessage: %s\n\r--------------------\n\r\x1E"

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* TEKDAQC_MESSAGEHEADERS_H_ */
