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
 * @file Tekdaqc_Locator.c
 * @brief Allows for discovery of a Tekdaqc on an Ethernet network.
 *
 * The locator listens for specially formatted UDP packets
 *
 * @author Jared Woolston (jwoolston@tenkiv.com)
 * @since v1.0.0.0
 */

/*--------------------------------------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------------------------------------*/

#include "Tekdaqc_Debug.h"
#include "Tekdaqc_Locator.h"
#include "TelnetServer.h"
#include "Tekdaqc_BSP.h"
#include "boolean.h"
#include "lwip/udp.h"

/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE DEFINES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @internal
 * @def CMD_DISCOVER_TARGET
 * @brief Encodes a response as being a locator status response.
 */
#define TAG_STATUS              ((uint8_t) 0xFE)

/**
 * @internal
 * @def CMD_DISCOVER_TARGET
 * @brief Encodes a response as being a discovery target.
 */
#define CMD_DISCOVER_TARGET     ((uint8_t) 0x02)

/**
 * @internal
 * @def LOCATOR_DATA_LENGTH
 * @brief The length of the data packet returned.
 */
#define LOCATOR_DATA_LENGTH     115U



/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE VARIABLES */
/*--------------------------------------------------------------------------------------------------------*/

/*
 * An array that contains the device locator response data.  The format of the
 * data is as follows:
 *
 * Byte        Description
 * --------    ------------------------
 * 0           TAG_STATUS
 * 1           Packet Length
 * 2           CMD_DISCOVER_TARGET
 * 3           Board Type
 * 4..35       Board ID (32 character serial number)
 * 36..39      Client IP Address
 * 40..45      MAC Address
 * 46..49      Firmware Version
 * 50..113     Application Title
 * 114         Checksum
 */
static unsigned char LocatorData[LOCATOR_DATA_LENGTH];

/* The contents of the received UDP packet that will cause the Tekdaqc to return a response */
static unsigned char LocatorMessage[] = "TEKDAQC CONNECT";

/* The length of the expected UDP packet message */
static uint8_t LengthLocatorMessage = 14U;



/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTION PROTOTYPES */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief Callback function to process received UDP packets on the locator port.
 */
static void Tekdaqc_LocatorReceive(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, uint16_t port);



/*--------------------------------------------------------------------------------------------------------*/
/* PRIVATE FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * This function is called by the lwIP TCP/IP stack when it receives a UDP
 * packet from the discovery port.  It produces the response packet, which is
 * sent back to the querying client.
 *
 * @param arg void* Argument pointer passed to the handler by the lwIP stack.
 * @param pcb udp_pcb* struct The PCB structure this callback is for.
 * @param p pbuf* struct The data buffer from the lwIP stack.
 * @param addr ip_addr* struct The IP address of the source of the UDP packet.
 * @param port uint16_t The port number the packet was sent to.
 * @retval none
 */
static void Tekdaqc_LocatorReceive(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, uint16_t port) {
#ifdef LOCATOR_DEBUG
	printf("[Locator] Packet received.\n\r");
#endif

	if (TelnetIsConnected() == TRUE) {
#ifdef LOCATOR_DEBUG
		printf("[Locator] Packet received while Telnet connection is active. Ignoring.\n\r");
#endif
		return;
	}

	unsigned char *data;
	int i = 0;

	/* Validate the contents of the datagram. This expects to see a message that matches the variable LocatorMessage */
	data = p->payload;
	if ((p->len != LengthLocatorMessage) || (p->len < LengthLocatorMessage)) {
		bool VALID = true;
		for (i = 0; i < p->len; ++i) {
			if (data[i] != LocatorMessage[i]) {
				VALID = false;
				break;
			}
		}
		if (!VALID) {
			pbuf_free(p);
			return;
		}
	}

	/* The incoming pbuf is no longer needed, so free it. */
	pbuf_free(p);

	/* Allocate a new pbuf for sending the response. */
	p = pbuf_alloc(PBUF_TRANSPORT, sizeof(LocatorData), PBUF_RAM);
	if (p == NULL ) {
		return;
	}

	/* Calculate and fill in the checksum on the response packet. */
	LocatorData[sizeof(LocatorData) - 1U] = 0U;
	for (i = 0U; i < (sizeof(LocatorData) - 1U); ++i) {
		LocatorData[sizeof(LocatorData) - 1U] -= LocatorData[i];
	}

	/* Copy the response packet data into the pbuf. */
	data = p->payload;
	for (i = 0; i < sizeof(LocatorData); ++i) {
		data[i] = LocatorData[i];
	}

	/* Send the response. */
	udp_sendto(pcb, p, addr, port);
	/* Free the pbuf. */
	pbuf_free(p);
}



/*--------------------------------------------------------------------------------------------------------*/
/* PUBLIC FUNCTIONS */
/*--------------------------------------------------------------------------------------------------------*/

/**
 * This function prepares the locator service to handle device discovery
 * requests.  A UDP server is created and the locator response data is
 * initialized to all empty.
 *
 * @param none
 * @retval none
 */
void Tekdaqc_LocatorInit(void) {
	void *pcb;

	/* Clear out the response data. */
	for (uint8_t i = 0U; i < LOCATOR_DATA_LENGTH; ++i) {
		LocatorData[i] = 0U;
	}

	/* Fill in the header for the response data. */
	LocatorData[0] = TAG_STATUS;
	LocatorData[1] = sizeof(LocatorData);
	LocatorData[2] = CMD_DISCOVER_TARGET;

	/* Fill in the MAC address for the response data. */
	LocatorData[40] = MAC_ADDR0;
	LocatorData[41] = MAC_ADDR1;
	LocatorData[42] = MAC_ADDR2;
	LocatorData[43] = MAC_ADDR3;
	LocatorData[44] = MAC_ADDR4;
	LocatorData[45] = MAC_ADDR5;

	/* Fill in the Board Type */
	Tekdaqc_LocatorBoardTypeSet(TEKDAQC_BOARD_TYPE );

	/* Create a new UDP port for listening to device locator requests. */
	pcb = udp_new();
	udp_recv(pcb, Tekdaqc_LocatorReceive, NULL);
	udp_bind(pcb, IP_ADDR_ANY, LOCATOR_PORT);
}

