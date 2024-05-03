/*
 * clcd.c
 *
 *  Created on: 2020. 2. 16.
 *      Author: isjeon
 */


#include "main.h"

#define BIT4_LINE2_DOT58  0x28
#define DISPON_CUROFF_BLKOFF 0x0c
#define DISPOFF_CUROFF_BLKOFF 0x08
#define INC_NOSHIFT  0x06
#define DISPCLEAR  0x01

#define CUR1LINE   0x80
#define CUR2LINE   0xc0
#define CURHOME    0x02

#define LCD_D4_Pin GPIO_PIN_5
#define LCD_D4_GPIO_Port GPIOC

// Notice
// BETA counter board
// RS : PC14
// NUCLEO board
// RS : A1

//#define delay_us(x) HAL_Delay(1)
extern TIM_HandleTypeDef htim1;
void delay_us(uint32_t us)
{
	uint16_t s;
	s = htim1.Instance->CNT;
	while((uint16_t)(htim1.Instance->CNT - s) <= us);
}
#define LCD_RS_GPIO_Port GPIOC // GPIOA
#define LCD_RS_Pin (1 << 14) // (1 << 1)

#define LCD_E_GPIO_Port GPIOB
#define LCD_E_Pin (1 << 3)


#define RS_LOW	  LCD_RS_GPIO_Port->BSRR = LCD_RS_Pin << 16
#define RS_HIGH	  LCD_RS_GPIO_Port->BSRR = LCD_RS_Pin << 0
#define E_LOW     LCD_E_GPIO_Port->BSRR  = LCD_E_Pin << 16
#define E_HIGH     LCD_E_GPIO_Port->BSRR  = LCD_E_Pin << 0
#define E_PULSE   E_HIGH; delay_us(1); E_LOW; delay_us(9)

void E_test()
{
	while(1)
	{
	E_LOW; HAL_Delay(1000);
	E_HIGH; HAL_Delay(1000);
	}
}
void RS_test()
{
	while(1)
	{
	RS_LOW; HAL_Delay(1000);
	RS_HIGH; HAL_Delay(1000);
	}
}
void D_test()
{
	while(1)
	{
		GPIOB->ODR = 0xf0; HAL_Delay(1000);
		GPIOB->ODR = 0x00; HAL_Delay(1000);
	}
}
//extern void delay_us();
void(*polling_fn)() = (void (*)())0;
void CLCD_cmd(uint8_t cmd)
{
	RS_LOW;// RS = 0
//	HAL_Delay(10);

	GPIOB->ODR = (GPIOB->ODR & 0xf) | (cmd & 0xf0);
	E_PULSE;
	GPIOB->ODR = (GPIOB->ODR & 0xf) | ((cmd & 0x0f) << 4);
	E_PULSE;
	HAL_Delay(10);
}

void CLCD_cmd_high_nibble(uint8_t cmd)
{
	RS_LOW;// RS = 0
//	HAL_Delay(10);

	GPIOB->ODR = (GPIOB->ODR & 0xf) | (cmd & 0xf0);
	E_PULSE;
	HAL_Delay(1);
}

void CLCD_data(uint8_t data)
{
	RS_HIGH; // RS = 1
	HAL_Delay(1);

	GPIOB->ODR = (GPIOB->ODR & 0xf) | (data & 0xf0);
	E_PULSE;
	GPIOB->ODR = (GPIOB->ODR & 0xf) | ((data & 0x0f) << 4);

	E_PULSE;
	HAL_Delay(1);
}
void CLCD_puts_fill(uint8_t *str)
{
	int cnt = 0;
	while(*str) {
		CLCD_data(*str++);
		cnt ++;
//		if(polling_fn) (*polling_fn)();
//		HAL_Delay(5);
	}
	while(cnt++ < 16) CLCD_data(' ');
}
void CLCD_puts(uint8_t *str)
{
	while(*str) {
		CLCD_data(*str++);
	}
}
void CLCD_printf(uint8_t x, uint8_t y, uint8_t *str)
{
	CLCD_cmd(0x80 + 0x40*y + x);
//	HAL_Delay(10);
	CLCD_puts(str);
}
void CLCD_init()
{

	CLCD_cmd_high_nibble(0x30);
	CLCD_cmd_high_nibble(0x30);
	CLCD_cmd_high_nibble(0x30);
	CLCD_cmd_high_nibble(0x20);
	CLCD_cmd(BIT4_LINE2_DOT58);
	CLCD_cmd(DISPON_CUROFF_BLKOFF);
	CLCD_cmd(INC_NOSHIFT);
	CLCD_cmd(DISPCLEAR);

	HAL_Delay(2);
	CLCD_cmd(CUR1LINE);
	HAL_Delay(2);
	CLCD_puts(" PULSE COUNTER ");
	HAL_Delay(2);
	CLCD_cmd(CUR2LINE);
	CLCD_puts("   v1.0(JCNET)");
	HAL_Delay(500);
}

void display_idle(uint32_t v, uint32_t idle_seconds)
{
	char buf[20];
#if 0
	CLCD_cmd(CUR1LINE);
	sprintf(buf,"IDLE: %9d", idle_seconds);
	CLCD_puts_fill(buf);
	CLCD_cmd(CUR2LINE);
	sprintf(buf,"ACC: %010d",v);
	CLCD_puts_fill(buf);
#else
	CLCD_cmd(CUR2LINE);
	sprintf(buf,"IDLE%5d/%6d", v, idle_seconds);
	CLCD_puts_fill(buf);
#endif
}
char *to_lcd_string(uint32_t v)
{
        int remain1,remain2;
        static char buf[36];
        remain1 = v % 1000;
        v /= 1000;
        remain2 = v % 1000;
        v /= 1000;
        if(v) {
                sprintf(buf,"%3d,%03d,%03d",v,remain2,remain1);

        }
        else {
                sprintf(buf,"    ");
                if(remain2) {
                        sprintf(buf+strlen(buf),"%3d,%03d",remain2,remain1);
                }
                else
                {
                        sprintf(buf+strlen(buf),"    %3d",remain1);
                }
        }
  //      printf("val = %08d %s\n",v,buf);
        return buf;
}
static int turn = 0;
void display_run(uint32_t min, uint32_t max, uint32_t cur, uint32_t remain_time)
{
	char buf[20];
	int ok;
	CLCD_cmd(CUR1LINE);
	if(turn == 0)
		sprintf(buf,"Min =%s", to_lcd_string(min));
	else
		sprintf(buf,"Max =%s",to_lcd_string(max));
	turn = !turn;
	CLCD_puts_fill(buf);
	CLCD_cmd(CUR2LINE);

	sprintf(buf," %08d/%05d ",cur,remain_time);
	CLCD_puts_fill(buf);
}
extern uint32_t idle_seconds;
void display_run_done(uint32_t min, uint32_t max, uint32_t cur)
{
	char buf[20];
	int ok;
	CLCD_cmd(CUR1LINE);
//	sprintf(buf," %6Xh<>%6Xh", min,max);
	if(min >= cur)
	{
		sprintf(buf,"FAIL :LOW");
	}
	else if(max <= cur)
	{
		sprintf(buf,"FAIL :HIGH");
	}
	else
	{
		sprintf(buf,"PASS            ");
	}
	CLCD_puts_fill(buf);
	CLCD_cmd(CUR2LINE);
	sprintf(buf," %08d/%05d ",cur,0);
	CLCD_puts_fill(buf);
	idle_seconds = 0;
}
