/*
 * mylib.c
 *
 *  Created on: 2021. 2. 20.
 *      Author: isjeon
 */

#include "main.h"
//#include "stm32l4xx_hal.h"
#include <stdio.h>
#include "jcnet.h"

extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
int _write(int file, char *data, int len)
{
    int bytes_written;
    HAL_UART_Transmit(&huart2,(uint8_t *)data, len, 1000);
    bytes_written = len;
    return bytes_written;
}
extern void display_idle(uint32_t v, uint32_t idle_seconds);
extern void display_run(uint32_t min, uint32_t max, uint32_t cur);
uint32_t g_tick, idle_seconds = 0;
uint32_t g_idle_acc_cnt = 0;

uint32_t state = 1;

uint32_t c_dur_ms = 0;
uint32_t test_cnt = 1990;
extern __IO uint32_t S_done_lead_time;
extern void uart_loop();
extern void counter_task();
extern __IO uint32_t S_run_flag;
extern __IO uint32_t idle_counter_prev;
struct _sys_param_tag {
	uint32_t tx_period_ms; //  send_period_param * 10 = miliseconds.
	uint32_t tx_period_param; // 1/100 second
	uint32_t tx_last_tick;
} sys_param =
{
		10*10, // 100 mili
		10 , // 1/100 * 10 = 0.1 seconds = 100 mili
};
uint8_t uart2_rx_Q_buf[64];
uart_rx_queue_t uart2_rx_q =
{
		.data = uart2_rx_Q_buf,
		.size = 64
};

// 800 us maximum
void _delay_us_tim15(uint32_t v)
{
	uint16_t start_tick, elapse;
	start_tick = htim15.Instance->CNT;
	v *= 80;
	while(1)
	{
		elapse =  (uint16_t)(htim15.Instance->CNT & 0xffff) - start_tick;
		if(elapse >= v) return;
	}
}
void _delay_us_sw(uint32_t v)
{
	volatile i ;
	for( i = 0 ; i < v ; i ++);
}


int insert_uart(uint8_t ch)
{
	uart_rx_queue_t *Q;
	Q = &uart2_rx_q;
	if((Q->wr + 1) % Q->size == Q->rd)
	{
	        return -1; // Full
	}
    Q->data[Q->wr] = ch;
	Q->wr = (Q->wr + 1) % Q->size;
	return 0;
}
int delete_uart_Q()
{
        int ch;
    	uart_rx_queue_t *Q;
    	Q = &uart2_rx_q;

        if(Q->wr == Q->rd) return -1;
        ch = Q->data[Q->rd];
        Q->rd = (Q->rd + 1) % Q->size;
        return ch;
}

int is_available()
{
		uart_rx_queue_t *Q;
		Q = &uart2_rx_q;
        return (Q->wr != Q->rd);
}

void my_loop()
{
	uint32_t led_tick;
	while(1)
	{
		if(HAL_GetTick() - led_tick >= 500)
		{
			if(GPIOB->IDR & LD3_Pin) GPIOB->BSRR = LD3_Pin << 16;
			else GPIOB->BSRR = LD3_Pin;
			led_tick = HAL_GetTick();
		}

		counter_task();
		uart_loop();
	}
}

//
// FLASH page 31 -> 127(256K flash)
// 0x0800:f800 ~ 0x0800:ffff : 0x800 = 2K

int erase_pages(int page, int num)
{
    FLASH_EraseInitTypeDef flash_erase;
    uint32_t ecode;
    int ret;
    flash_erase.TypeErase = 0;
    flash_erase.Banks = FLASH_BANK_1;
    flash_erase.Page = page;
    flash_erase.NbPages = num;
    ret = HAL_FLASH_Unlock();
    ret += HAL_FLASHEx_Erase(&flash_erase,&ecode);
    ecode = HAL_FLASH_GetError();
    ret += HAL_FLASH_Lock();
    return ret;
}
int param_set(uint32_t v)
{
    FLASH_EraseInitTypeDef flash_erase;
    int sector, num;
    uint32_t ecode;
    HAL_StatusTypeDef ret;
    uint32_t addr = 0x0803f800; // last 31 page start address
    uint64_t data;

    ret = erase_pages(127,1);

    if(ret) return ret;
    HAL_Delay(1);
    data = v | ((uint64_t)~v << 32);
    HAL_FLASH_Unlock();
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,addr, data);
    HAL_FLASH_Lock();
    return ret;
}

int param_get(uint32_t *v)
{
    uint32_t addr = 0x0803f800; // last 31 page start address
    uint32_t a,b;
    a = ((__IO uint32_t *)addr)[0];
    b = ((__IO uint32_t *)addr)[1];
    if(a == 0xffffffff && b == 0xffffffff) return 2;
    b = a + b;
    if(b == 0xffffffff)
    {
    	*v = a;
    	return 0;
    }
    return 1;
}
