# RTC

## 时间戳

| UTC / GMT | 时间戳转换 |
|---|---|
| ![[assets/ppt/s168_UTC_GMT.jpg]] | ![[assets/ppt/s170_时间戳转换.jpg]] |

UTC 是全球统一时间基准；本地时间是在 UTC 基础上加时区偏移。课程代码用东八区 `±8*60*60` 做转换。


课程先用 Unix 时间戳统一“时间”的表示方法：

**时间戳 = 从 1970-01-01 00:00:00 UTC 到当前时刻经过的秒数**

| Unix 时间戳 | 时间戳与日期转换 |
|---|---|
| ![[assets/ppt/s167_Unix时间戳.jpg]] | ![[assets/ppt/s169_时间戳转换.jpg]] |

对于 MCU 来说，用一个不断增加的秒计数器比直接维护“年/月/日/时/分/秒”简单；需要显示时再转换。

## BKP

![[assets/ppt/s171_BKP简介.jpg]]

*BKP 在 VDD 断电、系统复位和待机唤醒后仍可由 VBAT 保持。*


![[assets/ppt/s172_BKP基本结构.jpg]]

*BKP 备份寄存器基本结构。*

BKP 属于备份域。主电源掉电后，只要 VBAT 还在，备份域可以继续保留少量数据。

写 BKP 之前要：

**开 PWR 时钟 → 开 BKP 时钟 → `PWR_BackupAccessCmd(ENABLE)`**

课程 12-1 用两个备份寄存器保存测试数据：

