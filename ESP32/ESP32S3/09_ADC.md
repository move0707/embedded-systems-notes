# ADC

## 原理

ESP32-S3 有 SAR ADC1 / ADC2，共 20 路输入通道。PPT按 12 bit 举例：输入电压被量化为 0~4095。课程还专门讲了衰减，因为 SAR ADC 内部参考约 1.1 V，需要通过衰减器把更高的外部电压映射进可转换范围。

![[assets/ppt_087.png|900]]

*PPT：ADC 内部框图。*

![[assets/ppt_088.png|900]]

*PPT：复用器、衰减、SAR、控制器、DMA 的完整工作流程。*

PPT 给出的衰减表：`ADC_ATTEN_DB_11` 对应约 0~3.3 V，`DB_6` 约 0~2.2 V，`DB_2_5` 约 0~1.5 V，`DB_0` 约 0~1.1 V。

## 连续模式

连续转换不是“循环调用 oneshot”。硬件按转换表采样，结果形成帧进入内存，回调从 `conv_frame_buffer` 取数据。

![[assets/ppt_089.png|900]]

*PPT：连续模式内存中的“转换结果 / 转换帧”结构。*

![[assets/ppt_090.png|900]]

*PPT：连续 ADC 配置 API。*

课程源码给两个通道，各 12 bit，转换帧大小 8 字节；回调里从第 0/1 字节和第 4/5 字节取两路结果：

**源码：`v5.1.2/12_ADCContinuous/components/MYADC/myadc.c`**

```c
#include "myadc.h"
#include "hal/adc_types.h"
#include "esp_adc/adc_continuous.h"

adc_continuous_handle_t adc_continuous_handle;
uint8_t *data_value;
uint16_t adc_value_rl;
uint16_t adc_value_rp;

bool continuous_callback(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    data_value = edata->conv_frame_buffer;
    if(edata->size == 8)
    {
        adc_value_rl = ((data_value[1] & 0x0F) << 8) | data_value[0];
        adc_value_rp = ((data_value[5] & 0x0F) << 8) | data_value[4];
        return true;
    }
    return false;
}

void adc_init(void)
{
    adc_continuous_handle_cfg_t adc_contunuous_structure = {
            .conv_frame_size = 8,
            .max_store_buf_size = 1024,
    };
    adc_continuous_new_handle(&adc_contunuous_structure, &adc_continuous_handle);

    adc_digi_pattern_config_t adc_digi_arr[] ={
        {
            .atten = ADC_ATTEN_DB_11,
            .bit_width = ADC_BITWIDTH_12,
            .channel = ADC_CHANNEL_3,
            .unit = ADC_UNIT_1,
        },
        {
            .atten = ADC_ATTEN_DB_11,
            .bit_width = ADC_BITWIDTH_12,
            .channel = ADC_CHANNEL_4,
            .unit = ADC_UNIT_1,
        }
    };

    adc_continuous_config_t continuous_config_structure = {
        .adc_pattern = adc_digi_arr,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
        .pattern_num = 2,
        .sample_freq_hz = 20000,
    };
    adc_continuous_config(adc_continuous_handle, &continuous_config_structure);

    adc_continuous_evt_cbs_t evt_structure = {
        .on_conv_done = continuous_callback,
    };
    adc_continuous_register_event_callbacks(adc_continuous_handle, &evt_structure, NULL);

    adc_continuous_start(adc_continuous_handle);
}
```

主程序再把原始值按课程公式换算成电压并显示：

**源码：`v5.1.2/12_ADCContinuous/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lcd.h"
#include "myadc.h"

void app_main(void)
{
    lcd_init();
    adc_init();

    lcd_show_string(1,1,"adc_rl:",GREEN,BLACK);
    lcd_show_string(2,1,"vol_rl:",GREEN,BLACK);
    lcd_show_string(3,1,"adc_rp:",GREEN,BLACK);
    lcd_show_string(4,1,"vol_rp:",GREEN,BLACK);
    while (1)
    {
        lcd_show_num(1,8,adc_value_rl,4,YELLOW,BLACK);
        lcd_show_float(2,8,(float)adc_value_rl/4095*3.3,3,YELLOW,BLACK);
        lcd_show_num(3,8,adc_value_rp,4,YELLOW,BLACK);
        lcd_show_float(4,8,(float)adc_value_rp/4095*3.3,3,YELLOW,BLACK);
        vTaskDelay(500);
    }
}
```

PPT 的底板原理图中，可调电阻和光敏电阻分别接入 ADC 通道，光敏电阻特点是光越强阻值越小。

![[assets/ppt_093.png|900]]

*PPT：底板 ADC 输入原理图。*

## 单次模式

单次模式的流程只有：创建 ADC Unit → 配通道 → 需要时调用 `adc_oneshot_read()`。

![[assets/ppt_095.png|900]]

*PPT：ADC oneshot 配置流程。*

**源码：`v5.1.2/12_ADC/components/MYADC/myadc.c`**

```c
#include "myadc.h"
#include "esp_adc/adc_oneshot.h"

adc_oneshot_unit_handle_t adc_unit_handle ;

void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t oneshot_structure = {
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&oneshot_structure, &adc_unit_handle);

    adc_oneshot_chan_cfg_t oneshot_chan_structure = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_config_channel(adc_unit_handle, ADC_CHANNEL_3, &oneshot_chan_structure);
    adc_oneshot_config_channel(adc_unit_handle, ADC_CHANNEL_4, &oneshot_chan_structure);
}
```

**源码：`v5.1.2/12_ADC/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "myadc.h"
#include "lcd.h"

int adc1_value = 0 ;
int adc2_value = 0 ;

void app_main(void)
{
    lcd_init();
    adc_init();

    lcd_show_string(1,1,"adc1:",GREEN,BLACK);
    lcd_show_string(2,1,"adc2:",GREEN,BLACK);
    while (1)
    {
        adc_oneshot_read(adc_unit_handle, ADC_CHANNEL_3, &adc1_value);
        adc_oneshot_read(adc_unit_handle, ADC_CHANNEL_4, &adc2_value);
        lcd_show_num(1,6,adc1_value,4,YELLOW,BLACK);
        lcd_show_num(2,6,adc2_value,4,YELLOW,BLACK);
        vTaskDelay(500);
    }
}
```

![[assets/ppt_096.png|900]]

*PPT：连续模式与单次模式比较。*

连续模式适合高频、自动持续采样；单次模式配置简单，但每次读都由软件主动发起。

**源码工程：`12_ADCContinuous`、`12_ADC`；PPT 页：82~96。**
