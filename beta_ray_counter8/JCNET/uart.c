/*
 * uart.c
 *
 *  Created on: 2021. 2. 21.
 *      Author: isjeon
 */



#include "main.h"
#include "jcnet.h"
#define USART1_EN
extern UART_HandleTypeDef huart2;

#define PROTOCOL_DEBUG
#define CSUM_ERROR_IGNORE

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;

__IO uint32_t reinit_flag_1,reinit_flag_2;
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{

    if(huart == &huart1)
    {

    	reinit_flag_1 = 1;
//              printf("Usart5 Uart error !!\n");
    }

    if(huart == &huart2)
    {

    	reinit_flag_2 = 1;
//              printf("Usart5 Uart error !!\n");
     }
}
extern void MX_USART2_UART_Init(void);
void reinit_uart(int ch)
{
	extern void JCNET_USART1_UART_Init(void);
	extern void JCNET_USART2_UART_Init(void);
    if(ch == 1)
    {
          HAL_UART_DeInit(&huart1);
          JCNET_USART1_UART_Init();
          HAL_UART_Receive_DMA(&huart1,uart1_rx_buf,UART1_DMA_BUF_SZ);
          return;
    }
    if(ch == 2)
    {
            HAL_UART_DeInit(&huart2);
        	JCNET_USART2_UART_Init();
    }
}

#define STX 'S'

//uint8_t uart2_rx_dma_buf[RX_DMA_BUF_SZ];
uint32_t uart2_rx_ptr;

extern int rx_cmd_idx;
extern uint8_t rx_cmd_buf[];

extern int _write(int file, char *data, int len);
//
// 'S',xxxx(4),uuuuuu(6),llllll(6),xor(2),csum(2),'\n' = 1+4+6+6+2+2+1 = 22
// or
// 'S',xxxx(4),uuuuuu(6),llllll(6),'\n' = 1 + 4 + 6 + 6 + 1 = 18
// =

uint8_t gethex(uint8_t *p)
{
	uint8_t ch,val;
	ch = *p ++;
	if('0' <= ch && ch <= '9') val = ch -'0';
	else if('a' <= ch && ch <= 'f') val = ch - 'a' + 10;
	else if('A' <= ch && ch <= 'F') val = ch - 'A' + 10;
	val <<= 4;

	ch = *p ++;
	if('0' <= ch && ch <= '9') val |= ch -'0';
	else if('a' <= ch && ch <= 'f') val |= ch - 'a' + 10;
	else if('A' <= ch && ch <= 'F') val |= ch - 'A' + 10;
	return val;
}

static int state = 0;
void uart_rx_char(uint8_t ch)
{
#if 0
	if(ch == STX) {
		rx_cmd_idx = 0;
		rx_cmd_buf[rx_cmd_idx++] = ch;
		return;
	}
	else if(ch == '\n')
	{
		rx_cmd_buf[rx_cmd_idx++] = ch;
		rx_cmd_buf[rx_cmd_idx++] = 0;
		process_cmd();
		rx_cmd_idx = 0;
		return;
	}
	else if(rx_cmd_idx && rx_cmd_idx < 127)
	{
		rx_cmd_buf[rx_cmd_idx++] = ch;
	}
#else
	switch(state)
	{
	case 0 :
		if(ch == 'S') {
			state = 1;
		}
		break;
	case 1 :
		if(ch == 'T') {
			state = 2;
			rx_cmd_idx = 0;
		}
		else
		{
			state = 0;
		}
		break;
	case 2 :
		if(ch == '\n')
		{
			state = 3;
		}
		else
		{
			if(rx_cmd_idx < 30)
			{
				rx_cmd_buf[rx_cmd_idx++] = ch;
			}
			else
			{
				; // too long message
			}
		}
		break;
	case 3:
		if(ch == '\n')
		{
			state = 0;
			rx_cmd_buf[rx_cmd_idx ++] = '\0'; // null terminated string
			process_cmd();
		}
		else
		{
			state = 0; // illegal message
		}
		break;
	}
#endif
	return;
}

int uart1_rx_ptr;
void rx_dma_process()
{
	uint32_t wr_ptr;
	wr_ptr = UART1_DMA_BUF_SZ - hdma_usart1_rx.Instance->CNDTR;
	while(wr_ptr != uart1_rx_ptr)
	{
		uart_rx_char(uart1_rx_buf[uart1_rx_ptr]);
		uart1_rx_ptr = (uart1_rx_ptr + 1) % UART1_DMA_BUF_SZ;
		wr_ptr = UART1_DMA_BUF_SZ - hdma_usart1_rx.Instance->CNDTR;
	}
}
void uart_loop()
{
	rx_dma_process();

	if(reinit_flag_1)
	{
      reinit_flag_1 = 0;
      reinit_uart(1);
      printf("UART1 reinit\n");
	}

	if(reinit_flag_2)
	{
      reinit_flag_2 = 0;
      reinit_uart(2);
      printf("UART2 reinit\n");
	}
	if(is_available())
	{
		extern int delete_uart_Q();
		extern int is_available();
		extern int do_cmd(char ch);
		char ch;
		ch = delete_uart_Q();
		do_cmd(ch);
	}
}

#if 0
void uart1_init()
{
      HAL_UART_Receive_DMA(&huart1, uart1_rx_buf ,UART1_DMA_BUF_SZ);
}
#endif
