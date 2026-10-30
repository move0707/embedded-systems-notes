# TIM

TIM 这一章看起来内容很多：定时中断、外部时钟、PWM、舵机、电机、输入捕获、PWMI、编码器。实际上都围绕 **CNT 计数器** 展开。

## 简介

![[assets/ppt/s052_TIM简介.jpg]]

*TIM 可以做定时、外部计数、输出比较、输入捕获、编码器接口和主从触发。*

## 类型

| 通用定时器 | 基本定时器 |
|---|---|
| ![[assets/ppt/s055_通用定时器.jpg]] | ![[assets/ppt/s056_基本定时器.jpg]] |

STM32F103 的定时器分为高级、通用、基本三类。课程实验主要用通用定时器。

最重要的三个寄存器概念：

**PSC：预分频器　CNT：计数器　ARR：自动重装器**

![[assets/ppt/s057_时基单元.jpg]]

*时基单元：内部时钟、外部时钟、编码器等都可以成为 CNT 的计数来源。*

## 时基

内部时钟模式下：

**RCC 时钟 → PSC 分频 → CNT 计数 → CNT 达到 ARR → 更新事件 → CNT 重新开始**

| 预分频器时序 | 计数器更新时序 |
|---|---|
| ![[assets/ppt/s058_预分频器时序.jpg]] | ![[assets/ppt/s059_计数器时序.jpg]] |

如果定时器时钟为 `CK_PSC`：

**计数频率 = CK_PSC / (PSC + 1)**  
**更新频率 = CK_PSC / (PSC + 1) / (ARR + 1)**

`PSC` 和 `ARR` 都写 `目标值 - 1`，因为硬件实际参与的是 `(寄存器值 + 1)`。

### RCC 时钟

![[assets/ppt/s062_RCC时钟树.jpg]]

*课程 RCC 时钟树。定时器的实际时钟必须从时钟树往回追，不能想当然写 72 MHz。*

## 定时中断

实验 6-1：

**内部时钟 → PSC → CNT → ARR 更新 → Update 中断 → NVIC → TIM2_IRQHandler**

> [!code]- 视频源码：Timer.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /**
>   * 函    数：定时中断初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void Timer_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
> 	
> 	/*配置时钟源*/
> 	TIM_InternalClockConfig(TIM2);		//选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
> 	
> 	/*时基单元初始化*/
> 	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
> 	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
> 	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//计数器模式，选择向上计数
> 	TIM_TimeBaseInitStructure.TIM_Period = 10000 - 1;				//计数周期，即ARR的值
> 	TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;				//预分频器，即PSC的值
> 	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;			//重复计数器，高级定时器才会用到
> 	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);				//将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元	
> 	
> 	/*中断输出配置*/
> 	TIM_ClearFlag(TIM2, TIM_FLAG_Update);						//清除定时器更新标志位
> 																//TIM_TimeBaseInit函数末尾，手动产生了更新事件
> 																//若不清除此标志位，则开启中断后，会立刻进入一次中断
> 																//如果不介意此问题，则不清除此标志位也可
> 	
> 	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);					//开启TIM2的更新中断
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
> 	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;				//选择配置NVIC的TIM2线
> 	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
> 	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//指定NVIC线路的抢占优先级为2
> 	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//指定NVIC线路的响应优先级为1
> 	NVIC_Init(&NVIC_InitStructure);								//将结构体变量交给NVIC_Init，配置NVIC外设
> 	
> 	/*TIM使能*/
> 	TIM_Cmd(TIM2, ENABLE);			//使能TIM2，定时器开始运行
> }
> 
> /* 定时器中断函数，可以复制到使用它的地方
> void TIM2_IRQHandler(void)
> {
> 	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
> 	{
> 		
> 		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
> 	}
> }
> */
> ```
>
> 完整文件：[[源码/6-1 定时器定时中断/System/Timer.c|System/Timer.c]]

> [!code]- 视频源码：定时中断 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "Timer.h"
> 
> uint16_t Num;			//定义在定时器中断里自增的变量
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();		//OLED初始化
> 	Timer_Init();		//定时中断初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "Num:");			//1行1列显示字符串Num:
> 	
> 	while (1)
> 	{
> 		OLED_ShowNum(1, 5, Num, 5);			//不断刷新显示Num变量
> 	}
> }
> 
> /**
>   * 函    数：TIM2中断函数
>   * 参    数：无
>   * 返 回 值：无
>   * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
>   *           函数名为预留的指定名称，可以从启动文件复制
>   *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
>   */
> void TIM2_IRQHandler(void)
> {
> 	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)		//判断是否是TIM2的更新事件触发的中断
> 	{
> 		Num ++;												//Num变量自增，用于测试定时中断
> 		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);			//清除TIM2更新事件的中断标志位
> 															//中断标志位必须清除
> 															//否则中断将连续不断地触发，导致主程序卡死
> 	}
> }
> ```
>
> 完整文件：[[源码/6-1 定时器定时中断/User/main.c|User/main.c]]


代码里的核心顺序：

**RCC → `TIM_InternalClockConfig()` → `TIM_TimeBaseInit()` → `TIM_ITConfig()` → NVIC → `TIM_Cmd()`**

## 外部时钟

