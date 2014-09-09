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
 * @file Tekdaqc_Debug.c
 * @brief Source file to define methods for debugging sections of code.
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.1.0.0
 */

#include "Tekdaqc_Debug.h"
#include <stdio.h>
#include <inttypes.h>

#define LENGTH_BLOCK	16

void Debug_Hexdump(char* desc, void* addr, uint32_t length) {
	uint32_t i = 0;
	uint8_t buff[LENGTH_BLOCK + 1];
	uint8_t *pc = (uint8_t *) addr;

	/* Output description if given. */
	if (desc != NULL) printf("%s:\n\r", desc);

	/* Process every byte in the data. */
	for (; i < length; i++) {
		/* Multiple of 16 means new line (with line offset). */
		if ((i % LENGTH_BLOCK) == 0) {
			/* Just don't print ASCII for the zeroth byte. */
			if (i != 0) printf(" \t|%s|\n\r", buff);

			/* Output the offset. */
			printf("  %04" PRIX32 ": ", i);
		}

		/* Now the hex code for the specific character. */
		printf(" %02" PRIX8 "", pc[i]);

		/* And store a printable ASCII character for later. */
		buff[i % LENGTH_BLOCK] = ((pc[i] < 0x20) || (pc[i] > 0x7e)) ? '.' : pc[i];
		buff[(i % LENGTH_BLOCK) + 1] = '\0';
	}

	/* Pad out last line if not exactly 16 characters. */
	while ((i % LENGTH_BLOCK) != 0) {
		printf("   ");
		buff[i % LENGTH_BLOCK] = ' ';
		++i;
	}
	buff[LENGTH_BLOCK + 1] = '\0';

	/* And print the final ASCII bit. */
	printf("\t|%s|\n\r", buff);
}
