# EXTI

## 中断

中断的基本过程：**主程序运行 → 触发中断源 → CPU 暂停当前位置 → 执行中断服务 → 返回断点继续。** 多个中断同时到来时需要优先级；高优先级还能在低优先级 ISR 中再次打断，形成嵌套。

ESP32-S3 有大量外部中断源，而每个 CPU 的中断通道数量有限，所以使用 **Interrupt Matrix** 把外设中断源映射到 CPU 中断通道。

![[assets/ppt_054.png|900]]

*PPT：ESP32-S3 中断矩阵。*

## 触发

EXTI 可以监测 GPIO 的上升沿、下降沿、双边沿、高电平和低电平。课程按键实验选择 **下降沿**：按键平时上拉为 1，按下后变成 0，所以出现 1→0。

![[assets/ppt_056.png|900]]

*PPT：按键下降沿触发。*

配置流程：`gpio_config()` → `gpio_install_isr_service()` → `gpio_isr_handler_add()` → `gpio_intr_enable()`。

![[assets/ppt_057.png|900]]

*PPT：EXTI 配置流程。*

**源码：`v5.1.2/6_Exti/components/EXTI/exti.c`**

```c
#include "exti.h"
#include "driver/gpio.h"
#include "led.h"

void exti_isr_handler(void *param);

void exti_init(void)
{
    gpio_config_t gpio_structure = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ull << GPIO_NUM_9,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    } ;
    gpio_config(&gpio_structure);

    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);

    gpio_isr_handler_add(GPIO_NUM_9,exti_isr_handler,NULL);

    gpio_intr_enable(GPIO_NUM_9);
}

void exti_isr_handler(void *param)
{
    gpio_toggle(GPIO_NUM_38);
}
```

ISR 里课程只调用 `gpio_toggle(GPIO_NUM_38)` 切 LED。主程序初始化后只保持任务运行：

**源码：`v5.1.2/6_Exti/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "exti.h"

void app_main(void)
{
    led_init();
    exti_init();
    while (1)
    {
        vTaskDelay(10);
    }
}
```

> [!note]
> 这份课程源码没有使用 Queue 把中断事件转给任务，所以笔记不额外改成“ISR → Queue → Task”。这里按视频源码原样记录。

**源码工程：`6_Exti`；PPT 页：51~57。**
