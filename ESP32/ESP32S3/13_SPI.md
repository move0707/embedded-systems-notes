# SPI

## 总线

SPI 有 SCK、MOSI、MISO、SS/CS，属于同步、全双工总线。一主多从时 SCK/MOSI/MISO 可以共享，每个从机单独一根 CS。

![[assets/ppt_132.png|900]]

*PPT：一主多从 SPI 硬件连接。*

SPI 的核心不是“发送完再接收”，而是 **每一个 SCK 都同时把主机的一位推出 MOSI、把从机的一位送回 MISO**，所以一次传输本质是数据交换。

![[assets/ppt_133.png|900]]

*PPT：主从移位寄存器的数据交换。*

CS 高→低表示选中并开始一次事务，低→高表示结束。

## 模式

CPOL 决定 SCK 空闲电平；CPHA 决定奇/偶边沿是“移入”还是“移出”。PPT表：

![[assets/ppt_135.png|900]]

*PPT：Mode 0~3 的 CPOL / CPHA 真值表。*

- Mode 0：空闲低，奇数边沿移入，偶数边沿移出。
- Mode 1：空闲低，奇数边沿移出，偶数边沿移入。
- Mode 2：空闲高，奇数边沿移入，偶数边沿移出。
- Mode 3：空闲高，奇数边沿移出，偶数边沿移入。

![[assets/ppt_136.png|900]]

*PPT：Mode 0。*

![[assets/ppt_137.png|900]]

*PPT：Mode 1。*

![[assets/ppt_138.png|900]]

*PPT：Mode 2。*

![[assets/ppt_139.png|900]]

*PPT：Mode 3。*

LCD 使用 Mode 0。

## 外设

ESP32-S3 有 SPI0、SPI1、GP-SPI2、GP-SPI3；课程实验使用 SPI2。控制器把总线配置与“从设备配置”分开：先初始化 bus，再把 LCD/TF 卡等不同从机各自挂到 bus 上。

![[assets/ppt_142.png|900]]

*PPT：SPI 控制器内部架构。*

![[assets/ppt_143.png|900]]

*PPT：SPI Bus + 多从设备句柄。*

![[assets/ppt_144.png|900]]

*PPT：SPI API 配置顺序。*

课程 SPI component 原样：

**源码：`v5.1.2/16_SPILCD/components/SPI/spi.c`**

```c
#include "spi.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

spi_device_handle_t spi2_handle;

void spi2_init(void)
{
    spi_bus_config_t spibus_structure = {
        .flags = SPICOMMON_BUSFLAG_MASTER,
        .intr_flags = 0,
        .isr_cpu_id = INTR_CPU_ID_AUTO,
        .max_transfer_sz = 240*240*2,
        .miso_io_num = GPIO_NUM_13,
        .mosi_io_num = GPIO_NUM_11,
        .quadhd_io_num = -1,
        .quadwp_io_num = -1,
        .sclk_io_num = GPIO_NUM_12,
    };
    spi_bus_initialize(SPI2_HOST, &spibus_structure, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t spidevice_structure = {
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .clock_speed_hz = 60000000,
        .mode = 0,
        .queue_size = 7,
        .spics_io_num = GPIO_NUM_48,
    };
    spi_bus_add_device(SPI2_HOST, &spidevice_structure, &spi2_handle);
}

uint8_t spi2_transfer_byte(uint8_t data)
{
    spi_transaction_t t = {0};
    t.length = data * 8;
    t.tx_data[0] = data;
    spi_device_polling_transmit(spi2_handle, &t);
    return t.rx_data[0];
}
void spi2_write_data(uint8_t *data , uint32_t len)
{
    spi_transaction_t t = {0};
    t.length = len * 8;
    t.tx_buffer = data;
    spi_device_polling_transmit(spi2_handle, &t);
}
```

> [!note]
> 上传源码中的 `spi2_transfer_byte()` 写的是 `t.length = data * 8;`。笔记按源码原样保留；LCD 实际批量发送主要走 `spi2_write_data()`，其 `t.length = len * 8`。

**源码工程：`16_SPILCD`；PPT 页：131~144。**
