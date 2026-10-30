# SPI

SPI 这一章和 I2C 最大的思维差别：

**I2C 是半双工、共享一根 SDA；SPI 是全双工，MOSI/MISO 分开。**

## 总线

| SPI 通信 | SPI 硬件连接 |
|---|---|
| ![[assets/ppt/s146_SPI通信.jpg]] | ![[assets/ppt/s147_硬件电路.jpg]] |

常用 4 根线：

- `SCK`：主机输出时钟
- `MOSI`：Master Out Slave In
- `MISO`：Master In Slave Out
- `SS`：Slave Select，主机选择当前从机

多个从机时，SCK/MOSI/MISO 可以共用，但每个从机通常有独立 SS。

## 数据交换

![[assets/ppt/s148_移位示意图.jpg]]

*SPI 两侧移位寄存器。每个时钟沿都会同时移出一位、移入一位。*

SPI 一次传输本质是 **交换**：

**主机发 8 位的同时，也收回 8 位。**

所以即使主机“只想读”，也要发 Dummy Byte 来产生 SCK，让从机有时钟把数据移出来。

---

## SS

![[assets/ppt/s149_SPI时序基本单元.jpg]]

*SS 高→低开始一次事务，低→高结束一次事务。*

和 I2C 的 START/STOP 不同，SPI 不需要通过 SCK/MOSI 组合编码开始结束，直接靠 SS：

**SS=0：选中从机**  
**SS=1：释放从机**

---

## 四种模式

SPI 模式由 CPOL、CPHA 决定。

| Mode | CPOL | CPHA | SCK 空闲 | 第一个边沿 | 第二个边沿 |
|---|---:|---:|---|---|---|
| 0 | 0 | 0 | 低 | 移入/采样 | 移出/更新 |
| 1 | 0 | 1 | 低 | 移出/更新 | 移入/采样 |
| 2 | 1 | 0 | 高 | 移入/采样 | 移出/更新 |
| 3 | 1 | 1 | 高 | 移出/更新 | 移入/采样 |

| Mode 0 | Mode 1 |
|---|---|
| ![[assets/ppt/s150_SPI时序基本单元.jpg]] | ![[assets/ppt/s151_SPI时序基本单元.jpg]] |

| Mode 2 | Mode 3 |
|---|---|
| ![[assets/ppt/s152_SPI时序基本单元.jpg]] | ![[assets/ppt/s153_SPI时序基本单元.jpg]] |

课程 W25Q64 软件 SPI 使用 **Mode 0**：

**CPOL=0 → 空闲 SCK=0**  
**CPHA=0 → 第一个边沿采样**

源码动作：

**先设置 MOSI → SCK 拉高并读取 MISO → SCK 拉低 → 下一位**

---

## 时序

![[assets/ppt/s154_SPI时序.jpg]]

*SPI 完整时序示意。*

I2C 有 ACK；SPI 没有统一 ACK 机制。主机必须按照具体从机芯片协议决定：

**先发什么命令 → 后面跟几字节地址 → 是否需要 Dummy → 读/写多少数据**

---

## W25Q64

| W25Q64 简介 | W25Q64 内部框图 |
|---|---|
| ![[assets/ppt/s157_W25Q64简介.jpg]] | ![[assets/ppt/s159_W25Q64框图.jpg]] |

W25Q64 是 SPI Flash。和 MPU6050 的“寄存器地址 + 数据”不同，它更像：

**指令 → 地址 → 数据**

课程常用：

| 功能 | 典型动作 |
|---|---|
| 写使能 | 先发 Write Enable 指令 |
| 读状态 | 查询 BUSY / WEL 等状态 |
| 读数据 | Read Data + 24 位地址 + 连续接收 |
| 页编程 | Page Program + 地址 + 数据 |
| 扇区擦除 | Sector Erase + 地址 |
| 读 ID | JEDEC / Manufacturer ID 指令 |

## Flash 规则

![[assets/ppt/s160_Flash操作注意事项.jpg]]

*课程给出的 W25Q64 Flash 操作注意事项。*

