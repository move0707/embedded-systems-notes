# TIM

![高级定时器功能框图](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image215.png)

*图源：野火“TIM—高级定时器”。图中可以同时看到时钟源、控制器、时基、输入捕获、输出比较、Break。*

## 三类定时器

F103 里不要把所有 TIM 看成完全一样：

| 类型 | 典型 | 重点 |
|---|---|---|
| 基本 | TIM6/7（具体芯片看资源） | 计数、更新、TRGO |
| 通用 | TIM2/3/4/5 | PWM、输入捕获、编码器 |
| 高级 | TIM1/8 | 再加互补输出、死区、Break、RCR |

野火高级定时器章节重点指出：TIM1/TIM8 比通用定时器多 **重复计数器 RCR、互补输出、死区、刹车/Break**。

## CubeMX：定时中断

路径：

**Timers → TIM2**

配置：

`Clock Source → Internal Clock`

然后：

**Parameter Settings**

主要参数：

- Prescaler；
- Counter Period；
- Counter Mode；
- Clock Division。

最后：

**NVIC Settings → TIM2 global interrupt**

## 计数公式

```text
CK_CNT = TIM_CLK / (PSC + 1)

更新频率 =
TIM_CLK / (PSC + 1) / (ARR + 1)
```

F103 要注意 APB Timer ×2 规则。

## HAL：基本定时器

```c
HAL_TIM_Base_Start(&htim2);
```

带更新中断：

```c
HAL_TIM_Base_Start_IT(&htim2);
```

停止：

```c
HAL_TIM_Base_Stop(&htim2);
HAL_TIM_Base_Stop_IT(&htim2);
```

IRQ：

```c
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);
}
```

Callback：

```c
void HAL_TIM_PeriodElapsedCallback(
    TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        // 周期事件
    }
}
```

## CubeMX：PWM

**Timers → TIMx → Channelx → PWM Generation CHx**

再到：

**Parameter Settings**

配置 PSC / Period / Pulse / Polarity。

启动：

```c
HAL_TIM_PWM_Start(
    &htim2,
    TIM_CHANNEL_1
);
```

改 CCR：

```c
__HAL_TIM_SET_COMPARE(
    &htim2,
    TIM_CHANNEL_1,
    compare
);
```

### 标准库

```c
TIM_OC1Init()
TIM_SetCompare1()
TIM_Cmd()
```

### HAL

```c
HAL_TIM_PWM_ConfigChannel()
HAL_TIM_PWM_Start()
__HAL_TIM_SET_COMPARE()
```

## CubeMX：输入捕获

**Timers → TIMx → Channelx → Input Capture direct mode**

再配置：

- Polarity；
- Prescaler；
- Filter；
- NVIC。

启动：

```c
HAL_TIM_IC_Start_IT(
    &htim2,
    TIM_CHANNEL_1
);
```

读取 CCR：

```c
HAL_TIM_ReadCapturedValue(
    &htim2,
    TIM_CHANNEL_1
);
```

回调：

```c
void HAL_TIM_IC_CaptureCallback(
    TIM_HandleTypeDef *htim)
{
}
```

## 编码器

CubeMX：

**Timers → TIMx → Combined Channels → Encoder Mode**

启动：

```c
HAL_TIM_Encoder_Start(
    &htim3,
    TIM_CHANNEL_ALL
);
```

读 CNT：

```c
__HAL_TIM_GET_COUNTER(&htim3);
```

清 CNT：

```c
__HAL_TIM_SET_COUNTER(&htim3, 0);
```

## 高级定时器

如果后面做电机控制，TIM1/TIM8 还要认识：

- `HAL_TIMEx_PWMN_Start()`：互补 PWM；
- Dead-Time；
- Break；
- Repetition Counter；
- Main Output Enable。

这些是普通 TIM2/3/4 PWM 没有的重点。

## 对照

| 标准库 | HAL |
|---|---|
| `TIM_TimeBaseInit()` | `HAL_TIM_Base_Init()` |
| `TIM_Cmd()` | `HAL_TIM_Base_Start()` |
| `TIM_ITConfig()` | `HAL_TIM_Base_Start_IT()` |
| `TIM_OCxInit()` | `HAL_TIM_PWM_ConfigChannel()` |
| `TIM_SetComparex()` | `__HAL_TIM_SET_COMPARE()` |
| `TIM_GetCapturex()` | `HAL_TIM_ReadCapturedValue()` |
| `TIM_GetCounter()` | `__HAL_TIM_GET_COUNTER()` |
