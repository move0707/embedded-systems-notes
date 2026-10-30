# FreeRTOS

## 需求

课程实验同时要求：**LED1 每 100 ms 翻转、KEY1 控制 LED2、程序不能因为按键等待而整体卡住。**

![[assets/ppt_048.png|900]]

*PPT：多任务实验要求。*

裸机单个 `while(1)` 里如果调用前一章的 `key_scan()`，等待按键松开的那段时间会阻塞其他逻辑。课程直接利用 ESP-IDF 自带的 FreeRTOS，把两条逻辑拆成两个任务。

**按键任务 / LED 闪烁任务 → FreeRTOS 调度 → 同一 CPU 上交替执行，看起来并行。**

![[assets/ppt_049.png|900]]

*PPT：用 FreeRTOS 多任务实现多个 while 循环并行推进。*

**源码：`v5.1.2/5_Multitask/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led.h"
#include "key.h"

void led_task(void* param)
{
     uint8_t key_num = 0 ;
     while(1)
     {
         key_num = key_scan();
          if(key_num == 1)
          {
               gpio_toggle(GPIO_NUM_39);
          } 
          vTaskDelay(5);
     }
}

void key_task(void *param)
{
     while (1)
     {
          gpio_toggle(GPIO_NUM_38);
          vTaskDelay(100);
     }
}

void app_main(void)
{
    led_init();
    key_init();

    xTaskCreatePinnedToCore(led_task,"led_task",1024,NULL,1,NULL,0);
    xTaskCreatePinnedToCore(key_task,"key_task",1024,NULL,2,NULL,0);
}
```

代码里两个任务分别固定到 Core 0：

`xTaskCreatePinnedToCore(任务函数, 名称, 栈, 参数, 优先级, 句柄, CoreID)`

本工程中 `led_task` 优先级 1，`key_task` 优先级 2。两个任务内部都存在自己的无限循环，但都通过 `vTaskDelay()` 主动让出运行机会。

> [!important]
> 课程这一节只用到任务创建与延时，没有继续讲队列、信号量等 FreeRTOS 专题内容，不在这里额外加课外 API。

**源码工程：`5_Multitask`；PPT 页：48~50。**