> [!code]- 视频源码：BKP 实验 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "Key.h"
> 
> uint8_t KeyNum;					//定义用于接收按键键码的变量
> 
> uint16_t ArrayWrite[] = {0x1234, 0x5678};	//定义要写入数据的测试数组
> uint16_t ArrayRead[2];						//定义要读取数据的测试数组
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();				//OLED初始化
> 	Key_Init();					//按键初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "W:");
> 	OLED_ShowString(2, 1, "R:");
> 	
> 	/*开启时钟*/
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);		//开启PWR的时钟
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);		//开启BKP的时钟
> 	
> 	/*备份寄存器访问使能*/
> 	PWR_BackupAccessCmd(ENABLE);							//使用PWR开启对备份寄存器的访问
> 	
> 	while (1)
> 	{
> 		KeyNum = Key_GetNum();		//获取按键键码
> 		
> 		if (KeyNum == 1)			//按键1按下
> 		{
> 			ArrayWrite[0] ++;		//测试数据自增
> 			ArrayWrite[1] ++;
> 			
> 			BKP_WriteBackupRegister(BKP_DR1, ArrayWrite[0]);	//写入测试数据到备份寄存器
> 			BKP_WriteBackupRegister(BKP_DR2, ArrayWrite[1]);
> 			
> 			OLED_ShowHexNum(1, 3, ArrayWrite[0], 4);		//显示写入的测试数据
> 			OLED_ShowHexNum(1, 8, ArrayWrite[1], 4);
> 		}
> 		
> 		ArrayRead[0] = BKP_ReadBackupRegister(BKP_DR1);		//读取备份寄存器的数据
> 		ArrayRead[1] = BKP_ReadBackupRegister(BKP_DR2);
> 		
> 		OLED_ShowHexNum(2, 3, ArrayRead[0], 4);				//显示读取的备份寄存器数据
> 		OLED_ShowHexNum(2, 8, ArrayRead[1], 4);
> 	}
> }
> ```
>
> 完整文件：[[源码/12-1 读写备份寄存器/User/main.c|User/main.c]]


## RTC

![[assets/ppt/s173_RTC简介.jpg]]

*RTC 是后备域里的独立秒计数器，可选 HSE/128、LSE、LSI 时钟。*


| RTC 框图 | RTC 基本结构 |
|---|---|
| ![[assets/ppt/s174_RTC框图.jpg]] | ![[assets/ppt/s175_RTC基本结构.jpg]] |

STM32F1 RTC 本质是一个 **32 位秒计数器**。只要给它稳定的 1 Hz 时钟，CNT 每秒加 1。

课程使用 LSE 32.768 kHz：

**LSE 32768 Hz → Prescaler 32768-1 → 1 Hz → RTC_CNT 每秒 +1**

## 第一次初始化

RTC 不能每次复位都重设，否则时间永远从初始值开始。

课程使用 BKP_DR1 保存标志：

**BKP_DR1 != 0xA5A5 → 第一次配置 RTC**  
**BKP_DR1 == 0xA5A5 → 已配置，只做同步**

第一次：

**开 LSE → 等 LSERDY → 选 LSE 为 RTCCLK → Enable → 同步 → Prescaler → 设置 CNT → 写 0xA5A5**

以后：

**只同步 RTC，不再重写时间**

## 设置 / 读取

课程直接用 C 库 `time.h`：

**日期结构体 → `mktime()` → 秒计数 → `RTC_SetCounter()`**

读取时：

**`RTC_GetCounter()` → `localtime()` → 年月日时分秒**

代码里还显式做了东八区 ±8 小时调整。

> [!code]- 视频源码：MyRTC.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include <time.h>
> 
> uint16_t MyRTC_Time[] = {2023, 1, 1, 23, 59, 55};	//定义全局的时间数组，数组内容分别为年、月、日、时、分、秒
> 
> void MyRTC_SetTime(void);				//函数声明
> 
> /**
>   * 函    数：RTC初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void MyRTC_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);		//开启PWR的时钟
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);		//开启BKP的时钟
> 	
> 	/*备份寄存器访问使能*/
> 	PWR_BackupAccessCmd(ENABLE);							//使用PWR开启对备份寄存器的访问
> 	
> 	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)			//通过写入备份寄存器的标志位，判断RTC是否是第一次配置
> 															//if成立则执行第一次的RTC配置
> 	{
> 		RCC_LSEConfig(RCC_LSE_ON);							//开启LSE时钟
> 		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != SET);	//等待LSE准备就绪
> 		
> 		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);				//选择RTCCLK来源为LSE
> 		RCC_RTCCLKCmd(ENABLE);								//RTCCLK使能
> 		
> 		RTC_WaitForSynchro();								//等待同步
> 		RTC_WaitForLastTask();								//等待上一次操作完成
> 		
> 		RTC_SetPrescaler(32768 - 1);						//设置RTC预分频器，预分频后的计数频率为1Hz
> 		RTC_WaitForLastTask();								//等待上一次操作完成
> 		
> 		MyRTC_SetTime();									//设置时间，调用此函数，全局数组里时间值刷新到RTC硬件电路
> 		
> 		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);			//在备份寄存器写入自己规定的标志位，用于判断RTC是不是第一次执行配置
> 	}
> 	else													//RTC不是第一次配置
> 	{
> 		RTC_WaitForSynchro();								//等待同步
> 		RTC_WaitForLastTask();								//等待上一次操作完成
> 	}
> }
> 
> //如果LSE无法起振导致程序卡死在初始化函数中
> //可将初始化函数替换为下述代码，使用LSI当作RTCCLK
> //LSI无法由备用电源供电，故主电源掉电时，RTC走时会暂停
> /* 
> void MyRTC_Init(void)
> {
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);
> 	
> 	PWR_BackupAccessCmd(ENABLE);
> 	
> 	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
> 	{
> 		RCC_LSICmd(ENABLE);
> 		while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) != SET);
> 		
> 		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
> 		RCC_RTCCLKCmd(ENABLE);
> 		
> 		RTC_WaitForSynchro();
> 		RTC_WaitForLastTask();
> 		
> 		RTC_SetPrescaler(40000 - 1);
> 		RTC_WaitForLastTask();
> 		
> 		MyRTC_SetTime();
> 		
> 		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
> 	}
> 	else
> 	{
> 		RCC_LSICmd(ENABLE);				//即使不是第一次配置，也需要再次开启LSI时钟
> 		while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) != SET);
> 		
> 		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
> 		RCC_RTCCLKCmd(ENABLE);
> 		
> 		RTC_WaitForSynchro();
> 		RTC_WaitForLastTask();
> 	}
> }*/
> 
> /**
>   * 函    数：RTC设置时间
>   * 参    数：无
>   * 返 回 值：无
>   * 说    明：调用此函数后，全局数组里时间值将刷新到RTC硬件电路
>   */
> void MyRTC_SetTime(void)
> {
> 	time_t time_cnt;		//定义秒计数器数据类型
> 	struct tm time_date;	//定义日期时间数据类型
> 	
> 	time_date.tm_year = MyRTC_Time[0] - 1900;		//将数组的时间赋值给日期时间结构体
> 	time_date.tm_mon = MyRTC_Time[1] - 1;
> 	time_date.tm_mday = MyRTC_Time[2];
> 	time_date.tm_hour = MyRTC_Time[3];
> 	time_date.tm_min = MyRTC_Time[4];
> 	time_date.tm_sec = MyRTC_Time[5];
> 	
> 	time_cnt = mktime(&time_date) - 8 * 60 * 60;	//调用mktime函数，将日期时间转换为秒计数器格式
> 													//- 8 * 60 * 60为东八区的时区调整
> 	
> 	RTC_SetCounter(time_cnt);						//将秒计数器写入到RTC的CNT中
> 	RTC_WaitForLastTask();							//等待上一次操作完成
> }
> 
> /**
>   * 函    数：RTC读取时间
>   * 参    数：无
>   * 返 回 值：无
>   * 说    明：调用此函数后，RTC硬件电路里时间值将刷新到全局数组
>   */
> void MyRTC_ReadTime(void)
> {
> 	time_t time_cnt;		//定义秒计数器数据类型
> 	struct tm time_date;	//定义日期时间数据类型
> 	
> 	time_cnt = RTC_GetCounter() + 8 * 60 * 60;		//读取RTC的CNT，获取当前的秒计数器
> 													//+ 8 * 60 * 60为东八区的时区调整
> 	
> 	time_date = *localtime(&time_cnt);				//使用localtime函数，将秒计数器转换为日期时间格式
> 	
> 	MyRTC_Time[0] = time_date.tm_year + 1900;		//将日期时间结构体赋值给数组的时间
> 	MyRTC_Time[1] = time_date.tm_mon + 1;
> 	MyRTC_Time[2] = time_date.tm_mday;
> 	MyRTC_Time[3] = time_date.tm_hour;
> 	MyRTC_Time[4] = time_date.tm_min;
> 	MyRTC_Time[5] = time_date.tm_sec;
> }
> ```
>
> 完整文件：[[源码/12-2 实时时钟/System/MyRTC.c|System/MyRTC.c]]