实验 6-2 把 CNT 的计数来源改成外部脉冲：（这里配置的是ETP也就是一个固定的GPIO引脚通过这个电平引脚的变化来计时）

**外部边沿 → TIM 外部时钟输入 → CNT + 1**

因此“定时器”本质先是计数器。内部稳定时钟让它能计时间，外部信号让它能计事件。

> [!code]- 视频源码：外部时钟 Timer.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /**
>   * 函    数：定时中断初始化
>   * 参    数：无
>   * 返 回 值：无
>   * 注意事项：此函数配置为外部时钟，定时器相当于计数器
>   */
> void Timer_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA0引脚初始化为上拉输入
> 	
> 	/*外部时钟配置*/
> 	TIM_ETRClockMode2Config(TIM2, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 0x0F);
> 																//选择外部时钟模式2，时钟从TIM_ETR引脚输入
> 																//注意TIM2的ETR引脚固定为PA0，无法随意更改
> 																//最后一个滤波器参数加到最大0x0F，可滤除时钟信号抖动
> 	
> 	/*时基单元初始化*/
> 	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
> 	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
> 	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//计数器模式，选择向上计数
> 	TIM_TimeBaseInitStructure.TIM_Period = 10 - 1;					//计数周期，即ARR的值
> 	TIM_TimeBaseInitStructure.TIM_Prescaler = 1 - 1;				//预分频器，即PSC的值
> 	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;			//重复计数器，高级定时器才会用到
> 	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);				//将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元	
> 	
> 	/*中断输出配置*/
> 	TIM_ClearFlag(TIM2, TIM_FLAG_Update);						//清除定时器更新标志位
> 																//TIM_TimeBaseInit函数末尾，手动产生了更新事件
> 																//若不清除此标志位，则开启中断后，会立刻进入一次中断
> 																//如果不介意此问题，则不清除此标志位也可
> 																
> 	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);					//开启TIM2的更新中断
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
> 	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;				//选择配置NVIC的TIM2线
> 	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
> 	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//指定NVIC线路的抢占优先级为2
> 	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//指定NVIC线路的响应优先级为1
> 	NVIC_Init(&NVIC_InitStructure);								//将结构体变量交给NVIC_Init，配置NVIC外设
> 	
> 	/*TIM使能*/
> 	TIM_Cmd(TIM2, ENABLE);			//使能TIM2，定时器开始运行
> }
> 
> /**
>   * 函    数：返回定时器CNT的值
>   * 参    数：无
>   * 返 回 值：定时器CNT的值，范围：0~65535
>   */
> uint16_t Timer_GetCounter(void)
> {
> 	return TIM_GetCounter(TIM2);	//返回定时器TIM2的CNT
> }
> 
> /* 定时器中断函数，可以复制到使用它的地方
> void TIM2_IRQHandler(void)
> {
> 	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
> 	{
> 		
> 		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
> 	}
> }
> */
> ```
>
> 完整文件：[[源码/6-2 定时器外部时钟/System/Timer.c|System/Timer.c]]

> [!code]- 视频源码：外部时钟 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "Timer.h"
> 
> uint16_t Num;			//定义在定时器中断里自增的变量
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();		//OLED初始化
> 	Timer_Init();		//定时中断初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "Num:");			//1行1列显示字符串Num:
> 	OLED_ShowString(2, 1, "CNT:");			//2行1列显示字符串CNT:
> 	
> 	while (1)
> 	{
> 		OLED_ShowNum(1, 5, Num, 5);			//不断刷新显示Num变量
> 		OLED_ShowNum(2, 5, Timer_GetCounter(), 5);		//不断刷新显示CNT的值
> 	}
> }
> 
> /**
>   * 函    数：TIM2中断函数
>   * 参    数：无
>   * 返 回 值：无
>   * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
>   *           函数名为预留的指定名称，可以从启动文件复制
>   *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
>   */
> void TIM2_IRQHandler(void)
> {
> 	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)		//判断是否是TIM2的更新事件触发的中断
> 	{
> 		Num ++;												//Num变量自增，用于测试定时中断
> 		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);			//清除TIM2更新事件的中断标志位
> 															//中断标志位必须清除
> 															//否则中断将连续不断地触发，导致主程序卡死
> 	}
> }
> ```
>
> 完整文件：[[源码/6-2 定时器外部时钟/User/main.c|User/main.c]]


---

## 输出比较

| 输出比较概念 | 高级定时器输出比较通道 |
|---|---|
| ![[assets/ppt/s063_输出比较简介.jpg]] | ![[assets/ppt/s065_输出比较通道(高级).jpg]] |

| 通用定时器输出比较通道                         | 输出比较模式                          |
| ----------------------------------- | ------------------------------- |
| ![[assets/ppt/s066_输出比较通道(通用).jpg]] | ![[assets/ppt/s067_输出比较模式.jpg]] |

输出比较 OC 的本质是 **比较 CNT 与 CCR，并按配置改变输出**。PWM 只是输出比较最常用的一种模式；另外还可以做匹配置位、清零、翻转等。
这里OC引脚其实就是CH引脚，每个定时器的CH引脚是固定的

## PWM

![[assets/ppt/s064_PWM简介.jpg]]

*PWM 的频率、占空比和分辨率定义。(PWM频率其实就是1s内cnt跑到ARR的次数)*


