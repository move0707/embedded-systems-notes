# GPIO

## 结构

GPIO 是后面所有外设的入口。先把 **输入路径、输出路径、模式选择** 看清楚，比背 8 种模式有用。

| GPIO 基本结构 | 单个 GPIO 位结构 |
|---|---|
| ![[assets/ppt/s019_GPIO基本结构.jpg]] | ![[assets/ppt/s020_GPIO位结构.jpg]] |

输入从引脚经过施密特触发器进入输入数据寄存器；输出由输出数据寄存器/片上外设经过输出控制电路驱动引脚。

## 模式

课程把 GPIO 分成 8 种模式：

| 输入 | 输出 |
|---|---|
| 浮空输入 | 推挽输出 |
| 上拉输入 | 开漏输出 |
| 下拉输入 | 复用推挽输出 |
| 模拟输入 | 复用开漏输出 |

| 8 种 GPIO 模式 | 浮空 / 上拉 / 下拉输入 |
|---|---|
| ![[assets/ppt/s021_GPIO模式.jpg]] | ![[assets/ppt/s022_浮空_上拉_下拉输入.jpg]] |

| 模拟输入 | 开漏 / 推挽输出 |
|---|---|
| ![[assets/ppt/s023_模拟输入.jpg]] | ![[assets/ppt/s024_开漏_推挽输出.jpg]] |

| 复用开漏 / 推挽输出 |
|---|
| ![[assets/ppt/s025_复用开漏_推挽输出.jpg]] |

### 输入

**上拉输入：**没有外部驱动时默认高。  
**下拉输入：**没有外部驱动时默认低。  
**浮空输入：**内部不提供确定电平，外部必须保证不悬空。  
**模拟输入：**关闭数字输入通道，把引脚交给 ADC。

### 输出

**推挽：**既能主动拉高，也能主动拉低，普通 LED、蜂鸣器控制最常见。  
**开漏：**能主动拉低，输出“高”时实际是高阻态，需要上拉；I2C 会用到。  
**复用输出：**输出控制权交给 TIM、USART、SPI、I2C 等片上外设。

## LED / 蜂鸣器

![[assets/ppt/s027_硬件电路.jpg]]

*课程 LED 与蜂鸣器硬件电路。先看清楚器件是高电平有效还是低电平有效。*

GPIO 电平和“设备开/关”不是一回事。比如 LED 阳极接 VCC、阴极接 GPIO 时：

**GPIO=0 → 有电流 → LED 亮；GPIO=1 → 两端近似同电位 → LED 灭。**

### LED 闪烁

流程：

**RCC → GPIO 推挽输出 → Set/Reset → Delay**