必须记住：

1. **写/擦之前先 Write Enable**
2. 数据位只能直接 **1→0**，不能直接 **0→1**
3. 想把 0 恢复成 1，要先擦除
4. 擦除按最小擦除单元进行
5. 页编程不能盲目跨页
6. 写/擦后进入 BUSY，必须等忙结束
7. 读操作不需要写使能，但 BUSY 时不能正常读

---

## 软件 SPI

课程软件 SPI 引脚：

**PA4=SS　PA5=SCK　PA6=MISO　PA7=MOSI**

GPIO：

- SS/SCK/MOSI：推挽输出
- MISO：上拉输入

模式 0 初始化：

**SS=1，SCK=0**

> [!code]- 视频源码：软件 SPI MySPI.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /*引脚配置层*/
> 
> /**
>   * 函    数：SPI写SS引脚电平
>   * 参    数：BitValue 协议层传入的当前需要写入SS的电平，范围0~1
>   * 返 回 值：无
>   * 注意事项：此函数需要用户实现内容，当BitValue为0时，需要置SS为低电平，当BitValue为1时，需要置SS为高电平
>   */
> void MySPI_W_SS(uint8_t BitValue)
> {
> 	GPIO_WriteBit(GPIOA, GPIO_Pin_4, (BitAction)BitValue);		//根据BitValue，设置SS引脚的电平
> }
> 
> /**
>   * 函    数：SPI写SCK引脚电平
>   * 参    数：BitValue 协议层传入的当前需要写入SCK的电平，范围0~1
>   * 返 回 值：无
>   * 注意事项：此函数需要用户实现内容，当BitValue为0时，需要置SCK为低电平，当BitValue为1时，需要置SCK为高电平
>   */
> void MySPI_W_SCK(uint8_t BitValue)
> {
> 	GPIO_WriteBit(GPIOA, GPIO_Pin_5, (BitAction)BitValue);		//根据BitValue，设置SCK引脚的电平
> }
> 
> /**
>   * 函    数：SPI写MOSI引脚电平
>   * 参    数：BitValue 协议层传入的当前需要写入MOSI的电平，范围0~1
>   * 返 回 值：无
>   * 注意事项：此函数需要用户实现内容，当BitValue为0时，需要置MOSI为低电平，当BitValue为1时，需要置MOSI为高电平
>   */
> void MySPI_W_MOSI(uint8_t BitValue)
> {
> 	GPIO_WriteBit(GPIOA, GPIO_Pin_7, (BitAction)BitValue);		//根据BitValue，设置MOSI引脚的电平，BitValue要实现非0即1的特性
> }
> 
> /**
>   * 函    数：SPI读MISO引脚电平
>   * 参    数：无
>   * 返 回 值：协议层需要得到的当前MISO的电平，范围0~1
>   * 注意事项：此函数需要用户实现内容，当前MISO为低电平时，返回0，当前MISO为高电平时，返回1
>   */
> uint8_t MySPI_R_MISO(void)
> {
> 	return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6);			//读取MISO电平并返回
> }
> 
> /**
>   * 函    数：SPI初始化
>   * 参    数：无
>   * 返 回 值：无
>   * 注意事项：此函数需要用户实现内容，实现SS、SCK、MOSI和MISO引脚的初始化
>   */
> void MySPI_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA的时钟
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA4、PA5和PA7引脚初始化为推挽输出
> 	
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA6引脚初始化为上拉输入
> 	
> 	/*设置默认电平*/
> 	MySPI_W_SS(1);											//SS默认高电平
> 	MySPI_W_SCK(0);											//SCK默认低电平
> }
> 
> /*协议层*/
> 
> /**
>   * 函    数：SPI起始
>   * 参    数：无
>   * 返 回 值：无
>   */
> void MySPI_Start(void)
> {
> 	MySPI_W_SS(0);				//拉低SS，开始时序
> }
> 
> /**
>   * 函    数：SPI终止
>   * 参    数：无
>   * 返 回 值：无
>   */
> void MySPI_Stop(void)
> {
> 	MySPI_W_SS(1);				//拉高SS，终止时序
> }
> 
> /**
>   * 函    数：SPI交换传输一个字节，使用SPI模式0
>   * 参    数：ByteSend 要发送的一个字节
>   * 返 回 值：接收的一个字节
>   */
> uint8_t MySPI_SwapByte(uint8_t ByteSend)
> {
> 	uint8_t i, ByteReceive = 0x00;					//定义接收的数据，并赋初值0x00，此处必须赋初值0x00，后面会用到
> 	
> 	for (i = 0; i < 8; i ++)						//循环8次，依次交换每一位数据
> 	{
> 		/*两个!可以对数据进行两次逻辑取反，作用是把非0值统一转换为1，即：!!(0) = 0，!!(非0) = 1*/
> 		MySPI_W_MOSI(!!(ByteSend & (0x80 >> i)));	//使用掩码的方式取出ByteSend的指定一位数据并写入到MOSI线
> 		MySPI_W_SCK(1);								//拉高SCK，上升沿移出数据
> 		if (MySPI_R_MISO()){ByteReceive |= (0x80 >> i);}	//读取MISO数据，并存储到Byte变量
> 															//当MISO为1时，置变量指定位为1，当MISO为0时，不做处理，指定位为默认的初值0
> 		MySPI_W_SCK(0);								//拉低SCK，下降沿移入数据
> 	}
> 	
> 	return ByteReceive;								//返回接收到的一个字节数据
> }
> ```
>
> 完整文件：[[源码/11-1 软件SPI读写W25Q64/Hardware/MySPI.c|Hardware/MySPI.c]]

> [!code]- 视频源码：软件 SPI W25Q64.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "MySPI.h"
> #include "W25Q64_Ins.h"
> 
> /**
>   * 函    数：W25Q64初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void W25Q64_Init(void)
> {
> 	MySPI_Init();					//先初始化底层的SPI
> }
> 
> /**
>   * 函    数：W25Q64读取ID号
>   * 参    数：MID 工厂ID，使用输出参数的形式返回
>   * 参    数：DID 设备ID，使用输出参数的形式返回
>   * 返 回 值：无
>   */
> void W25Q64_ReadID(uint8_t *MID, uint16_t *DID)
> {
> 	MySPI_Start();								//SPI起始
> 	MySPI_SwapByte(W25Q64_JEDEC_ID);			//交换发送读取ID的指令
> 	*MID = MySPI_SwapByte(W25Q64_DUMMY_BYTE);	//交换接收MID，通过输出参数返回
> 	*DID = MySPI_SwapByte(W25Q64_DUMMY_BYTE);	//交换接收DID高8位
> 	*DID <<= 8;									//高8位移到高位
> 	*DID |= MySPI_SwapByte(W25Q64_DUMMY_BYTE);	//或上交换接收DID的低8位，通过输出参数返回
> 	MySPI_Stop();								//SPI终止
> }
> 
> /**
>   * 函    数：W25Q64写使能
>   * 参    数：无
>   * 返 回 值：无
>   */
> void W25Q64_WriteEnable(void)
> {
> 	MySPI_Start();								//SPI起始
> 	MySPI_SwapByte(W25Q64_WRITE_ENABLE);		//交换发送写使能的指令
> 	MySPI_Stop();								//SPI终止
> }
> 
> /**
>   * 函    数：W25Q64等待忙
>   * 参    数：无
>   * 返 回 值：无
>   */
> void W25Q64_WaitBusy(void)
> {
> 	uint32_t Timeout;
> 	MySPI_Start();								//SPI起始
> 	MySPI_SwapByte(W25Q64_READ_STATUS_REGISTER_1);				//交换发送读状态寄存器1的指令
> 	Timeout = 100000;							//给定超时计数时间
> 	while ((MySPI_SwapByte(W25Q64_DUMMY_BYTE) & 0x01) == 0x01)	//循环等待忙标志位
> 	{
> 		Timeout --;								//等待时，计数值自减
> 		if (Timeout == 0)						//自减到0后，等待超时
> 		{
> 			/*超时的错误处理代码，可以添加到此处*/
> 			break;								//跳出等待，不等了
> 		}
> 	}
> 	MySPI_Stop();								//SPI终止
> }
> 
> /**
>   * 函    数：W25Q64页编程
>   * 参    数：Address 页编程的起始地址，范围：0x000000~0x7FFFFF
>   * 参    数：DataArray	用于写入数据的数组
>   * 参    数：Count 要写入数据的数量，范围：0~256
>   * 返 回 值：无
>   * 注意事项：写入的地址范围不能跨页
>   */
> void W25Q64_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count)
> {
> 	uint16_t i;
> 	
> 	W25Q64_WriteEnable();						//写使能
> 	
> 	MySPI_Start();								//SPI起始
> 	MySPI_SwapByte(W25Q64_PAGE_PROGRAM);		//交换发送页编程的指令
> 	MySPI_SwapByte(Address >> 16);				//交换发送地址23~16位
> 	MySPI_SwapByte(Address >> 8);				//交换发送地址15~8位
> 	MySPI_SwapByte(Address);					//交换发送地址7~0位
> 	for (i = 0; i < Count; i ++)				//循环Count次
> 	{
> 		MySPI_SwapByte(DataArray[i]);			//依次在起始地址后写入数据
> 	}
> 	MySPI_Stop();								//SPI终止
> 	
> 	W25Q64_WaitBusy();							//等待忙
> }
> 
> /**
>   * 函    数：W25Q64扇区擦除（4KB）
>   * 参    数：Address 指定扇区的地址，范围：0x000000~0x7FFFFF
>   * 返 回 值：无
>   */
> void W25Q64_SectorErase(uint32_t Address)
> {
> 	W25Q64_WriteEnable();						//写使能
> 	
> 	MySPI_Start();								//SPI起始
> 	MySPI_SwapByte(W25Q64_SECTOR_ERASE_4KB);	//交换发送扇区擦除的指令
> 	MySPI_SwapByte(Address >> 16);				//交换发送地址23~16位
> 	MySPI_SwapByte(Address >> 8);				//交换发送地址15~8位
> 	MySPI_SwapByte(Address);					//交换发送地址7~0位
> 	MySPI_Stop();								//SPI终止
> 	
> 	W25Q64_WaitBusy();							//等待忙
> }
> 
> /**
>   * 函    数：W25Q64读取数据
>   * 参    数：Address 读取数据的起始地址，范围：0x000000~0x7FFFFF
>   * 参    数：DataArray 用于接收读取数据的数组，通过输出参数返回
>   * 参    数：Count 要读取数据的数量，范围：0~0x800000
>   * 返 回 值：无
>   */
> void W25Q64_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count)
> {
> 	uint32_t i;
> 	MySPI_Start();								//SPI起始
> 	MySPI_SwapByte(W25Q64_READ_DATA);			//交换发送读取数据的指令
> 	MySPI_SwapByte(Address >> 16);				//交换发送地址23~16位
> 	MySPI_SwapByte(Address >> 8);				//交换发送地址15~8位
> 	MySPI_SwapByte(Address);					//交换发送地址7~0位
> 	for (i = 0; i < Count; i ++)				//循环Count次
> 	{
> 		DataArray[i] = MySPI_SwapByte(W25Q64_DUMMY_BYTE);	//依次在起始地址后读取数据
> 	}
> 	MySPI_Stop();								//SPI终止
> }
> ```
>
> 完整文件：[[源码/11-1 软件SPI读写W25Q64/Hardware/W25Q64.c|Hardware/W25Q64.c]]