输出比较的逻辑是：

**CNT 不断计数 → 硬件持续比较 CNT 与 CCR → 比较结果控制输出电平**

| PWM 基本结构 | PWM 参数计算 |
|---|---|
| ![[assets/ppt/s068_PWM基本结构.jpg]] | ![[assets/ppt/s069_参数计算.jpg]] |

课程公式：

**PWM 频率：`Freq = CK_PSC / (PSC + 1) / (ARR + 1)`**  
**PWM 占空比：`Duty = CCR / (ARR + 1)`**  
**PWM 分辨率：`Reso = 1 / (ARR + 1)`**

所以三组参数职责不同：

| 参数 | 主要作用 |
|---|---|
| PSC | 先把定时器时钟降到合适范围 |
| ARR | 决定 PWM 周期，也影响占空比分辨率 |
| CCR | 决定高电平持续到哪里，也就是占空比 |

GPIO 必须用 **复用推挽输出**，因为 PWM 波形由 TIM 外设自动输出，不是 CPU 在循环里手动 Set/Reset。

### 呼吸灯

实验 6-3 中 PWM 频率保持不变，只缓慢修改 CCR：

**CCR 逐渐增大 → 平均亮度增加 → CCR 再减小 → 平均亮度下降**

人眼看不到高频 PWM 闪烁，只感受到平均亮度变化。

> [!code]- 视频源码：PWM.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /**
>   * 函    数：PWM初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void PWM_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
> 	
> 	/*GPIO重映射*/
> //	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);			//开启AFIO的时钟，重映射必须先开启AFIO的时钟
> //	GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);			//将TIM2的引脚部分重映射，具体的映射方案需查看参考手册
> //	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);		//将JTAG引脚失能，作为普通GPIO引脚使用
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;		//GPIO_Pin_15;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);							//将PA0引脚初始化为复用推挽输出	
> 																	//受外设控制的引脚，均需要配置为复用模式		
> 	
> 	/*配置时钟源*/
> 	TIM_InternalClockConfig(TIM2);		//选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
> 	
> 	/*时基单元初始化*/
> 	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
> 	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
> 	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
> 	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;					//计数周期，即ARR的值
> 	TIM_TimeBaseInitStructure.TIM_Prescaler = 720 - 1;				//预分频器，即PSC的值
> 	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
> 	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元
> 	
> 	/*输出比较初始化*/
> 	TIM_OCInitTypeDef TIM_OCInitStructure;							//定义结构体变量
> 	TIM_OCStructInit(&TIM_OCInitStructure);							//结构体初始化，若结构体没有完整赋值
> 																	//则最好执行此函数，给结构体所有成员都赋一个默认值
> 																	//避免结构体初值不确定的问题
> 	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;				//输出比较模式，选择PWM模式1
> 	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;		//输出极性，选择为高，若选择极性为低，则输出高低电平取反
> 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;	//输出使能
> 	TIM_OCInitStructure.TIM_Pulse = 0;								//初始的CCR值
> 	TIM_OC1Init(TIM2, &TIM_OCInitStructure);						//将结构体变量交给TIM_OC1Init，配置TIM2的输出比较通道1
> 	
> 	/*TIM使能*/
> 	TIM_Cmd(TIM2, ENABLE);			//使能TIM2，定时器开始运行
> }
> 
> /**
>   * 函    数：PWM设置CCR
>   * 参    数：Compare 要写入的CCR的值，范围：0~100
>   * 返 回 值：无
>   * 注意事项：CCR和ARR共同决定占空比，此函数仅设置CCR的值，并不直接是占空比
>   *           占空比Duty = CCR / (ARR + 1)
>   */
> void PWM_SetCompare1(uint16_t Compare)
> {
> 	TIM_SetCompare1(TIM2, Compare);		//设置CCR1的值
> }
> ```
>
> 完整文件：[[源码/6-3 PWM驱动LED呼吸灯/Hardware/PWM.c|Hardware/PWM.c]]

> [!code]- 视频源码：呼吸灯 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "PWM.h"
> 
> uint8_t i;			//定义for循环的变量
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();		//OLED初始化
> 	PWM_Init();			//PWM初始化
> 	
> 	while (1)
> 	{
> 		for (i = 0; i <= 100; i++)
> 		{
> 			PWM_SetCompare1(i);			//依次将定时器的CCR寄存器设置为0~100，PWM占空比逐渐增大，LED逐渐变亮
> 			Delay_ms(10);				//延时10ms
> 		}
> 		for (i = 0; i <= 100; i++)
> 		{
> 			PWM_SetCompare1(100 - i);	//依次将定时器的CCR寄存器设置为100~0，PWM占空比逐渐减小，LED逐渐变暗
> 			Delay_ms(10);				//延时10ms
> 		}
> 	}
> }
> ```
>
> 完整文件：[[源码/6-3 PWM驱动LED呼吸灯/User/main.c|User/main.c]]


### 舵机

| 舵机控制原理 | 舵机硬件电路 |
|---|---|
| ![[assets/ppt/s070_舵机简介.jpg]] | ![[assets/ppt/s071_硬件电路.jpg]] |

课程使用舵机时，PWM 周期固定，角度由高电平脉宽决定。代码把角度映射为 CCR。

