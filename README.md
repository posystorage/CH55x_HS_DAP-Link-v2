# CH55x_HS_DAP-Link-v2
基于CH55x低成本USB单片机实现的汇编级优化高速DAP-Link-v2

介绍见：开源CH551/2实现的汇编优化高速DAP-Link (CMSIS-DAP v2)https://whycan.com/t_7786.html

SWD引脚 P1.6和P1.7需并联
SWC引脚 P1.5
串口引脚 P3.0 P3.1
5V下使用应该接1T45电平转换芯片 SWC-DIR P3.4 SWD-DIR P3.2
LED P1.1
复位1.4（接三极管）
拉高RST重新进ISP
