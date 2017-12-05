/*
 ******************************************************************************
 * @file    netconf.c
 * @author  MCD Application Team
 * @version V1.1.0
 * @date    31-July-2013
 * @brief   Network connection configuration
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/**
 * This file modified by Tenkiv, Inc.
 * @since v1.0.0.0
 *
 * @modified by Pachia Cha (pcha@tenkiv.com)
 * @since v1.2.0.0
 */

/* Includes ------------------------------------------------------------------*/
#include "Tekdaqc_Debug.h"
#include "Tekdaqc_BSP.h"
#include "TelnetServer.h"
#include "stm32f4x7_eth.h"
#include "stm32f4x7_eth_bsp.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/tcp.h"
#include "lwip/tcp_impl.h"
#include "lwip/udp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "ethernetif.h"
#include "netconf.h"
#include "Tekdaqc_Locator.h"
#include <inttypes.h>

#ifdef PRINTF_OUTPUT
#include <stdio.h>
#endif

/* Private typedef -----------------------------------------------------------*/
#define MAX_DHCP_TRIES        4

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
struct netif gnetif;
uint64_t TCPTimer = 0U;
uint64_t ARPTimer = 0U;
uint64_t IPaddress = 0U;

#ifdef USE_DHCP
uint32_t DHCPfineTimer = 0U;
uint32_t DHCPcoarseTimer = 0U;
__IO uint8_t DHCP_state;
uint8_t statusLinkOff = 0;
#endif
extern __IO uint32_t EthStatus;

/* Private functions ---------------------------------------------------------*/
void LwIP_DHCP_Process_Handle(void);
/**
 * @brief  Initializes the lwIP stack
 * @param  None
 * @retval None
 */
void LwIP_Init(void) {
	struct ip_addr ipaddr;
	struct ip_addr netmask;
	struct ip_addr gw;
#ifndef USE_DHCP
	uint8_t iptab[4] = {0};
	uint8_t iptxt[20];
#endif

	/* Initializes the dynamic memory heap defined by MEM_SIZE.*/
	mem_init();

	/* Initializes the memory pools defined by MEMP_NUM_x.*/
	memp_init();

#ifdef USE_DHCP
	ipaddr.addr = 0U;
	netmask.addr = 0U;
	gw.addr = 0U;
#else
	IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
	IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
	IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
#endif

	/* - netif_add(struct netif *netif, struct ip_addr *ipaddr,
	 struct ip_addr *netmask, struct ip_addr *gw,
	 void *state, err_t (* init)(struct netif *netif),
	 err_t (* input)(struct pbuf *p, struct netif *netif))

	 Adds your network interface to the netif_list. Allocate a struct
	 netif and pass a pointer to this structure as the first argument.
	 Give pointers to cleared ip_addr structures when using DHCP,
	 or fill them with sane numbers otherwise. The state pointer may be NULL.

	 The init function pointer must point to a initialization function for
	 your ethernet netif interface. The following code illustrates it's use.*/
	netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &ethernet_input);

	/*  Registers the default network interface.*/
	netif_set_default(&gnetif);

	/* Set Ethernet link flag */
	gnetif.flags |= NETIF_FLAG_LINK_UP;

	/* When the netif is fully configured this function must be called.*/
	netif_set_up(&gnetif);
#ifdef USE_DHCP
	DHCP_state = DHCP_START;
#else
#ifdef DEBUG
	iptab[0] = IP_ADDR3;
	iptab[1] = IP_ADDR2;
	iptab[2] = IP_ADDR1;
	iptab[3] = IP_ADDR0;

	printf("Static IP Address: %d.%d.%d.%d\n\r", iptab[3], iptab[2], iptab[1], iptab[0]);
#endif /* DEBUG */
#endif  /* USE_DHCP */


	/* Set the link callback function, this function is called on change of link status*/
	netif_set_link_callback(&gnetif, ETH_link_callback);
}

/**
 * @brief  Called when a frame is received
 * @param  None
 * @retval None
 */
void LwIP_Pkt_Handle(void) {
	/* Read a received packet from the Ethernet buffers and send it to the lwIP for handling */
	ethernetif_input(&gnetif);
}

/**
 * @brief  LwIP periodic tasks
 * @param  localtime the current LocalTime value
 * @retval None
 */
