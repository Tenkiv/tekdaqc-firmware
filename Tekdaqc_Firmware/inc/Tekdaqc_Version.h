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
 * @file Tekdaqc_Version.h
 * @brief Contains constant values used for firmware versioning. These should be updated for every release.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef VERSION_H_
#define VERSION_H_

/* Define to provide proper behavior with C++ compilers ----------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------------*/
/* EXPORTED CONSTANTS */
/*--------------------------------------------------------------------------------------------------------*/

static const uint8_t MAJOR_VERSION = 1;  // Hardware changes/rearchitecture
static const uint8_t MINOR_VERSION = 1;  // New features
static const uint8_t BUILD_NUMBER  = 11;  // Bug fixes
static const uint8_t SPECIAL_BUILD = 0;  // Special unique release, otherwise 0

#ifdef __cplusplus
}
#endif

#endif /* VERSION_H_ */
