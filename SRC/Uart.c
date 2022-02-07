#include "CH552.H"
#include "Uart.H"
#include "Debug.H"


#define ENDP1_SIZE 64

UINT8 FIFO_Write_Pointer;
UINT8 FIFO_Read_Pointer;
UINT8 UART_RX_Data_Pointer;
BOOL EP1_TX_BUSY;
BOOL UART_TX_BUSY;

UINT8I UART_RX_Data_Buff[ENDP1_SIZE];
UINT8X UART_TX_Data_Buff[256]_at_ 0x0300;

void UART_Setup(void)
{
	FIFO_Write_Pointer=0;
	FIFO_Read_Pointer=0;	
	UART_RX_Data_Pointer=0;
	UART_TX_BUSY=FALSE;	
	EP1_TX_BUSY=FALSE;		
	
#if USE_UART0 == 1
	P3_MOD_OC|=0x03;
	P3_DIR_PU|=0x03;
	
            //使用Timer1作为波特率发生器	
  RCLK = 0; //UART0接收时钟
  TCLK = 0; //UART0发送时钟
  PCON |= SMOD;

  TMOD = TMOD & ~bT1_GATE & ~bT1_CT & ~MASK_T1_MOD | bT1_M1; //0X20，Timer1作为8位自动重载定时器
  T2MOD = T2MOD | bTMR_CLK | bT1_CLK;                        //Timer1时钟选择
  TH1 = 0 - (UINT32)(FREQ_SYS+UART_BUAD*8) / UART_BUAD / 16;//12MHz晶振,buad/12为实际需设置波特率
  TR1 = 1; 																	//启动定时器1
	SCON = 0x50;//串口0使用模式1    TI = 1;    REN = 1;       
	ES = 1;	
#elif	USE_UART1 == 1	
	P1_MOD_OC|=0xC0;//开漏
	P1_DIR_PU|=0xC0;//使能输出上拉电阻
  U1SM0 = 0;                                                                   //UART1选择8位数据位
  U1SMOD = 1;                                                                  //快速模式
  U1REN = 1;                                                                   //使能接收
  SBAUD1 = 0 - (FREQ_SYS+UART_BUAD*8) / 16 / UART_BUAD;
  IE_UART1 = 1;
#else
	#Error "no UARTx interface define"
#endif	
	
}

//R7 = 接收数量
void UART_Get_USB_Data(UINT8 Nums)
{
#pragma asm	
	//MOV   A,R0//记录R0的只 怕冲掉
	//MOV   R6,A

	MOV  	DPTR,#0040H

	CLR  	C
	MOV  	A,FIFO_Read_Pointer //计算缓存剩余
	SUBB 	A,FIFO_Write_Pointer
	//CLR  	C
	DEC  	A
	CLR  	C
	SUBB 	A,R7
	JC   	ERR_OUT//缓存不够
	//来到这里的 C=0
	//开始搬数据
	//INC DPTR //设置到达源数据开始位置
	MOV	P2,#03H
	MOV R0,FIFO_Write_Pointer //写指针就绪  p2在程序开始已经置高地址0x03 因为缓存已经对齐 所以写指针就是缓存地址的低地址
DATA_LOP:
	MOVX A,@DPTR //获取数据源 XRAM 1T
	MOVX @R0,A //写入 XRAM 1T
	INC DPTR// 1T
	INC R0// 1T
	DJNZ R7,DATA_LOP//如果剩余数量不为0 继续搬 4-6T
	MOV FIFO_Write_Pointer,R0 //结束循环 回写

	JB   	UART_TX_BUSY,NEXT_NAK //串口忙 跳走 串口不忙 开始新的发送
	MOV 	R0,FIFO_Read_Pointer//得到读指针
	MOVX 	A,@R0
#if USE_UART0 == 1	
	MOV  	SBUF,A //Uart0
#elif	USE_UART1 == 1	
	MOV  	SBUF1,A //Uart1
#else
	#Error "no UARTx interface define"
#endif		
	INC 	FIFO_Read_Pointer
	SETB 	UART_TX_BUSY

NEXT_NAK:
		//处理64->下次缓存是否够问题
	CLR  	C
	MOV  	A,FIFO_Read_Pointer //计算缓存剩余
	SUBB 	A,FIFO_Write_Pointer
	DEC  	A
	CLR  	C
	SUBB 	A,#040H
	JC   	ERR_OUT
	ANL  	UEP1_CTRL,#0F3H //设置USB状态为ASK 	
	RET
ERR_OUT:
	MOV  	A,UEP1_CTRL  //nak
	ANL  	A,#0F3H
	ORL  	A,#08H
	MOV  	UEP1_CTRL,A
	
	//MOV   A,R6
	//MOV   R0,A
	RET  
#pragma endasm	
}

/*
//获取从USB来的数据 注意 此函数不管能不能搬完 都要硬着头皮说完了
void UART_Get_USB_Data(uint8_t Nums)//通讯中的收到包数量
{
	uint8_t i;
	uint8_t Buff_Last_nums;//缓存剩余数量
	Buff_Last_nums=FIFO_Read_Pointer-FIFO_Write_Pointer;		
	Buff_Last_nums--;
	if(Buff_Last_nums>=i)//通讯中的收到包数量 如果缓存足够
	{
		for(i=0;i<Nums;i++)
		{
			UART_TX_Data_Buff[FIFO_Write_Pointer]=Ep1Buffer[i];
			FIFO_Write_Pointer++;
		}	
		if(UART_TX_BUSY==FALSE)//串口空闲
		{
			SBUF1=UART_TX_Data_Buff[FIFO_Read_Pointer];//开始送第一个
			FIFO_Read_Pointer++;
			UART_TX_BUSY=TRUE;		
		}
		if(Buff_Last_nums>=64*2)
		{
			UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_ACK;
			return;
		}
	}
	UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_NAK;//缓存满了 不要不要	
}
*/


