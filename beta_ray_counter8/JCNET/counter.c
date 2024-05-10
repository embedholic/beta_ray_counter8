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
#include "jcnet.h"
#include "i2c_clcd.h"

extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern LPTIM_HandleTypeDef hlptim1;
extern LPTIM_HandleTypeDef hlptim2;
extern UART_HandleTypeDef huart1;

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



#include <string.h>
#include <stdio.h>
__IO uint32_t idle_counter_prev = 0;
uint8_t ack_buf[128];
extern void display_run(uint32_t min, uint32_t max, uint32_t cur, uint32_t remain_time);
static uint32_t pre_tick = 0;
counter_type ray_counter =
{
//		.update_period_tick = COUNTER_UPDATE_DFT_PERIOD,
};

void exec_counter_param(char *buf)
{
	uint32_t tmp;
	system_type info;
	int i;
	if(strlen(buf) < 8) return; // illegal size
	if(buf[1] == 'D') info.dis_format = D_FMT_DEC;
	else if(buf[1] == 'H') info.dis_format = D_FMT_HEX;
	else return; // format error

	if(buf[2] == 'W') info.cnt_type = CNT_TYPE_W;
	else if(buf[2] == 'O') info.cnt_type = CNT_TYPE_O;
	else return; // format error

	sscanf(buf+3,"%d",&tmp);
	if(tmp > 100) // 10초보다 크면 10초로 고정
    tmp = 100;
	if(tmp < 10) // 0.1~0.9
	{
		info.update_period_tick = tmp * 100; // ms 단위로 환산
	}
	else
	{
		tmp = tmp / 10; // 소숫점은 버림.. 1.1초 같은건 없음.
		info.update_period_tick = tmp * 1000; // ms 단위로 환산
	}
	if(!param_set(*(uint32_t *)&info))
	{
		memcpy(&sys_info, &info, sizeof(info));
		ray_counter.rd_tick = HAL_GetTick();
		for( i = 0 ; i < 8 ; i ++)
		{
			ray_counter.pre_cntrs[i] = ray_counter.CNT[i] & 0xffff;
			ray_counter.acc_cntrs[i] = 0;
		}
		ray_counter.update_tick = HAL_GetTick();

	    info_printf("New param applied CNT=%s\nFMT=%s\nPERIOD=%dmS\n",
				  (sys_info.cnt_type == CNT_TYPE_W)?"WINDOW":"OUT",
				  (sys_info.dis_format == D_FMT_DEC)?"DEC":"HEX",
						  sys_info.update_period_tick);
	}
	else
	{
		info_printf("New param save failed !\n");
	}
}


