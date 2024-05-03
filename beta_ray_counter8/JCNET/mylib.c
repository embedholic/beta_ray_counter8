/*
 * mylib.c
 *
 *  Created on: 2021. 2. 20.
 *      Author: isjeon
 */

#include "main.h"
#include "stm32l4xx_hal.h"
#include <stdio.h>
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

void my_loop()
{
	uint32_t cur_tick,pre_tick;
	uint32_t elapsed_tick = 0;
	pre_tick = htim1.Instance->CNT;
	while(1)
	{
//		printf("HI ~ isjeon \n");
//		HAL_Delay(1000);

		counter_task();
		uart_loop();
		if(S_run_flag == 0)
		{
			cur_tick = HAL_GetTick();
#if 0
			if(cur_tick - pre_tick >= 1000)
			{
				pre_tick = cur_tick;
				idle_seconds += 1;
				g_idle_acc_cnt = htim2.Instance->CNT;
				if(S_done_lead_time)
				{
					S_done_lead_time--;
					idle_counter_prev = htim2.Instance->CNT;
				}
				else {
					display_idle(g_idle_acc_cnt - idle_counter_prev ,idle_seconds);
					idle_counter_prev = g_idle_acc_cnt;
				}
			}
#else
			if(cur_tick - sys_param.tx_last_tick >= sys_param.tx_period_ms)
			{
				sys_param.tx_last_tick = cur_tick;
				printf("Tx..\n");
				GPIOB->ODR ^= LD3_Pin;
			}
#endif
		}
	}
}
