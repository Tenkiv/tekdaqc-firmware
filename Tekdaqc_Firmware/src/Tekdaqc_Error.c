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
 * @file Tekdaqc_Error.c
 * @brief Source file for the Tekdaqc error handling methods.
 *
 * Contains methods for determining the error status of a Tekdaqc and converting them to a human readable string.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Error.h"
#include "Digital_Output.h"
#include "TelnetServer.h"
#include <stdlib.h>



/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC METHODS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * Checks the overall error status of the Tekdaqc. Currently this consists only of checking the digital output
 * error conditions. Future firmware can expand upon this.
 *
 * @param none
 * @retval none
 */
void Tekdaqc_CheckStatus(void) {
	/* Check on the digital outputs */
	if (CheckDigitalOutputStatus() == true) {
		if (TelnetIsConnected() == true) {
			/* A fault was detected */
			uint8_t n = snprintf(TOSTRING_BUFFER, SIZE_TOSTRING_BUFFER, "\n\r*DIGITAL OUTPUT FAULT DETECTED!*\n\r");
			if (n > 0) {
				TelnetWriteString(TOSTRING_BUFFER);
			} else {
#ifdef DO_STATE_MACHINE_DEBUG
				printf("[DO STATE MACHINE] Failed to write fault warning for digital output.\n\r");
#endif
			}
		}
	}
}

/**
 * Converts the specified Tekdaqc_Command_Error_t into a human readable string.
 *
 * @param error Tekdaqc_Command_Error_t The command error status to convert.
 * @retval const char* Pointer to C-String containing the human readable string.
 */
const char* Tekdaqc_CommandError_ToString(Tekdaqc_Command_Error_t error) {
	static const char* strings[] = {"COMMAND: OK", "COMMAND: BAD PARAMETER", "COMMAND: BAD COMMAND", "COMMAND: PARSE ERROR",
			"COMMAND: FUNCTION ERROR", "COMMAND: INVALID ADC OPERATION", "COMMAND: INVALID DIGITAL INPUT OPERATION",
			"COMMAND: INVALID DIGITAL OUTPUT OPERATION", "COMMAND: UNKNOWN ERROR"};
	return strings[error];
}

/**
 * Converts the specified Tekdaqc_Function_Error_t into a human readable string.
 *
 * @param error Tekdaqc_Function_Error_t The function error status to convert.
 * @retval const char* Pointer to C-String containing the human readable string.
 */
const char* Tekdaqc_FunctionError_ToString(Tekdaqc_Function_Error_t error) {
	static const char* strings[] = {"FUNCTION: OK", "AIN: INPUT OUT OF RANGE", "AIN: PARSE MISSING KEY",
			"AIN: INPUT NOT FOUND", "AIN: PARSE ERROR", "AIN: INPUT EXISTS", "AIN: INPUT UNSPECIFIED",
			"AIN: FAILED WRITE", "DIN: INPUT OUT OF RANGE", "DIN: PARSE MISSING KEY", "DIN: INPUT NOT FOUND",
			"DIN: PARSE ERROR", "DIN: INPUT UNSPECIFIED", "DIN: INPUT EXISTS", "DIN: FAILED WRITE",
			"DOUT: OUTPUT OUT OF RANGE", "DOUT: PARSE MISSING KEY", "OUT: OUTPUT NOT FOUND", "DOUT: PARSE ERROR",
			"DOUT: OUTPUT EXISTS", "DOUT: OUTPUT UNSPECIFIED", "DOUT: DOES NOT EXIST", "DOUT: FAILED WRITE",
			"CALIBRATION: MODE ENTRY FAILED", "CALIBRATION: WRITE FAILED", "CALIBRATION: PARSE ERROR",
			"CALIBRATION: PARSE MISSING KEY"};
	return strings[error];
}
