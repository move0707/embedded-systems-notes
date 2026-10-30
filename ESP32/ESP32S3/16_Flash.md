# Flash

课程核心板是 N16R8：**PSRAM 8 MB，Flash 16 MB**。RAM 快、掉电丢失；Flash 较慢但掉电保存。

## 分区

16 MB Flash 地址范围是 0x00000000 ~ 0x00FFFFFF。ESP-IDF 不是把整块 Flash 当“一个文件”，而是用 Partition Table 划分 boot、partition table、nvs、phy_init、app、storage 等区域。

![[assets/ppt_161.png|900]]

*PPT：Flash 地址空间与默认分区。*

![[assets/ppt_162.png|900]]

*PPT：分区表格式。*

自定义分区时 app 通常按 64 KB 对齐，data 分区常按 4 KB 对齐。PPT还列出 data 的 fat/ota/phy/nvs/spiffs/coredump 与 app 的 factory/ota/test 等 subtype。

![[assets/ppt_163.png|900]]

*PPT：自定义 factory + storage 分区。*

![[assets/ppt_164.png|900]]

*PPT：Type / SubType。*

你上传源码的 `partitions_spiffs.csv` 原样：

**源码：`v5.1.2/21_SPIFFS/partitions_spiffs.csv`**

```csv
# Name,   Type, SubType, Offset,  Size, Flags
# Note: if you have increased the bootloader size, make sure to update the offsets to avoid overlap,,,,
nvs,      data, nvs,     ,        0x6000,
phy_init, data, phy,     ,        0x1000,
factory,  app,  factory, ,        1M,
storage,data,spiffs, ,        5M,
```

## SPIFFS

课程流程：menuconfig 改成自定义分区表 → 分区编辑 → `esp_vfs_spiffs_register()` 注册文件系统 → 用标准 C 文件 API 读写。

![[assets/ppt_165.png|900]]

*PPT：内部 Flash / SPIFFS 代码流程。*

**源码：`v5.1.2/21_SPIFFS/components/SPIFFS/ffs.c`**

```c
#include "ffs.h"

void spiffs_init(void)
{
    esp_vfs_spiffs_conf_t  spiffs_structure = {
        .base_path = SPIFFS_BASE_PATH,
        .format_if_mount_failed = true,
        .max_files = 2,
        .partition_label = NULL,
    };
    esp_vfs_spiffs_register(&spiffs_structure);
}
```

**源码：`v5.1.2/21_SPIFFS/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ffs.h"
#include "lcd.h"

/*注意：该源码使用时，需要将sdkconfig中的partition设置成partitions_spiffs.csv*/

const char *test_string = "SPIFFS_Test!";
char read_buf[64];

void app_main(void)
{
    lcd_init();
    spiffs_init();
    lcd_show_string(1,1,"read_buf:",GREEN,BLACK);

    FILE* f = fopen("/spiffs/test.txt", "w");
    fprintf(f, test_string);
    fclose(f);

    vTaskDelay(1000);

    f =fopen("/spiffs/test.txt", "r");
    fgets(read_buf,sizeof(read_buf),f);
    fclose(f);

    lcd_show_string(2,1,read_buf,YELLOW,BLACK);

    while(1)
    {
        vTaskDelay(500);
    }
}
```

源码注释明确要求使用 `partitions_spiffs.csv`，否则工程的 storage 分区与代码不匹配。

**源码工程：`21_SPIFFS`；PPT 页：159~165。**
