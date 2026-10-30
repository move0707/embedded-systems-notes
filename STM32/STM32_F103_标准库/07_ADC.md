# ADC

![[assets/ppt/s086_ADC简介.jpg]]

*STM32F103 ADC：12 位逐次逼近、规则组/注入组、外部与内部通道。*


## 原理

ADC 把 GPIO 上连续变化的模拟电压转换成数字量。

STM32F103 的 ADC 是 **12 位逐次逼近型 ADC**，12 位数字量范围为：

**0 ~ 4095**

| 逐次逼近型 ADC                         | ADC 框图                         |
| --------------------------------- | ------------------------------ |
| ![[assets/ppt/s087_逐次逼近型ADC.jpg]] | ![[assets/ppt/s088_ADC框图.jpg]] |
这个逐次逼近就是为了求ADC未知电压的，这里的比较器是物理比较器，虽然你不知道他具体的数值，但是能够比较大小，通过二分法逐次比较来确定未知电压。

| ADC 基本结构 |
|---|
| ![[assets/ppt/s089_ADC基本结构.jpg]] |

逐次逼近可以理解成硬件内部不断“试一个数字 → DAC 生成比较电压 → 比较器判断高低 → 逐位确定结果”。

## 通道

![[assets/ppt/s090_输入通道.jpg]]

*ADC 输入通道与 GPIO / 内部通道。*

GPIO 做 ADC 输入时必须设成 **模拟输入 `GPIO_Mode_AIN`**。

## 规则组

