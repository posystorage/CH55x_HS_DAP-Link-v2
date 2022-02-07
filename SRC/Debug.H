/* 调试 */
/* 提供printf子程序和延时函数 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

//定义函数返回值
#ifndef SUCCESS
#define SUCCESS 0
#endif
#ifndef FAIL
#define FAIL 0xFF
#endif

//定义定时器起始
#ifndef START
#define START 1
#endif
#ifndef STOP
#define STOP 0
#endif

//#ifndef  DE_PRINTF
//#define  DE_PRINTF     1
//#endif
#define FREQ_SYS 16000000 //系统主频16MHz

void CfgFsys();                         //CH552时钟选择和配置
void mDelayuS(UINT16 n);                // 以uS为单位延时
void mDelaymS(UINT16 n);                // 以mS为单位延时
void CH552UART0Alter();                 //CH552串口0引脚映射到P0.2/P0.3
void UART0Setup();                      //T1作为波特率发生器
UINT8 CH552UART0RcvByte();              //CH552 UART0查询方式接收一个字节
void CH552UART0SendByte(UINT8 SendDat); //CH552UART0发送一个字节

void UART1Setup();                      //
UINT8 CH552UART1RcvByte();              //CH552 UART1查询方式接收一个字节
void CH552UART1SendByte(UINT8 SendDat); //CH552UART1发送一个字节

void CH552WatchDog(UINT8 mode);    //CH552看门狗模式设置
void CH552WatchDogFeed(UINT8 tim); //CH552看门狗喂狗
#endif
