# DMA

DMA 的意义不是“让 CPU 算得更快”，而是 **数据搬运不再由 CPU 一次一次复制**。

## 数据从哪到哪

DMA 可以在外设和存储器之间搬运数据，也可以做存储器到存储器传输。

| DMA 简介 | STM32 存储器映像 |
|---|---|
| ![[assets/ppt/s100_DMA简介.jpg]] | ![[assets/ppt/s101_存储器映像.jpg]] |

| DMA 框图                         | DMA 基本结构                         |
| ------------------------------ | -------------------------------- |
| ![[assets/ppt/s102_DMA框图.jpg]] | ![[assets/ppt/s103_DMA基本结构.jpg]] |
|                                | EN位是开关控制                         |

CPU 只负责一开始告诉 DMA：

**源地址 → 目标地址 → 搬几个 → 每个多宽 → 地址是否自增 → 谁来触发**

之后 DMA 控制器自己完成传输。

## 请求

![[assets/ppt/s104_DMA请求.jpg]]

*不同外设固定连接到对应 DMA Channel。*

外设产生 DMA Request 后，DMA 才开始一次传输。所以使用 DMA 时不能只看“DMA1 Channel1/2/3”，还要核对 **这个外设请求到底接到哪个 Channel**。

## 宽度与对齐

![[assets/ppt/s105_数据宽度与对齐.jpg]]

*源/目标宽度不同时的数据对齐规则。*

课程配置里常见 Byte / HalfWord / Word。宽度必须和源数据、目标数组的实际类型对应，否则容易出现截断或排列错误。

## 内存搬运

实验 8-1 直接演示数组/存储器搬运：

![[assets/ppt/s106_数据转运+DMA.jpg]]

*DMA 数据转运实验。*

> [!code]- 视频源码：MyDMA.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> uint16_t MyDMA_Size;					//定义全局变量，用于记住Init函数的Size，供Transfer函数使用
> 
> /**
>   * 函    数：DMA初始化
>   * 参    数：AddrA 原数组的首地址
>   * 参    数：AddrB 目的数组的首地址
>   * 参    数：Size 转运的数据大小（转运次数）
>   * 返 回 值：无
>   */
> void MyDMA_Init(uint32_t AddrA, uint32_t AddrB, uint16_t Size)
> {
> 	MyDMA_Size = Size;					//将Size写入到全局变量，记住参数Size
> 	
> 	/*开启时钟*/
> 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);						//开启DMA的时钟
> 	
> 	/*DMA初始化*/
> 	DMA_InitTypeDef DMA_InitStructure;										//定义结构体变量
> 	DMA_InitStructure.DMA_PeripheralBaseAddr = AddrA;						//外设基地址，给定形参AddrA
> 	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//外设数据宽度，选择字节
> 	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Enable;			//外设地址自增，选择使能
> 	DMA_InitStructure.DMA_MemoryBaseAddr = AddrB;							//存储器基地址，给定形参AddrB
> 	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;			//存储器数据宽度，选择字节
> 	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;					//存储器地址自增，选择使能
> 	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;						//数据传输方向，选择由外设到存储器，必须填，但M2M开启后这个值不起作用
> 	DMA_InitStructure.DMA_BufferSize = Size;								//转运的数据大小（转运次数）
> 	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							//模式，选择正常模式
> 	DMA_InitStructure.DMA_M2M = DMA_M2M_Enable;								//存储器到存储器，选择使能
> 	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;					//优先级，选择中等
> 	DMA_Init(DMA1_Channel1, &DMA_InitStructure);							//将结构体变量交给DMA_Init，配置DMA1的通道1
> 	
> 	/*DMA使能*/
> 	DMA_Cmd(DMA1_Channel1, DISABLE);	//这里先不给使能，初始化后不会立刻工作，等后续调用Transfer后，再开始
> }
> 
> /**
>   * 函    数：启动DMA数据转运
>   * 参    数：无
>   * 返 回 值：无
>   */
> void MyDMA_Transfer(void)
> {
> 	DMA_Cmd(DMA1_Channel1, DISABLE);					//DMA失能，在写入传输计数器之前，需要DMA暂停工作
> 	DMA_SetCurrDataCounter(DMA1_Channel1, MyDMA_Size);	//写入传输计数器，指定将要转运的次数
> 	DMA_Cmd(DMA1_Channel1, ENABLE);						//DMA使能，开始工作
> 	
> 	while (DMA_GetFlagStatus(DMA1_FLAG_TC1) == RESET);	//等待DMA工作完成
> 	DMA_ClearFlag(DMA1_FLAG_TC1);						//清除工作完成标志位
> }
> ```
>
> 完整文件：[[源码/8-1 DMA数据转运/System/MyDMA.c|System/MyDMA.c]]

> [!code]- 视频源码：DMA 数据转运 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "MyDMA.h"
> 
> uint8_t DataA[] = {0x01, 0x02, 0x03, 0x04};				//定义测试数组DataA，为数据源
> uint8_t DataB[] = {0, 0, 0, 0};							//定义测试数组DataB，为数据目的地
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();				//OLED初始化
> 	
> 	MyDMA_Init((uint32_t)DataA, (uint32_t)DataB, 4);	//DMA初始化，把源数组和目的数组的地址传入
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "DataA");
> 	OLED_ShowString(3, 1, "DataB");
> 	
> 	/*显示数组的首地址*/
> 	OLED_ShowHexNum(1, 8, (uint32_t)DataA, 8);
> 	OLED_ShowHexNum(3, 8, (uint32_t)DataB, 8);
> 		
> 	while (1)
> 	{
> 		DataA[0] ++;		//变换测试数据
> 		DataA[1] ++;
> 		DataA[2] ++;
> 		DataA[3] ++;
> 		
> 		OLED_ShowHexNum(2, 1, DataA[0], 2);		//显示数组DataA
> 		OLED_ShowHexNum(2, 4, DataA[1], 2);
> 		OLED_ShowHexNum(2, 7, DataA[2], 2);
> 		OLED_ShowHexNum(2, 10, DataA[3], 2);
> 		OLED_ShowHexNum(4, 1, DataB[0], 2);		//显示数组DataB
> 		OLED_ShowHexNum(4, 4, DataB[1], 2);
> 		OLED_ShowHexNum(4, 7, DataB[2], 2);
> 		OLED_ShowHexNum(4, 10, DataB[3], 2);
> 		
> 		Delay_ms(1000);		//延时1s，观察转运前的现象
> 		
> 		MyDMA_Transfer();	//使用DMA转运数组，从DataA转运到DataB
> 		
> 		OLED_ShowHexNum(2, 1, DataA[0], 2);		//显示数组DataA
> 		OLED_ShowHexNum(2, 4, DataA[1], 2);
> 		OLED_ShowHexNum(2, 7, DataA[2], 2);
> 		OLED_ShowHexNum(2, 10, DataA[3], 2);
> 		OLED_ShowHexNum(4, 1, DataB[0], 2);		//显示数组DataB
> 		OLED_ShowHexNum(4, 4, DataB[1], 2);
> 		OLED_ShowHexNum(4, 7, DataB[2], 2);
> 		OLED_ShowHexNum(4, 10, DataB[3], 2);
> 
> 		Delay_ms(1000);		//延时1s，观察转运后的现象
> 	}
> }
> ```
>
> 完整文件：[[源码/8-1 DMA数据转运/User/main.c|User/main.c]]


