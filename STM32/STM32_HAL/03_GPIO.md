# GPIO

![LED 原理图](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image113.png)

*图源：野火 F103 HAL GPIO 章节。图中 LED 阳极接 3.3V，因此 GPIO 输出低电平时点亮。*

## CubeMX

**Pinout & Configuration → 点击目标引脚**

选择：

- `GPIO_Output`
- `GPIO_Input`
- `GPIO_EXTIx`
- `ADCx_INx`
- 或某个外设复用功能

详细 GPIO 参数：

**System Core → GPIO**

![CubeMX GPIO](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image13.png)

## 初始化

标准库：

```c
GPIO_InitTypeDef g;

g.GPIO_Pin = GPIO_Pin_1;
g.GPIO_Mode = GPIO_Mode_Out_PP;
g.GPIO_Speed = GPIO_Speed_50MHz;

GPIO_Init(GPIOA, &g);
```

HAL：

```c
GPIO_InitTypeDef g = {0};

g.Pin = GPIO_PIN_1;
g.Mode = GPIO_MODE_OUTPUT_PP;
g.Pull = GPIO_NOPULL;
g.Speed = GPIO_SPEED_FREQ_HIGH;

HAL_GPIO_Init(GPIOA, &g);
```

HAL 把“输入模式”和上下拉分开了：

| 标准库 | HAL |
|---|---|
| `GPIO_Mode_IPU` | `GPIO_MODE_INPUT + GPIO_PULLUP` |
| `GPIO_Mode_IPD` | `GPIO_MODE_INPUT + GPIO_PULLDOWN` |
| `GPIO_Mode_AIN` | `GPIO_MODE_ANALOG` |
| `GPIO_Mode_Out_PP` | `GPIO_MODE_OUTPUT_PP` |
| `GPIO_Mode_Out_OD` | `GPIO_MODE_OUTPUT_OD` |
| `GPIO_Mode_AF_PP` | `GPIO_MODE_AF_PP` |

## 时钟

标准库：

```c
RCC_APB2PeriphClockCmd(
    RCC_APB2Periph_GPIOA,
    ENABLE
);
```

HAL：

```c
__HAL_RCC_GPIOA_CLK_ENABLE();
```

CubeMX 会自动把时钟宏生成到初始化代码中。

## 输出

标准库：

```c
GPIO_SetBits(GPIOA, GPIO_Pin_1);
GPIO_ResetBits(GPIOA, GPIO_Pin_1);
```

HAL：

```c
HAL_GPIO_WritePin(
    GPIOA,
    GPIO_PIN_1,
    GPIO_PIN_SET
);

HAL_GPIO_WritePin(
    GPIOA,
    GPIO_PIN_1,
    GPIO_PIN_RESET
);
```

翻转：

```c
HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
```

## 输入

标准库：

```c
GPIO_ReadInputDataBit(
    GPIOB,
    GPIO_Pin_1
);
```

HAL：

```c
GPIO_PinState state =
    HAL_GPIO_ReadPin(
        GPIOB,
        GPIO_PIN_1
    );
```

返回：

`GPIO_PIN_SET / GPIO_PIN_RESET`

## 常用函数

| 函数 | 作用 |
|---|---|
| `HAL_GPIO_Init()` | 初始化 GPIO |
| `HAL_GPIO_DeInit()` | 反初始化 |
| `HAL_GPIO_WritePin()` | 输出高/低 |
| `HAL_GPIO_ReadPin()` | 读取电平 |
| `HAL_GPIO_TogglePin()` | 翻转 |
| `HAL_GPIO_LockPin()` | 锁定配置 |
