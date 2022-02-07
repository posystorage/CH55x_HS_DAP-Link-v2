/********************************** (C) COPYRIGHT *******************************
* File Name          : DEBUG.C
* Author             : WCH
* Version            : V1.0
* Date               : 2017/01/20
* Description        : CH552 DEBUG Interface
                     (1)、串口0输出打印信息，波特率可变;
*******************************************************************************/

#include "CH552.H"
#include "DEBUG.H"

/*******************************************************************************
* Function Name  : CfgFsys( )
* Description    : CH552时钟选择和配置函数,默认使用内部晶振12MHz，如果定义了FREQ_SYS可以
                   根据PLL_CFG和CLOCK_CFG配置得到，公式如下：
                   Fsys = (Fosc * ( PLL_CFG & MASK_PLL_MULT ))/(CLOCK_CFG & MASK_SYS_CK_DIV);
                   具体时钟需要自己配置
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void CfgFsys()
{
#ifdef OSC_EN_XT
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    CLOCK_CFG |= bOSC_EN_XT;   //使能外部晶振
    CLOCK_CFG &= ~bOSC_EN_INT; //关闭内部晶振
#endif
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
#if FREQ_SYS == 24000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x06; // 24MHz
#endif
#if FREQ_SYS == 16000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x05; // 16MHz
#endif
#if FREQ_SYS == 12000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x04; // 12MHz
#endif
#if FREQ_SYS == 6000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x03; // 6MHz
#endif
#if FREQ_SYS == 3000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x02; // 3MHz
#endif
#if FREQ_SYS == 750000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x01; // 750KHz
#endif
#if FREQ_SYS == 187500
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x00; // 187.5KHz
#endif
		GLOBAL_CFG = GLOBAL_CFG | bLDO3V3_OFF;//3.3V LDO OFF 
    SAFE_MOD = 0x00;
}

/*******************************************************************************
* Function Name  : mDelayus(UNIT16 n)
* Description    : us延时函数
* Input          : UNIT16 n
* Output         : None
* Return         : None
*******************************************************************************/
void mDelayuS(UINT16 n) // 以uS为单位延时
{
#ifdef FREQ_SYS
#if FREQ_SYS <= 6000000
    n >>= 2;
#endif
#if FREQ_SYS <= 3000000
    n >>= 2;
#endif
#if FREQ_SYS <= 750000
    n >>= 4;
#endif
#endif
    while (n)
    {
        // total = 12~13 Fsys cycles, 1uS @Fsys=12MHz
        ++SAFE_MOD; // 2 Fsys cycles, for higher Fsys, add operation here
#ifdef FREQ_SYS
#if FREQ_SYS >= 14000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 16000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 18000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 20000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 22000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 24000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 26000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 28000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 30000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 32000000
        ++SAFE_MOD;
#endif
#endif
        --n;
    }
}

/*******************************************************************************
* Function Name  : mDelayms(UNIT16 n)
* Description    : ms延时函数
* Input          : UNIT16 n
* Output         : None
* Return         : None
*******************************************************************************/
void mDelaymS(UINT16 n) // 以mS为单位延时
{
    while (n)
    {
#ifdef DELAY_MS_HW
        while ((TKEY_CTRL & bTKC_IF) == 0)
            ;
        while (TKEY_CTRL & bTKC_IF)
            ;
#else
        mDelayuS(1000);
#endif
        --n;
    }
}

/*******************************************************************************
* Function Name  : CH552WatchDog(UINT8 mode)
* Description    : CH552看门狗模式设置
* Input          : UINT8 mode
                   0  timer
                   1  watchDog
* Output         : None
* Return         : None
*******************************************************************************/
//void CH552WatchDog(UINT8 mode)
//{
//  SAFE_MOD = 0x55;
//  SAFE_MOD = 0xaa;                                                             //进入安全模式
//  if(mode){
//    GLOBAL_CFG |= bWDOG_EN;
//  }
//  else GLOBAL_CFG &= ~bWDOG_EN;
//  SAFE_MOD = 0x00;                                                             //退出安全模式
//  WDOG_COUNT = 0;                                                              //看门狗赋初值
//}

/*******************************************************************************
* Function Name  : CH552WatchDogFeed(UINT8 tim)
* Description    : CH552看门狗喂狗
* Input          : UINT8 tim 看门狗复位时间设置
                   00H(6MHz)=2.8s
                   80H(6MHz)=1.4s
* Output         : None
* Return         : None
*******************************************************************************/
//void CH552WatchDogFeed(UINT8 tim)
//{
//  WDOG_COUNT = tim;                                                             //看门狗赋初值
//}

/*******************************************************************************
 * Function Name  : CH552SoftReset()
 * Description    : CH552软复位
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
// void CH552SoftReset( )
// {
//     SAFE_MOD = 0x55;
//     SAFE_MOD = 0xAA;
//     GLOBAL_CFG   |=bSW_RESET;
// }
