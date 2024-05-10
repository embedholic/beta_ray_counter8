/*
 * jcnet.h
 *
 *  Created on: May 3, 2024
 *      Author: isjeon
 */

#ifndef JCNET_H_
#define JCNET_H_
#define MASTER_MODE
#define USART2_DMA
//#define CLCD_TIME_EVAL


typedef struct _counters_tag_ {
	uint32_t seq;
	uint32_t rd_tick;
	uint32_t update_tick;
//	uint32_t update_period_tick;
	uint32_t CNT[8];
	uint32_t acc_cntrs[8]; // 0-3 : master, 4-7 : slave
	uint32_t pre_cntrs[8];
} counter_type;
#define START_CHAR 'S'
#define END_CHAR '\x0a'

#define CNT_TYPE_W  0
#define CNT_TYPE_O  1
#define D_FMT_DEC 0
#define D_FMT_HEX 1
typedef struct _system_info_tag_ {
	uint32_t cnt_type : 2;  // 0 : Window in, 1 : Out
	uint32_t dis_format : 2; // 0 : decimal, 1 : hex
	uint32_t update_period_tick : 28;
} system_type;
#define SEQ_ADV(x) do {\
		if(sys_info.dis_format == D_FMT_DEC && x >= 99999) x = 0 ; \
		else if(x >= 0xffff) x = 0; \
		else x ++; \
	} while(0)

typedef struct _uart_tag_ {
        uint8_t *data; // [UART_Q_SZ];
        uint32_t size;
        uint32_t wr, rd;
} uart_rx_queue_t;

#define NUM_SLAVE_COUNTERS 4
#define COUNTER_DIGIT_NUM 4 // "0000"~"FFFF" in hex(16bits)
#define UART1_DMA_BUF_SZ (NUM_SLAVE_COUNTERS * COUNTER_DIGIT_NUM + 2) // $aaaabbbbccccdddd0a
#define COUNTER_GATHER_PERIOD 10 // 10 mili gathering
#define COUNTER_UPDATE_DFT_PERIOD 100 // 100 ms = 0.1 Hz
#define STX 02

extern char __IO uart1_rx_buf[UART1_DMA_BUF_SZ];
extern char __IO uart2_rx_buf[1];

extern I2C_HandleTypeDef hi2c1;

extern LPTIM_HandleTypeDef hlptim1;
extern LPTIM_HandleTypeDef hlptim2;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim15;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;


int param_get(uint32_t *v);
int param_set(uint32_t v);
int erase_pages(int page, int num);
extern system_type sys_info;

extern void info_printf(char *fmt,...);
extern void (*g_dbg_print)(char*);
extern __IO g_dma_tx_flag;
extern __IO uint32_t g_output_to_pc;
#endif /* JCNET_H_ */