> [!code]- 视频源码：舵机底层 PWM.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /**
>   * 函    数：PWM初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void PWM_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);							//将PA1引脚初始化为复用推挽输出	
> 																	//受外设控制的引脚，均需要配置为复用模式
> 	
> 	/*配置时钟源*/
> 	TIM_InternalClockConfig(TIM2);		//选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
> 	
> 	/*时基单元初始化*/
> 	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
> 	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
> 	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
> 	TIM_TimeBaseInitStructure.TIM_Period = 20000 - 1;				//计数周期，即ARR的值
> 	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;				//预分频器，即PSC的值
> 	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
> 	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元
> 	
> 	/*输出比较初始化*/ 
> 	TIM_OCInitTypeDef TIM_OCInitStructure;							//定义结构体变量
> 	TIM_OCStructInit(&TIM_OCInitStructure);                         //结构体初始化，若结构体没有完整赋值
> 	                                                                //则最好执行此函数，给结构体所有成员都赋一个默认值
> 	                                                                //避免结构体初值不确定的问题
> 	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;               //输出比较模式，选择PWM模式1
> 	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;       //输出极性，选择为高，若选择极性为低，则输出高低电平取反
> 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;   //输出使能
> 	TIM_OCInitStructure.TIM_Pulse = 0;								//初始的CCR值
> 	TIM_OC2Init(TIM2, &TIM_OCInitStructure);                        //将结构体变量交给TIM_OC2Init，配置TIM2的输出比较通道2
> 	
> 	/*TIM使能*/
> 	TIM_Cmd(TIM2, ENABLE);			//使能TIM2，定时器开始运行
> }
> 
> /**
>   * 函    数：PWM设置CCR
>   * 参    数：Compare 要写入的CCR的值，范围：0~100
>   * 返 回 值：无
>   * 注意事项：CCR和ARR共同决定占空比，此函数仅设置CCR的值，并不直接是占空比
>   *           占空比Duty = CCR / (ARR + 1)
>   */
> void PWM_SetCompare2(uint16_t Compare)
> {
> 	TIM_SetCompare2(TIM2, Compare);		//设置CCR2的值
> }
> ```
>
> 完整文件：[[源码/6-4 PWM驱动舵机/Hardware/PWM.c|Hardware/PWM.c]]

> [!code]- 视频源码：Servo.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "PWM.h"
> 
> /**
>   * 函    数：舵机初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void Servo_Init(void)
> {
> 	PWM_Init();									//初始化舵机的底层PWM
> }
> 
> /**
>   * 函    数：舵机设置角度
>   * 参    数：Angle 要设置的舵机角度，范围：0~180
>   * 返 回 值：无
>   */
> void Servo_SetAngle(float Angle)
> {
> 	PWM_SetCompare2(Angle / 180 * 2000 + 500);	//设置占空比
> 												//将角度线性变换，对应到舵机要求的占空比范围上
> }
> ```
>
> 完整文件：[[源码/6-4 PWM驱动舵机/Hardware/Servo.c|Hardware/Servo.c]]

> [!code]- 视频源码：舵机 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "Servo.h"
> #include "Key.h"
> 
> uint8_t KeyNum;			//定义用于接收键码的变量
> float Angle;			//定义角度变量
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();		//OLED初始化
> 	Servo_Init();		//舵机初始化
> 	Key_Init();			//按键初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "Angle:");	//1行1列显示字符串Angle:
> 	
> 	while (1)
> 	{
> 		KeyNum = Key_GetNum();			//获取按键键码
> 		if (KeyNum == 1)				//按键1按下
> 		{
> 			Angle += 30;				//角度变量自增30
> 			if (Angle > 180)			//角度变量超过180后
> 			{
> 				Angle = 0;				//角度变量归零
> 			}
> 		}
> 		Servo_SetAngle(Angle);			//设置舵机的角度为角度变量
> 		OLED_ShowNum(1, 7, Angle, 3);	//OLED显示角度变量
> 	}
> }
> ```
>
> 完整文件：[[源码/6-4 PWM驱动舵机/User/main.c|User/main.c]]


### 直流电机

| 直流电机与驱动                            | 电机硬件电路                        |
| ---------------------------------- | ----------------------------- |
| ![[assets/ppt/s072_直流电机及驱动简介.jpg]] | ![[assets/ppt/s073_硬件电路.jpg]] |

GPIO 不能直接给电机供电。课程使用电机驱动芯片，STM32 负责：

**方向 GPIO → 决定正/反转　PWM → 决定平均功率和速度**

> [!code]- 视频源码：电机 PWM.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /**
>   * 函    数：PWM初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void PWM_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);							//将PA2引脚初始化为复用推挽输出	
> 																	//受外设控制的引脚，均需要配置为复用模式
> 	
> 	/*配置时钟源*/
> 	TIM_InternalClockConfig(TIM2);		//选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
> 	
> 	/*时基单元初始化*/
> 	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
> 	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
> 	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
> 	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;                 //计数周期，即ARR的值
> 	TIM_TimeBaseInitStructure.TIM_Prescaler = 36 - 1;               //预分频器，即PSC的值
> 	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
> 	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元
> 	
> 	/*输出比较初始化*/ 
> 	TIM_OCInitTypeDef TIM_OCInitStructure;							//定义结构体变量
> 	TIM_OCStructInit(&TIM_OCInitStructure);                         //结构体初始化，若结构体没有完整赋值
> 	                                                                //则最好执行此函数，给结构体所有成员都赋一个默认值
> 	                                                                //避免结构体初值不确定的问题
> 	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;               //输出比较模式，选择PWM模式1
> 	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;       //输出极性，选择为高，若选择极性为低，则输出高低电平取反
> 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;   //输出使能
> 	TIM_OCInitStructure.TIM_Pulse = 0;								//初始的CCR值
> 	TIM_OC3Init(TIM2, &TIM_OCInitStructure);                        //将结构体变量交给TIM_OC3Init，配置TIM2的输出比较通道3
> 	
> 	/*TIM使能*/
> 	TIM_Cmd(TIM2, ENABLE);			//使能TIM2，定时器开始运行
> }
> 
> /**
>   * 函    数：PWM设置CCR
>   * 参    数：Compare 要写入的CCR的值，范围：0~100
>   * 返 回 值：无
>   * 注意事项：CCR和ARR共同决定占空比，此函数仅设置CCR的值，并不直接是占空比
>   *           占空比Duty = CCR / (ARR + 1)
>   */
> void PWM_SetCompare3(uint16_t Compare)
> {
> 	TIM_SetCompare3(TIM2, Compare);		//设置CCR3的值
> }
> ```
>
> 完整文件：[[源码/6-5 PWM驱动直流电机/Hardware/PWM.c|Hardware/PWM.c]]