/**
 * This function sets the board type field in the locator response packet.
 *
 * @param type uint8_t The ASCII character type of the board.
 * @retval none
 */
void Tekdaqc_LocatorBoardTypeSet(uint8_t type) {
	/* Save the board type in the response data. */
	LocatorData[3] = type;
}

/*
 * This function sets the board ID field in the locator response packet.
 *
 * @param id char* Pointer to a C-String containing the serial number ID of the board.
 * @retval none
 */
void Tekdaqc_LocatorBoardIDSet(const unsigned char* id) {
	/* Save the board ID in the response data, without the Null Terminator. */
	for (int i = 0; i < BOARD_SERIAL_NUM_LENGTH; ++i) {
		LocatorData[i + 4U] = id[i] & 0xffU;
	}
}

/**
 * This function sets the IP address of the board in the locator response packet.
 *
 * @param ip uint32_t The IP address of the currently connected client.
 * @retval none
 */
void Tekdaqc_LocatorClientIPSet(uint32_t ip) {
	/* Save the client IP address in the response data. */
	LocatorData[36] = (unsigned char) (ip & 0xffU);
	LocatorData[37] = (unsigned char) ((ip >> 8U) & 0xffU);
	LocatorData[38] = (unsigned char) ((ip >> 16U) & 0xffU);
	LocatorData[39] = (unsigned char) ((ip >> 24U) & 0xffU);
}

/**
 * This function sets the MAC address of the network interface in the locator
 * response packet.
 *
 * @param MAC unsigned char* Pointer to a C-String containing the MAC address of the network interface.
 * @retval none
 */
void Tekdaqc_LocatorMACAddrSet(const unsigned char MAC[]) {
	/* Save the MAC address. */
	LocatorData[40] = (uint8_t) MAC[0U];
	LocatorData[41] = (uint8_t) MAC[1U];
	LocatorData[42] = (uint8_t) MAC[2U];
	LocatorData[43] = (uint8_t) MAC[3U];
	LocatorData[44] = (uint8_t) MAC[4U];
	LocatorData[45] = (uint8_t) MAC[5U];
}

/**
 * This function sets the version number of the device firmware in the locator
 * response packet.
 *
 * @param version uint32_t The version number of the device firmware.
 * @retval none
 */
void Tekdaqc_LocatorVersionSet(uint32_t version) {
	/* Save the firmware version number in the response data. */
	LocatorData[46] = (uint8_t) ((version >> 24U) & 0xffU);
	LocatorData[47] = (uint8_t) ((version >> 16U) & 0xffU);
	LocatorData[48] = (uint8_t) ((version >> 8U) & 0xffU);
	LocatorData[49] = (uint8_t) (version & 0xffU);
}

/**
 * This function sets the application title in the locator response packet.
 * The string is truncated at 64 characters if it is longer (without a
 * terminating 0), and is zero-filled to 64 characters if it is shorter.
 *
 * @param title const char* Pointer to a C-String containing the application title string.
 * @retval none
 */
void Tekdaqc_LocatorAppTitleSet(const unsigned char *title) {
	uint32_t count;

	/* Copy the application title string into the response data. */
	for (count = 0U; (count < 64U) && *title; ++count) {
		LocatorData[count + 19U] = *title++;
	}

	/* Zero-fill the remainder of the space in the response data (if any). */
	for (; count < 64U; ++count) {
		LocatorData[count + 19U] = 0U;
	}
}

/**
 * Retrieves the board type as it is stored in the locator packet data.
 *
 * @param none
 * @retval uint8_t The board type ASCII code.
 */
uint8_t Tekdaqc_GetLocatorBoardType(void) {
	return LocatorData[3];
}

/**
 * Retrieves the board serial number ID from the locator packet data. This only retrieves
 * the starting pointer of the C-String.
 *
 * @param none
 * @retval unsigned char* Pointer to the start of the C-String.
 */
unsigned char* Tekdaqc_GetLocatorBoardID(void) {
	return LocatorData + 4;
}

/**
 * Retrieves the board IP address from the locator packet data.
 *
 * @param none
 * @retval uint32_t The board's IP address.
 */
uint32_t Tekdaqc_GetLocatorIp(void) {
	uint32_t ip = 0U;
	ip |= (((uint32_t) LocatorData[39]) << 24);
	ip |= (((uint32_t) LocatorData[38]) << 16);
	ip |= (((uint32_t) LocatorData[37]) << 8);
	ip |= (LocatorData[36]);
	return ip;
}

/**
 * Retrieves the board MAC address from the locator packet data.
 *
 * @param none
 * @retval unsigned char* Pointer to the board MAC address from the locator packet data.
 */
unsigned char* Tekdaqc_GetLocatorMAC(void) {
	return LocatorData + 40U;
}

/**
 * Retrieves the board firmware version number from the locator packet data.
 *
 * @param none
 * @retval uint32_t The board firmware version number.
 */
uint32_t Tekdaqc_GetLocatorVersion(void) {
	uint32_t version = 0U;
	version |= (((uint32_t) LocatorData[49]) << 24U);
	version |= (((uint32_t) LocatorData[48]) << 16U);
	version |= (((uint32_t) LocatorData[47]) << 8U);
	version |= LocatorData[46];
	return version;
}