ADC 规则组可以按 Rank 排多个通道。课程把常见转换方式分成单次/连续与扫描/非扫描的组合。
(这里转换完所有通道就给EOC置1）

![[assets/ppt/s091_转换模式.jpg]]

*单次、非扫描模式示意。*

![[Pasted image 20260916160317.png]]

![[Pasted image 20260916160242.png]]

![[Pasted image 20260916160337.png]]
四种模式可以直接用两个开关理解：

**ContinuousConvMode：一轮结束后要不要自动再来一轮**  
**ScanConvMode：一轮里要不要按 Rank 扫多个通道**

理解方式：

- **非扫描**：规则组只真正转换一个指定通道。
- **扫描**：按照 Rank1、Rank2、Rank3……依次转换。
- **单次**：触发一次后做完一轮就停。
- **连续**：一轮做完后自动开始下一轮。

## 触发

![[assets/ppt/s095_触发控制.jpg]]

*ADC 可以由软件或定时器/外部事件触发。*

课程入门实验主要用软件触发：

`ADC_SoftwareStartConvCmd(ADC1, ENABLE);`

## 对齐

![[assets/ppt/s096_数据对齐.jpg]]

*12 位结果可以左对齐或右对齐存入 16 位数据寄存器。*

课程常用右对齐，此时直接读取就是 0~4095。

## 转换时间

![[assets/ppt/s097_转换时间.jpg]]

*ADC 总转换时间 = 采样时间 + 12.5 个 ADC 周期。*

课程给出的公式：

**`TCONV = 采样时间 + 12.5 个 ADC 周期`**

采样时间不是越短越好。模拟源需要时间给 ADC 内部采样保持电容充电，源阻抗较高时过短采样时间可能导致误差。

## 校准

![[assets/ppt/s098_校准.jpg]]

*ADC 校准流程。*

初始化后课程代码会执行复位校准和开始校准，并等待完成。

## 单通道

硬件链：

**PA0 模拟电压 → ADC1 CH0 → 软件触发 → EOC → DR → CPU**

![[assets/ppt/s099_硬件电路.jpg]]

*课程 ADC 实验硬件电路。*

> [!code]- 视频源码：AD 单通道 AD.c
> ```c
> #include "stm32f10x.h"                  // Device header
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
> 	
> 	/*设置ADC时钟*/
> 	RCC_ADCCLKConfig(RCC_PCLK2_Div6);						//选择时钟6分频，ADCCLK = 72MHz / 6 = 12MHz
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA0引脚初始化为模拟输入
> 	
> 	/*规则组通道配置*/
> 	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);		//规则组序列1的位置，配置为通道0
> 	
> 	/*ADC初始化*/
> 	ADC_InitTypeDef ADC_InitStructure;						//定义结构体变量
> 	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;		//模式，选择独立模式，即单独使用ADC1
> 	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//数据对齐，选择右对齐
> 	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//外部触发，使用软件触发，不需要外部触发
> 	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;		//连续转换，失能，每转换一次规则组序列后停止
> 	ADC_InitStructure.ADC_ScanConvMode = DISABLE;			//扫描模式，失能，只转换规则组的序列1这一个位置
> 	ADC_InitStructure.ADC_NbrOfChannel = 1;					//通道数，为1，仅在扫描模式下，才需要指定大于1的数，在非扫描模式下，只能是1
> 	ADC_Init(ADC1, &ADC_InitStructure);						//将结构体变量交给ADC_Init，配置ADC1
> 	
> 	/*ADC使能*/
> 	ADC_Cmd(ADC1, ENABLE);									//使能ADC1，ADC开始运行
> 	
> 	/*ADC校准*/
> 	ADC_ResetCalibration(ADC1);								//固定流程，内部有电路会自动执行校准
> 	while (ADC_GetResetCalibrationStatus(ADC1) == SET);
> 	ADC_StartCalibration(ADC1);
> 	while (ADC_GetCalibrationStatus(ADC1) == SET);
> }
> 
> /**
>   * 函    数：获取AD转换的值
>   * 参    数：无
>   * 返 回 值：AD转换的值，范围：0~4095
>   */
> uint16_t AD_GetValue(void)
> {
> 	ADC_SoftwareStartConvCmd(ADC1, ENABLE);					//软件触发AD转换一次
> 	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);	//等待EOC标志位，即等待AD转换结束
> 	return ADC_GetConversionValue(ADC1);					//读数据寄存器，得到AD转换的结果
> }
> ```
>
> 完整文件：[[源码/7-1 AD单通道/Hardware/AD.c|Hardware/AD.c]]

> [!code]- 视频源码：AD 单通道 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "AD.h"
> 
> uint16_t ADValue;			//定义AD值变量
> float Voltage;				//定义电压变量
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();			//OLED初始化
> 	AD_Init();				//AD初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "ADValue:");
> 	OLED_ShowString(2, 1, "Voltage:0.00V");
> 	
> 	while (1)
> 	{
> 		ADValue = AD_GetValue();					//获取AD转换的值
> 		Voltage = (float)ADValue / 4095 * 3.3;		//将AD值线性变换到0~3.3的范围，表示电压
> 		
> 		OLED_ShowNum(1, 9, ADValue, 4);				//显示AD值
> 		OLED_ShowNum(2, 9, Voltage, 1);				//显示电压值的整数部分
> 		OLED_ShowNum(2, 11, (uint16_t)(Voltage * 100) % 100, 2);	//显示电压值的小数部分
> 		
> 		Delay_ms(100);			//延时100ms，手动增加一些转换的间隔时间
> 	}
> }
> ```
>
> 完整文件：[[源码/7-1 AD单通道/User/main.c|User/main.c]]


核心读数流程就是：

**`ADC_RegularChannelConfig()` → Start → 等 EOC → `ADC_GetConversionValue()`**

## 多通道

多通道实验仍然可以软件逐个切换通道读取：

> [!code]- 视频源码：AD 多通道 AD.c
> ```c
> #include "stm32f10x.h"                  // Device header
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
> 	/*不在此处配置规则组序列，而是在每次AD转换前配置，这样可以灵活更改AD转换的通道*/
> 	
> 	/*ADC初始化*/
> 	ADC_InitTypeDef ADC_InitStructure;						//定义结构体变量
> 	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;		//模式，选择独立模式，即单独使用ADC1
> 	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//数据对齐，选择右对齐
> 	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//外部触发，使用软件触发，不需要外部触发
> 	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;		//连续转换，失能，每转换一次规则组序列后停止
> 	ADC_InitStructure.ADC_ScanConvMode = DISABLE;			//扫描模式，失能，只转换规则组的序列1这一个位置
> 	ADC_InitStructure.ADC_NbrOfChannel = 1;					//通道数，为1，仅在扫描模式下，才需要指定大于1的数，在非扫描模式下，只能是1
> 	ADC_Init(ADC1, &ADC_InitStructure);						//将结构体变量交给ADC_Init，配置ADC1
> 	
> 	/*ADC使能*/
> 	ADC_Cmd(ADC1, ENABLE);									//使能ADC1，ADC开始运行
> 	
> 	/*ADC校准*/
> 	ADC_ResetCalibration(ADC1);								//固定流程，内部有电路会自动执行校准
> 	while (ADC_GetResetCalibrationStatus(ADC1) == SET);
> 	ADC_StartCalibration(ADC1);
> 	while (ADC_GetCalibrationStatus(ADC1) == SET);
> }
> 
> /**
>   * 函    数：获取AD转换的值
>   * 参    数：ADC_Channel 指定AD转换的通道，范围：ADC_Channel_x，其中x可以是0/1/2/3
>   * 返 回 值：AD转换的值，范围：0~4095
>   */
> uint16_t AD_GetValue(uint8_t ADC_Channel)
> {
> 	ADC_RegularChannelConfig(ADC1, ADC_Channel, 1, ADC_SampleTime_55Cycles5);	//在每次转换前，根据函数形参灵活更改规则组的通道1
> 	ADC_SoftwareStartConvCmd(ADC1, ENABLE);					//软件触发AD转换一次
> 	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);	//等待EOC标志位，即等待AD转换结束
> 	return ADC_GetConversionValue(ADC1);					//读数据寄存器，得到AD转换的结果
> }
> ```
>
> 完整文件：[[源码/7-2 AD多通道/Hardware/AD.c|Hardware/AD.c]]

> [!code]- 视频源码：AD 多通道 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "AD.h"
> 
> uint16_t AD0, AD1, AD2, AD3;	//定义AD值变量
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
> 		AD0 = AD_GetValue(ADC_Channel_0);		//单次启动ADC，转换通道0
> 		AD1 = AD_GetValue(ADC_Channel_1);		//单次启动ADC，转换通道1
> 		AD2 = AD_GetValue(ADC_Channel_2);		//单次启动ADC，转换通道2
> 		AD3 = AD_GetValue(ADC_Channel_3);		//单次启动ADC，转换通道3
> 		
> 		OLED_ShowNum(1, 5, AD0, 4);				//显示通道0的转换结果AD0
> 		OLED_ShowNum(2, 5, AD1, 4);				//显示通道1的转换结果AD1
> 		OLED_ShowNum(3, 5, AD2, 4);				//显示通道2的转换结果AD2
> 		OLED_ShowNum(4, 5, AD3, 4);				//显示通道3的转换结果AD3
> 		
> 		Delay_ms(100);			//延时100ms，手动增加一些转换的间隔时间
> 	}
> }
> ```
>
> 完整文件：[[源码/7-2 AD多通道/User/main.c|User/main.c]]


这种方式直观，但 CPU 每次都要重新选通道、触发、等待。下一章 DMA 会让 ADC 扫描和数据搬运自动进行。

## 电压换算

如果参考电压近似 3.3 V、右对齐 12 位：

**`Voltage ≈ ADValue / 4095 × 3.3 V`**

课程重点是理解量化关系；实际高精度测量还会受 VREF、模拟源、采样时间与噪声影响。
