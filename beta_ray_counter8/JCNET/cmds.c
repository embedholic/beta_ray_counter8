/*
 * cmds.c
 *
 *  Created on: 2022. 1. 31.
 *      Author: isjeon
 */

#include "main.h"
#include "stdio.h"
#include "string.h"
#include "jcnet.h"
void clock_gen(int ac, char *av[]);

extern counter_type ray_counter;
void disp_counter(int ac, char *av[])
{
	int i;
	for( i = 0 ; i < 8 ; i ++)
	{
		printf("CH=%d CNT=%8d\n",i,ray_counter.acc_cntrs[i]);
	}
}
typedef struct _cmd_node_tag {
        const char *cmd;
        void (*fn)(int ac, char *av[]);
        const char *help;
} cmd_node_t;
const cmd_node_t cmd_tbl[] =
{
                {"clock_gen",     clock_gen,    "clock_gen number"},
				{"counter",       disp_counter, "display 8 channel counters"},
};

void _delay_us(uint32_t v)
{
	volatile i ;
	for( i = 0 ; i < v ; i ++);
}
void clock_gen(int ac, char *av[])
{
	int cnt = 0;
	int i;
	if(ac >= 2)
	{
		cnt = atoi(av[1]);

	}
	printf("CNT=%d\n",cnt);
	for( i = 0 ; i < cnt ; i ++)
	{
		GPIOA->BSRR = EXTR_UOUT_Pin; // PA4
		_delay_us(10);
		GPIOA->BSRR = (EXTR_UOUT_Pin << 16);
		_delay_us(10);
//		HAL_Delay(1);
	}
}

int exec_cmd(uint8_t *cmd, int ac,char *av[])
{
        int i;
        for( i = 0 ; i < sizeof(cmd_tbl) / sizeof(cmd_tbl[0]) ; i ++)
        {
                if(!strcmp(cmd, cmd_tbl[i].cmd))
                {
                        cmd_tbl[i].fn(ac, av);
                        return 0;
                }
        }
        return -1;
}
extern int _write(int file, char *data, int len);
extern void my_putchar(char c)
{
	_write(0, &c,1);
}
int get_args(char *buf, char *av[])
{
        int     num, start, end;
        start = end = num = 0;
        while (1)
        {
//printf("buf+start = [%s] start=%d end=%d buf[end]=%x num=%d\n",buf, start,end,buf[end],num);
                if(buf[end] == '\0' || buf[end] == '\n' || buf[end] == '\r')
                {
                        if(buf[end]) buf[end] = '\0';
                        if(start != end)
                        {
                                strcpy(av[num],buf+start);
                                num ++;
                                return num;
                        }
                        else
                        {
                                return num;
                        }
                }
                if(buf[end] != ' ' && buf[end] != '\t' ) {
                        end ++;
                }
                else
                {
                        buf[end] = 0;
                        strcpy(av[num],buf+start);
                        num ++;
                        end ++;
                        start = end ;
                }
        }
        return 0;
}

char avbuf[6][64];
char *av[6] = {
                &avbuf[0][0],
                &avbuf[1][0],
                &avbuf[2][0],
                &avbuf[3][0],
                &avbuf[4][0],
                &avbuf[5][0]
};
const char *prompt="jcnet";
char *version="ray_cnt";
static char cmd_buf[128],old_buf[128];
static int idx = 0;
#define _DBG_MODE

int do_cmd(char ch)
{

        char buf[128];
        int ac,i;
        if(ch == '\n' || ch == '\r')
        {
#ifdef _DBG_MODE
                 my_putchar('\n');
#endif
                 cmd_buf[idx] = '\0';
                 if(cmd_buf[0] == STX)
                 {
                	 extern void exec_counter_param(char *);
                	 exec_counter_param(cmd_buf);
                 }
#if 1
                 if(!strncmp(cmd_buf,"!!",2))
                 {
                         strcpy(cmd_buf,old_buf);
                 }
#endif
                 strcpy(buf,cmd_buf);
                 for( i = 0 ; i < 6 ; i ++) av[i] = &avbuf[i][0];
                 ac = get_args(cmd_buf, av);
                 if(idx == 0 || !ac) {
                         idx = 0;
                         printf("%s-%s> ",prompt,version); fflush(stdout);
                         return 0;
                 }
                 strcpy(old_buf,buf);
                 exec_cmd(av[0],ac, av);
                 printf("%s-%s> ",prompt,version); fflush(stdout);
                 idx = 0;
        }
        else if(ch == '\b')
        {
                 if(idx > 0)
                 {
                	 	 idx --;
#ifdef _DBG_MODE
                	 	 my_putchar('\b'); my_putchar(' '); my_putchar('\b');
#endif
                	 	 return 0;
                 }
        }
        else if(idx < 63) {
                 cmd_buf[idx++] = ch;
#ifdef _DBG_MODE
                 my_putchar(ch);
#endif
        }

        return 0;

}
