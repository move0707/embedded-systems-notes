# PWM

## 参数

PWM 用周期脉冲的平均效果控制有惯性的负载。PPT 的三个核心量：

**频率 = 1 / Ts；占空比 = Ton / Ts；分辨率 = 占空比可以变化的最小步距。**

LEDC 有 8 个 PWM 通道和 4 个定时器。定时器决定基准频率 / 分辨率，通道选择其中一个定时器，再用 `hpoint` 和 `duty` 形成输出。

![[assets/ppt_074.png|900]]

*PPT：LEDC PWM 控制器架构。*

![[assets/ppt_075.png|900]]

*PPT：LEDC 配置流程。*

PPT 对脉冲的描述是：Timer CNT 从 0 增长，到 overflow 重置；`hpoint` 决定脉冲开始位置，`duty` 决定有效宽度。

![[assets/ppt_076.png|900]]

*PPT：PWM 脉冲生成。*

![[assets/ppt_077.png|900]]

*PPT：频率与分辨率配置。*

## 呼吸灯

课程工程设置 1 kHz、10 bit 分辨率，初始 duty=512：

**源码：`v5.1.2/10_PWM/components/PWM/pwm.c`**

```c
#include "pwm.h"
#include "driver/ledc.h"

void pwm_init(void)
{
    ledc_timer_config_t ledctimer_structure = {
        .clk_cfg = LEDC_AUTO_CLK,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 1000,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
    };
    ledc_timer_config(&ledctimer_structure);

    ledc_channel_config_t   ledcchannel_structure = {
        .channel = LEDC_CHANNEL_1,
        .duty = 512,
        .flags.output_invert = 0,
        .gpio_num = GPIO_NUM_38,
        .hpoint = 0,
        .intr_type = LEDC_INTR_DISABLE,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_1,
    };
    ledc_channel_config(&ledcchannel_structure);
}

void duty_set(uint16_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty); 
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);        
}
```

主程序把 duty 逐渐从 0 增加到 999，再逐渐减小，形成亮度渐变：

**源码：`v5.1.2/10_PWM/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pwm.h"

void app_main(void)
{
    pwm_init();
    while (1)
    {
        for( uint16_t i = 0 ; i < 1000 ; i++ )
        {
            duty_set(i);
            vTaskDelay(1);
        }
        for( uint16_t i = 0 ; i < 1000 ; i++ )
        {
            duty_set(1000 - i);
            vTaskDelay(1);
        }
    }
}
```

## 舵机

SG90 要求约 20 ms 周期（50 Hz），高电平约 0.5~2.5 ms，因此占空比大约 2.5%~12.5%。

![[assets/ppt_080.png|900]]

*PPT：SG90 50 Hz 与 0.5~2.5 ms 高电平。*

![[assets/ppt_081.png|900]]

*PPT：duty 与 angle 映射。*

视频源码把 PWM 改为 50 Hz：

**源码：`v5.1.2/11_PWMServoMotor/components/PWM/pwm.c`**

```c
#include "pwm.h"
#include "driver/ledc.h"

void pwm_init(void)
{
    ledc_timer_config_t ledctimer_structure = {
        .clk_cfg = LEDC_AUTO_CLK,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 50,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
    };
    ledc_timer_config(&ledctimer_structure);

    ledc_channel_config_t   ledcchannel_structure = {
        .channel = LEDC_CHANNEL_1,
        .duty = 25,
        .flags.output_invert = 0,
        .gpio_num = GPIO_NUM_38,
        .hpoint = 0,
        .intr_type = LEDC_INTR_DISABLE,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_1,
    };
    ledc_channel_config(&ledcchannel_structure);
}

void duty_set(uint16_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty); 
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);        
}
```

角度映射函数保持源码原样：

**源码：`v5.1.2/11_PWMServoMotor/components/MOTOR/motor.c`**

```c
#include "motor.h"
#include "pwm.h"
/*
    角度    占空比
     0      0.025
     45     0.05
     90     0.75
     145    0.1
     180    0.125
占空比 = (角度 * 1/1800 +0.025) * 1024
*/
void set_angle(uint8_t angle)
{
    if( angle > 180 )
    {
        angle = 180 ;
    }
    uint16_t duty = ((float)angle / 1800 + 0.025 ) * 1024 ;
    duty_set( duty );
}
```

> [!warning]
> 源码注释中写了“90 → 0.75”，而 PPT 对应位置是 `0.075`，并且下面公式 `angle/1800 + 0.025` 计算 90° 也得到 0.075。这里不替源码改字，只把 PPT 与源码的差异明确标出来。

按键每次加 / 减 45°：

**源码：`v5.1.2/11_PWMServoMotor/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pwm.h"
#include "motor.h"
#include "lcd.h"
#include "key.h"

uint8_t current_angle = 0 ;
uint8_t key_num = 0 ;
void app_main(void)
{
    lcd_init();
    key_init();
    pwm_init();

    set_angle(0);
    lcd_show_string(1,1,"angle:",GREEN,BLACK);
    while (1)
    {  
        key_num = key_scan();
        if(key_num == 1)
        {
            if(current_angle < 180)
            {
                current_angle += 45;
            }
            set_angle(current_angle);
        }
        else if(key_num == 2)
        {
            if(current_angle > 0)
            {
                current_angle -= 45;
            }
            set_angle(current_angle);
        }
        lcd_show_num(1,7,current_angle,3,YELLOW,BLACK);
        vTaskDelay(5);
    }
}
```

**源码工程：`10_PWM`、`11_PWMServoMotor`；PPT 页：70~81。**