//从UART发送缓存拷贝到usb发送缓存 只输入长度即可
void memcpy_TXBUFF_USBBUFF(UINT8 data_len)//R7传参
{
#pragma asm		//节约寻址方法 加快速度
	MOV  	R0,#LOW (UART_RX_Data_Buff)
	INC XBUS_AUX
	MOV DPTR,#0080H //DPTR1 ep1发送缓存
	DEC XBUS_AUX
LOOP:
	MOV A,@R0
	INC R0
	DB 0A5H  //MOVX @DPTR1,A & INC DPTR1
	DJNZ R7,LOOP
	RET
#pragma endasm			
}

void UART_Send_USB_Data(void)
{
	if(UART_RX_Data_Pointer)//有要发的
	{			
		memcpy_TXBUFF_USBBUFF(UART_RX_Data_Pointer);
		UEP1_T_LEN=UART_RX_Data_Pointer;
		UART_RX_Data_Pointer=0;
		UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;//使能发送
		EP1_TX_BUSY = TRUE;
	}
}


#if USE_UART0 == 1
void Uart0_ISR(void) interrupt INT_NO_UART0 using 1
{
    if (RI)  //收到数据
    {
        UART_RX_Data_Buff[UART_RX_Data_Pointer] = SBUF;
        UART_RX_Data_Pointer++;                    //当前缓冲区剩余待取字节数
        if (UART_RX_Data_Pointer > ENDP1_SIZE)UART_RX_Data_Pointer = 0;//写入指针           
        RI = 0;
				if(EP1_TX_BUSY==FALSE)UART_Send_USB_Data();
    }
		else if(TI)//发送完成 
		{
			if(FIFO_Read_Pointer!=FIFO_Write_Pointer)//如果不等 表示需要发送
			{				
#pragma asm		//节约寻址方法 加快速度
				MOV		DPH,#03H
				MOV 	DPL,FIFO_Read_Pointer
				MOVX 	A,@DPTR
				MOV  	SBUF,A
				INC  	FIFO_Read_Pointer
#pragma endasm	
				//SBUF=UART_TX_Data_Buff[FIFO_Read_Pointer];
				//FIFO_Read_Pointer++;			
			}
			else//发送完了 最后一次进完成中断 设置串口空闲
			{
				UART_TX_BUSY=FALSE;
			}
#pragma asm		
			CLR  	C
			MOV  	A,FIFO_Read_Pointer
			SUBB 	A,FIFO_Write_Pointer
			DEC		A
			CLR  	C
			SUBB 	A,#040H
			JC		T_NAK
			ANL  	UEP1_CTRL,#0F3H //设置USB状态为ASK 
			T_NAK:
#pragma endasm				
//			if(((UINT8)FIFO_Read_Pointer-FIFO_Write_Pointer-1)>64)
//			{
//				UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_ACK;//缓存空 要
//			}
			TI =0;	
		}		
}
#elif	USE_UART1 == 1	
void Uart1_ISR(void) interrupt INT_NO_UART1 using 1
{
	if(U1RI)   //收到数据
	{		
		UART_RX_Data_Buff[UART_RX_Data_Pointer] = SBUF1;
		UART_RX_Data_Pointer++;
		if(UART_RX_Data_Pointer > ENDP1_SIZE)UART_RX_Data_Pointer=0;//出现错误 直接丢包
		U1RI =0;		
		if(EP1_TX_BUSY==FALSE)UART_Send_USB_Data();//负责第一包的发送
	}
	else if(U1TI)//发送完成 
	{
		if(FIFO_Read_Pointer!=FIFO_Write_Pointer)//如果不等 表示需要发送
		{			
#pragma asm		//节约寻址方法 加快速度
				MOV		DPH,#03H
				MOV 	DPL,FIFO_Read_Pointer
				MOVX 	A,@DPTR
				MOV  	SBUF,A
				INC  	FIFO_Read_Pointer
#pragma endasm	
			//SBUF1=ICP_Write_Buff[FIFO_Read_Pointer];
			//FIFO_Read_Pointer++;			
		}
		else//发送完了 最后一次进完成中断 设置串口空闲
		{
			UART_TX_BUSY=FALSE;
		}
#pragma asm		
			CLR  	C
			MOV  	A,FIFO_Read_Pointer
			SUBB 	A,FIFO_Write_Pointer
			DEC		A
			CLR  	C
			SUBB 	A,#040H
			JC		T_NAK
			ANL  	UEP1_CTRL,#0F3H //设置USB状态为ASK 
			T_NAK:
#pragma endasm				
//			if(((UINT8)FIFO_Read_Pointer-FIFO_Write_Pointer-1)>64)
//			{
//				UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_ACK;//缓存空 要
//			}
		U1TI =0;	
	}
}
#else
	#Error "no UARTx interface define"
#endif	