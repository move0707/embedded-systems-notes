# UART

![串口帧](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image7.jpeg)

![USART 功能框图](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image83.png)

*图源：野火 USART 章节。框图重点看 TDR/RDR、移位寄存器、TX/RX、BRR、控制器。*

## CubeMX

路径：

**Connectivity → USART1**

Mode：

`Asynchronous`

Parameter Settings：

- Baud Rate；
- Word Length；
- Parity；
- Stop Bits；
- Hardware Flow Control。

中断：

**NVIC Settings → USART1 global interrupt**

DMA：

**DMA Settings → Add USART1_RX / USART1_TX**

## 初始化

```c
UART_HandleTypeDef huart1;

huart1.Instance = USART1;
huart1.Init.BaudRate = 115200;
huart1.Init.WordLength = UART_WORDLENGTH_8B;
huart1.Init.StopBits = UART_STOPBITS_1;
huart1.Init.Parity = UART_PARITY_NONE;
huart1.Init.Mode = UART_MODE_TX_RX;

HAL_UART_Init(&huart1);
```

## Blocking

发送：

```c
HAL_UART_Transmit(
    &huart1,
    data,
    len,
    1000
);
```

接收：

```c
HAL_UART_Receive(
    &huart1,
    buf,
    len,
    1000
);
```

最后一个参数是 Timeout，单位 ms。

## IT

```c
HAL_UART_Receive_IT(
    &huart1,
    &rx_byte,
    1
);
```

IRQ：

```c
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}
```

完成：

```c
void HAL_UART_RxCpltCallback(
    UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // 处理 rx_byte

        HAL_UART_Receive_IT(
            &huart1,
            &rx_byte,
            1
        );
    }
}
```

> [!important]
> `HAL_UART_Receive_IT()` 是启动“一次指定长度接收”。收完后如果还要继续，一般要重新启动。

## DMA

```c
HAL_UART_Transmit_DMA(
    &huart1,
    data,
    len
);

HAL_UART_Receive_DMA(
    &huart1,
    rx_buf,
    sizeof(rx_buf)
);
```

## 变长接收

新版本 STM32Cube HAL 中，如果对应 F1 HAL 版本提供 `ReceiveToIdle` 系列，可以用 UART Idle 事件解决变长帧；如果项目库版本没有，就用：

**DMA + IDLE 中断** 或 **单字节 IT + 状态机**。

这是实际项目比固定长度 `HAL_UART_Receive()` 更常见的处理方式。

## 标准库对照

| 标准库 | HAL |
|---|---|
| `USART_Init()` | `HAL_UART_Init()` |
| `USART_SendData()` | `HAL_UART_Transmit()` |
| `USART_ReceiveData()` | `HAL_UART_Receive()` / Callback |
| `USART_ITConfig()` | `HAL_UART_Receive_IT()` |
| 手动查 RXNE/TXE | `HAL_UART_IRQHandler()` 内部处理 |
| DMA 手动联动 | `HAL_UART_xxx_DMA()` |

## TXE / TC

标准库阶段学过的概念并没有消失：

**TXE：发送数据寄存器空，可以装下一个。**  
**TC：移位寄存器也发完了，整个帧真正结束。**

HAL Blocking/IT/DMA 只是帮你封装了这些状态流程。