> [!code]- 视频源码：Motor.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "PWM.h"
> 
> /**
>   * 函    数：直流电机初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void Motor_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
> 	
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA4和PA5引脚初始化为推挽输出	
> 	
> 	PWM_Init();													//初始化直流电机的底层PWM
> }
> 
> /**
>   * 函    数：直流电机设置速度
>   * 参    数：Speed 要设置的速度，范围：-100~100
>   * 返 回 值：无
>   */
> void Motor_SetSpeed(int8_t Speed)
> {
> 	if (Speed >= 0)							//如果设置正转的速度值
> 	{
> 		GPIO_SetBits(GPIOA, GPIO_Pin_4);	//PA4置高电平
> 		GPIO_ResetBits(GPIOA, GPIO_Pin_5);	//PA5置低电平，设置方向为正转
> 		PWM_SetCompare3(Speed);				//PWM设置为速度值
> 	}
> 	else									//否则，即设置反转的速度值
> 	{
> 		GPIO_ResetBits(GPIOA, GPIO_Pin_4);	//PA4置低电平
> 		GPIO_SetBits(GPIOA, GPIO_Pin_5);	//PA5置高电平，设置方向为反转
> 		PWM_SetCompare3(-Speed);			//PWM设置为负的速度值，因为此时速度值为负数，而PWM只能给正数
> 	}
> }
> ```
>
> 完整文件：[[源码/6-5 PWM驱动直流电机/Hardware/Motor.c|Hardware/Motor.c]]

> [!code]- 视频源码：电机 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "Motor.h"
> #include "Key.h"
> 
> uint8_t KeyNum;		//定义用于接收按键键码的变量
> int8_t Speed;		//定义速度变量
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();		//OLED初始化
> 	Motor_Init();		//直流电机初始化
> 	Key_Init();			//按键初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "Speed:");		//1行1列显示字符串Speed:
> 	
> 	while (1)
> 	{
> 		KeyNum = Key_GetNum();				//获取按键键码
> 		if (KeyNum == 1)					//按键1按下
> 		{
> 			Speed += 20;					//速度变量自增20
> 			if (Speed > 100)				//速度变量超过100后
> 			{
> 				Speed = -100;				//速度变量变为-100
> 											//此操作会让电机旋转方向突然改变，可能会因供电不足而导致单片机复位
> 											//若出现了此现象，则应避免使用这样的操作
> 			}
> 		}
> 		Motor_SetSpeed(Speed);				//设置直流电机的速度为速度变量
> 		OLED_ShowSignedNum(1, 7, Speed, 3);	//OLED显示速度变量
> 	}
> }
> ```
>
> 完整文件：[[源码/6-5 PWM驱动直流电机/User/main.c|User/main.c]]


---

## 输入捕获

![[assets/ppt/s074_输入捕获简介.jpg]]

*输入捕获：指定边沿到来时将 CNT 锁存到 CCR。（一旦检测到上升沿或下降沿就是把cnt的值保存到CCR里，顺便触发个中断TGIO然后选择触发源等等）*


输出比较是“CNT 数到 CCR”；输入捕获正好反过来：

**外部边沿到来 → 把当前 CNT 的值锁存进 CCR**

| 输入捕获通道 | 主从触发模式 |
|---|---|
| ![[assets/ppt/s076_输入捕获通道.jpg]] | ![[assets/ppt/s077_主从触发模式.jpg]] |

| 输入捕获基本结构 |
|---|
| ![[assets/ppt/s078_输入捕获基本结构.jpg]] |

课程先对比两种测频思路：

![[assets/ppt/s075_频率测量.jpg]]

*测频方法：测频法与测周法，中界频率决定哪种误差更合适。*

实验最终使用输入捕获的测周思路。课程测频率时，把定时器配置成固定计数频率。两个相邻上升沿之间的计数值就是一个周期对应的计数数。

