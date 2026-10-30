# EXTI

![EXTI 功能框图](https://doc.embedfire.com/mcu/stm32/f103/hal_general/zh/latest/_images/image116.png)

*图源：野火 EXTI 章节。红色虚线走向 NVIC，绿色虚线是 Event 路径。*

## CubeMX

配置顺序：

**Pinout → 点击 GPIO → `GPIO_EXTIx` → System Core → GPIO → 选择 Rising/Falling → System Core → NVIC**

例如 PA0：

`PA0 → GPIO_EXTI0`

然后在 NVIC 开：

`EXTI line0 interrupt`

## HAL 配置

```c
GPIO_InitTypeDef g = {0};

g.Pin  = GPIO_PIN_0;
g.Mode = GPIO_MODE_IT_FALLING;
g.Pull = GPIO_PULLUP;

HAL_GPIO_Init(GPIOA, &g);

HAL_NVIC_SetPriority(
    EXTI0_IRQn,
    1,
    0
);

HAL_NVIC_EnableIRQ(
    EXTI0_IRQn
);
```

常见触发：

```c
GPIO_MODE_IT_RISING
GPIO_MODE_IT_FALLING
GPIO_MODE_IT_RISING_FALLING
```

## 中断流程

CubeMX 常生成：

```c
void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(
        GPIO_PIN_0
    );
}
```

用户业务通常写：

```c
void HAL_GPIO_EXTI_Callback(
    uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        // 按键事件
    }
}
```

即：

**GPIO 边沿 → EXTI → NVIC → IRQHandler → HAL Handler → Callback**

## 和标准库对比

标准库：

```c
if (EXTI_GetITStatus(
        EXTI_Line0) == SET)
{
    ...
    EXTI_ClearITPendingBit(
        EXTI_Line0
    );
}
```

HAL 常见 CubeMX 方式：

```c
HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
```

HAL 在内部处理 Pending，再进入 Callback。

> 野火部分旧例程也会直接使用 `__HAL_GPIO_EXTI_GET_IT()` / `__HAL_GPIO_EXTI_CLEAR_IT()`。这和标准库的“自己查标志并清除”非常接近；CubeMX 常见生成方式则更倾向 `HAL_GPIO_EXTI_IRQHandler()` + Callback。

## 主要函数

`HAL_NVIC_SetPriority()`：设置 NVIC 优先级。  
`HAL_NVIC_EnableIRQ()`：使能 IRQ。  
`HAL_NVIC_DisableIRQ()`：关闭 IRQ。  
`HAL_GPIO_EXTI_IRQHandler()`：HAL 的 EXTI 中断处理入口。  
`HAL_GPIO_EXTI_Callback()`：用户回调。