> [!code]- 视频源码：LED 闪烁 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> 
> int main(void)
> {
> 	/*开启时钟*/
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA的时钟
> 															//使用各个外设前必须开启时钟，否则对外设的操作无效
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;					//定义结构体变量
> 	
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		//GPIO模式，赋值为推挽输出模式
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;				//GPIO引脚，赋值为第0号引脚
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//GPIO速度，赋值为50MHz
> 	
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将赋值后的构体变量传递给GPIO_Init函数
> 															//函数内部会自动根据结构体的参数配置相应寄存器
> 															//实现GPIOA的初始化
> 	
> 	/*主循环，循环体内的代码会一直循环执行*/
> 	while (1)
> 	{
> 		/*设置PA0引脚的高低电平，实现LED闪烁，下面展示3种方法*/
> 		
> 		/*方法1：GPIO_ResetBits设置低电平，GPIO_SetBits设置高电平*/
> 		GPIO_ResetBits(GPIOA, GPIO_Pin_0);					//将PA0引脚设置为低电平
> 		Delay_ms(500);										//延时500ms
> 		GPIO_SetBits(GPIOA, GPIO_Pin_0);					//将PA0引脚设置为高电平
> 		Delay_ms(500);										//延时500ms
> 		
> 		/*方法2：GPIO_WriteBit设置低/高电平，由Bit_RESET/Bit_SET指定*/
> 		GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);		//将PA0引脚设置为低电平
> 		Delay_ms(500);										//延时500ms
> 		GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_SET);			//将PA0引脚设置为高电平
> 		Delay_ms(500);										//延时500ms
> 		
> 		/*方法3：GPIO_WriteBit设置低/高电平，由数据0/1指定，数据需要强转为BitAction类型*/
> 		GPIO_WriteBit(GPIOA, GPIO_Pin_0, (BitAction)0);		//将PA0引脚设置为低电平
> 		Delay_ms(500);										//延时500ms
> 		GPIO_WriteBit(GPIOA, GPIO_Pin_0, (BitAction)1);		//将PA0引脚设置为高电平
> 		Delay_ms(500);										//延时500ms
> 	}
> }
> ```
>
> 完整文件：[[源码/3-1 LED闪烁/User/main.c|User/main.c]]


### 流水灯

同一个原理，只是同时操作多个 Pin：

> [!code]- 视频源码：LED 流水灯 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> 
> int main(void)
> {
> 	/*开启时钟*/
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA的时钟
> 															//使用各个外设前必须开启时钟，否则对外设的操作无效
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;					//定义结构体变量
> 	
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		//GPIO模式，赋值为推挽输出模式
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;				//GPIO引脚，赋值为所有引脚
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//GPIO速度，赋值为50MHz
> 	
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将赋值后的构体变量传递给GPIO_Init函数
> 															//函数内部会自动根据结构体的参数配置相应寄存器
> 															//实现GPIOA的初始化
> 	
> 	/*主循环，循环体内的代码会一直循环执行*/
> 	while (1)
> 	{
> 		/*使用GPIO_Write，同时设置GPIOA所有引脚的高低电平，实现LED流水灯*/
> 		GPIO_Write(GPIOA, ~0x0001);	//0000 0000 0000 0001，PA0引脚为低电平，其他引脚均为高电平，注意数据有按位取反
> 		Delay_ms(100);				//延时100ms
> 		GPIO_Write(GPIOA, ~0x0002);	//0000 0000 0000 0010，PA1引脚为低电平，其他引脚均为高电平
> 		Delay_ms(100);				//延时100ms
> 		GPIO_Write(GPIOA, ~0x0004);	//0000 0000 0000 0100，PA2引脚为低电平，其他引脚均为高电平
> 		Delay_ms(100);				//延时100ms
> 		GPIO_Write(GPIOA, ~0x0008);	//0000 0000 0000 1000，PA3引脚为低电平，其他引脚均为高电平
> 		Delay_ms(100);				//延时100ms
> 		GPIO_Write(GPIOA, ~0x0010);	//0000 0000 0001 0000，PA4引脚为低电平，其他引脚均为高电平
> 		Delay_ms(100);				//延时100ms
> 		GPIO_Write(GPIOA, ~0x0020);	//0000 0000 0010 0000，PA5引脚为低电平，其他引脚均为高电平
> 		Delay_ms(100);				//延时100ms
> 		GPIO_Write(GPIOA, ~0x0040);	//0000 0000 0100 0000，PA6引脚为低电平，其他引脚均为高电平
> 		Delay_ms(100);				//延时100ms
> 		GPIO_Write(GPIOA, ~0x0080);	//0000 0000 1000 0000，PA7引脚为低电平，其他引脚均为高电平
> 		Delay_ms(100);				//延时100ms
> 	}
> }
> ```
>
> 完整文件：[[源码/3-2 LED流水灯/User/main.c|User/main.c]]


### 蜂鸣器