如果 CNT 频率为 1 MHz，捕获值是 1000：

**周期 = 1000 µs = 1 ms → 频率 = 1 kHz**

### Reset 模式

主从触发模式可以让输入边沿触发 CNT 自动清零。这样每次 CCR 里直接得到“本周期长度”，不用软件再做第二次减第一次。

> [!code]- 视频源码：输入捕获 IC.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /**
>   * 函    数：输入捕获初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void IC_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);			//开启TIM3的时钟
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);							//将PA6引脚初始化为上拉输入
> 	
> 	/*配置时钟源*/
> 	TIM_InternalClockConfig(TIM3);		//选择TIM3为内部时钟，若不调用此函数，TIM默认也为内部时钟
> 	
> 	/*时基单元初始化*/
> 	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
> 	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
> 	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
> 	TIM_TimeBaseInitStructure.TIM_Period = 65536 - 1;               //计数周期，即ARR的值
> 	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;               //预分频器，即PSC的值
> 	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
> 	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM3的时基单元
> 	
> 	/*输入捕获初始化*/
> 	TIM_ICInitTypeDef TIM_ICInitStructure;							//定义结构体变量
> 	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;				//选择配置定时器通道1
> 	TIM_ICInitStructure.TIM_ICFilter = 0xF;							//输入滤波器参数，可以过滤信号抖动
> 	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;		//极性，选择为上升沿触发捕获
> 	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;			//捕获预分频，选择不分频，每次信号都触发捕获
> 	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;	//输入信号交叉，选择直通，不交叉
> 	TIM_ICInit(TIM3, &TIM_ICInitStructure);							//将结构体变量交给TIM_ICInit，配置TIM3的输入捕获通道
> 	
> 	/*选择触发源及从模式*/
> 	TIM_SelectInputTrigger(TIM3, TIM_TS_TI1FP1);					//触发源选择TI1FP1
> 	TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);					//从模式选择复位
> 																	//即TI1产生上升沿时，会触发CNT归零
> 	
> 	/*TIM使能*/
> 	TIM_Cmd(TIM3, ENABLE);			//使能TIM3，定时器开始运行
> }
> 
> /**
>   * 函    数：获取输入捕获的频率
>   * 参    数：无
>   * 返 回 值：捕获得到的频率
>   */
> uint32_t IC_GetFreq(void)
> {
> 	return 1000000 / (TIM_GetCapture1(TIM3) + 1);		//测周法得到频率fx = fc / N，这里不执行+1的操作也可
> }
> ```
>
> 完整文件：[[源码/6-6 输入捕获模式测频率/Hardware/IC.c|Hardware/IC.c]]

> [!code]- 视频源码：输入捕获 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "PWM.h"
> #include "IC.h"
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();		//OLED初始化
> 	PWM_Init();			//PWM初始化
> 	IC_Init();			//输入捕获初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "Freq:00000Hz");		//1行1列显示字符串Freq:00000Hz
> 	
> 	/*使用PWM模块提供输入捕获的测试信号*/
> 	PWM_SetPrescaler(720 - 1);					//PWM频率Freq = 72M / (PSC + 1) / 100
> 	PWM_SetCompare1(50);						//PWM占空比Duty = CCR / 100
> 	
> 	while (1)
> 	{
> 		OLED_ShowNum(1, 6, IC_GetFreq(), 5);	//不断刷新显示输入捕获测得的频率
> 	}
> }
> ```
>
> 完整文件：[[源码/6-6 输入捕获模式测频率/User/main.c|User/main.c]]


## PWMI

普通输入捕获只测一个边沿间隔；PWMI 用两个捕获通道分别记录周期和高电平时间。

![[assets/ppt/s079_PWMI基本结构.jpg]]

*PWMI 基本结构：一个通道测周期，一个通道测高电平宽度。*

因此：

**频率 = 计数频率 / 周期捕获值**  
**占空比 = 高电平捕获值 / 周期捕获值 × 100%**

