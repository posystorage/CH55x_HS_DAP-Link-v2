
/********************************** (C) COPYRIGHT *******************************
* File Name          :CompatibilityHID.C
* Author             : WCH
* Version            : V1.2
* Date               : 2018/02/28
* Description        : CH554模拟HID兼容设备，支持中断上下传，支持控制端点上下传，支持设置全速，低速
*******************************************************************************/

#include "CH552.H"
#include "Debug.H"
#include "DAP.h"
#include "Uart.H"

#define Fullspeed 1
#define WINUSB 1
#define THIS_ENDP0_SIZE 64

UINT8X Ep0Buffer[THIS_ENDP0_SIZE] _at_ 0x0000;  //端点0 OUT&IN缓冲区，必须是偶地址
UINT8X Ep1BufferO[THIS_ENDP0_SIZE] _at_ 0x0040; //端点1 OUT双缓冲区,必须是偶地址 Not Change!!!!!!
UINT8X Ep1BufferI[THIS_ENDP0_SIZE] _at_ 0x0080; //端点1 IN双缓冲区,必须是偶地址 Not Change!!!!!!

//100,140,180,1C0
UINT8X Ep2BufferO[4 * THIS_ENDP0_SIZE] _at_ 0x0100; //端点2 OUT双缓冲区,必须是偶地址
//200,240,280,2C0
UINT8X Ep3BufferI[4 * THIS_ENDP0_SIZE] _at_ 0x0200; //端点3 IN双缓冲区,必须是偶地址


//UINT8I Endp1Busy;
//UINT8I USBByteCount = 0;       //代表USB端点接收到的数据
//UINT8I USBBufOutPoint = 0;     //取数据指针
extern BOOL UART_TX_BUSY;
extern BOOL EP1_TX_BUSY;
//UINT8I UartByteCount = 0;      //当前缓冲区剩余待取字节数
//UINT8I Uart_Input_Point = 0;   //循环缓冲区写入指针，总线复位需要初始化为0
//UINT8I Uart_Output_Point = 0;  //循环缓冲区取出指针，总线复位需要初始化为0
BOOL DAP_LED_BUSY;

UINT8I Ep2Oi, Ep2Oo;            //OUT 索引
UINT8I Ep3Ii, Ep3Io;            //IN 索引
UINT8I Ep3Is[DAP_PACKET_COUNT]; //发送包长

PUINT8 pDescr; //USB配置标志
UINT8I Endp3Busy = 0;
UINT8I SetupReq, SetupLen, Ready, Count, UsbConfig;
#define UsbSetupBuf ((PUSB_SETUP_REQ)Ep0Buffer)

#if (WINUSB == 1)
UINT8C DevDesc[] =
{
    0x12, 0x01, 0x10, 0x02, 0xEF, 0x02, 0x01, THIS_ENDP0_SIZE,
    0x28, 0x0D, 0x04, 0x02, 0x00, 0x01, 0x01, 0x02,
    0x03, 0x01
};
UINT8C CfgDesc[] =
{
    0x09, 0x02, 0x62, 0x00, 0x03, 0x01, 0x00, 0x80, 0xfa, //配置描述符
    //DAP
    0x09, 0x04, 0x00, 0x00, 0x02, 0xff, 0x00, 0x00, 0x04, //接口描述符

    0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00, //端点描述符
    0x07, 0x05, 0x83, 0x02, 0x40, 0x00, 0x00,
    //CDC
    0x08, 0x0b, 0x02, 0x02, 0x02, 0x02, 0x01, 0x05,
    0x09, 0x04, 0x02, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    0x07, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00,
    0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00,
    0x09, 0x04, 0x03, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    0x05, 0x24, 0x00, 0x10, 0x01,
    0x05, 0x24, 0x01, 0x00, 0x01,
    0x04, 0x24, 0x02, 0x02,
    0x05, 0x24, 0x06, 0x03, 0x02,
    0x07, 0x05, 0x84, 0x03, 0x08, 0x00, 0xFF,
};
#else
#Err no USB interface define
#endif