> [!code]- 视频源码：蜂鸣器 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> 
> int main(void)
> {
> 	/*开启时钟*/
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//开启GPIOB的时钟
> 															//使用各个外设前必须开启时钟，否则对外设的操作无效
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;					//定义结构体变量
> 	
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		//GPIO模式，赋值为推挽输出模式
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;				//GPIO引脚，赋值为第12号引脚
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//GPIO速度，赋值为50MHz
> 	
> 	GPIO_Init(GPIOB, &GPIO_InitStructure);					//将赋值后的构体变量传递给GPIO_Init函数
> 															//函数内部会自动根据结构体的参数配置相应寄存器
> 															//实现GPIOB的初始化
> 	
> 	/*主循环，循环体内的代码会一直循环执行*/
> 	while (1)
> 	{
> 		GPIO_ResetBits(GPIOB, GPIO_Pin_12);		//将PB12引脚设置为低电平，蜂鸣器鸣叫
> 		Delay_ms(100);							//延时100ms
> 		GPIO_SetBits(GPIOB, GPIO_Pin_12);		//将PB12引脚设置为高电平，蜂鸣器停止
> 		Delay_ms(100);							//延时100ms
> 		GPIO_ResetBits(GPIOB, GPIO_Pin_12);		//将PB12引脚设置为低电平，蜂鸣器鸣叫
> 		Delay_ms(100);							//延时100ms
> 		GPIO_SetBits(GPIOB, GPIO_Pin_12);		//将PB12引脚设置为高电平，蜂鸣器停止
> 		Delay_ms(700);							//延时700ms
> 	}
> }
> ```
>
> 完整文件：[[源码/3-3 蜂鸣器/User/main.c|User/main.c]]


## 按键

课程按键电路强调先判断默认电平：

![[assets/ppt/s031_硬件电路.jpg]]

*按键与光敏传感器实验硬件电路。*

如果按键松开时被上拉为 1，按下接地，则：

**松开=1 → 按下=0**

机械按键有抖动，所以课程代码用“检测 → 延时消抖 → 等待松手 → 再延时”的方式保证一次按下只返回一次事件。

> [!code]- 视频源码：Key.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> 
> /**
>   * 函    数：按键初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void Key_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_11;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOB, &GPIO_InitStructure);						//将PB1和PB11引脚初始化为上拉输入
> }
> 
> /**
>   * 函    数：按键获取键码
>   * 参    数：无
>   * 返 回 值：按下按键的键码值，范围：0~2，返回0代表没有按键按下
>   * 注意事项：此函数是阻塞式操作，当按键按住不放时，函数会卡住，直到按键松手
>   */
> uint8_t Key_GetNum(void)
> {
> 	uint8_t KeyNum = 0;		//定义变量，默认键码值为0
> 	
> 	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)			//读PB1输入寄存器的状态，如果为0，则代表按键1按下
> 	{
> 		Delay_ms(20);											//延时消抖
> 		while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0);	//等待按键松手
> 		Delay_ms(20);											//延时消抖
> 		KeyNum = 1;												//置键码为1
> 	}
> 	
> 	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0)			//读PB11输入寄存器的状态，如果为0，则代表按键2按下
> 	{
> 		Delay_ms(20);											//延时消抖
> 		while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0);	//等待按键松手
> 		Delay_ms(20);											//延时消抖
> 		KeyNum = 2;												//置键码为2
> 	}
> 	
> 	return KeyNum;			//返回键码值，如果没有按键按下，所有if都不成立，则键码为默认值0
> }
> ```
>
> 完整文件：[[源码/3-4 按键控制LED/Hardware/Key.c|Hardware/Key.c]]

> [!code]- 视频源码：LED.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /**
>   * 函    数：LED初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void LED_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA1和PA2引脚初始化为推挽输出
> 	
> 	/*设置GPIO初始化后的默认电平*/
> 	GPIO_SetBits(GPIOA, GPIO_Pin_1 | GPIO_Pin_2);				//设置PA1和PA2引脚为高电平
> }
> 
> /**
>   * 函    数：LED1开启
>   * 参    数：无
>   * 返 回 值：无
>   */
> void LED1_ON(void)
> {
> 	GPIO_ResetBits(GPIOA, GPIO_Pin_1);		//设置PA1引脚为低电平
> }
> 
> /**
>   * 函    数：LED1关闭
>   * 参    数：无
>   * 返 回 值：无
>   */
> void LED1_OFF(void)
> {
> 	GPIO_SetBits(GPIOA, GPIO_Pin_1);		//设置PA1引脚为高电平
> }
> 
> /**
>   * 函    数：LED1状态翻转
>   * 参    数：无
>   * 返 回 值：无
>   */
> void LED1_Turn(void)
> {
> 	if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_1) == 0)		//获取输出寄存器的状态，如果当前引脚输出低电平
> 	{
> 		GPIO_SetBits(GPIOA, GPIO_Pin_1);					//则设置PA1引脚为高电平
> 	}
> 	else													//否则，即当前引脚输出高电平
> 	{
> 		GPIO_ResetBits(GPIOA, GPIO_Pin_1);					//则设置PA1引脚为低电平
> 	}
> }
> 
> /**
>   * 函    数：LED2开启
>   * 参    数：无
>   * 返 回 值：无
>   */
> void LED2_ON(void)
> {
> 	GPIO_ResetBits(GPIOA, GPIO_Pin_2);		//设置PA2引脚为低电平
> }
> 
> /**
>   * 函    数：LED2关闭
>   * 参    数：无
>   * 返 回 值：无
>   */
> void LED2_OFF(void)
> {
> 	GPIO_SetBits(GPIOA, GPIO_Pin_2);		//设置PA2引脚为高电平
> }
> 
> /**
>   * 函    数：LED2状态翻转
>   * 参    数：无
>   * 返 回 值：无
>   */
> void LED2_Turn(void)
> {
> 	if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_2) == 0)		//获取输出寄存器的状态，如果当前引脚输出低电平
> 	{                                                  
> 		GPIO_SetBits(GPIOA, GPIO_Pin_2);               		//则设置PA2引脚为高电平
> 	}                                                  
> 	else                                               		//否则，即当前引脚输出高电平
> 	{                                                  
> 		GPIO_ResetBits(GPIOA, GPIO_Pin_2);             		//则设置PA2引脚为低电平
> 	}
> }
> ```
>
> 完整文件：[[源码/3-4 按键控制LED/Hardware/LED.c|Hardware/LED.c]]

> [!code]- 视频源码：按键控制 LED main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "LED.h"
> #include "Key.h"
> 
> uint8_t KeyNum;		//定义用于接收按键键码的变量
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	LED_Init();		//LED初始化
> 	Key_Init();		//按键初始化
> 	
> 	while (1)
> 	{
> 		KeyNum = Key_GetNum();		//获取按键键码
> 		
> 		if (KeyNum == 1)			//按键1按下
> 		{
> 			LED1_Turn();			//LED1翻转
> 		}
> 		
> 		if (KeyNum == 2)			//按键2按下
> 		{
> 			LED2_Turn();			//LED2翻转
> 		}
> 	}
> }
> ```
>
> 完整文件：[[源码/3-4 按键控制LED/User/main.c|User/main.c]]