void LwIP_Periodic_Handle(__IO uint64_t localtime) {
	uint64_t time = (localtime / 1000U);
	//printf("[NETCONF] Servicing LWIP with time %" PRIu64 " ms.\n\r", time);
#if LWIP_TCP
	/* TCP periodic process every 5 ms */
	if (time - TCPTimer >= TCP_TMR_INTERVAL) {
		TCPTimer = time;
		tcp_tmr();
	}
#endif

	/* ARP periodic process every 5s */
	if ((time - ARPTimer) >= ARP_TMR_INTERVAL) {
		ARPTimer = time;
		etharp_tmr();
	}

#ifdef USE_DHCP
	/* Fine DHCP periodic process every 500ms */
	if (time - DHCPfineTimer >= DHCP_FINE_TIMER_MSECS) {
		DHCPfineTimer = time;
		dhcp_fine_tmr();

		if (!TelnetIsConnected()) {
			if ((ETH_ReadPHYRegister(DP83848_PHY_ADDRESS, PHY_SR) & 1) == 0) { //detect status link off
				statusLinkOff++; //status link went off
			}
			else if ((statusLinkOff>=11) && (ETH_ReadPHYRegister(DP83848_PHY_ADDRESS, PHY_SR) & 1)) { //detect status link on after it was off first
				//off for 5 seconds
				//=> reassign tekdaqc a new ip because connection changed, else keep ip
				DHCP_state = DHCP_START; //reset state to detect router or direct device connection
				gnetif.ip_addr.addr = 0U; //reset tekdaqc ip_address
				//^will allow switching connections while still keeping past commands on the tekdaqc
				statusLinkOff = 0; //status link went on
			}
			else {
				statusLinkOff = 0;
			}
		}

		if ((DHCP_state != DHCP_ADDRESS_ASSIGNED) && (DHCP_state != DHCP_TIMEOUT) && (DHCP_state != DHCP_LINK_DOWN)) {
			/* process DHCP state machine */
			LwIP_DHCP_Process_Handle();
#ifdef DEBUG
			/*Output a dot to indicate ongoing progress...*/
			printf(".");
#endif
		}
	}

	/* DHCP Coarse periodic process every 60s */
	if (time - DHCPcoarseTimer >= DHCP_COARSE_TIMER_MSECS) {
		DHCPcoarseTimer = time;
		dhcp_coarse_tmr();
	}

#endif
}

#ifdef USE_DHCP
/**
 * @brief  LwIP_DHCP_Process_Handle
 * @param  None
 * @retval None
 */
void LwIP_DHCP_Process_Handle() {
	struct ip_addr ipaddr;
	struct ip_addr netmask;
	struct ip_addr gw;
	uint8_t iptab[4] = { 0 };

	switch (DHCP_state) {
	case DHCP_START: {
		DHCP_state = DHCP_WAIT_ADDRESS;
		dhcp_start(&gnetif);
		/* IP address should be set to 0
		 every time we want to assign a new DHCP address */
		IPaddress = 0;
#ifdef DEBUG
		printf("Looking for DHCP server, please wait...");
#endif
	}
		break;

	case DHCP_WAIT_ADDRESS: {
		/* Read the new IP address */
		IPaddress = gnetif.ip_addr.addr;

		if (IPaddress != 0) {
			DHCP_state = DHCP_ADDRESS_ASSIGNED;

			/* Stop DHCP */
			dhcp_stop(&gnetif);

			iptab[0] = (uint8_t) (IPaddress >> 24);
			iptab[1] = (uint8_t) (IPaddress >> 16);
			iptab[2] = (uint8_t) (IPaddress >> 8);
			iptab[3] = (uint8_t) (IPaddress);

			Tekdaqc_LocatorClientIPSet((uint32_t) IPaddress);

#ifdef DEBUG
			/* Display the IP address */
			printf("\n\rIP address assigned by a DHCP server: %d.%d.%d.%d\n\r", iptab[3], iptab[2], iptab[1], iptab[0]);
#endif /* DEBUG */
		} else {
			/* DHCP timeout */
			if (gnetif.dhcp->tries > MAX_DHCP_TRIES) {
				DHCP_state = DHCP_TIMEOUT;

				/* Stop DHCP */
				dhcp_stop(&gnetif);

				/* Static address used */
				IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
				IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
				IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
				netif_set_addr(&gnetif, &ipaddr, &netmask, &gw);

				IPaddress = gnetif.ip_addr.addr;
				Tekdaqc_LocatorClientIPSet((uint32_t) IPaddress);

#ifdef DEBUG
				printf("DHCP Timeout\n\r");

				iptab[0] = IP_ADDR3;
				iptab[1] = IP_ADDR2;
				iptab[2] = IP_ADDR1;
				iptab[3] = IP_ADDR0;

				printf("Static IP address:  %d.%d.%d.%d\n\r", iptab[3], iptab[2], iptab[1], iptab[0]);
#endif /* DEBUG */
			}
		}
	}
		break;
	default:
		break;
	}
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