UINT16I USB_STATUS = 0;
//cdc参数
UINT8I LineCoding[7] = {0x00, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x08}; //初始化波特率为57600，1停止位，无校验，8数据位。

/*字符串描述符 略*/
// 语言描述符
UINT8C MyLangDescr[] = {0x04, 0x03, 0x09, 0x04};
// 厂家信息:ARM
UINT8C MyManuInfo[] = {0x08, 0x03, 'A', 0x00, 'R', 0x00, 'M', 0x00};
// 产品信息: DAP-Link-II
UINT8C MyProdInfo[] =
{
    36,
    0x03,
    'D', 0, 'A', 0, 'P', 0, 'L', 0, 'i', 0, 'n', 0, 'k', 0, ' ', 0,
    'C', 0, 'M', 0, 'S', 0, 'I', 0, 'S', 0, '-', 0, 'D', 0, 'A', 0,
    'P', 0
};
// 序列号: 3V3-IO-12345
UINT8C MySerNumber[] =
{
    0x1A,
    0x03,
    '3', 0, 'V', 0, '3', 0, '-', 0, 'I', 0, 'O', 0, '-', 0, '1', 0,
    '2', 0, '3', 0, '4', 0, '5', 0
};
// 接口: CMSIS-DAP
UINT8C MyInterface[] =
{
    26,
    0x03,
    'C', 0, 'M', 0, 'S', 0, 'I', 0, 'S', 0, '-', 0, 'D', 0, 'A', 0,
    'P', 0, ' ', 0, 'v', 0, '2', 0
};
//CDC
UINT8C CDC_String[] =
{
    30,
    0x03,
    'D', 0, 'A', 0, 'P', 0, 'L', 0, 'i', 0, 'n', 0, 'k', 0, '-', 0, 'C', 0, 'D', 0, 'C', 0, 'E', 0, 'x', 0, 't', 0
};

UINT8C USB_BOSDescriptor[] =
{
    0x05,                                      /* bLength */
    0x0F,                                      /* bDescriptorType */
    0x28, 0x00,                                /* wTotalLength */
    0x02,                                      /* bNumDeviceCaps */
    0x07, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x1C,                                      /* bLength */
    0x10,                                      /* bDescriptorType */
    0x05,                                      /* bDevCapabilityType */
    0x00,                                      /* bReserved */
    0xDF, 0x60, 0xDD, 0xD8,                    /* PlatformCapabilityUUID */
    0x89, 0x45, 0xC7, 0x4C,
    0x9C, 0xD2, 0x65, 0x9D,
    0x9E, 0x64, 0x8A, 0x9F,
    0x00, 0x00, 0x03, 0x06,    /* >= Win 8.1 *//* dwWindowsVersion*/
    0xAA, 0x00,                                /* wDescriptorSetTotalLength */
    0x20,                                      /* bVendorCode */
    0x00                                       /* bAltEnumCode */
};

