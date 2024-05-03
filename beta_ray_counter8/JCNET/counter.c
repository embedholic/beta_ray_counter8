/*
 * counter.c
 *
 *  Created on: 2021. 2. 21.
 *      Author: isjeon
 */


/*
 * uart.c
 *
 *  Created on: 2021. 2. 21.
 *      Author: isjeon
 */


/* ----- PC checksum generation code and sample message for test
$ cat x.c
#include <stdio.h>

uint8_t sample1[32] = {'S','1','2','3','4','F','f','f','a','a','8',
        '0','1','2','3','4','5','\0'};
uint8_t sample2[32] = {'S','a','0','0','0','F','f','f','a','a','8',
        '0','1','2','3','4','5','\0'};
main()
{
        uint8_t xor = 0, csum = 0;
        int i;
        for( i = 0 ; i < (1+4+6+6) ; i ++)
        {
                xor ^= sample1[i] ;
                csum += sample1[i] ;
        }
        sprintf(sample1+1+4+6+6,"%02x%02x\n",xor,csum);
        printf("msg1=%s",sample1);

        xor = 0, csum = 0;

        for( i = 0 ; i < (1+4+6+6) ; i ++)
        {
                xor ^= sample2[i] ;
                csum += sample2[i] ;
        }
        sprintf(sample2+1+4+6+6,"%02x%02x\n",xor,csum);
        printf("msg2=%s",sample2);
}

isjeon@MY_NOTEBOOK ~
$ ./a.exe
msg1=S1234Fffaa80123452858
msg2=Sa000Fffaa80123457d7f


 */

#include "main.h"
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;

#define GET_COUNT() htim2.Instance->CNT

#define PC_CMD_WCSUM_LEN (1+4+6+6+2+2+1)
#define PC_CMD_WOCSUM_LEN (PC_CMD_WCSUM_LEN - 4)
#define XOR_LOC (PC_CMD_WCSUM_LEN- 5)
#define CSUM_LOC (PC_CMD_WCSUM_LEN - 3)

int rx_cmd_idx = 0;
int check_sum_flag = 0;
uint8_t rx_cmd_buf[128];
__IO uint32_t S_counter_h_limit;
__IO uint32_t S_counter_l_limit;
__IO uint32_t S_run_time;
__IO uint32_t S_counter_current;
__IO uint32_t S_run_flag, S_done_flag;
__IO uint32_t S_run_display_flag;
__IO uint32_t S_counter_start;
__IO uint32_t S_counter_display_tick;
__IO uint32_t S_done_lead_time = 0;


void my_tick_callback()
{
	if(--S_counter_display_tick == 0)
	{
		S_counter_display_tick = 1000;
		S_run_display_flag = 1;
	}
	if(S_run_flag)
	{
		if(-- S_run_time == 0)
		{
			S_counter_current = GET_COUNT() - S_counter_start;
			S_done_flag = 1;
			S_run_flag = 0;
		}
	}
}
#include <string.h>
#include <stdio.h>
__IO uint32_t idle_counter_prev = 0;
uint8_t ack_buf[128];
extern void display_run(uint32_t min, uint32_t max, uint32_t cur, uint32_t remain_time);
void counter_task()
{
	if(S_run_flag)
	{
		if(S_run_display_flag)
		{
			S_run_display_flag = 0;
			display_run(
					S_counter_l_limit,
					S_counter_h_limit,
					GET_COUNT() - S_counter_start,
					S_run_time/1000
			);
			GPIOB->ODR ^= HB_LED_Pin;
		}
	}
	if(S_done_flag)
	{
		extern void display_run_done(uint32_t min, uint32_t max, uint32_t cur);
		display_run_done(
				S_counter_l_limit,
				S_counter_h_limit,
				S_counter_current
		);
#if 0
		if( S_counter_l_limit >= S_counter_current)
		{
			sprintf(ack_buf,"STFL%06X%03XL\n\n",S_counter_current,0x0000);
//			GPIOA->BSRR = FAIL_LED_Pin;
		}
		else
		if( S_counter_current >= S_counter_h_limit)
		{
			sprintf(ack_buf,"STFL%06X%03XH\n\n",S_counter_current,0x0000);
//			GPIOA->BSRR = FAIL_LED_Pin;
		}
		else
		{
			sprintf(ack_buf,"STPS%06X%03XN\n\n",S_counter_current,0x0000);
//			GPIOA->BSRR = SUCC_LED_Pin;
		}

		_write(0, ack_buf,strlen(ack_buf));
#else
#endif
		S_done_flag = 0;
		S_done_lead_time = 3;

	}
}



//#define PROTOCOL_DEBUG
void process_cmd()
{
	uint32_t run_time, counter_h_limit, counter_l_limit;
	uint8_t save_ch;
	save_ch = rx_cmd_buf[4];
	rx_cmd_buf[4] = 0;
	sscanf((char *)rx_cmd_buf,"%x",(unsigned int*)&run_time);
	rx_cmd_buf[4] = save_ch;

	save_ch = rx_cmd_buf[ 4 + 6];
	rx_cmd_buf[4 + 6] = 0;
	sscanf((char *)rx_cmd_buf + 4,"%x",(unsigned int*)&counter_h_limit);
	rx_cmd_buf[ 4 + 6] = save_ch;

	save_ch = rx_cmd_buf[4+6+6];
	rx_cmd_buf[4 + 6 + 6] = 0;
	sscanf((char *)rx_cmd_buf + 4 + 6,"%x",(unsigned int*)&counter_l_limit);
	rx_cmd_buf[4 + 6 + 6] = save_ch;
#ifdef PROTOCOL_DEBUG
	printf("RUN_TIME=%x, U_LIMIT=%06x, L_LIMIT=%06x\n",
			run_time, counter_h_limit, counter_l_limit);
#endif
	S_counter_h_limit = counter_h_limit;
	S_counter_l_limit = counter_l_limit;
	S_run_time = run_time;
	S_counter_current = 0;
	S_counter_start = GET_COUNT();
	S_run_flag = 1;
	S_counter_display_tick = 1000; // 1 seconds display

//	GPIOA->BSRR = (HB_LED_Pin | SUCC_LED_Pin | FAIL_LED_Pin) << 16;
}
