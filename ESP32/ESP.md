



## LCD屏幕

| 函数                                                 | 功能             |
| ---------------------------------------------------- | ---------------- |
| `lcd_init()`                                         | 初始化           |
| `lcd_clear(BLACK)`                                   | 清屏             |
| `lcd_show_char(1, 1, 'A', YELLOW, BLACK)`            | 显示一个字符     |
| `lcd_show_string(2, 1, "HELLO WORLD", GREEN, BLACK)` | 显示字符串       |
| `lcd_show_num(3, 1, 12345678, 8, RED, BLACK)`        | 显示十进制数字   |
| `lcd_show_hexnum(4, 1, 0x1234, 4, BLUE, BLACK)`      | 显示十六进制数字 |
| `lcd_show_float(5, 1, 123.45, 5, BROWN, BLACK)`      | 显示浮点数       |

## 通用定时器

1. 使用 `gptimer_handle_t` 定义定时器句柄（给定时器起名）  
2. 使用 `gptimer_new_timer` 函数配置时钟源和计数器  
3. 使用 `gptimer_set_alarm_action` 函数配置比较器的动作  
4. 使用 `gptimer_register_event_callbacks` 函数配置报警事件  
5. 使用 `gptimer_enable` 函数使能通用定时器  
6. 使用 `gptimer_start` 函数开启通用定时器

| ESP32N16R8外设   | XTAL_CLK | APB_CLK | PLL_F160M_CLK | PLL_D2_CLK | RC_FAST_CLK | CRYPTO_PWM_CLK | LEDC_CLK |
| ---------------- | -------- | ------- | ------------- | ---------- | ----------- | -------------- | -------- |
| TIMG             | Y        | Y       |               |            |             |                |          |
| I2S              | Y        |         | Y             | Y          |             |                |          |
| UHCI             |          | Y       |               |            |             |                |          |
| UART             | Y        | Y       |               |            | Y           |                |          |
| RMT              | Y        | Y       |               |            | Y           |                |          |
| PWM              |          |         |               |            |             | Y              |          |
| I2C              | Y        |         |               |            | Y           |                |          |
| SPI              | Y        | Y       |               |            |             |                |          |
| PCNT             |          | Y       |               |            |             |                |          |
| eFuse Controller | Y        | Y       |               |            |             |                |          |
| SARADC           |          | Y       |               | Y          |             |                |          |
| USB              |          | Y       |               |            |             |                |          |
| CRYPTO           |          |         |               |            |             | Y              |          |
| TWAI Controller  |          | Y       |               |            |             |                |          |
| SDIO HOST        | Y        |         | Y             |            |             |                |          |
| LEDC             | Y        | Y       |               |            | Y           |                | Y        |
| LCD_CAM          | Y        |         | Y             | Y          |             |                |          |
| SYS_TIMER        | Y        | Y       |               |            |             |                |          |

## 系统定时器

1. 使用 `esp_timer_handle_t` 定义定时器句柄  

2. 使用 `esp_timer_create` 函数创建一个定时器并配置报警事件  

3. 使用 

   ```
   esp_timer_start_periodic  （周期性）
   ```

    函数配置比较器，并开启定时器  

   - `esp_timer_start_once` （一次）

4. 注意：计数器的计数长度是固定的（1us）

## pwm比较器