extern system_type sys_info;
void update_slave_cnt(uint32_t *v)
{
	ray_counter.CNT[4] = v[0];
	ray_counter.CNT[5] = v[1];
	ray_counter.CNT[6] = v[2];
	ray_counter.CNT[7] = v[3];
}
char S2M_data[UART1_DMA_BUF_SZ + 2]; // +1 : NULL for string, +1 dummy for safety
void counter_task()
{
	int i;
	uint32_t cur_tick, tmp_cnt;

	cur_tick = HAL_GetTick();
	if(cur_tick - ray_counter.rd_tick >= COUNTER_GATHER_PERIOD)
	{
		ray_counter.CNT[0] = htim1.Instance->CNT & 0xffff;
		ray_counter.CNT[1] = htim2.Instance->CNT & 0xffff;
		ray_counter.CNT[2] = hlptim1.Instance->CNT & 0xffff;
		ray_counter.CNT[3] = hlptim2.Instance->CNT & 0xffff;

		for( i = 0 ; i < 8 ; i ++)
		{
			tmp_cnt = ray_counter.CNT[i] & 0xffff;
			ray_counter.acc_cntrs[i] += (uint32_t)((tmp_cnt - ray_counter.pre_cntrs[i]) & 0xffff);
			ray_counter.pre_cntrs[i] = tmp_cnt;
		}
		ray_counter.rd_tick = cur_tick;
		sprintf(S2M_data,"$%04x%04x%04x%04x\x0a",
				ray_counter.CNT[0] & 0xffff,
				ray_counter.CNT[1] & 0xffff,
				ray_counter.CNT[2] & 0xffff,
				ray_counter.CNT[3] & 0xffff
				);
		HAL_UART_Transmit_DMA(&huart1, (const uint8_t *)S2M_data, UART1_DMA_BUF_SZ);
	}
	if(cur_tick - ray_counter.update_tick >= sys_info.update_period_tick)
	{
		static char buf[3 + 6 + 7*4 + 2]; // 3(S/D/W), SEQ(5+1(,)), COUNT(7*4) + NULL + EXTRA) = 38bytes tx = 3.3ms at 115200
		static char disp_buf[20];
		static uint32_t a,b,c,d;

#ifdef CLCD_TIME_EVAL
		uint32_t start_tick,end_tick;
#endif
		a ++;
		b = a + a;
		c = a + a + a;
		d = a + a + a + a;
		if(sys_info.cnt_type == CNT_TYPE_W)
		{

		}
		else // CNT_TYPE_O
		{

		}
		if(a > 999999) a = 0;
		if(b > 999999) b = 0;
		if(c > 999999) c = 0;
		if(d > 999999) d = 0;
		sprintf(disp_buf,"A:%06dB:%06d",a,b);
#ifdef CLCD_TIME_EVAL
		start_tick = htim15.Instance->CNT;
#endif
		i2c_lcd_string(0, 0,disp_buf); //34.117ms , x 2 = 68.234 ms lcd display
#ifdef CLCD_TIME_EVAL
		end_tick = htim15.Instance->CNT;
#endif
		sprintf(disp_buf,"C:%06dD:%06d",c,d);
		i2c_lcd_string(1, 0,disp_buf);
#ifdef CLCD_TIME_EVAL
		end_tick -= start_tick;
		end_tick &= 0xffff;
		info_printf("Elpased = %d (%d us)\n", end_tick, (int)( end_tick / 1. + 0.5));
#endif
		if(g_output_to_pc){
			SEQ_ADV(ray_counter.seq);
			if(sys_info.dis_format == D_FMT_DEC)
			{
				sprintf(buf,"%cD%c%05d,",START_CHAR,sys_info.cnt_type == CNT_TYPE_W ? 'W':'O', ray_counter.seq);
				sprintf(buf+strlen(buf),"%06d,%06d,%06d,%06d%c",a,b,c,d,END_CHAR);
#ifdef USART2_DMA
				while(g_dma_tx_flag); g_dma_tx_flag = 1;
			    HAL_UART_Transmit_DMA(&huart2,(uint8_t *)buf, strlen(buf));
				while(huart2.hdmatx->Instance->CNDTR);
#else
			    HAL_UART_Transmit(&huart2,(uint8_t *)buf, strlen(buf), 1000);
#endif
			}
			else if(sys_info.dis_format == D_FMT_HEX)
			{
				sprintf(buf,"%cH%c%05x,",START_CHAR,sys_info.cnt_type == CNT_TYPE_W ? 'W':'O', ray_counter.seq);
				sprintf(buf+strlen(buf),"%06x,%06x,%06x,%06x%c",a,b,c,d,END_CHAR);
#ifdef USART2_DMA
				while(g_dma_tx_flag); g_dma_tx_flag = 1;
			    HAL_UART_Transmit_DMA(&huart2,(uint8_t *)buf, strlen(buf));
#else
			    HAL_UART_Transmit(&huart2,(uint8_t *)buf, strlen(buf), 1000);
#endif
			}
			else info_printf("Error display format = %d\n",sys_info.dis_format);
		}
		for( i = 0 ; i < 8 ; i ++) ray_counter.acc_cntrs[i] = 0;
		ray_counter.update_tick = cur_tick;
	}


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
	info_printf("RUN_TIME=%x, U_LIMIT=%06x, L_LIMIT=%06x\n",
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
