# IT + DMA + Callback

## 三种模式

以 UART 为例：

```c
HAL_UART_Receive(...)
HAL_UART_Receive_IT(...)
HAL_UART_Receive_DMA(...)
```

Blocking：

**调用 → CPU 在 HAL 函数里等 → 完成/超时 → 返回**

IT：

**调用 `_IT` → 函数返回 → 外设中断 → HAL Handler → Callback**

DMA：

**调用 `_DMA` → DMA 搬数据 → 完成中断 → HAL → Callback**

## IRQHandler 和 Callback

真正硬件中断入口：

```c
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}
```

用户回调：

```c
void HAL_UART_RxCpltCallback(
    UART_HandleTypeDef *huart)
{
}
```

完整链：

**USART 硬件 → NVIC → USART1_IRQHandler → HAL_UART_IRQHandler → HAL_UART_RxCpltCallback**

## weak

很多 Callback 的默认实现带：

```c
__weak
```

用户写一个同名强函数即可覆盖默认实现。

这就是经典 HAL Callback 的来源，不是“神秘地自动运行”。

## Handle 区分实例

```c
if (huart->Instance == USART1)
{
}
```

多个 UART 共用同一个 Callback 时必须这么区分。

## HAL_BUSY

IT / DMA 尚未完成又发起下一笔操作，可能返回：

```c
HAL_BUSY
```

因此异步接口要配合：

- Callback；
- 状态变量；
- 队列/环形缓冲；
- `HAL_xxx_GetState()`。

## Callback 速记

| 外设 | 常见 Callback |
|---|---|
| GPIO | `HAL_GPIO_EXTI_Callback()` |
| TIM | `HAL_TIM_PeriodElapsedCallback()` |
| TIM IC | `HAL_TIM_IC_CaptureCallback()` |
| ADC | `HAL_ADC_ConvCpltCallback()` |
| UART | `HAL_UART_RxCpltCallback()` |
| I2C | `HAL_I2C_MasterRxCpltCallback()` |
| SPI | `HAL_SPI_TxRxCpltCallback()` |