UINT8C WINUSB_Descriptor[] =
{
    0x0A, 0x00,                                 /* wLength */
    0x00, 0x00,                                 /* wDescriptorType */
    0x00, 0x00, 0x03, 0x06,                     /* dwWindowsVersion*/
    0xAA, 0x00,                                 /* wDescriptorSetTotalLength */
    /* ... */
    0x08, 0x00,
    0x02, 0x00,
    0x00, 0x00,
    0xA0, 0x00,
    /* ... */
    0x14, 0x00,                                 /* wLength */
    0x03, 0x00,                                 /* wDescriptorType */
    'W', 'I', 'N', 'U', 'S', 'B', 0, 0,         /* CompatibleId*/
    0, 0, 0, 0, 0, 0, 0, 0,                     /* SubCompatibleId*/
    0x84, 0x00,                                 /* wLength */
    0x04, 0x00,                                 /* wDescriptorType */
    0x07, 0x00,                                 /* wPropertyDataType */
    0x2A, 0x00,                                 /* wPropertyNameLength */
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0,
    'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0,
    'G', 0, 'U', 0, 'I', 0, 'D', 0, 's', 0, 0, 0,
    0x50, 0x00,                                 /* wPropertyDataLength */
    '{', 0,
    'C', 0, 'D', 0, 'B', 0, '3', 0, 'B', 0, '5', 0, 'A', 0, 'D', 0, '-', 0,
    '2', 0, '9', 0, '3', 0, 'B', 0, '-', 0,
    '4', 0, '6', 0, '6', 0, '3', 0, '-', 0,
    'A', 0, 'A', 0, '3', 0, '6', 0, '-',
    0, '1', 0, 'A', 0, 'A', 0, 'E', 0, '4', 0, '6', 0, '4', 0, '6', 0, '3', 0, '7', 0, '7', 0, '6', 0,
    '}', 0, 0, 0, 0, 0,
};


//void Config_Uart1(UINT8 *cfg_uart);
void Config_Uart0(UINT8 *cfg_uart);

