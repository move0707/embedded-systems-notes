# CubeMX

> 野火截图使用的是较早版本 CubeMX，当前版本界面布局会变化，但菜单逻辑基本一致。

## 选芯片

![选择 MCU](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image10.png)

F103C8T6 就搜索对应 Part Number；不要只选一个“差不多的 F103”。

## RCC

路径：

**Pinout & Configuration → System Core → RCC**

![RCC HSE](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image111.png)

外部晶振常选择：

`High Speed Clock (HSE) → Crystal/Ceramic Resonator`

## GPIO

先在 Pinout 搜索引脚：

![搜索 GPIO](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image12.png)

再点引脚选择用途：

![GPIO Output](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image13.png)

例如：

`PB5 → GPIO_Output`

更详细的 Pull、Speed、Output Level、Label 在：

**System Core → GPIO**

## 时钟树

![Clock Configuration](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image14.png)

路径：

**Clock Configuration**

F103 常见目标：

`HSE 8 MHz → PLL ×9 → SYSCLK 72 MHz`

同时注意：

- APB1 最大 36 MHz；
- APB2 可到 72 MHz；
- TIM 时钟在 APB 分频不为 1 时存在 ×2 规则。

## SWD

![SYS Serial Wire](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image16.png)

路径：

**System Core → SYS → Debug → Serial Wire**

一般保留 SWD，避免把调试口完全关掉。

## 工程

![Project Manager](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image17.png)

路径：

**Project Manager**

主要设置：

- Project Name；
- Project Location；
- Toolchain / IDE；
- Code Generator。

## 生成

![Generate Code](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image18.png)

完成配置后：

**GENERATE CODE**

## 生成了什么

常见工程：

```text
Core/
├─ Inc/
│  ├─ main.h
│  ├─ gpio.h
│  ├─ usart.h
│  └─ ...
└─ Src/
   ├─ main.c
   ├─ gpio.c
   ├─ usart.c
   ├─ stm32f1xx_hal_msp.c
   └─ stm32f1xx_it.c

Drivers/
├─ CMSIS/
└─ STM32F1xx_HAL_Driver/

工程.ioc
```

关键文件：

| 文件 | 主要内容 |
|---|---|
| `.ioc` | CubeMX 配置本体 |
| `main.c` | 程序入口 |
| `gpio.c` | `MX_GPIO_Init()` |
| `usart.c` | UART Handle + `MX_USARTx_UART_Init()` |
| `stm32f1xx_hal_msp.c` | RCC / GPIO AF / DMA / NVIC |
| `stm32f1xx_it.c` | 真正的 IRQHandler |

> [!important]
> CubeMX 的价值不是“替你写业务代码”，而是把大量**初始化样板代码**稳定生成出来。业务逻辑仍然写在 USER CODE 区。