> [!code]- 视频源码：软件 SPI main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "W25Q64.h"
> 
> uint8_t MID;							//定义用于存放MID号的变量
> uint16_t DID;							//定义用于存放DID号的变量
> 
> uint8_t ArrayWrite[] = {0x01, 0x02, 0x03, 0x04};	//定义要写入数据的测试数组
> uint8_t ArrayRead[4];								//定义要读取数据的测试数组
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();						//OLED初始化
> 	W25Q64_Init();						//W25Q64初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "MID:   DID:");
> 	OLED_ShowString(2, 1, "W:");
> 	OLED_ShowString(3, 1, "R:");
> 	
> 	/*显示ID号*/
> 	W25Q64_ReadID(&MID, &DID);			//获取W25Q64的ID号
> 	OLED_ShowHexNum(1, 5, MID, 2);		//显示MID
> 	OLED_ShowHexNum(1, 12, DID, 4);		//显示DID
> 	
> 	/*W25Q64功能函数测试*/
> 	W25Q64_SectorErase(0x000000);					//扇区擦除
> 	W25Q64_PageProgram(0x000000, ArrayWrite, 4);	//将写入数据的测试数组写入到W25Q64中
> 	
> 	W25Q64_ReadData(0x000000, ArrayRead, 4);		//读取刚写入的测试数据到读取数据的测试数组中
> 	
> 	/*显示数据*/
> 	OLED_ShowHexNum(2, 3, ArrayWrite[0], 2);		//显示写入数据的测试数组
> 	OLED_ShowHexNum(2, 6, ArrayWrite[1], 2);
> 	OLED_ShowHexNum(2, 9, ArrayWrite[2], 2);
> 	OLED_ShowHexNum(2, 12, ArrayWrite[3], 2);
> 	
> 	OLED_ShowHexNum(3, 3, ArrayRead[0], 2);			//显示读取数据的测试数组
> 	OLED_ShowHexNum(3, 6, ArrayRead[1], 2);
> 	OLED_ShowHexNum(3, 9, ArrayRead[2], 2);
> 	OLED_ShowHexNum(3, 12, ArrayRead[3], 2);
> 	
> 	while (1)
> 	{
> 		
> 	}
> }
> ```
>
> 完整文件：[[源码/11-1 软件SPI读写W25Q64/User/main.c|User/main.c]]


`MySPI_SwapByte()` 的核心是：

**MOSI 放一位 → SCK 上升 → 读取 MISO → SCK 下降**

每次循环完成 1 位，8 次完成 1 字节交换。

---

## 硬件 SPI

| SPI 外设框图 | SPI 基本结构 |
|---|---|
| ![[assets/ppt/s162_SPI框图.jpg]] | ![[assets/ppt/s163_SPI基本结构.jpg]] |

| 主模式全双工连续传输 |
|---|
| ![[assets/ppt/s164_主模式全双工连续传输.jpg]] |

硬件 SPI 后：

- PA5 SCK → 复用推挽
- PA7 MOSI → 复用推挽
- PA6 MISO → 输入
- PA4 SS → 课程仍用普通 GPIO 软件控制

课程配置：

**Master + 2Lines FullDuplex + 8bit + MSB First + Prescaler 128 + CPOL Low + CPHA 1Edge**

这就是 Mode 0。

硬件交换 1 字节：

**等 TXE → 写 DR → 硬件自动产生 8 个时钟 → 等 RXNE → 读 DR**

> [!code]- 视频源码：硬件 SPI MySPI.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /**
>   * 函    数：SPI写SS引脚电平，SS仍由软件模拟
>   * 参    数：BitValue 协议层传入的当前需要写入SS的电平，范围0~1
>   * 返 回 值：无
>   * 注意事项：此函数需要用户实现内容，当BitValue为0时，需要置SS为低电平，当BitValue为1时，需要置SS为高电平
>   */
> void MySPI_W_SS(uint8_t BitValue)
> {
> 	GPIO_WriteBit(GPIOA, GPIO_Pin_4, (BitAction)BitValue);		//根据BitValue，设置SS引脚的电平
> }
> 
> /**
>   * 函    数：SPI初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void MySPI_Init(void)
> {
> 	/*开启时钟*/
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA的时钟
> 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);	//开启SPI1的时钟
> 	
> 	/*GPIO初始化*/
> 	GPIO_InitTypeDef GPIO_InitStructure;
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA4引脚初始化为推挽输出
> 	
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA5和PA7引脚初始化为复用推挽输出
> 	
> 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
> 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
> 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
> 	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA6引脚初始化为上拉输入
> 	
> 	/*SPI初始化*/
> 	SPI_InitTypeDef SPI_InitStructure;						//定义结构体变量
> 	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;			//模式，选择为SPI主模式
> 	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;	//方向，选择2线全双工
> 	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//数据宽度，选择为8位
> 	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;		//先行位，选择高位先行
> 	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;	//波特率分频，选择128分频
> 	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;				//SPI极性，选择低极性
> 	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;			//SPI相位，选择第一个时钟边沿采样，极性和相位决定选择SPI模式0
> 	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;				//NSS，选择由软件控制
> 	SPI_InitStructure.SPI_CRCPolynomial = 7;				//CRC多项式，暂时用不到，给默认值7
> 	SPI_Init(SPI1, &SPI_InitStructure);						//将结构体变量交给SPI_Init，配置SPI1
> 	
> 	/*SPI使能*/
> 	SPI_Cmd(SPI1, ENABLE);									//使能SPI1，开始运行
> 	
> 	/*设置默认电平*/
> 	MySPI_W_SS(1);											//SS默认高电平
> }
> 
> /**
>   * 函    数：SPI起始
>   * 参    数：无
>   * 返 回 值：无
>   */
> void MySPI_Start(void)
> {
> 	MySPI_W_SS(0);				//拉低SS，开始时序
> }
> 
> /**
>   * 函    数：SPI终止
>   * 参    数：无
>   * 返 回 值：无
>   */
> void MySPI_Stop(void)
> {
> 	MySPI_W_SS(1);				//拉高SS，终止时序
> }
> 
> /**
>   * 函    数：SPI交换传输一个字节，使用SPI模式0
>   * 参    数：ByteSend 要发送的一个字节
>   * 返 回 值：接收的一个字节
>   */
> uint8_t MySPI_SwapByte(uint8_t ByteSend)
> {
> 	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != SET);	//等待发送数据寄存器空
> 	
> 	SPI_I2S_SendData(SPI1, ByteSend);								//写入数据到发送数据寄存器，开始产生时序
> 	
> 	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != SET);	//等待接收数据寄存器非空
> 	
> 	return SPI_I2S_ReceiveData(SPI1);								//读取接收到的数据并返回
> }
> ```
>
> 完整文件：[[源码/11-2 硬件SPI读写W25Q64/Hardware/MySPI.c|Hardware/MySPI.c]]

