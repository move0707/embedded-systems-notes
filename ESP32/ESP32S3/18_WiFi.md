# Wi-Fi

ESP32-S3 只支持 2.4 GHz Wi-Fi，标准为 802.11 b/g/n；支持 STA、AP、STA+AP。

PPT 还把 Wi-Fi 标准代际放在一起：802.11b/g 只在 2.4 GHz，802.11n 可工作在 2.4/5 GHz，更后的 ac/ax/be 支持更高吞吐；这些是协议背景，不代表 ESP32-S3 本身支持 5 GHz。

## 网络

课程用星型拓扑解释日常路由器网络：各客户端都通过路由器通信。需要分清客户端、服务端、路由器、IP 地址，以及 ESP-IDF 里的 lwIP TCP/IP 协议栈。

![[assets/ppt_176.png|900]]

*PPT：星型网络拓扑。*

![[assets/ppt_177.png|900]]

*PPT：客户端、服务端、路由器、IP、lwIP。*

## 工作流

Wi-Fi 不是一个“connect() 阻塞到成功”的简单函数，而是事件驱动：Wi-Fi 模块产生 STA_START、CONNECTED、DISCONNECTED 等 WIFI_EVENT；网络层取得 IP 后产生 IP_EVENT。

![[assets/ppt_179.png|900]]

*PPT：Wi-Fi 模块、STA/AP、事件循环与 lwIP。*

初始化主链：**NVS → esp_netif → 默认 event loop → 注册事件 → 创建 STA/AP netif → esp_wifi_init → set_mode → set_config → start**。

![[assets/ppt_180.png|900]]

*PPT：ESP32-S3 Wi-Fi 配置流程。*

## STA

源码 `wifista.c` 原样：

**源码：`v5.1.2/22_WIFI_STA/components/WIFISTA/wifista.c`**

```c
#include "wifista.h"
#include "esp_wifi.h"
#include "lcd.h"

void wifista_event_handler(void* event_handler_arg,esp_event_base_t event_base,int32_t event_id,void* event_data)
{
    if(event_base == WIFI_EVENT)
    {
        if(event_id == WIFI_EVENT_STA_START)
        {
            esp_wifi_connect();
        }
        else if(event_id == WIFI_EVENT_STA_CONNECTED)
        {
            lcd_show_string(1,1,"connected   ",YELLOW,BLACK);
        }
        else if(event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            lcd_show_string(1,1,"disconnected",YELLOW,BLACK);
            esp_wifi_stop();
        }
    }
    else if(event_base == IP_EVENT)
    {
        if(event_id == IP_EVENT_STA_GOT_IP)
        {
            esp_netif_ip_info_t *event = (esp_netif_ip_info_t *)event_data;
            lcd_show_num(2,1,esp_ip4_addr1_16(&event->ip),3,GREEN,BLACK);
            lcd_show_string(2,4,".",GREEN,BLACK);
            lcd_show_num(2,5,esp_ip4_addr2_16(&event->ip),3,GREEN,BLACK);
            lcd_show_string(2,8,".",GREEN,BLACK);
            lcd_show_num(2,9,esp_ip4_addr3_16(&event->ip),3,GREEN,BLACK);
            lcd_show_string(2,12,".",GREEN,BLACK);
            lcd_show_num(2,13,esp_ip4_addr4_16(&event->ip),3,GREEN,BLACK);
        }
    }
}

void wifista_init(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&wifista_event_handler,NULL);
    esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&wifista_event_handler,NULL);
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifista_config = {
        .sta = {
            .ssid = DEFAULT_SSID,
            .password = DEFAULT_PWD,
        }
    };
    esp_wifi_set_config(WIFI_IF_STA, &wifista_config);

    esp_wifi_start();
}
```

**源码：`v5.1.2/22_WIFI_STA/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lcd.h"
#include "wifista.h"
#include "nvs_flash.h"


void app_main(void)
{
    nvs_flash_init();
    lcd_init();
    wifista_init();
    while (1)
    {
        vTaskDelay(500);
    }
}
```

STA_START 时调用 `esp_wifi_connect()`；CONNECTED 更新 LCD；GOT_IP 显示 IP。源码在断开事件里调用 `esp_wifi_stop()`，笔记保持原样。

> [!note]
> `DEFAULT_SSID` / `DEFAULT_PWD` 在源码头文件中。笔记不复制其中的具体 Wi-Fi 凭据，只保留 `.c` 里的原始宏引用；你自己的工程直接修改头文件即可。

## AP

AP 模式创建 `esp_netif_create_default_wifi_ap()`，设置 SSID、密码、最大连接数与加密方式；手机连接/断开时收到 `WIFI_EVENT_AP_STACONNECTED` / `...DISCONNECTED`。

**源码：`v5.1.2/23_WIFI_AP/components/WIFIAP/wifiap.c`**

```c
#include "wifiap.h"
#include "esp_wifi.h"
#include <string.h>
#include "lcd.h"

void wifista_event_handler(void* event_handler_arg,esp_event_base_t event_base,int32_t event_id,void* event_data)
{
    if(event_base == WIFI_EVENT)
    {
        if(event_id == WIFI_EVENT_AP_STACONNECTED)
        {
            lcd_show_string(3,5,"connected   ",YELLOW,BLACK);
        }
        else if(event_id == WIFI_EVENT_AP_STADISCONNECTED)
        {
            lcd_show_string(3,5,"disconnected",YELLOW,BLACK);
        }
    }
}

void wifiap_init(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&wifista_event_handler,NULL);
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_set_mode(WIFI_MODE_AP);

    wifi_config_t wifiap_config = {
        .ap = {
            .ssid = DEFAULT_SSID,
            .ssid_len = strlen(DEFAULT_SSID),
            .password = DEFAULT_PWD,
            .max_connection = 2,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        }
    };
    esp_wifi_set_config(ESP_IF_WIFI_AP, &wifiap_config);

    esp_wifi_start();

    lcd_show_string(1,1,"ID:",GREEN,BLACK);
    lcd_show_string(1,4,DEFAULT_SSID,YELLOW,BLACK);
    lcd_show_string(2,1,"PWD:",GREEN,BLACK);
    lcd_show_string(2,5,DEFAULT_PWD,YELLOW,BLACK);
    lcd_show_string(3,1,"STA:",GREEN,BLACK);
}
```

**源码：`v5.1.2/23_WIFI_AP/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lcd.h"
#include "wifiap.h"
#include "nvs_flash.h"

void app_main(void)
{
    nvs_flash_init();
    lcd_init();
    wifiap_init();
    while (1)
    {
        vTaskDelay(500);
    }
}
```

**源码工程：`22_WIFI_STA`、`23_WIFI_AP`；PPT 页：173~180。**
