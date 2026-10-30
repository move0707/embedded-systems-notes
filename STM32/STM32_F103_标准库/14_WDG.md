# WDG

看门狗的目的：

**程序正常 → 周期喂狗 → 不复位**  
**程序卡死 → 不能按要求喂狗 → 看门狗超时 → 系统自动复位**

![[assets/ppt/s187_WDG简介.jpg]]

*IWDG / WWDG 简介。*

---

## IWDG

| IWDG 框图 | IWDG 键寄存器 |
|---|---|
| ![[assets/ppt/s188_IWDG框图.jpg]] | ![[assets/ppt/s189_IWDG键寄存器.jpg]] |

| IWDG 超时时间 |
|---|
| ![[assets/ppt/s190_IWDG超时时间.jpg]] |

IWDG 使用独立低速时钟 LSI，所以即使主系统时钟异常，它仍能继续计数。

课程公式：

**`TIWDG = TLSI × PR预分频系数 × (RL + 1)`**  
其中 `TLSI = 1 / FLSI`

初始化顺序：

**写使能 → Prescaler → Reload → 先喂一次 → Enable**

课程配置：

```c
IWDG_SetPrescaler(IWDG_Prescaler_16);
IWDG_SetReload(2499);
```

目标超时约 1000 ms；主循环每 800 ms 喂狗。按键扫描函数故意阻塞，用来模拟程序卡死。

> [!code]- 视频源码：IWDG main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "Key.h"
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();						//OLED初始化
> 	Key_Init();							//按键初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "IWDG TEST");
> 	
> 	/*判断复位信号来源*/
> 	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) == SET)	//如果是独立看门狗复位
> 	{
> 		OLED_ShowString(2, 1, "IWDGRST");			//OLED闪烁IWDGRST字符串
> 		Delay_ms(500);
> 		OLED_ShowString(2, 1, "       ");
> 		Delay_ms(100);
> 		
> 		RCC_ClearFlag();							//清除标志位
> 	}
> 	else											//否则，即为其他复位
> 	{
> 		OLED_ShowString(3, 1, "RST");				//OLED闪烁RST字符串
> 		Delay_ms(500);
> 		OLED_ShowString(3, 1, "   ");
> 		Delay_ms(100);
> 	}
> 	
> 	/*IWDG初始化*/
> 	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);	//独立看门狗写使能
> 	IWDG_SetPrescaler(IWDG_Prescaler_16);			//设置预分频为16
> 	IWDG_SetReload(2499);							//设置重装值为2499，独立看门狗的超时时间为1000ms
> 	IWDG_ReloadCounter();							//重装计数器，喂狗
> 	IWDG_Enable();									//独立看门狗使能
> 	
> 	while (1)
> 	{
> 		Key_GetNum();								//调用阻塞式的按键扫描函数，模拟主循环卡死
> 		
> 		IWDG_ReloadCounter();						//重装计数器，喂狗
> 		
> 		OLED_ShowString(4, 1, "FEED");				//OLED闪烁FEED字符串
> 		Delay_ms(200);								//喂狗间隔为200+600=800ms
> 		OLED_ShowString(4, 1, "    ");
> 		Delay_ms(600);
> 	}
> }
> ```
>
> 完整文件：[[源码/14-1 独立看门狗/User/main.c|User/main.c]]


### 关键

喂狗位置必须真正代表“系统整体运行正常”。

如果某个永远能运行的中断一直喂狗，而主循环已经死了，IWDG 也不会复位。

---

## WWDG

| WWDG 框图 | WWDG 工作特性 |
|---|---|
| ![[assets/ppt/s191_WWDG框图.jpg]] | ![[assets/ppt/s192_WWDG工作特性.jpg]] |

| WWDG 超时时间 |
|---|
| ![[assets/ppt/s193_WWDG超时时间.jpg]] |

WWDG 不仅限制“不能太晚”，还限制“不能太早”。

计数器从高值向下减：

- 还没进入窗口就喂：太早 → 复位
- 落入允许窗口：可以喂
- 减到下限还没喂：太晚 → 复位

课程公式：

**`TWWDG = TPCLK1 × 4096 × WDGTB × (T[5:0] + 1)`**

窗口时间：

**`TWIN = TPCLK1 × 4096 × WDGTB × (T[5:0] - W[5:0])`**

课程设置允许窗口，并让主循环约 40 ms 喂一次；按键阻塞则模拟程序异常。

> [!code]- 视频源码：WWDG main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "Key.h"
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();						//OLED初始化
> 	Key_Init();							//按键初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "WWDG TEST");
> 	
> 	/*判断复位信号来源*/
> 	if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) == SET)	//如果是窗口看门狗复位
> 	{
> 		OLED_ShowString(2, 1, "WWDGRST");			//OLED闪烁WWDGRST字符串
> 		Delay_ms(500);
> 		OLED_ShowString(2, 1, "       ");
> 		Delay_ms(100);
> 		
> 		RCC_ClearFlag();							//清除标志位
> 	}
> 	else											//否则，即为其他复位
> 	{
> 		OLED_ShowString(3, 1, "RST");				//OLED闪烁RST字符串
> 		Delay_ms(500);
> 		OLED_ShowString(3, 1, "   ");
> 		Delay_ms(100);
> 	}
> 	
> 	/*开启时钟*/
> 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE);	//开启WWDG的时钟
> 	
> 	/*WWDG初始化*/
> 	WWDG_SetPrescaler(WWDG_Prescaler_8);			//设置预分频为8
> 	WWDG_SetWindowValue(0x40 | 21);					//设置窗口值，窗口时间为30ms
> 	WWDG_Enable(0x40 | 54);							//使能并第一次喂狗，超时时间为50ms
> 	
> 	while (1)
> 	{
> 		Key_GetNum();								//调用阻塞式的按键扫描函数，模拟主循环卡死
> 		
> 		OLED_ShowString(4, 1, "FEED");				//OLED闪烁FEED字符串
> 		Delay_ms(20);								//喂狗间隔为20+20=40ms
> 		OLED_ShowString(4, 1, "    ");
> 		Delay_ms(20);
> 		
> 		WWDG_SetCounter(0x40 | 54);					//重装计数器，喂狗
> 	}
> }
> ```
>
> 完整文件：[[源码/14-2 窗口看门狗/User/main.c|User/main.c]]


---

## 对比

![[assets/ppt/s194_IWDG和WWDG对比.jpg]]

*课程 IWDG / WWDG 对比。*

| | IWDG | WWDG |
|---|---|---|
| 时钟 | 独立 LSI | APB1/PCLK1 |
| 重点 | 不能太晚喂 | 不能太早，也不能太晚 |
| 适合 | 防程序彻底卡死 | 检查程序执行节奏 |
| 停止/异常时钟 | 独立性更强 | 依赖系统时钟 |

## 速记

**IWDG：只管“多久没喂”。**  
**WWDG：还管“喂得是不是太早”。**
