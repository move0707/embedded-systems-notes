# GPIO

## 模式

PPT 把 IO 配置归纳为 6 类：

| 模式 | 能做什么 | 高电平含义 | 低电平含义 |
|---|---|---|---|
| 关闭输入输出 | GPIO 数字功能关闭 | - | - |
| 输入 | 只读引脚 | 读到高 | 读到低 |
| 输出 | 只输出 | 主动约 3.3 V | 主动约 0 V |
| 开漏输出 | 只输出 | 高阻态/释放 | 主动拉低 |
| 开漏输入输出 | 可读可写 | 高阻态/释放 | 主动拉低 |
| 输入输出 | 可读可写 | 主动约 3.3 V | 主动约 0 V |

普通推挽输出能主动输出 3.3 V / 0 V；开漏输出的“高”是高阻态，需要上拉才能得到高电平。

![[assets/ppt_033.png|900]]

*PPT：IO 引脚内部架构。*

ESP-IDF 里课程主要用三个 API：`gpio_config()` 配置、`gpio_set_level()` 输出、`gpio_get_level()` 读取。

## LED

底板 LED 有低电平点亮的接法，因此程序里初始化后先输出 1 表示熄灭。流水灯工程一次配置 38、39、41、42 四个 GPIO：

![[assets/ppt_037.png|900]]

*PPT：LED 两种接法，注意低有效与高有效。*

**源码：`v5.1.2/2_FlowingLED/components/LED/led.c`**

```c
#include "led.h"

void led_init(void)
{
    gpio_config_t gpio_structure = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pin_bit_mask = (1ull<<GPIO_NUM_38)|(1ull<<GPIO_NUM_39)|(1ull<<GPIO_NUM_41)|(1ull<<GPIO_NUM_42),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&gpio_structure);

    gpio_set_level(GPIO_NUM_38,1);
    gpio_set_level(GPIO_NUM_39,1);
    gpio_set_level(GPIO_NUM_41,1);
    gpio_set_level(GPIO_NUM_42,1);
}

void gpio_toggle(gpio_num_t gpio_num)
{
    if(gpio_get_level(gpio_num) == 1)
    {
        gpio_set_level(gpio_num,0);
    }
    else
    {
        gpio_set_level(gpio_num,1);
    }
}
```

流水灯 `main.c` 逐个把一个 GPIO 拉低、其他保持高：

**源码：`v5.1.2/2_FlowingLED/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"

void app_main(void)
{
    uint8_t i = 0;
    led_init();
    while (1)
    {
        i++;
        if(i == 1)
        {
            gpio_set_level(GPIO_NUM_38,0);
            gpio_set_level(GPIO_NUM_39,1);
            gpio_set_level(GPIO_NUM_41,1);
            gpio_set_level(GPIO_NUM_42,1);
        }
        else if(i == 2)
        {
            gpio_set_level(GPIO_NUM_38,1);
            gpio_set_level(GPIO_NUM_39,0);
            gpio_set_level(GPIO_NUM_41,1);
            gpio_set_level(GPIO_NUM_42,1);
        }
        else if(i == 3)
        {
            gpio_set_level(GPIO_NUM_38,1);
            gpio_set_level(GPIO_NUM_39,1);
            gpio_set_level(GPIO_NUM_41,0);
            gpio_set_level(GPIO_NUM_42,1);
        }
        else if(i == 4)
        {
            gpio_set_level(GPIO_NUM_38,1);
            gpio_set_level(GPIO_NUM_39,1);
            gpio_set_level(GPIO_NUM_41,1);
            gpio_set_level(GPIO_NUM_42,0);
            i = 0;
        }
        vTaskDelay(500);
    }
}
```

## 蜂鸣器

课程区分有源 / 无源蜂鸣器；本实验 GPIO 控制的是板上蜂鸣器驱动电路。PPT同时说明 PNP/NPN 驱动的有效电平可能相反，所以一定看原理图。

![[assets/ppt_043.png|900]]

*PPT：蜂鸣器驱动电路的高/低有效关系。*

**源码：`v5.1.2/3_Beep/components/BEEP/beep.c`**

```c
#include "beep.h"
#include "driver/gpio.h"

void beep_init(void)
{
    gpio_config_t gpio_structure = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pin_bit_mask = 1ull << GPIO_NUM_41,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&gpio_structure);

    gpio_set_level(GPIO_NUM_41,1);
}
```

## 按键

底板按键使用上拉逻辑：**没按 = 高，按下 = 低**。机械触点在按下、松开瞬间会产生抖动，因此课程代码用“延时 → 等待松手 → 再延时”消抖。

![[assets/ppt_046.png|900]]

*PPT：底板按键电路。*

![[assets/ppt_047.png|900]]

*PPT：机械按键抖动。*

**源码：`v5.1.2/4_Key/components/KEY/key.c`**

```c
#include "key.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

void key_init(void)
{
    gpio_config_t gpio_structure = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ull<<GPIO_NUM_9)|(1ull<<GPIO_NUM_10)|(1ull<<GPIO_NUM_11)|(1ull<<GPIO_NUM_12),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&gpio_structure);
}

uint8_t key_scan(void)
{
    uint8_t key_num = 0 ;
    if(gpio_get_level(GPIO_NUM_9) == 0)
    {
        vTaskDelay(20);
        while(gpio_get_level(GPIO_NUM_9) == 0); 
        vTaskDelay(20);
        key_num = 1;
    }
    if(gpio_get_level(GPIO_NUM_10) == 0)
    {
        vTaskDelay(20);
        while(gpio_get_level(GPIO_NUM_10) == 0); 
        vTaskDelay(20);
        key_num = 2;
    }
    if(gpio_get_level(GPIO_NUM_11) == 0)
    {
        vTaskDelay(20);
        while(gpio_get_level(GPIO_NUM_11) == 0); 
        vTaskDelay(20);
        key_num = 3;
    }
    if(gpio_get_level(GPIO_NUM_12) == 0)
    {
        vTaskDelay(20);
        while(gpio_get_level(GPIO_NUM_12) == 0); 
        vTaskDelay(20);
        key_num = 4;
    }
    return key_num;
}
```

主程序按 `key_num` 切换对应 LED：

**源码：`v5.1.2/4_Key/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led.h"
#include "key.h"

void app_main(void)
{
    uint8_t key_num = 0 ;
    led_init();
    key_init();

    while (1)
    {
        key_num = key_scan();
        if(key_num == 1)
        {
             gpio_toggle(GPIO_NUM_38);
        }
        else if(key_num == 2)
        {
             gpio_toggle(GPIO_NUM_39);
        }
        else if(key_num == 3)
        {
             gpio_toggle(GPIO_NUM_41);
        }
        else if(key_num == 4)
        {
             gpio_toggle(GPIO_NUM_42);
        }
        vTaskDelay(5);
    }
}
```

注意这个 `key_scan()` 中的 `while(gpio_get_level(...) == 0);` 会一直等到松手，因此会阻塞当前任务。这正是下一节使用 FreeRTOS 多任务解决的背景。

**源码工程：`1_LED`、`2_FlowingLED`、`3_Beep`、`4_Key`；PPT 页：30~47。**
