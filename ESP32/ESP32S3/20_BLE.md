# BLE

蓝牙 4.0 以后常分为经典蓝牙与 BLE：经典蓝牙偏持续高带宽（耳机、音箱），BLE 偏低功耗、快速连接、小数据量（传感器、手环、智能家居）。

ESP32-S3 只支持 BLE，不支持经典蓝牙；PPT标注最高 BLE 5.0。Wi-Fi 与蓝牙共用天线，需要分时复用。

## GAP / GATT

**GAP 管发现和连接；GATT 管连接后的数据交换。**

![[assets/ppt_188.png|900]]

*PPT：GAP 负责连接，GATT 负责通信。*

GAP 又分外围设备与中央设备。课程里 ESP32-S3 是外围设备，手机是中央设备：外围设备广播，中央设备扫描并发起连接。

![[assets/ppt_189.png|900]]

*PPT：Peripheral 与 Central。*

![[assets/ppt_190.png|900]]

*PPT：广播与扫描。*

广播前先配置设备名称、长度、发射功率等字段，再 `ble_gap_adv_start()`。

![[assets/ppt_191.png|900]]

*PPT：广播内容。*

## Service / Characteristic

连接成功后进入 GATT。配置文件由 Service 与 Characteristic 组成：Service = UUID + 多个特征；Characteristic = UUID + 属性 + 值。

![[assets/ppt_193.png|900]]

*PPT：服务与特征。*

课程逻辑中 RX 特征由手机写给 ESP32-S3，TX 特征用于 ESP32-S3 给手机读/Notify。

![[assets/ppt_195.png|900]]

*PPT：BLE 完整连接与通信流程。*

## NimBLE

代码结构：初始化 NimBLE → GAP/GATT service 初始化 → 设备名 → 注册 Service/Characteristic → 设置 `sync_cb` → sync 完成后广播 → FreeRTOS 任务中 `nimble_port_run()` 持续处理协议栈事件。

![[assets/ppt_197.png|900]]

*PPT：NimBLE、GAP/GATT、回调和事件监听结构。*

![[assets/ppt_198.png|900]]

*PPT：NimBLE 配置 API 顺序。*

### 三类回调

**`on_sync()`**：不是手机连接回调，而是 NimBLE Host 与 Controller 已完成同步、协议栈可以工作，此时开始广播。  
**`gap_event_handler()`**：连接成功、连接失败、断开等 GAP 状态变化。  
**`gatt_event_handler()`**：手机真正读 / 写 Characteristic 时执行。

课程 BLE component 全部原样：

**源码：`v5.1.2/25_BLE/components/MYBLE/myble.c`**

```c
#include "myble.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#define TAG "NimBLE-Demo"
#define DEVICE_NAME "ESP32S3-NimBLE"
static bool ble_adv_active = false ;
static uint16_t rx_value_handler;
static uint16_t tx_value_handler;

static int gatt_event_handler(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if(ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
    {
        if(attr_handle == rx_value_handler)
        {
            if(ctxt->om->om_data[0] == 0x00)
            {
                gpio_set_level(GPIO_NUM_38,1);
            }
            else if(ctxt->om->om_data[0] == 0x01)
            {
                gpio_set_level(GPIO_NUM_38,0);
            }
            if(ctxt->om->om_data[1] == 0x00)
            {
                gpio_set_level(GPIO_NUM_39,1);
            }
            else if(ctxt->om->om_data[1] == 0x01)
            {
                gpio_set_level(GPIO_NUM_39,0);
            }
        }
    }
    else if(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
    {
        if(attr_handle == tx_value_handler)
        {
            const char *value = "HELLO";
            os_mbuf_append(ctxt->om, value, strlen(value));
        }
    }
    return 0;
}

static int gap_event_handler(struct ble_gap_event *event, void *arg)
{
    if(event->type == BLE_GAP_EVENT_CONNECT)
    {
        if(event->connect.status == 0 )
        {
            ESP_LOGI(TAG,"设备已连接");
            ble_adv_active = false;
        }
        else
        {
            ESP_LOGI(TAG,"连接失败，重新广播");
            if(!ble_adv_active)
            {
                start_advertising();
            }
        }
    }
    else if(event->type == BLE_GAP_EVENT_DISCONNECT)
    {
        ESP_LOGI(TAG,"设备断开连接");
        if(!ble_adv_active)
        {
            start_advertising();
        }
    }
    return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x00FF),
        .characteristics = (struct ble_gatt_chr_def[])
        {
            //ESP32S3接收手机数据
            {
                .uuid = BLE_UUID16_DECLARE(0xFF01),
                .access_cb = gatt_event_handler,
                .flags = BLE_GATT_CHR_F_WRITE,
                .val_handle = &rx_value_handler,
                .arg = NULL,
            },
            //ESP32S3向手机发送数据
            {
                .uuid = BLE_UUID16_DECLARE(0xFF02),
                .access_cb = gatt_event_handler,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &tx_value_handler,
                .arg = NULL,
            },
            { 0 }
        }
    },
    { 0 }
};

void start_advertising(void)
{
    struct ble_hs_adv_fields fields = {0};
    fields.name = (uint8_t *)DEVICE_NAME;
    fields.name_len = strlen(DEVICE_NAME);
    fields.name_is_complete = 1;
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    int rc = ble_gap_adv_set_fields(&fields);
    if(rc != 0)
    {
        ESP_LOGE(TAG,"广播内容设置失败");
        return;
    }

    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
        .itvl_min = BLE_GAP_ADV_ITVL_MS(200),
        .itvl_max = BLE_GAP_ADV_ITVL_MS(500),
    };
    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL,BLE_HS_FOREVER,&adv_params,gap_event_handler, NULL);
    if(rc != 0)
    {
        ESP_LOGE(TAG,"广播启动失败");
    }
    else
    {
        ESP_LOGI(TAG,"广播已启动，等待连接...");
        ble_adv_active = true;
    }
}

static void on_sync(void)
{
    ESP_LOGI(TAG,"设备初始化完毕");
    start_advertising();
}

void host_task( void * arg)
{
    nimble_port_run();
}

void ble_init(void)
{
    nimble_port_init();
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set(DEVICE_NAME);
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    ble_hs_cfg.sync_cb = on_sync;

    nimble_port_freertos_init(host_task);
}
```

**源码：`v5.1.2/25_BLE/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led.h"
#include "nvs_flash.h"
#include "myble.h"

void app_main(void)
{
    nvs_flash_init();
    led_init();
    ble_init();
}
```

源码里 RX UUID=`0xFF01`（WRITE），TX UUID=`0xFF02`（READ | NOTIFY），Primary Service UUID=`0x00FF`。手机向 RX 写两个字节时分别控制 GPIO38 / GPIO39；读 TX 时返回字符串 `HELLO`。

**源码工程：`25_BLE`；PPT 页：184~198。**
