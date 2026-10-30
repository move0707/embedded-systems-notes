# LCD + SPI

TFT-LCD 为 240×240，使用 RGB565：一个像素 16 bit，R 5 bit + G 6 bit + B 5 bit。

![[assets/ppt_146.png|900]]

*PPT：240×240 与 RGB565。*

## 引脚

课程底板：SCL/SCK=IO12，SDA/MOSI=IO11，MISO=IO13，RES=IO21，DC=IO47，CS=IO48，BLK=IO40。

![[assets/ppt_147.png|900]]

*PPT：LCD 引脚定义。*

DC=0 表示命令，DC=1 表示数据；RES 低复位；BLK 低关闭背光。

## 命令

课程重点命令：`0x2A` 列地址、`0x2B` 行地址、`0x2C` RAM 写、`0x36` 扫描方向。

![[assets/ppt_148.png|900]]

*PPT：LCD 常用命令。*

初始化就是按照屏幕控制器数据手册，先硬件复位，再连续发送一串命令和参数。

![[assets/ppt_149.png|900]]

*PPT：LCD 初始化的含义。*

底层写命令 / 写数据 / 初始化函数均直接来自源码：

**源码：`v5.1.2/16_SPILCD/components/LCD/lcd.c`（以下函数原样摘录）**

```c
void lcd_write_cmd(uint8_t cmd)
{
    LCD_DC(0);
    spi2_write_data(&cmd , 1);
}

void lcd_write_data(uint8_t data)
{
    LCD_DC(1);
    spi2_write_data(&data , 1);    
}

void lcd_write_data16(uint16_t data)
{
    uint8_t databuf[2] = {0,0};
    databuf[0] = data >> 8;
    databuf[1] = data & 0xFF;
    LCD_DC(1);
    spi2_write_data(databuf , 2); 
}

void lcd_hard_reset(void)
{
    LCD_RST(0);
    vTaskDelay(100);
    LCD_RST(1);
    vTaskDelay(100);
}

void lcd_on(void)
{
    LCD_BLK(1);
    vTaskDelay(10);
}

void lcd_init(void)
{
    spi2_init();

    gpio_config_t gpio_structure = {0};
    gpio_structure.intr_type = GPIO_INTR_DISABLE;
    gpio_structure.mode = GPIO_MODE_OUTPUT;
    gpio_structure.pin_bit_mask = 1ull << GPIO_NUM_21;
    gpio_structure.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_structure.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&gpio_structure);

    gpio_structure.intr_type = GPIO_INTR_DISABLE;
    gpio_structure.mode = GPIO_MODE_OUTPUT;
    gpio_structure.pin_bit_mask = 1ull << GPIO_NUM_47;
    gpio_structure.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_structure.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&gpio_structure);

    gpio_structure.intr_type = GPIO_INTR_DISABLE;
    gpio_structure.mode = GPIO_MODE_OUTPUT;
    gpio_structure.pin_bit_mask = 1ull << GPIO_NUM_40;
    gpio_structure.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_structure.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&gpio_structure);

    lcd_hard_reset();
    lcd_on();
    vTaskDelay(100);

    lcd_write_cmd(0x11);
    vTaskDelay(120);

    lcd_write_cmd(0xB2);
    lcd_write_data(0x0C);
    lcd_write_data(0x0C);
    lcd_write_data(0x00);
    lcd_write_data(0x33);
    lcd_write_data(0x33);

    lcd_write_cmd(0x35);
    lcd_write_data(0x00);

    lcd_write_cmd(0x36);
    lcd_write_data(0x70);
    lcd_write_data(0xA0);

    lcd_write_cmd(0x3A);
    lcd_write_data(0x05);

    lcd_write_cmd(0xB7);
    lcd_write_data(0x35);

    lcd_write_cmd(0xBB);
    lcd_write_data(0x2D);

    lcd_write_cmd(0xC0);
    lcd_write_data(0x2C);

    lcd_write_cmd(0xC2);
    lcd_write_data(0x01);

    lcd_write_cmd(0xC3);
    lcd_write_data(0x15);

    lcd_write_cmd(0xC4);
    lcd_write_data(0x20);

    lcd_write_cmd(0xC6);
    lcd_write_data(0x0F);

    lcd_write_cmd(0xD0);
    lcd_write_data(0xA4);
    lcd_write_data(0xA1);

    lcd_write_cmd(0xD6);
    lcd_write_data(0xA1);

    lcd_write_cmd(0xE0);
    lcd_write_data(0x70);
    lcd_write_data(0x05);
    lcd_write_data(0x0A);
    lcd_write_data(0x0B);
    lcd_write_data(0x0A);
    lcd_write_data(0x27);
    lcd_write_data(0x2F);
    lcd_write_data(0x44);
    lcd_write_data(0x47);
    lcd_write_data(0x37);
    lcd_write_data(0x14);
    lcd_write_data(0x14);
    lcd_write_data(0x29);
    lcd_write_data(0x2F);

    lcd_write_cmd(0xE1);
    lcd_write_data(0x70);
    lcd_write_data(0x07);
    lcd_write_data(0x0C);
    lcd_write_data(0x08);
    lcd_write_data(0x08);
    lcd_write_data(0x04);
    lcd_write_data(0x2F);
    lcd_write_data(0x33);
    lcd_write_data(0x46);
    lcd_write_data(0x18);
    lcd_write_data(0x15);
    lcd_write_data(0x15);
    lcd_write_data(0x2B);
    lcd_write_data(0x2D);

    lcd_write_cmd(0x21);
    lcd_write_cmd(0x29);
    lcd_write_cmd(0x2C);

    lcd_clear(WHITE);
}

void lcd_set_window(uint16_t xstar,uint16_t ystar,uint16_t xend,uint16_t yend)
{
    lcd_write_cmd(0x2a);
    lcd_write_data16(xstar);
    lcd_write_data16(xend);

    lcd_write_cmd(0x2b);
    lcd_write_data16(ystar);
    lcd_write_data16(yend);

    lcd_write_cmd(0x2c);
}

void lcd_clear(uint16_t color)
{
    uint8_t data[2] = {0,0};
    data[0] = color >> 8;
    data[1] = color & 0xFF; 
    lcd_set_window(0,0,239,239);

    for(uint16_t i = 0; i < 240*240/10 ; i ++)
    {
        lcd_buf[2 * i] = data[0];
        lcd_buf[2 * i + 1] = data[1];
    }
    for(uint16_t j = 0 ; j < 10 ; j ++)
    {
        lcd_write_datan(lcd_buf,11520);
    } 
}

void lcd_show_picture(uint8_t *img)
{
    unsigned long i = 0;
    unsigned long j = 0;
    lcd_set_window(0, 0, 239, 239);
    for (j = 0; j < 240 * 240; j++)
    {
        lcd_buf[2 * j] = img[2 * i] ;
        lcd_buf[2 * j + 1] =  img[2 * i + 1];
        i ++;
    }

    for(j = 0; j < (240 * 240 * 2 / 11520); j++)
    {
        lcd_write_datan(&lcd_buf[j * 11520] , 11520);
    }   
}
```

`16_SPILCD/main/main.c` 前面包含一整张 240×240 RGB565 图片数组，文件超过 580 KB；这类像素常量没有重复塞进笔记。其真正的 `app_main()` 原样是：

```c
void app_main(void)
{
    lcd_init();
    lcd_show_picture(gImage_image);
    while (1)
    {
        vTaskDelay(500);
    }
}
```

这里省略的是静态图片数据，不是驱动逻辑；原始数组仍在你上传的 `16_SPILCD/main/main.c` 中。

**源码工程：`16_SPILCD`；PPT 页：145~149。**
