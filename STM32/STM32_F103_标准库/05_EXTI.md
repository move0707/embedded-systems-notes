# EXTI

## 中断

中断的核心不是“另一个函数”，而是 CPU 正在执行主程序时，被某个事件临时打断：

**主程序 → 中断点 → IRQHandler → 返回中断点继续执行**

![[assets/ppt/s041_中断系统.jpg]]

*课程中断系统总览。*

## NVIC

NVIC 管理 Cortex-M3 的中断允许、优先级和分发。

| NVIC 基本结构 | NVIC 优先级分组 |
|---|---|
| ![[assets/ppt/s044_NVIC基本结构.jpg]] | ![[assets/ppt/s045_NVIC优先级分组.jpg]] |

STM32F1 标准库里优先级分成：

- **抢占优先级**：更高者可以嵌套打断更低者。
- **响应优先级**：抢占级相同同时到来时，响应级高者先执行。
- 两者都相同，再按中断号排队。

注意：数字越小，优先级越高。

## EXTI

EXTI 负责监测 GPIO 的边沿/电平变化，并产生中断或事件。

| EXTI 基本结构 | AFIO 复用 IO |
|---|---|
| ![[assets/ppt/s047_EXTI基本结构.jpg]] | ![[assets/ppt/s048_AFIO复用IO口.jpg]] |

| EXTI 框图 |
|---|
| ![[assets/ppt/s049_EXTI框图.jpg]] |

数据链：

**GPIO → AFIO 选择端口 → EXTI Line → NVIC → CPU**

同一个编号的 GPIO 共用一条 EXTI Line，例如 PA0/PB0/PC0 都对应 EXTI0，因此必须通过 AFIO 指定当前由哪个端口接入。

## 红外计次

课程实验把对射式红外传感器接到 EXTI。物体遮挡使电平产生指定边沿，中断里执行计数。

流程：

**GPIO 输入 → AFIO 映射 → EXTI 下降沿 → NVIC → `EXTI15_10_IRQHandler()` → Count++ → 清 Pending**

> [!code]- 视频源码：CountSensor.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> uint16_t CountSensor_Count;				//全局变量，用于计数
> 
> /**
>   * 函    数：计数传感器初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void CountSensor_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);		//开启AFIO的时钟，外部中断必须开启AFIO的时钟
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOB, &GPIO_InitStructure);						//将PB14引脚初始化为上拉输入
> 	
> 	/*AFIO选择中断引脚*/
> 	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource14);//将外部中断的14号线映射到GPIOB，即选择PB14为外部中断引脚
> 	
> 	/*EXTI初始化*/
> 	EXTI_InitTypeDef EXTI_InitStructure;						//定义结构体变量
> 	EXTI_InitStructure.EXTI_Line = EXTI_Line14;					//选择配置外部中断的14号线
> 	EXTI_InitStructure.EXTI_LineCmd = ENABLE;					//指定外部中断线使能
> 	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;			//指定外部中断线为中断模式
> 	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;		//指定外部中断线为下降沿触发
> 	EXTI_Init(&EXTI_InitStructure);								//将结构体变量交给EXTI_Init，配置EXTI外设
> 	
> 	/*NVIC中断分组*/
> 	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);				//配置NVIC为分组2
> 																//即抢占优先级范围：0~3，响应优先级范围：0~3
> 																//此分组配置在整个工程中仅需调用一次
> 																//若有多个中断，可以把此代码放在main函数内，while循环之前
> 																//若调用多次配置分组的代码，则后执行的配置会覆盖先执行的配置
> 	
> 	/*NVIC配置*/
> 	NVIC_InitTypeDef NVIC_InitStructure;						//定义结构体变量
> 	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;		//选择配置NVIC的EXTI15_10线
> 	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
> 	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//指定NVIC线路的抢占优先级为1
> 	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//指定NVIC线路的响应优先级为1
> 	NVIC_Init(&NVIC_InitStructure);								//将结构体变量交给NVIC_Init，配置NVIC外设
> }
> 
> /**
>   * 函    数：获取计数传感器的计数值
>   * 参    数：无
>   * 返 回 值：计数值，范围：0~65535
>   */
> uint16_t CountSensor_Get(void)
> {
> 	return CountSensor_Count;
> }
> 
> /**
>   * 函    数：EXTI15_10外部中断函数
>   * 参    数：无
>   * 返 回 值：无
>   * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
>   *           函数名为预留的指定名称，可以从启动文件复制
>   *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
>   */
> void EXTI15_10_IRQHandler(void)
> {
> 	if (EXTI_GetITStatus(EXTI_Line14) == SET)		//判断是否是外部中断14号线触发的中断
> 	{
> 		/*如果出现数据乱跳的现象，可再次判断引脚电平，以避免抖动*/
> 		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 0)
> 		{
> 			CountSensor_Count ++;					//计数值自增一次
> 		}
> 		EXTI_ClearITPendingBit(EXTI_Line14);		//清除外部中断14号线的中断标志位
> 													//中断标志位必须清除
> 													//否则中断将连续不断地触发，导致主程序卡死
> 	}
> }
> ```
>
> 完整文件：[[源码/5-1 对射式红外传感器计次/Hardware/CountSensor.c|Hardware/CountSensor.c]]

> [!code]- 视频源码：红外计次 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "CountSensor.h"
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();			//OLED初始化
> 	CountSensor_Init();		//计数传感器初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "Count:");	//1行1列显示字符串Count:
> 	
> 	while (1)
> 	{
> 		OLED_ShowNum(1, 7, CountSensor_Get(), 5);		//OLED不断刷新显示CountSensor_Get的返回值
> 	}
> }
> ```
>
> 完整文件：[[源码/5-1 对射式红外传感器计次/User/main.c|User/main.c]]


