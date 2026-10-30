# HAL

## 层次

![CMSIS 与库层次](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image01.png)

*图源：野火《STM32 HAL库开发实战指南》“初识 HAL 固件库”。*

HAL 仍然是在操作 STM32 寄存器，只是在寄存器和应用代码之间加入统一驱动层。

可以把关系理解成：

**应用代码 → HAL Driver → CMSIS / 设备定义 → 寄存器 → 硬件**

## Handle

HAL 最明显的新东西是句柄：

```c
UART_HandleTypeDef huart1;
ADC_HandleTypeDef  hadc1;
TIM_HandleTypeDef  htim2;
I2C_HandleTypeDef  hi2c1;
SPI_HandleTypeDef  hspi1;
```

标准库习惯直接传：

```c
USART_Init(USART1, &USART_InitStructure);
```

HAL 则把“哪个外设 + 初始化参数 + 当前状态 + DMA关联”等信息放在 Handle 里：

```c
huart1.Instance = USART1;
huart1.Init.BaudRate = 115200;

HAL_UART_Init(&huart1);
```

因此以后看到：

```c
HAL_UART_Transmit(&huart1, ...);
```

`&huart1` 不只是“USART1 的别名”，而是 HAL 管理 USART1 的整套软件状态。

## 三种调用方式

HAL 外设 API 最常见三类：

| 类型 | 示例 | 特点 |
|---|---|---|
| Blocking | `HAL_UART_Transmit()` | CPU 在函数中等待 |
| IT | `HAL_UART_Transmit_IT()` | 中断推进，函数很快返回 |
| DMA | `HAL_UART_Transmit_DMA()` | DMA 搬数据 |

后面 UART、ADC、I2C、SPI 都按这个规律看。

## MX 函数

CubeMX 生成：

```c
MX_GPIO_Init();
MX_USART1_UART_Init();
MX_TIM2_Init();
```

这些**不是 HAL 库 API**，只是 CubeMX 帮你生成的工程函数。

真正的 HAL API 在里面：

```c
static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    ...
    HAL_UART_Init(&huart1);
}
```

## MSP

常见：

```c
HAL_UART_MspInit()
HAL_ADC_MspInit()
HAL_TIM_Base_MspInit()
HAL_SPI_MspInit()
```

它们通常负责：

**RCC 时钟 → GPIO 复用 → DMA → NVIC**

因此不要看到 `HAL_UART_Init()` 里没有 GPIO 配置，就以为 HAL 少配了一步。

## 返回值

大量 HAL API 返回：

```c
HAL_StatusTypeDef
```

| 返回 | 含义 |
|---|---|
| `HAL_OK` | 成功 |
| `HAL_ERROR` | 错误 |
| `HAL_BUSY` | 当前 Handle 正忙 |
| `HAL_TIMEOUT` | 超时 |

## 时间

```c
HAL_Delay(1000);
uint32_t tick = HAL_GetTick();
```

`HAL_Delay()` 是阻塞延时，复杂程序里不能拿它代替任务调度。