> [!code]- 视频源码：PWMI IC.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /**
>   * 函    数：输入捕获初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void IC_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);			//开启TIM3的时钟
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);							//将PA6引脚初始化为上拉输入
> 	
> 	/*配置时钟源*/
> 	TIM_InternalClockConfig(TIM3);		//选择TIM3为内部时钟，若不调用此函数，TIM默认也为内部时钟
> 	
> 	/*时基单元初始化*/
> 	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
> 	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
> 	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
> 	TIM_TimeBaseInitStructure.TIM_Period = 65536 - 1;               //计数周期，即ARR的值
> 	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;               //预分频器，即PSC的值
> 	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
> 	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM3的时基单元
> 	
> 	/*PWMI模式初始化*/
> 	TIM_ICInitTypeDef TIM_ICInitStructure;							//定义结构体变量
> 	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;				//选择配置定时器通道1
> 	TIM_ICInitStructure.TIM_ICFilter = 0xF;							//输入滤波器参数，可以过滤信号抖动
> 	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;		//极性，选择为上升沿触发捕获
> 	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;			//捕获预分频，选择不分频，每次信号都触发捕获
> 	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;	//输入信号交叉，选择直通，不交叉
> 	TIM_PWMIConfig(TIM3, &TIM_ICInitStructure);						//将结构体变量交给TIM_PWMIConfig，配置TIM3的输入捕获通道
> 																	//此函数同时会把另一个通道配置为相反的配置，实现PWMI模式
> 
> 	/*选择触发源及从模式*/
> 	TIM_SelectInputTrigger(TIM3, TIM_TS_TI1FP1);					//触发源选择TI1FP1
> 	TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);					//从模式选择复位
> 																	//即TI1产生上升沿时，会触发CNT归零
> 	
> 	/*TIM使能*/
> 	TIM_Cmd(TIM3, ENABLE);			//使能TIM3，定时器开始运行
> }
> 
> /**
>   * 函    数：获取输入捕获的频率
>   * 参    数：无
>   * 返 回 值：捕获得到的频率
>   */
> uint32_t IC_GetFreq(void)
> {
> 	return 1000000 / (TIM_GetCapture1(TIM3) + 1);		//测周法得到频率fx = fc / N，这里不执行+1的操作也可
> }
> 
> /**
>   * 函    数：获取输入捕获的占空比
>   * 参    数：无
>   * 返 回 值：捕获得到的占空比
>   */
> uint32_t IC_GetDuty(void)
> {
> 	return (TIM_GetCapture2(TIM3) + 1) * 100 / (TIM_GetCapture1(TIM3) + 1);	//占空比Duty = CCR2 / CCR1 * 100，这里不执行+1的操作也可
> }
> ```
>
> 完整文件：[[源码/6-7 PWMI模式测频率占空比/Hardware/IC.c|Hardware/IC.c]]

> [!code]- 视频源码：PWMI main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "PWM.h"
> #include "IC.h"
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();		//OLED初始化
> 	PWM_Init();			//PWM初始化
> 	IC_Init();			//输入捕获初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "Freq:00000Hz");		//1行1列显示字符串Freq:00000Hz
> 	OLED_ShowString(2, 1, "Duty:00%");			//2行1列显示字符串Duty:00%
> 	
> 	/*使用PWM模块提供输入捕获的测试信号*/
> 	PWM_SetPrescaler(720 - 1);					//PWM频率Freq = 72M / (PSC + 1) / 100
> 	PWM_SetCompare1(50);						//PWM占空比Duty = CCR / 100
> 	
> 	while (1)
> 	{
> 		OLED_ShowNum(1, 6, IC_GetFreq(), 5);	//不断刷新显示输入捕获测得的频率
> 		OLED_ShowNum(2, 6, IC_GetDuty(), 2);	//不断刷新显示输入捕获测得的占空比
> 	}
> }
> ```
>
> 完整文件：[[源码/6-7 PWMI模式测频率占空比/User/main.c|User/main.c]]


## 编码器

| 编码器接口简介 | 正交编码器 A/B 相 |
|---|---|
| ![[assets/ppt/s080_编码器接口简介.jpg]] | ![[assets/ppt/s081_正交编码器.jpg]] |


正交编码器 A/B 两相信号相差 90°。TIM 编码器接口根据谁超前自动让 CNT 加或减。

| 编码器接口基本结构 | 编码器工作模式 |
|---|---|
| ![[assets/ppt/s082_编码器接口基本结构.jpg]] | ![[assets/ppt/s083_工作模式.jpg]] |

编码器接口可以选择 TI1/TI2 边沿及极性（分别对应编码器的A/B相），课程还给出不同反相配置下的计数实例：

| TI1/TI2 均不反相                      | TI1 反相实例                           |
| --------------------------------- | ---------------------------------- |
| ![[assets/ppt/s084_实例（均不反相）.jpg]] | ![[assets/ppt/s085_实例（TI1反相）.jpg]] |

和 EXTI 方案相比：

**EXTI：每个边沿都进 CPU，软件判断方向**  
**TIM 编码器接口：硬件直接判断方向并修改 CNT**

课程测速实验每隔固定时间读取一次 CNT，再清零。这样“固定时间窗口内的 CNT 增量”就对应速度。