> [!important]
> EXTI 的 Pending 位处理后要清掉，否则中断请求仍然存在。不同外设的标志清除规则不同，不能把“所有中断最后都调 Clear”当通用模板。

## 旋转编码器

| 旋转编码器原理 | 旋转编码器硬件电路 |
|---|---|
| ![[assets/ppt/s050_旋转编码器简介.jpg]] | ![[assets/ppt/s051_硬件电路.jpg]] |

课程这一节还是用 EXTI 做计次：A 相出现边沿时读取 B 相电平，由 A/B 相位关系判断方向。

> [!code]- 视频源码：Encoder.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> int16_t Encoder_Count;					//全局变量，用于计数旋转编码器的增量值
> 
> /**
>   * 函    数：旋转编码器初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void Encoder_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);		//开启AFIO的时钟，外部中断必须开启AFIO的时钟
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOB, &GPIO_InitStructure);						//将PB0和PB1引脚初始化为上拉输入
> 	
> 	/*AFIO选择中断引脚*/
> 	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);//将外部中断的0号线映射到GPIOB，即选择PB0为外部中断引脚
> 	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);//将外部中断的1号线映射到GPIOB，即选择PB1为外部中断引脚
> 	
> 	/*EXTI初始化*/
> 	EXTI_InitTypeDef EXTI_InitStructure;						//定义结构体变量
> 	EXTI_InitStructure.EXTI_Line = EXTI_Line0 | EXTI_Line1;		//选择配置外部中断的0号线和1号线
> 	EXTI_InitStructure.EXTI_LineCmd = ENABLE;					//指定外部中断线使能
> 	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;			//指定外部中断线为中断模式
> 	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;		//指定外部中断线为下降沿触发
> 	EXTI_Init(&EXTI_InitStructure);								//将结构体变量交给EXTI_Init，配置EXTI外设
> 	
> 	/*NVIC中断分组*/
> 	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);				//配置NVIC为分组2
> 																//即抢占优先级范围：0~3，响应优先级范围：0~3
> 																//此分组配置在整个工程中仅需调用一次
> 																//若有多个中断，可以把此代码放在main函数内，while循环之前
> 																//若调用多次配置分组的代码，则后执行的配置会覆盖先执行的配置
> 	
> 	/*NVIC配置*/
> 	NVIC_InitTypeDef NVIC_InitStructure;						//定义结构体变量
> 	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;			//选择配置NVIC的EXTI0线
> 	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
> 	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//指定NVIC线路的抢占优先级为1
> 	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//指定NVIC线路的响应优先级为1
> 	NVIC_Init(&NVIC_InitStructure);								//将结构体变量交给NVIC_Init，配置NVIC外设
> 
> 	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;			//选择配置NVIC的EXTI1线
> 	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
> 	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//指定NVIC线路的抢占优先级为1
> 	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;			//指定NVIC线路的响应优先级为2
> 	NVIC_Init(&NVIC_InitStructure);								//将结构体变量交给NVIC_Init，配置NVIC外设
> }
> 
> /**
>   * 函    数：旋转编码器获取增量值
>   * 参    数：无
>   * 返 回 值：自上此调用此函数后，旋转编码器的增量值
>   */
> int16_t Encoder_Get(void)
> {
> 	/*使用Temp变量作为中继，目的是返回Encoder_Count后将其清零*/
> 	/*在这里，也可以直接返回Encoder_Count
> 	  但这样就不是获取增量值的操作方法了
> 	  也可以实现功能，只是思路不一样*/
> 	int16_t Temp;
> 	Temp = Encoder_Count;
> 	Encoder_Count = 0;
> 	return Temp;
> }
> 
> /**
>   * 函    数：EXTI0外部中断函数
>   * 参    数：无
>   * 返 回 值：无
>   * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
>   *           函数名为预留的指定名称，可以从启动文件复制
>   *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
>   */
> void EXTI0_IRQHandler(void)
> {
> 	if (EXTI_GetITStatus(EXTI_Line0) == SET)		//判断是否是外部中断0号线触发的中断
> 	{
> 		/*如果出现数据乱跳的现象，可再次判断引脚电平，以避免抖动*/
> 		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0)
> 		{
> 			if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)		//PB0的下降沿触发中断，此时检测另一相PB1的电平，目的是判断旋转方向
> 			{
> 				Encoder_Count --;					//此方向定义为反转，计数变量自减
> 			}
> 		}
> 		EXTI_ClearITPendingBit(EXTI_Line0);			//清除外部中断0号线的中断标志位
> 													//中断标志位必须清除
> 													//否则中断将连续不断地触发，导致主程序卡死
> 	}
> }
> 
> /**
>   * 函    数：EXTI1外部中断函数
>   * 参    数：无
>   * 返 回 值：无
>   * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
>   *           函数名为预留的指定名称，可以从启动文件复制
>   *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
>   */
> void EXTI1_IRQHandler(void)
> {
> 	if (EXTI_GetITStatus(EXTI_Line1) == SET)		//判断是否是外部中断1号线触发的中断
> 	{
> 		/*如果出现数据乱跳的现象，可再次判断引脚电平，以避免抖动*/
> 		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
> 		{
> 			if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0)		//PB1的下降沿触发中断，此时检测另一相PB0的电平，目的是判断旋转方向
> 			{
> 				Encoder_Count ++;					//此方向定义为正转，计数变量自增
> 			}
> 		}
> 		EXTI_ClearITPendingBit(EXTI_Line1);			//清除外部中断1号线的中断标志位
> 													//中断标志位必须清除
> 													//否则中断将连续不断地触发，导致主程序卡死
> 	}
> }
> ```
>
> 完整文件：[[源码/5-2 旋转编码器计次/Hardware/Encoder.c|Hardware/Encoder.c]]

> [!code]- 视频源码：旋转编码器 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "Encoder.h"
> 
> int16_t Num;			//定义待被旋转编码器调节的变量
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();		//OLED初始化
> 	Encoder_Init();		//旋转编码器初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "Num:");			//1行1列显示字符串Num:
> 	
> 	while (1)
> 	{
> 		Num += Encoder_Get();				//获取自上此调用此函数后，旋转编码器的增量值，并将增量值加到Num上
> 		OLED_ShowSignedNum(1, 5, Num, 5);	//显示Num
> 	}
> }
> ```
>
> 完整文件：[[源码/5-2 旋转编码器计次/User/main.c|User/main.c]]


后面的 [[06_TIM#编码器]] 会用 TIM 硬件编码器接口完成同一件事，CPU 不再每个边沿都进入 EXTI。