> [!warning]
> 这种等待松手的写法会阻塞 CPU。入门阶段是为了把“按下一次只触发一次”讲清楚；复杂项目一般会改成定时扫描 + 状态机。

## 光敏

光敏传感器模块把光照变化转换成数字电平，STM32 只读取模块输出，不直接测电阻值。

**传感器输出 → GPIO 输入 → 条件判断 → 蜂鸣器输出**

> [!code]- 视频源码：LightSensor.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /**
>   * 函    数：光敏传感器初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void LightSensor_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOB, &GPIO_InitStructure);						//将PB13引脚初始化为上拉输入
> }
> 
> /**
>   * 函    数：获取当前光敏传感器输出的高低电平
>   * 参    数：无
>   * 返 回 值：光敏传感器输出的高低电平，范围：0/1
>   */
> uint8_t LightSensor_Get(void)
> {
> 	return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13);			//返回PB13输入寄存器的状态
> }
> ```
>
> 完整文件：[[源码/3-5 光敏传感器控制蜂鸣器/Hardware/LightSensor.c|Hardware/LightSensor.c]]

> [!code]- 视频源码：Buzzer.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /**
>   * 函    数：蜂鸣器初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void Buzzer_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOB, &GPIO_InitStructure);						//将PB12引脚初始化为推挽输出
> 	
> 	/*设置GPIO初始化后的默认电平*/
> 	GPIO_SetBits(GPIOB, GPIO_Pin_12);							//设置PB12引脚为高电平
> }
> 
> /**
>   * 函    数：蜂鸣器开启
>   * 参    数：无
>   * 返 回 值：无
>   */
> void Buzzer_ON(void)
> {
> 	GPIO_ResetBits(GPIOB, GPIO_Pin_12);		//设置PB12引脚为低电平
> }
> 
> /**
>   * 函    数：蜂鸣器关闭
>   * 参    数：无
>   * 返 回 值：无
>   */
> void Buzzer_OFF(void)
> {
> 	GPIO_SetBits(GPIOB, GPIO_Pin_12);		//设置PB12引脚为高电平
> }
> 
> /**
>   * 函    数：蜂鸣器状态翻转
>   * 参    数：无
>   * 返 回 值：无
>   */
> void Buzzer_Turn(void)
> {
> 	if (GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_12) == 0)		//获取输出寄存器的状态，如果当前引脚输出低电平
> 	{
> 		GPIO_SetBits(GPIOB, GPIO_Pin_12);						//则设置PB12引脚为高电平
> 	}
> 	else														//否则，即当前引脚输出高电平
> 	{
> 		GPIO_ResetBits(GPIOB, GPIO_Pin_12);						//则设置PB12引脚为低电平
> 	}
> }
> ```
>
> 完整文件：[[源码/3-5 光敏传感器控制蜂鸣器/Hardware/Buzzer.c|Hardware/Buzzer.c]]

> [!code]- 视频源码：光敏控制蜂鸣器 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "Buzzer.h"
> #include "LightSensor.h"
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	Buzzer_Init();			//蜂鸣器初始化
> 	LightSensor_Init();		//光敏传感器初始化
> 	
> 	while (1)
> 	{
> 		if (LightSensor_Get() == 1)		//如果当前光敏输出1
> 		{
> 			Buzzer_ON();				//蜂鸣器开启
> 		}
> 		else							//否则
> 		{
> 			Buzzer_OFF();				//蜂鸣器关闭
> 		}
> 	}
> }
> ```
>
> 完整文件：[[源码/3-5 光敏传感器控制蜂鸣器/User/main.c|User/main.c]]


## 重点

**先看电路决定有效电平，再写程序。**  
**普通 GPIO 控制用输出模式；TIM/USART 等外设接管引脚时改复用模式。**  
**ADC 引脚使用模拟输入，不走数字输入逻辑。**
