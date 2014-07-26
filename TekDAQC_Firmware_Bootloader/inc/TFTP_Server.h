/*
 * TFTP_Server.h
 *
 *  Created on: Dec 3, 2013
 *      Author: web_000
 */

#ifndef TFTP_SERVER_H_
#define TFTP_SERVER_H_

#include "lwip/mem.h"
#include "lwip/udp.h"
#include "stm32f4xx.h"
#include <stdbool.h>

#define TFTP_OPCODE_LEN         2
#define TFTP_BLKNUM_LEN         2
#define TFTP_DATA_LEN_MAX       512
#define TFTP_DATA_PKT_HDR_LEN   (TFTP_OPCODE_LEN + TFTP_BLKNUM_LEN)
#define TFTP_ERR_PKT_HDR_LEN    (TFTP_OPCODE_LEN + TFTP_ERRCODE_LEN)
#define TFTP_ACK_PKT_LEN        (TFTP_OPCODE_LEN + TFTP_BLKNUM_LEN)
#define TFTP_DATA_PKT_LEN_MAX   (TFTP_DATA_PKT_HDR_LEN + TFTP_DATA_LEN_MAX)
#define TFTP_MAX_RETRIES        3
#define TFTP_TIMEOUT_INTERVAL   5

void TFTP_Init(void);

typedef struct {
  int op;    /**< WRQ */
  char data[TFTP_DATA_PKT_LEN_MAX]; /**< Last block read */
  int  data_len;
  struct ip_addr to_ip; /**< Destination ip:port */
  int to_port;
  int block; /**< Next block number */
  int tot_bytes; /**< total number of bytes transferred */
  unsigned long long last_time; /**< Timer interrupt count when last packet was sent. This should be used to resend packets on timeout */
} tftp_connection_args;


/* TFTP opcodes as specified in RFC1350   */
typedef enum {
  TFTP_RRQ = 1,
  TFTP_WRQ = 2,
  TFTP_DATA = 3,
  TFTP_ACK = 4,
  TFTP_ERROR = 5
} tftp_opcode;


/* TFTP error codes as specified in RFC1350  */
typedef enum {
  TFTP_ERR_NOTDEFINED,
  TFTP_ERR_FILE_NOT_FOUND,
  TFTP_ERR_ACCESS_VIOLATION,
  TFTP_ERR_DISKFULL,
  TFTP_ERR_ILLEGALOP,
  TFTP_ERR_UKNOWN_TRANSFER_ID,
  TFTP_ERR_FILE_ALREADY_EXISTS,
  TFTP_ERR_NO_SUCH_USER,
} tftp_errorcode;

#endif /* TFTP_SERVER_H_ */
