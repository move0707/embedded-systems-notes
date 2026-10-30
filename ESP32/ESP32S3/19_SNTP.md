# SNTP

UTC 是 0 时区统一时间；时间戳表示从 1970-01-01 00:00:00 UTC 到当前时刻的秒数。时区只影响“怎么把时间戳显示成人类时间”，不改变时间戳本身。

![[assets/ppt_181.png|900]]

*PPT：UTC、时区与 Unix 时间戳。*

课程流程：**先 STA 联网 → SNTP 从 NTP 服务器同步 → 设置 TZ → `time()` 取时间戳 → `localtime()` 转年月日时分秒。**

![[assets/ppt_182.png|900]]

*PPT：获取网络时间的三步流程。*

PPT使用的 NTP 服务器包括 `pool.ntp.org`、`cn.pool.ntp.org`、`ntp1.aliyun.com`，源码也是这三个：

**源码：`v5.1.2/24_NTCTime/components/MYNTC/myntc.c`**

```c
#include "myntc.h"
#include "esp_sntp.h"
#include <time.h>


void myntc_init(void)
{
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "cn.pool.ntp.org");
    esp_sntp_setservername(2, "ntp1.aliyun.com");
    esp_sntp_init();
    setenv ("TZ", "CST-8", 1);
    tzset();
}
```

**源码：`v5.1.2/24_NTCTime/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lcd.h"
#include "wifista.h"
#include "nvs_flash.h"
#include "myntc.h"
#include <time.h>


void app_main(void)
{
    time_t timer;
    struct tm * local_time;

    nvs_flash_init();
    lcd_init();
    wifista_init();
    myntc_init();

    lcd_show_string(1,1,"Time:",GREEN,BLACK);
    lcd_show_string(2,1,"    .  .",GREEN,BLACK);
    lcd_show_string(3,1,"  :  :",GREEN,BLACK);
    while (1)
    {
        timer = time (NULL);
        local_time = localtime (&timer);
        lcd_show_num(2,1,local_time->tm_year + 1900,4,YELLOW,BLACK);
        lcd_show_num(2,6,local_time->tm_mon + 1,2,YELLOW,BLACK);
        lcd_show_num(2,9,local_time->tm_mday,2,YELLOW,BLACK);
        lcd_show_num(3,1,local_time->tm_hour,2,YELLOW,BLACK);
        lcd_show_num(3,4,local_time->tm_min,2,YELLOW,BLACK);
        lcd_show_num(3,7,local_time->tm_sec,2,YELLOW,BLACK);
        vTaskDelay(1000);
    }
}
```

源码通过 `setenv("TZ", "CST-8", 1)` + `tzset()` 设置时区，然后每秒在 LCD 显示一次本地时间。

**源码工程：`24_NTCTime`；PPT 页：181~183。**