关键参数：

- `DMA_PeripheralBaseAddr`：源/外设地址
- `DMA_MemoryBaseAddr`：目标内存地址
- `DMA_DIR`：传输方向
- `DMA_BufferSize`：传输数量
- `PeripheralInc / MemoryInc`：每次传输后地址是否递增
- `PeripheralDataSize / MemoryDataSize`：数据宽度
- `Mode`：Normal / Circular

## ADC + DMA

多通道 ADC 最经典的数据链：

**模拟输入 → ADC 扫描 Rank → ADC_DR → DMA → ADValue[]**

![[assets/ppt/s107_ADC扫描模式+DMA.jpg]]

*ADC 扫描模式 + DMA：多个通道的转换结果依次搬到数组。*

这里为什么：

**外设地址不自增，内存地址自增？**

因为 ADC 每次结果都出现在同一个 `ADC1->DR`，但我们希望第 1 次放 `ADValue[0]`、第 2 次放 `ADValue[1]`……

所以：

**`PeripheralInc = Disable`**  
**`MemoryInc = Enable`**

连续扫描时再配合 Circular，数组会不断保存最新一轮采样结果。

> [!code]- 视频源码：ADC + DMA AD.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> uint16_t AD_Value[4];					//定义用于存放AD转换结果的全局数组
> 
> /**
>   * 函    数：AD初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void AD_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	//开启ADC1的时钟
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA的时钟
> 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);		//开启DMA1的时钟
> 	
> 	/*设置ADC时钟*/
> 	RCC_ADCCLKConfig(RCC_PCLK2_Div6);						//选择时钟6分频，ADCCLK = 72MHz / 6 = 12MHz
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA0、PA1、PA2和PA3引脚初始化为模拟输入
> 	
> 	/*规则组通道配置*/
> 	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);	//规则组序列1的位置，配置为通道0
> 	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_55Cycles5);	//规则组序列2的位置，配置为通道1
> 	ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_55Cycles5);	//规则组序列3的位置，配置为通道2
> 	ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 4, ADC_SampleTime_55Cycles5);	//规则组序列4的位置，配置为通道3
> 	
> 	/*ADC初始化*/
> 	ADC_InitTypeDef ADC_InitStructure;											//定义结构体变量
> 	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;							//模式，选择独立模式，即单独使用ADC1
> 	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;						//数据对齐，选择右对齐
> 	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;			//外部触发，使用软件触发，不需要外部触发
> 	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;							//连续转换，使能，每转换一次规则组序列后立刻开始下一次转换
> 	ADC_InitStructure.ADC_ScanConvMode = ENABLE;								//扫描模式，使能，扫描规则组的序列，扫描数量由ADC_NbrOfChannel确定
> 	ADC_InitStructure.ADC_NbrOfChannel = 4;										//通道数，为4，扫描规则组的前4个通道
> 	ADC_Init(ADC1, &ADC_InitStructure);											//将结构体变量交给ADC_Init，配置ADC1
> 	
> 	/*DMA初始化*/
> 	DMA_InitTypeDef DMA_InitStructure;											//定义结构体变量
> 	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;				//外设基地址，给定形参AddrA
> 	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	//外设数据宽度，选择半字，对应16为的ADC数据寄存器
> 	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;			//外设地址自增，选择失能，始终以ADC数据寄存器为源
> 	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)AD_Value;					//存储器基地址，给定存放AD转换结果的全局数组AD_Value
> 	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;			//存储器数据宽度，选择半字，与源数据宽度对应
> 	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;						//存储器地址自增，选择使能，每次转运后，数组移到下一个位置
> 	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;							//数据传输方向，选择由外设到存储器，ADC数据寄存器转到数组
> 	DMA_InitStructure.DMA_BufferSize = 4;										//转运的数据大小（转运次数），与ADC通道数一致
> 	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;								//模式，选择循环模式，与ADC的连续转换一致
> 	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;								//存储器到存储器，选择失能，数据由ADC外设触发转运到存储器
> 	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;						//优先级，选择中等
> 	DMA_Init(DMA1_Channel1, &DMA_InitStructure);								//将结构体变量交给DMA_Init，配置DMA1的通道1
> 	
> 	/*DMA和ADC使能*/
> 	DMA_Cmd(DMA1_Channel1, ENABLE);							//DMA1的通道1使能
> 	ADC_DMACmd(ADC1, ENABLE);								//ADC1触发DMA1的信号使能
> 	ADC_Cmd(ADC1, ENABLE);									//ADC1使能
> 	
> 	/*ADC校准*/
> 	ADC_ResetCalibration(ADC1);								//固定流程，内部有电路会自动执行校准
> 	while (ADC_GetResetCalibrationStatus(ADC1) == SET);
> 	ADC_StartCalibration(ADC1);
> 	while (ADC_GetCalibrationStatus(ADC1) == SET);
> 	
> 	/*ADC触发*/
> 	ADC_SoftwareStartConvCmd(ADC1, ENABLE);	//软件触发ADC开始工作，由于ADC处于连续转换模式，故触发一次后ADC就可以一直连续不断地工作
> }
> ```
>
> 完整文件：[[源码/8-2 DMA+AD多通道/Hardware/AD.c|Hardware/AD.c]]

> [!code]- 视频源码：ADC + DMA main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "AD.h"
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();				//OLED初始化
> 	AD_Init();					//AD初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "AD0:");
> 	OLED_ShowString(2, 1, "AD1:");
> 	OLED_ShowString(3, 1, "AD2:");
> 	OLED_ShowString(4, 1, "AD3:");
> 	
> 	while (1)
> 	{
> 		OLED_ShowNum(1, 5, AD_Value[0], 4);		//显示转换结果第0个数据
> 		OLED_ShowNum(2, 5, AD_Value[1], 4);		//显示转换结果第1个数据
> 		OLED_ShowNum(3, 5, AD_Value[2], 4);		//显示转换结果第2个数据
> 		OLED_ShowNum(4, 5, AD_Value[3], 4);		//显示转换结果第3个数据
> 		
> 		Delay_ms(100);							//延时100ms，手动增加一些转换的间隔时间
> 	}
> }
> ```
>
> 完整文件：[[源码/8-2 DMA+AD多通道/User/main.c|User/main.c]]


## 重点

DMA 学会后不要只记“ADC 配 DMA”。真正通用的是：

**谁产生请求 → 固定地址还是递增地址 → 一次传多少 → 搬完后停还是循环**