```
## PWM 比较器脉冲生成过程
### 核心流程

- `Timer_cnt[13:0]` 是定时器计数值，从 `0` 开始递增。
- 当计数值达到 `overflow` 时，计数器清零并重新开始计数。
- `hpoint` 和 `lpoint` 是两个比较点，用于控制输出波形的高、低电平。

### 输出逻辑

```text
Timer_cnt < hpoint              → 低电平
hpoint < Timer_cnt < lpoint     → 高电平
Timer_cnt > lpoint              → 低电平
Timer_cnt = overflow            → Timer_cnt 重置
```

1. 使用 `ledc_timer_config()` 函数配置时钟源和定时器参数。  
2. 使用 `ledc_channel_config()` 函数配置 PWM 控制器参数。  
3. 使用 `ledc_set_duty()` 函数修改占空比，使用 `ledc_update_duty()` 函数更新占空比。

## ADC

![image-20260908140709809](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260908140709809.png)

- 使用 `adc_continuous_new_handle` 函数配置转换结果存储单元大小、转换帧存储单元大小。
- 使用 `adc_digi_pattern_config_t` 结构体配置转换表。
- 使用 `adc_continuous_config` 函数配置 ADC 控制器需要下发和运行的参数。
- 使用 `adc_continuous_register_event_callbacks` 函数注册回调函数，用于数据读取。
- 使用 `adc_continuous_start` 函数开启连续转换。

![image-20260908141842177](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260908141842177.png)

一个转换帧对应的是这一次的转换的结果



## 串口

![image-20260908154926826](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260908154926826.png)



## IIC

初始化

![image-20260908201905475](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260908201905475.png)

提取如下：

- 使用 `i2c_param_config` 函数配置时钟源、GPIO交换矩阵引脚、I2C参数
- 使用 `i2c_driver_install` 向CPU注册I2C
- 使用 `i2c_cmd_link_create` 函数创建命令寄存器中的命令链（存放命令的数据组）
- 使用 `i2c_master_start` 函数向命令链中存入 `I2C_START` 命令
- 使用 `i2c_master_write_byte` 函数向命令链中存入 `I2C_WRITE` 命令
- 使用 `i2c_master_read_byte` 函数向命令链中存入 `I2C_READ` 命令
- 使用 `i2c_master_stop` 函数向命令链中存入 `I2C_STOP` 命令
- 使用 `i2c_cmd_begin` 函数开启命令控制器，开始 IIC 通信
- 使用 `i2c_cmd_link_delete` 函数删除命令链，释放内存空间



新版6.0以上

- 使用 `i2c_new_master_bus` 函数创建并初始化 I2C Master 总线
- 使用 `i2c_master_bus_add_device` 函数向 I2C 总线添加从设备，并获取设备句柄
- 使用 `i2c_master_transmit` 函数向从设备发送数据
- 使用 `i2c_master_receive` 函数从从设备接收数据
- 使用 `i2c_master_transmit_receive` 函数先向从设备发送数据，再接收数据（常用于读寄存器）
- 使用 `i2c_master_probe` 函数检测指定 I2C 地址上是否存在设备
- 使用 `i2c_master_bus_rm_device` 函数从 I2C 总线上移除设备
- 使用 `i2c_del_master_bus` 函数删除 I2C Master 总线，释放相关资源

## SPI

| SPI模式 | CPOL | CPHA | 时钟空闲状态 | 采样数据的边沿 |
| ------- | ---- | ---- | ------------ | -------------- |
| Mode 0  | 0    | 0    | SCK低电平    | 上升沿采样     |
| Mode 1  | 0    | 1    | SCK低电平    | 下降沿采样     |
| Mode 2  | 1    | 0    | SCK高电平    | 下降沿采样     |
| Mode 3  | 1    | 1    | SCK高电平    | 上升沿采样     |

![image-20260909132852184](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260909132852184.png)

**SPI 的一次传输其实是一次数据交换。**

![image-20260909132633829](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260909132633829.png)

| 引脚                 | 全称                | 方向（以主机为参考） | 作用                           |
| -------------------- | ------------------- | -------------------- | ------------------------------ |
| **SCK / SCLK / CLK** | Serial Clock        | 主机 → 从机          | 时钟线，决定什么时候传一位数据 |
| **MOSI**             | Master Out Slave In | 主机 → 从机          | 主机发送数据给从机             |
| **MISO**             | Master In Slave Out | 从机 → 主机          | 从机返回数据给主机             |
| **CS / SS / NSS**    | Chip Select         | 主机 → 从机          | **选择当前要通信的从设备**     |



**这个是每个设备用一个时钟源**

![image-20260909133918959](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260909133918959.png)

使用 `spi_bus_initialize` 函数配置 SPI 控制器参数和 GPIO 交换矩阵

使用 `spi_bus_add_device` 函数向 SPI 总线添加从设备信息

使用 `spi_device_polling_transmit` 函数进行数据交换

## TF卡

![image-20260909145125584](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260909145125584.png)



**spi下三种模式**

![image-20260909145538578](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260909145538578.png)

**配置TF卡**

`配置映射关系，调用库函数`

![image-20260909151723686](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260909151723686.png)

![image-20260909152346135](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260909152346135.png)



## 摄像头

**原理图**

![image-20260909194210872](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260909194210872.png)



**两种接口**

![image-20260909194309358](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260909194309358.png)

**引脚**

![image-20260909194850358](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260909194850358.png)

**代码编写流程**

`就是调用官方的库，改代码`

![image-20260909200749140](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260909200749140.png)

## Flash分区

![image-20260910192447863](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260910192447863.png)

**自定义划分**

![image-20260910192754893](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260910192754893.png)

**自定义分区流程**

![image-20260910193921484](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260910193921484.png)



## WiFi

**配置流程**

![image-20260910202438258](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260910202438258.png)

**链接服务器**

![image-20260910210357323](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260910210357323.png)

## 蓝牙

![image-20260911104301732](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260911104301732.png)

**代码配置流程**

![image-20260911104727612](C:\Users\86091\AppData\Roaming\Typora\typora-user-images\image-20260911104727612.png)