/*
 * jcnet.h
 *
 *  Created on: May 3, 2024
 *      Author: isjeon
 */

#ifndef JCNET_H_
#define JCNET_H_

#define NUM_SLAVE_CONUNTERS 4
#define COUNTER_DIGIT_NUM 4 // "0000"~"FFFF" in hex(16bits)
#define UART1_DMA_BUF_SZ (NUM_SLAVE_CONUNTERS*COUNTER_DIGIT_NUM + 2) // $aaaabbbbccccdddd0a
extern char uart1_rx_buf[UART1_DMA_BUF_SZ];
extern char uart2_rx_buf[1];
#endif /* JCNET_H_ */