> [!code]- 视频源码：RTC main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "MyRTC.h"
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();		//OLED初始化
> 	MyRTC_Init();		//RTC初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "Date:XXXX-XX-XX");
> 	OLED_ShowString(2, 1, "Time:XX:XX:XX");
> 	OLED_ShowString(3, 1, "CNT :");
> 	OLED_ShowString(4, 1, "DIV :");
> 	
> 	while (1)
> 	{
> 		MyRTC_ReadTime();							//RTC读取时间，最新的时间存储到MyRTC_Time数组中
> 		
> 		OLED_ShowNum(1, 6, MyRTC_Time[0], 4);		//显示MyRTC_Time数组中的时间值，年
> 		OLED_ShowNum(1, 11, MyRTC_Time[1], 2);		//月
> 		OLED_ShowNum(1, 14, MyRTC_Time[2], 2);		//日
> 		OLED_ShowNum(2, 6, MyRTC_Time[3], 2);		//时
> 		OLED_ShowNum(2, 9, MyRTC_Time[4], 2);		//分
> 		OLED_ShowNum(2, 12, MyRTC_Time[5], 2);		//秒
> 		
> 		OLED_ShowNum(3, 6, RTC_GetCounter(), 10);	//显示32位的秒计数器
> 		OLED_ShowNum(4, 6, RTC_GetDivider(), 10);	//显示余数寄存器
> 	}
> }
> ```
>
> 完整文件：[[源码/12-2 实时时钟/User/main.c|User/main.c]]


## 硬件

![[assets/ppt/s176_硬件电路.jpg]]

*RTC 实验硬件电路。*

## 注意

![[assets/ppt/s177_RTC操作注意事项.jpg]]

*课程 RTC 操作注意事项。*

最常见问题：

- LSE 不起振，程序卡在等待 `LSERDY`
- 每次启动都重设 CNT，时间无法保持
- 没开 PWR/BKP 时钟或没开放备份域写权限
- 使用 LSI 替代 LSE 后，主电源掉电时 RTC 不再由 VBAT 持续走时

## 关系

**BKP 保存“是否初始化过”等少量标志**  
**RTC 保存“现在走到第几秒”**  
**`time.h` 负责秒数 ↔ 日期格式转换**
