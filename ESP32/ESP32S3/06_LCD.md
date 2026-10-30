# LCD

LCD 在课程里首先是调试工具：串口适合把日志送到电脑；LCD 可以把数字、状态直接显示在开发板上。

PPT 给出的常用函数：

| 函数 | 作用 |
|---|---|
| `lcd_init()` | 初始化 |
| `lcd_clear(BLACK)` | 清屏 |
| `lcd_show_char(...)` | 字符 |
| `lcd_show_string(...)` | 字符串 |
| `lcd_show_num(...)` | 十进制整数 |
| `lcd_show_hexnum(...)` | 十六进制 |
| `lcd_show_float(...)` | 浮点数 |

![[assets/ppt_060.png|900]]

*PPT：LCD 调试函数。*

视频对应工程的调用方式：

**源码：`v5.1.2/7_LCDDisplay/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lcd.h"


void app_main(void)
{
    lcd_init();
    lcd_clear(BLACK);
    lcd_show_char(1,1,'A',RED,BLACK);
    lcd_show_string(2,1,"HELLOWORLD",BLUE,BLACK);
    lcd_show_num(3,1,12345678,8,YELLOW,BLACK);
    lcd_show_hexnum(4, 1, 0x1234, 4,BROWN,BLACK);
    lcd_show_float(5, 1, 123.45, 5,RED,BLACK);
    while (1)
    {
        vTaskDelay(500);
    }
}
```

后面的 GPTimer、ADC、BH1750、Wi-Fi、时间等实验都继续复用同一套 LCD component。SPI LCD 的底层驱动放到 [[14_LCD_SPI]] 再展开。

**源码工程：`7_LCDDisplay`；PPT 页：58~60。**