> [!code]- 视频源码：编码器接口 Encoder.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /**
>   * 函    数：编码器初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void Encoder_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);			//开启TIM3的时钟
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);							//将PA6和PA7引脚初始化为上拉输入
> 	
> 	/*时基单元初始化*/
> 	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
> 	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
> 	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
> 	TIM_TimeBaseInitStructure.TIM_Period = 65536 - 1;               //计数周期，即ARR的值
> 	TIM_TimeBaseInitStructure.TIM_Prescaler = 1 - 1;                //预分频器，即PSC的值
> 	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
> 	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM3的时基单元
> 	
> 	/*输入捕获初始化*/
> 	TIM_ICInitTypeDef TIM_ICInitStructure;							//定义结构体变量
> 	TIM_ICStructInit(&TIM_ICInitStructure);							//结构体初始化，若结构体没有完整赋值
> 																	//则最好执行此函数，给结构体所有成员都赋一个默认值
> 																	//避免结构体初值不确定的问题
> 	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;				//选择配置定时器通道1
> 	TIM_ICInitStructure.TIM_ICFilter = 0xF;							//输入滤波器参数，可以过滤信号抖动
> 	TIM_ICInit(TIM3, &TIM_ICInitStructure);							//将结构体变量交给TIM_ICInit，配置TIM3的输入捕获通道
> 	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;				//选择配置定时器通道2
> 	TIM_ICInitStructure.TIM_ICFilter = 0xF;							//输入滤波器参数，可以过滤信号抖动
> 	TIM_ICInit(TIM3, &TIM_ICInitStructure);							//将结构体变量交给TIM_ICInit，配置TIM3的输入捕获通道
> 	
> 	/*编码器接口配置*/
> 	TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
> 																	//配置编码器模式以及两个输入通道是否反相
> 																	//注意此时参数的Rising和Falling已经不代表上升沿和下降沿了，而是代表是否反相
> 																	//此函数必须在输入捕获初始化之后进行，否则输入捕获的配置会覆盖此函数的部分配置
> 	
> 	/*TIM使能*/
> 	TIM_Cmd(TIM3, ENABLE);			//使能TIM3，定时器开始运行
> }
> 
> /**
>   * 函    数：获取编码器的增量值
>   * 参    数：无
>   * 返 回 值：自上此调用此函数后，编码器的增量值
>   */
> int16_t Encoder_Get(void)
> {
> 	/*使用Temp变量作为中继，目的是返回CNT后将其清零*/
> 	int16_t Temp;
> 	Temp = TIM_GetCounter(TIM3);
> 	TIM_SetCounter(TIM3, 0);
> 	return Temp;
> }
> ```
>
> 完整文件：[[源码/6-8 编码器接口测速/Hardware/Encoder.c|Hardware/Encoder.c]]

> [!code]- 视频源码：测速定时器 Timer.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /**
>   * 函    数：定时中断初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void Timer_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
> 	
> 	/*配置时钟源*/
> 	TIM_InternalClockConfig(TIM2);		//选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
> 	
> 	/*时基单元初始化*/
> 	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
> 	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
> 	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//计数器模式，选择向上计数
> 	TIM_TimeBaseInitStructure.TIM_Period = 10000 - 1;				//计数周期，即ARR的值
> 	TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;				//预分频器，即PSC的值
> 	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;			//重复计数器，高级定时器才会用到
> 	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);				//将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元	
> 	
> 	/*中断输出配置*/
> 	TIM_ClearFlag(TIM2, TIM_FLAG_Update);						//清除定时器更新标志位
> 																//TIM_TimeBaseInit函数末尾，手动产生了更新事件
> 																//若不清除此标志位，则开启中断后，会立刻进入一次中断
> 																//如果不介意此问题，则不清除此标志位也可
> 	
> 	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);					//开启TIM2的更新中断
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
> 	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;				//选择配置NVIC的TIM2线
> 	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
> 	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//指定NVIC线路的抢占优先级为2
> 	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//指定NVIC线路的响应优先级为1
> 	NVIC_Init(&NVIC_InitStructure);								//将结构体变量交给NVIC_Init，配置NVIC外设
> 	
> 	/*TIM使能*/
> 	TIM_Cmd(TIM2, ENABLE);			//使能TIM2，定时器开始运行
> }
> 
> /* 定时器中断函数，可以复制到使用它的地方
> void TIM2_IRQHandler(void)
> {
> 	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
> 	{
> 		
> 		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
> 	}
> }
> */
> ```
>
> 完整文件：[[源码/6-8 编码器接口测速/System/Timer.c|System/Timer.c]]

> [!code]- 视频源码：编码器测速 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "Timer.h"
> #include "Encoder.h"
> 
> int16_t Speed;			//定义速度变量
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();		//OLED初始化
> 	Timer_Init();		//定时器初始化
> 	Encoder_Init();		//编码器初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "Speed:");		//1行1列显示字符串Speed:
> 	
> 	while (1)
> 	{
> 		OLED_ShowSignedNum(1, 7, Speed, 5);	//不断刷新显示编码器测得的最新速度
> 	}
> }
> 
> /**
>   * 函    数：TIM2中断函数
>   * 参    数：无
>   * 返 回 值：无
>   * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
>   *           函数名为预留的指定名称，可以从启动文件复制
>   *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
>   */
> void TIM2_IRQHandler(void)
> {
> 	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)		//判断是否是TIM2的更新事件触发的中断
> 	{
> 		Speed = Encoder_Get();								//每隔固定时间段读取一次编码器计数增量值，即为速度值
> 		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);			//清除TIM2更新事件的中断标志位
> 															//中断标志位必须清除
> 															//否则中断将连续不断地触发，导致主程序卡死
> 	}
> }
> ```
>
> 完整文件：[[源码/6-8 编码器接口测速/User/main.c|User/main.c]]


## 一张表

| 功能 | CNT 在做什么 | CCR 在做什么 |
|---|---|---|
| 定时中断 | 按时钟计数 | 不一定使用 |
| 外部计数 | 按外部脉冲计数 | 不一定使用 |
| PWM | 周期性从 0 数到 ARR | 作为比较阈值 |
| 输入捕获 | 持续计数 | 边沿到来时保存 CNT |
| PWMI | 持续计数/可被触发清零 | 两个通道分别保存周期和高电平 |
| 编码器 | A/B 相决定加减 | 通常不作为核心 |

真正掌握 TIM 的标志，是看到实验先问：**CNT 的时钟是谁？什么时候清零？CCR 是比较值还是捕获值？**
