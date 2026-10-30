# OLED

OLED 在课程里主要承担“把 MCU 内部变量显示出来”的作用，后面计数、ADC、输入捕获、RTC、Flash 实验都会复用它。

| OLED 简介 | OLED 硬件电路 |
|---|---|
| ![[assets/ppt/s038_OLED简介.jpg]] | ![[assets/ppt/s039_硬件电路.jpg]] |

| 课程 OLED 驱动函数 |
|---|
| ![[assets/ppt/s040_OLED驱动函数.jpg]] |

## 常用函数

课程封装的 API 包括：

`OLED_Init()`　`OLED_Clear()`　`OLED_ShowChar()`　`OLED_ShowString()`  
`OLED_ShowNum()`　`OLED_ShowSignedNum()`　`OLED_ShowHexNum()`　`OLED_ShowBinNum()`

调用层不需要反复关心底层时序，直接把数据显示出来即可。

> [!code]- 视频源码：OLED 实验 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();		//OLED初始化
> 	
> 	/*OLED显示*/
> 	OLED_ShowChar(1, 1, 'A');				//1行1列显示字符A
> 	
> 	OLED_ShowString(1, 3, "HelloWorld!");	//1行3列显示字符串HelloWorld!
> 	
> 	OLED_ShowNum(2, 1, 12345, 5);			//2行1列显示十进制数字12345，长度为5
> 	
> 	OLED_ShowSignedNum(2, 7, -66, 2);		//2行7列显示有符号十进制数字-66，长度为2
> 	
> 	OLED_ShowHexNum(3, 1, 0xAA55, 4);		//3行1列显示十六进制数字0xA5A5，长度为4
> 	
> 	OLED_ShowBinNum(4, 1, 0xAA55, 16);		//4行1列显示二进制数字0xA5A5，长度为16
> 											//C语言无法直接写出二进制数字，故需要用十六进制表示
> 	
> 	while (1)
> 	{
> 		
> 	}
> }
> ```
>
> 完整文件：[[源码/4-1 OLED显示屏/User/main.c|User/main.c]]


完整 OLED 驱动也已经原样放进笔记包：

[[源码/4-1 OLED显示屏/Hardware/OLED.c|OLED.c]]  
[[源码/4-1 OLED显示屏/Hardware/OLED.h|OLED.h]]

学习后面外设时，把 OLED 当成 **可视化调试窗口**，不要每一章重新研究显示底层。