/*******************************************************************************
* Function Name  : USBDeviceInit()
* Description    : USB设备模式配置,设备模式启动，收发端点配置，中断开启
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceInit()
{
    IE_USB = 0;
    USB_CTRL = 0x00;        // 先设定USB设备模式
    UDEV_CTRL = bUD_PD_DIS; // 禁止DP/DM下拉电阻
#ifndef Fullspeed
    UDEV_CTRL |= bUD_LOW_SPEED; //选择低速1.5M模式
    USB_CTRL |= bUC_LOW_SPEED;
#else
    UDEV_CTRL &= ~bUD_LOW_SPEED; //选择全速12M模式，默认方式
    USB_CTRL &= ~bUC_LOW_SPEED;
#endif

    UEP0_DMA = Ep0Buffer;                                      //端点0数据传输地址
    UEP4_1_MOD &= ~(bUEP4_RX_EN | bUEP4_TX_EN);                //端点0单64字节收发缓冲区
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;                 //OUT事务返回ACK，IN事务返回NAK

    UEP1_DMA = Ep1BufferO;                                     //端点1数据传输地址
    UEP4_1_MOD |= bUEP1_TX_EN | bUEP1_RX_EN;                   //端点1发送接收使能
    UEP4_1_MOD &= ~bUEP1_BUF_MOD;                              //端点1收发各64字节缓冲区
    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK; //端点1自动翻转同步标志位，IN事务返回NAK，OUT返回ACK

    UEP2_DMA = Ep2BufferO;                                     //端点2数据传输地址
    UEP3_DMA = Ep3BufferI;                                     //端点2数据传输地址
    UEP2_3_MOD |= (bUEP3_TX_EN | bUEP2_RX_EN);                   //端点2发送接收使能
    UEP2_3_MOD &= ~(bUEP2_BUF_MOD | bUEP3_BUF_MOD);            //端点2收发各64字节缓冲区
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK; //端点2自动翻转同步标志位，IN事务返回NAK，OUT返回ACK
    UEP3_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_NAK;//端点3自动翻转同步标志位，IN事务返回NAK，OUT返回NACK

    USB_DEV_AD = 0x00;
    USB_CTRL |= bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN; // 启动USB设备及DMA，在中断期间中断标志未清除前自动返回NAK
    UDEV_CTRL |= bUD_PORT_EN;                              // 允许USB端口
    USB_INT_FG = 0xFF;                                     // 清中断标志
    USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
    IE_USB = 1;
}
/*******************************************************************************
* Function Name  : DeviceInterrupt()
* Description    : CH559USB中断处理函数
*******************************************************************************/
void DeviceInterrupt(void) interrupt INT_NO_USB using 1 //USB中断服务程序,使用寄存器组1
{
    UINT8 len;
    if (UIF_TRANSFER) //USB传输完成标志
    {
        switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
        {
        case UIS_TOKEN_OUT | 2: //endpoint 2# 端点批量下传 DAP-CMD
            if (U_TOG_OK)         // 不同步的数据包将丢弃
            {
                Ep2Oi += 64;
                UEP2_DMA_L = Ep2Oi;
            }
            break;

        case UIS_TOKEN_IN | 3: //endpoint 3# 端点批量上传 DAP_ASK
            Endp3Busy = 0;
            UEP3_T_LEN = 0;      //预使用发送长度一定要清空
            UEP3_CTRL = UEP3_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK; //默认应答NAK
            break;

        case UIS_TOKEN_IN | 1: //endpoint 1# 端点批量上传 CDC
            UEP1_T_LEN = 0;      //预使用发送长度一定要清空
            //Endp1Busy = 0;
						EP1_TX_BUSY = FALSE;
            UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK; //默认应答NAK
            break;
        case UIS_TOKEN_OUT | 1: //endpoint 1# 端点批量下传 CDC
            if (U_TOG_OK)         // 不同步的数据包将丢弃
            {
                //USBByteCount = USB_RX_LEN;
                //USBBufOutPoint = 0;                                             //取数据指针复位
                //UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_NAK;       //收到一包数据就NAK，主函数处理完，由主函数修改响应方式
							UART_Get_USB_Data(USB_RX_LEN);
            }
            break;

        case UIS_TOKEN_SETUP | 0: //SETUP事务
            len = USB_RX_LEN;
            if (len == (sizeof(USB_SETUP_REQ)))
            {
                SetupLen = UsbSetupBuf->wLengthL;
                if (UsbSetupBuf->wLengthH)
                    SetupLen = 0xFF; // 限制总长度
                len = 0;           // 默认为成功并且上传0长度
                SetupReq = UsbSetupBuf->bRequest;
                switch (UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK)
                {
                case USB_REQ_TYP_STANDARD:
                    switch (SetupReq) //请求码
                    {
                    case USB_GET_DESCRIPTOR:
                        switch (UsbSetupBuf->wValueH)
                        {
                        case 1:             //设备描述符
                            pDescr = DevDesc; //把设备描述符送到要发送的缓冲区
                            len = sizeof(DevDesc);
                            break;
                        case 2:             //配置描述符
                            pDescr = CfgDesc; //把设备描述符送到要发送的缓冲区
                            len = sizeof(CfgDesc);
                            break;
                        case 3: // 字符串描述符
                            switch (UsbSetupBuf->wValueL)
                            {
                            case 0:
                                pDescr = (PUINT8)(&MyLangDescr[0]);
                                len = sizeof(MyLangDescr);
                                break;
                            case 1:
                                pDescr = (PUINT8)(&MyManuInfo[0]);
                                len = sizeof(MyManuInfo);
                                break;
                            case 2:
                                pDescr = (PUINT8)(&MyProdInfo[0]);
                                len = sizeof(MyProdInfo);
                                break;
                            case 3:
                                pDescr = (PUINT8)(&MySerNumber[0]);
                                len = sizeof(MySerNumber);
                                break;
                            case 4:
                                pDescr = (PUINT8)(&MyInterface[0]);
                                len = sizeof(MyInterface);
                                break;
                            case 5:
                                pDescr = (PUINT8)(&CDC_String[0]);
                                len = sizeof(CDC_String);
                                break;
                            default:
                                len = 0xFF; // 不支持的字符串描述符
                                break;
                            }
                            break;
                        case 15:
                            pDescr = (PUINT8)(&USB_BOSDescriptor[0]);
                            len = sizeof(USB_BOSDescriptor);
                            break;
                        default:
                            len = 0xff; //不支持的命令或者出错
                            break;
                        }
                        break;
                    case USB_SET_ADDRESS:
                        SetupLen = UsbSetupBuf->wValueL; //暂存USB设备地址
                        break;
                    case USB_GET_CONFIGURATION:
                        Ep0Buffer[0] = UsbConfig;
                        if (SetupLen >= 1)
                        {
                            len = 1;
                        }
                        break;
                    case USB_SET_CONFIGURATION:
                        UsbConfig = UsbSetupBuf->wValueL;
                        if (UsbConfig)
                        {
                            Ready = 1; //set config命令一般代表usb枚举完成的标志
                        }
                        break;
                    case 0x0A:
                        break;
                    case USB_CLEAR_FEATURE:                                                       //Clear Feature
                        if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) // 端点
                        {
                            switch (UsbSetupBuf->wIndexL)
                            {
                            case 0x82:
                                UEP2_CTRL = UEP2_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                                break;
                            case 0x81:
                                UEP1_CTRL = UEP1_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                                break;
                            case 0x02:
                                UEP2_CTRL = UEP2_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
                                break;
                            default:
                                len = 0xFF; // 不支持的端点
                                break;
                            }
                        }
                        else
                        {
                            len = 0xFF; // 不是端点不支持
                        }
                        break;
                    case USB_SET_FEATURE:                             /* Set Feature */
                        if ((UsbSetupBuf->bRequestType & 0x1F) == 0x00) /* 设置设备 */
                        {
                            if ((((UINT16)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01)
                            {
                                if (CfgDesc[7] & 0x20)
                                {
                                    /* 设置唤醒使能标志 */
                                }
                                else
                                {
                                    len = 0xFF; /* 操作失败 */
                                }
                            }
                            else
                            {
                                len = 0xFF; /* 操作失败 */
                            }
                        }
                        else if ((UsbSetupBuf->bRequestType & 0x1F) == 0x02) /* 设置端点 */
                        {
                            if ((((UINT16)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x00)
                            {
                                switch (((UINT16)UsbSetupBuf->wIndexH << 8) | UsbSetupBuf->wIndexL)
                                {
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL; /* 设置端点2 IN STALL */
                                    break;
                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL; /* 设置端点2 OUT Stall */
                                    break;
                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL; /* 设置端点1 IN STALL */
                                    break;
                                default:
                                    len = 0xFF; /* 操作失败 */
                                    break;
                                }
                            }
                            else
                            {
                                len = 0xFF; /* 操作失败 */
                            }
                        }
                        else
                        {
                            len = 0xFF; /* 操作失败 */
                        }
                        break;
                    case USB_GET_STATUS:
                        pDescr = (PUINT8)&USB_STATUS;
                        if (SetupLen >= 2)
                        {
                            len = 2;
                        }
                        else
                        {
                            len = SetupLen;
                        }
                        break;
                    default:
                        len = 0xff; //操作失败
                        break;
                    }

                    break;
                case USB_REQ_TYP_CLASS: /*HID类命令*/
                    if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_INTERF)
                    {
                        switch (SetupReq)
                        {
                        case 0x20://Configure
                            break;
                        case 0x21://currently configured
                            pDescr = LineCoding;
                            len = sizeof(LineCoding);
                            break;
                        case 0x22://generates RS-232/V.24 style control signals
                            break;
                        default:
                            len = 0xFF; /*命令不支持*/
                            break;
                        }
                    }
                    break;
                case USB_REQ_TYP_VENDOR:
                    if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)
                    {
                        switch (SetupReq)
                        {
                        case 0x20:                         //GetReport
                            if (UsbSetupBuf->wIndexL == 0x07)
                            {
                                pDescr = WINUSB_Descriptor; //把设备描述符送到要发送的缓冲区
                                len = sizeof(WINUSB_Descriptor);
                            }
                            break;
                        default:
                            len = 0xFF; /*命令不支持*/
                            break;
                        }
                    }
                    break;
                default:
                    len = 0xFF;
                    break;
                }
                if (len != 0 && len != 0xFF)
                {
                    if (SetupLen > len)
                    {
                        SetupLen = len; //限制总长度
                    }
                    len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen; //本次传输长度
                    memcpy(Ep0Buffer, pDescr, len);                                 //加载上传数据
                    SetupLen -= len;
                    pDescr += len;
                }
            }
            else
            {
                len = 0xff; //包长度错误
            }
            if (len == 0xff)
            {
                SetupReq = 0xFF;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL; //STALL
            }
            else if (len <= THIS_ENDP0_SIZE) //上传数据或者状态阶段返回0长度包
            {
                UEP0_T_LEN = len;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; //默认数据包是DATA1，返回应答ACK
            }
            else
            {
                UEP0_T_LEN = 0;                                                      //虽然尚未到状态阶段，但是提前预置上传0长度数据包以防主机提前进入状态阶段
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; //默认数据包是DATA1,返回应答ACK
            }
            break;
        case UIS_TOKEN_IN | 0: //endpoint0 IN
            switch (SetupReq)
            {
            case USB_GET_DESCRIPTOR:
            case 0x20:
                len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen; //本次传输长度
                memcpy(Ep0Buffer, pDescr, len);                                 //加载上传数据
                SetupLen -= len;
                pDescr += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG; //同步标志位翻转
                break;
            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            default:
                UEP0_T_LEN = 0; //状态阶段完成中断或者是强制上传0长度数据包结束控制传输
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;
        case UIS_TOKEN_OUT | 0: // endpoint0 OUT
            len = USB_RX_LEN;
            if (SetupReq == 0x20) //设置串口属性
            {
                if (U_TOG_OK)
                {
                    memcpy(LineCoding, UsbSetupBuf, USB_RX_LEN);
                    //Config_Uart1(LineCoding);
										Config_Uart0(LineCoding);
                    UEP0_T_LEN = 0;
                    UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_ACK;  // 准备上传0包
                }
            }
            else if (SetupReq == 0x09)
            {
                if (Ep0Buffer[0])
                {
                }
                else if (Ep0Buffer[0] == 0)
                {
                }
            }
            UEP0_CTRL ^= bUEP_R_TOG; //同步标志位翻转
            break;
        default:
            break;
        }
        UIF_TRANSFER = 0; //写0清空中断
    }
    if (UIF_BUS_RST) //设备模式USB总线复位中断
    {
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        Endp3Busy = 0;
        UIF_BUS_RST = 0; //清中断标志
    }
    if (UIF_SUSPEND) //USB总线挂起/唤醒完成
    {
        UIF_SUSPEND = 0;
        if (USB_MIS_ST & bUMS_SUSPEND) //挂起
        {
        }
    }
    else
    {
        //意外的中断,不可能发生的情况
        USB_INT_FG = 0xFF; //清中断标志
    }
}


typedef void( *goISP)( void );
goISP ISP_ADDR=0x3800;	
UINT16 LED_Timer;

void main(void)
{
    UINT8 Uart_Timeout = 0;

    CfgFsys();   //CH559时钟选择配置
    mDelaymS(5); //修改主频等待内部晶振稳定,必加
   
    USBDeviceInit(); //USB设备模式初始化
		UART_Setup();
    P1_MOD_OC = P1_MOD_OC & ~(1 << 4);
    P1_DIR_PU = P1_DIR_PU | (1 << 4);
    P1_MOD_OC = P1_MOD_OC & ~(1 << 5);
    P1_DIR_PU = P1_DIR_PU | (1 << 5);
	
    EA = 1;          //允许单片机中断
    UEP1_T_LEN = 0;  //预使用发送长度一定要清空
    UEP2_T_LEN = 0;  //预使用发送长度一定要清空
    Ready = 0;

    Ep2Oi = 0;
    Ep2Oo = 0;
    Ep3Ii = 0;
    Ep3Io = 0;
    Endp3Busy = 0;
		DAP_LED_BUSY = 0;
		LED_Timer = 0;

    while (!UsbConfig) {;};

    while (1)
    {
        DAP_Thread();

        if (Endp3Busy != 1 && Ep3Ii != Ep3Io)
        {
            Endp3Busy = 1;
            UEP3_T_LEN = Ep3Is[0];//Ep3Io>>6];
            UEP3_DMA_L = Ep3Io;
            
            UEP3_CTRL = UEP3_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; //有数据时上传数据并应答ACK
            Ep3Io += 64;
        }
				if(DAP_LED_BUSY)
				{
					LED = 0;
					LED_Timer = 0;
				}
				else
				{
					LED_Timer++;
					if(((UINT8*)&LED_Timer)[0]==0x10)
					{
						LED = 1;
					}							
					if(((UINT8*)&LED_Timer)[0]==0xC0)
					{
						LED_Timer = 0;
						LED = 0;
					}			
				}

				//Uart_Main_Service();
//        if (USBByteCount)
//        {
//            //CH552UART1SendByte(Ep1BufferO[USBBufOutPoint++]);
//						SBUF = Ep1BufferO[USBBufOutPoint++];	
//						while(TI == 0);
//						TI = 0;											
//            USBByteCount--;
//            if (USBByteCount == 0)
//                UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_ACK;
//        }
//        if (UartByteCount)
//        {
//            Uart_Timeout++;
//            if (UartByteCount > 39 || Uart_Timeout > 100)
//            {
//                if (!Endp1Busy)
//                {
//                    Uart_Timeout = 0;
//                    UEP1_T_LEN = UartByteCount;                                                    //预使用发送长度一定要清空
//                    UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;            //应答ACK
//                    Endp1Busy = 1;
//                    UartByteCount = 0;
//                    Uart_Input_Point = 0;
//                }
//            }
//        }
				if(CLOCK_CFG&bRST)//isp
				{
					USB_CTRL=0;
					UDEV_CTRL=0x80;
					mDelaymS(10);			
					(ISP_ADDR)();
				}				
						
    }
}

//void Config_Uart1(UINT8 *cfg_uart)
//{
//    UINT32 uart1_buad = 0;
//    *((UINT8 *)&uart1_buad) = cfg_uart[3];
//    *((UINT8 *)&uart1_buad + 1) = cfg_uart[2];
//    *((UINT8 *)&uart1_buad + 2) = cfg_uart[1];
//    *((UINT8 *)&uart1_buad + 3) = cfg_uart[0];
//    IE_UART1 = 0;
//    SBAUD1 = 0 - FREQ_SYS / 16 / uart1_buad;
//    IE_UART1 = 1;
//}

//void Uart1_ISR(void) interrupt INT_NO_UART1
//{
//    if (U1RI)  //收到数据
//    {
//        Ep1BufferI[Uart_Input_Point++] = SBUF1;
//        UartByteCount++;                    //当前缓冲区剩余待取字节数
//        if (Uart_Input_Point >= THIS_ENDP0_SIZE)
//            Uart_Input_Point = 0;           //写入指针
//        U1RI = 0;
//    }
//}
void Config_Uart0(UINT8 *cfg_uart)
{
    UINT32 uart0_buad = 0;
    *((UINT8 *)&uart0_buad) = cfg_uart[3];
    *((UINT8 *)&uart0_buad + 1) = cfg_uart[2];
    *((UINT8 *)&uart0_buad + 2) = cfg_uart[1];
    *((UINT8 *)&uart0_buad + 3) = cfg_uart[0];
    ES = 0;
    TH1 = 0 - ((FREQ_SYS+8*uart0_buad) / 16 / uart0_buad);
    ES = 1;
}

//void Uart0_ISR(void) interrupt INT_NO_UART0
//{
//    if (RI)  //收到数据
//    {
//        Ep1BufferI[Uart_Input_Point++] = SBUF;
//        UartByteCount++;                    //当前缓冲区剩余待取字节数
//        if (Uart_Input_Point >= THIS_ENDP0_SIZE)
//            Uart_Input_Point = 0;           //写入指针
//        RI = 0;
//    }
//}