> [!code]- 视频源码：硬件 SPI W25Q64.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "MySPI.h"
> #include "W25Q64_Ins.h"
> 
> /**
>   * 函    数：W25Q64初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void W25Q64_Init(void)
> {
> 	MySPI_Init();					//先初始化底层的SPI
> }
> 
> /**
>   * 函    数：W25Q64读取ID号
>   * 参    数：MID 工厂ID，使用输出参数的形式返回
>   * 参    数：DID 设备ID，使用输出参数的形式返回
>   * 返 回 值：无
>   */
> void W25Q64_ReadID(uint8_t *MID, uint16_t *DID)
> {
> 	MySPI_Start();								//SPI起始
> 	MySPI_SwapByte(W25Q64_JEDEC_ID);			//交换发送读取ID的指令
> 	*MID = MySPI_SwapByte(W25Q64_DUMMY_BYTE);	//交换接收MID，通过输出参数返回
> 	*DID = MySPI_SwapByte(W25Q64_DUMMY_BYTE);	//交换接收DID高8位
> 	*DID <<= 8;									//高8位移到高位
> 	*DID |= MySPI_SwapByte(W25Q64_DUMMY_BYTE);	//或上交换接收DID的低8位，通过输出参数返回
> 	MySPI_Stop();								//SPI终止
> }
> 
> /**
>   * 函    数：W25Q64写使能
>   * 参    数：无
>   * 返 回 值：无
>   */
> void W25Q64_WriteEnable(void)
> {
> 	MySPI_Start();								//SPI起始
> 	MySPI_SwapByte(W25Q64_WRITE_ENABLE);		//交换发送写使能的指令
> 	MySPI_Stop();								//SPI终止
> }
> 
> /**
>   * 函    数：W25Q64等待忙
>   * 参    数：无
>   * 返 回 值：无
>   */
> void W25Q64_WaitBusy(void)
> {
> 	uint32_t Timeout;
> 	MySPI_Start();								//SPI起始
> 	MySPI_SwapByte(W25Q64_READ_STATUS_REGISTER_1);				//交换发送读状态寄存器1的指令
> 	Timeout = 100000;							//给定超时计数时间
> 	while ((MySPI_SwapByte(W25Q64_DUMMY_BYTE) & 0x01) == 0x01)	//循环等待忙标志位
> 	{
> 		Timeout --;								//等待时，计数值自减
> 		if (Timeout == 0)						//自减到0后，等待超时
> 		{
> 			/*超时的错误处理代码，可以添加到此处*/
> 			break;								//跳出等待，不等了
> 		}
> 	}
> 	MySPI_Stop();								//SPI终止
> }
> 
> /**
>   * 函    数：W25Q64页编程
>   * 参    数：Address 页编程的起始地址，范围：0x000000~0x7FFFFF
>   * 参    数：DataArray	用于写入数据的数组
>   * 参    数：Count 要写入数据的数量，范围：0~256
>   * 返 回 值：无
>   * 注意事项：写入的地址范围不能跨页
>   */
> void W25Q64_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count)
> {
> 	uint16_t i;
> 	
> 	W25Q64_WriteEnable();						//写使能
> 	
> 	MySPI_Start();								//SPI起始
> 	MySPI_SwapByte(W25Q64_PAGE_PROGRAM);		//交换发送页编程的指令
> 	MySPI_SwapByte(Address >> 16);				//交换发送地址23~16位
> 	MySPI_SwapByte(Address >> 8);				//交换发送地址15~8位
> 	MySPI_SwapByte(Address);					//交换发送地址7~0位
> 	for (i = 0; i < Count; i ++)				//循环Count次
> 	{
> 		MySPI_SwapByte(DataArray[i]);			//依次在起始地址后写入数据
> 	}
> 	MySPI_Stop();								//SPI终止
> 	
> 	W25Q64_WaitBusy();							//等待忙
> }
> 
> /**
>   * 函    数：W25Q64扇区擦除（4KB）
>   * 参    数：Address 指定扇区的地址，范围：0x000000~0x7FFFFF
>   * 返 回 值：无
>   */
> void W25Q64_SectorErase(uint32_t Address)
> {
> 	W25Q64_WriteEnable();						//写使能
> 	
> 	MySPI_Start();								//SPI起始
> 	MySPI_SwapByte(W25Q64_SECTOR_ERASE_4KB);	//交换发送扇区擦除的指令
> 	MySPI_SwapByte(Address >> 16);				//交换发送地址23~16位
> 	MySPI_SwapByte(Address >> 8);				//交换发送地址15~8位
> 	MySPI_SwapByte(Address);					//交换发送地址7~0位
> 	MySPI_Stop();								//SPI终止
> 	
> 	W25Q64_WaitBusy();							//等待忙
> }
> 
> /**
>   * 函    数：W25Q64读取数据
>   * 参    数：Address 读取数据的起始地址，范围：0x000000~0x7FFFFF
>   * 参    数：DataArray 用于接收读取数据的数组，通过输出参数返回
>   * 参    数：Count 要读取数据的数量，范围：0~0x800000
>   * 返 回 值：无
>   */
> void W25Q64_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count)
> {
> 	uint32_t i;
> 	MySPI_Start();								//SPI起始
> 	MySPI_SwapByte(W25Q64_READ_DATA);			//交换发送读取数据的指令
> 	MySPI_SwapByte(Address >> 16);				//交换发送地址23~16位
> 	MySPI_SwapByte(Address >> 8);				//交换发送地址15~8位
> 	MySPI_SwapByte(Address);					//交换发送地址7~0位
> 	for (i = 0; i < Count; i ++)				//循环Count次
> 	{
> 		DataArray[i] = MySPI_SwapByte(W25Q64_DUMMY_BYTE);	//依次在起始地址后读取数据
> 	}
> 	MySPI_Stop();								//SPI终止
> }
> ```
>
> 完整文件：[[源码/11-2 硬件SPI读写W25Q64/Hardware/W25Q64.c|Hardware/W25Q64.c]]

> [!code]- 视频源码：硬件 SPI main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "W25Q64.h"
> 
> uint8_t MID;							//定义用于存放MID号的变量
> uint16_t DID;							//定义用于存放DID号的变量
> 
> uint8_t ArrayWrite[] = {0x01, 0x02, 0x03, 0x04};	//定义要写入数据的测试数组
> uint8_t ArrayRead[4];								//定义要读取数据的测试数组
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();						//OLED初始化
> 	W25Q64_Init();						//W25Q64初始化
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "MID:   DID:");
> 	OLED_ShowString(2, 1, "W:");
> 	OLED_ShowString(3, 1, "R:");
> 	
> 	/*显示ID号*/
> 	W25Q64_ReadID(&MID, &DID);			//获取W25Q64的ID号
> 	OLED_ShowHexNum(1, 5, MID, 2);		//显示MID
> 	OLED_ShowHexNum(1, 12, DID, 4);		//显示DID
> 	
> 	/*W25Q64功能函数测试*/
> 	W25Q64_SectorErase(0x000000);					//扇区擦除
> 	W25Q64_PageProgram(0x000000, ArrayWrite, 4);	//将写入数据的测试数组写入到W25Q64中
> 	
> 	W25Q64_ReadData(0x000000, ArrayRead, 4);		//读取刚写入的测试数据到读取数据的测试数组中
> 	
> 	/*显示数据*/
> 	OLED_ShowHexNum(2, 3, ArrayWrite[0], 2);		//显示写入数据的测试数组
> 	OLED_ShowHexNum(2, 6, ArrayWrite[1], 2);
> 	OLED_ShowHexNum(2, 9, ArrayWrite[2], 2);
> 	OLED_ShowHexNum(2, 12, ArrayWrite[3], 2);
> 	
> 	OLED_ShowHexNum(3, 3, ArrayRead[0], 2);			//显示读取数据的测试数组
> 	OLED_ShowHexNum(3, 6, ArrayRead[1], 2);
> 	OLED_ShowHexNum(3, 9, ArrayRead[2], 2);
> 	OLED_ShowHexNum(3, 12, ArrayRead[3], 2);
> 	
> 	while (1)
> 	{
> 		
> 	}
> }
> ```
>
> 完整文件：[[源码/11-2 硬件SPI读写W25Q64/User/main.c|User/main.c]]


## 波形

![[assets/ppt/s166_软件_硬件波形对比.jpg]]

*课程软件 / 硬件 SPI 波形对比。*

协议层波形应该一致；区别只是：

**软件 SPI：CPU 手动翻 SCK/MOSI**  
**硬件 SPI：SPI1 自动移位，CPU 只写/读 DR**

## 易错

| 现象 | 先查 |
|---|---|
| ID 读错 | CPOL/CPHA、SS、MOSI/MISO、SCK |
| 总是 0xFF | MISO 没被从机驱动、SS 没拉低、命令错 |
| 写不进去 | Write Enable、BUSY、是否先擦除 |
| 页尾后数据错 | 跨页写导致回卷覆盖 |
| 软件正常硬件不正常 | GPIO 复用、SPI1 时钟、Mode、分频、NSS 配置 |

## 速记

**SPI 是交换，不是单向“发送函数”。**  
**Mode 决定哪个边沿采样。**  
**SS 决定当前和谁通信。**  
**W25Q64 写：Enable → Erase/Program → WaitBusy。**
