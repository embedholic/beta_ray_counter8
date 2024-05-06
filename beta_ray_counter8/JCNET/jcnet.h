/*
 * jcnet.h
 *
 *  Created on: May 3, 2024
 *      Author: isjeon
 */

#ifndef JCNET_H_
#define JCNET_H_

typedef struct _counters_tag_ {
	uint32_t rd_tick;
	uint32_t CNT[8];
	uint32_t acc_cntrs[8]; // 0-3 : master, 4-7 : slave
	uint32_t pre_cntrs[8];
} counter_type;
typedef struct _uart_tag_ {
        uint8_t *data; // [UART_Q_SZ];
        uint32_t size;
        uint32_t wr, rd;
} uart_rx_queue_t;

#define NUM_SLAVE_COUNTERS 4
#define COUNTER_DIGIT_NUM 4 // "0000"~"FFFF" in hex(16bits)
#define UART1_DMA_BUF_SZ (NUM_SLAVE_COUNTERS * COUNTER_DIGIT_NUM + 2) // $aaaabbbbccccdddd0a
#define COUNTER_GATHER_PERIOD 10 // 10 mili gathering

#define STX 02
extern char uart1_rx_buf[UART1_DMA_BUF_SZ];
extern char uart2_rx_buf[1];
#endif /* JCNET_H_ */
