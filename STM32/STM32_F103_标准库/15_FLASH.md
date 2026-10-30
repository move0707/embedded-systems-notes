# Flash

## 组织

| Flash 简介 | 闪存模块组织 |
|---|---|
| ![[assets/ppt/s195_FLASH简介.jpg]] | ![[assets/ppt/s196_闪存模块组织.jpg]] |

| Flash 基本结构 |
|---|
| ![[assets/ppt/s197_FLASH基本结构.jpg]] |

STM32F103 的程序代码本身就在内部 Flash 中。Flash 也可以留出一页保存掉电不丢失的参数。

Flash 与 SRAM 最大区别：

**SRAM：读写快、可随时改、掉电丢失**  
**Flash：掉电保留，但写入前受擦除规则限制，擦写寿命有限**

## 解锁

![[assets/ppt/s198_FLASH解锁.jpg]]

*Flash 解锁流程。*

写/擦之前要 Unlock，完成后再 Lock。

课程封装：

**Unlock → Erase / Program → Lock**

---

## 指针读取

![[assets/ppt/s199_使用指针访问存储器.jpg]]

*使用指针直接访问存储器地址。*

读取 Flash 不需要特殊“读命令”，CPU 可以像访问内存一样按地址解引用：

```c
*((__IO uint32_t *)Address)
*((__IO uint16_t *)Address)
*((__IO uint8_t  *)Address)
```

课程 `MyFLASH.c` 就是这样封装 Word / HalfWord / Byte 读取。

---

## 编程

![[assets/ppt/s200_程序存储器编程.jpg]]

*程序存储器编程流程。*

Flash 写入不是覆盖 RAM 那样随便改。

课程封装：

```c
FLASH_Unlock();
FLASH_ProgramHalfWord(Address, Data);
FLASH_Lock();
```

真正存参数时要先考虑该位是否需要从 0 变回 1；如果需要，必须先擦页。

## 擦除

![[assets/ppt/s201_程序存储器页擦除.jpg]]

*页擦除流程。*

课程 15-1 用指定页保存参数：

**上电 → 检查页首标志 `0xA5A5` → 首次使用则擦页并初始化 → Flash 内容加载到 SRAM 数组**

修改数据时先改 SRAM 中的 `Store_Data[]`，需要保存时：

**Erase Page → 把整个数组重新写回 Flash**

这样应用层平时读写 SRAM，只有保存动作才真正擦写 Flash。

> [!code]- 视频源码：MyFLASH.c
> ```c
> #include "stm32f10x.h"                  // Device header
> 
> /**
>   * 函    数：FLASH读取一个32位的字
>   * 参    数：Address 要读取数据的字地址
>   * 返 回 值：指定地址下的数据
>   */
> uint32_t MyFLASH_ReadWord(uint32_t Address)
> {
> 	return *((__IO uint32_t *)(Address));	//使用指针访问指定地址下的数据并返回
> }
> 
> /**
>   * 函    数：FLASH读取一个16位的半字
>   * 参    数：Address 要读取数据的半字地址
>   * 返 回 值：指定地址下的数据
>   */
> uint16_t MyFLASH_ReadHalfWord(uint32_t Address)
> {
> 	return *((__IO uint16_t *)(Address));	//使用指针访问指定地址下的数据并返回
> }
> 
> /**
>   * 函    数：FLASH读取一个8位的字节
>   * 参    数：Address 要读取数据的字节地址
>   * 返 回 值：指定地址下的数据
>   */
> uint8_t MyFLASH_ReadByte(uint32_t Address)
> {
> 	return *((__IO uint8_t *)(Address));	//使用指针访问指定地址下的数据并返回
> }
> 
> /**
>   * 函    数：FLASH全擦除
>   * 参    数：无
>   * 返 回 值：无
>   * 说    明：调用此函数后，FLASH的所有页都会被擦除，包括程序文件本身，擦除后，程序将不复存在
>   */
> void MyFLASH_EraseAllPages(void)
> {
> 	FLASH_Unlock();					//解锁
> 	FLASH_EraseAllPages();			//全擦除
> 	FLASH_Lock();					//加锁
> }
> 
> /**
>   * 函    数：FLASH页擦除
>   * 参    数：PageAddress 要擦除页的页地址
>   * 返 回 值：无
>   */
> void MyFLASH_ErasePage(uint32_t PageAddress)
> {
> 	FLASH_Unlock();					//解锁
> 	FLASH_ErasePage(PageAddress);	//页擦除
> 	FLASH_Lock();					//加锁
> }
> 
> /**
>   * 函    数：FLASH编程字
>   * 参    数：Address 要写入数据的字地址
>   * 参    数：Data 要写入的32位数据
>   * 返 回 值：无
>   */
> void MyFLASH_ProgramWord(uint32_t Address, uint32_t Data)
> {
> 	FLASH_Unlock();							//解锁
> 	FLASH_ProgramWord(Address, Data);		//编程字
> 	FLASH_Lock();							//加锁
> }
> 
> /**
>   * 函    数：FLASH编程半字
>   * 参    数：Address 要写入数据的半字地址
>   * 参    数：Data 要写入的16位数据
>   * 返 回 值：无
>   */
> void MyFLASH_ProgramHalfWord(uint32_t Address, uint16_t Data)
> {
> 	FLASH_Unlock();							//解锁
> 	FLASH_ProgramHalfWord(Address, Data);	//编程半字
> 	FLASH_Lock();							//加锁
> }
> ```
>
> 完整文件：[[源码/15-1 读写内部FLASH/System/MyFLASH.c|System/MyFLASH.c]]

> [!code]- 视频源码：Store.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "MyFLASH.h"
> 
> #define STORE_START_ADDRESS		0x0800FC00		//存储的起始地址
> #define STORE_COUNT				512				//存储数据的个数
> 
> uint16_t Store_Data[STORE_COUNT];				//定义SRAM数组
> 
> /**
>   * 函    数：参数存储模块初始化
>   * 参    数：无
>   * 返 回 值：无
>   */
> void Store_Init(void)
> {
> 	/*判断是不是第一次使用*/
> 	if (MyFLASH_ReadHalfWord(STORE_START_ADDRESS) != 0xA5A5)	//读取第一个半字的标志位，if成立，则执行第一次使用的初始化
> 	{
> 		MyFLASH_ErasePage(STORE_START_ADDRESS);					//擦除指定页
> 		MyFLASH_ProgramHalfWord(STORE_START_ADDRESS, 0xA5A5);	//在第一个半字写入自己规定的标志位，用于判断是不是第一次使用
> 		for (uint16_t i = 1; i < STORE_COUNT; i ++)				//循环STORE_COUNT次，除了第一个标志位
> 		{
> 			MyFLASH_ProgramHalfWord(STORE_START_ADDRESS + i * 2, 0x0000);		//除了标志位的有效数据全部清0
> 		}
> 	}
> 	
> 	/*上电时，将闪存数据加载回SRAM数组，实现SRAM数组的掉电不丢失*/
> 	for (uint16_t i = 0; i < STORE_COUNT; i ++)					//循环STORE_COUNT次，包括第一个标志位
> 	{
> 		Store_Data[i] = MyFLASH_ReadHalfWord(STORE_START_ADDRESS + i * 2);		//将闪存的数据加载回SRAM数组
> 	}
> }
> 
> /**
>   * 函    数：参数存储模块保存数据到闪存
>   * 参    数：无
>   * 返 回 值：无
>   */
> void Store_Save(void)
> {
> 	MyFLASH_ErasePage(STORE_START_ADDRESS);				//擦除指定页
> 	for (uint16_t i = 0; i < STORE_COUNT; i ++)			//循环STORE_COUNT次，包括第一个标志位
> 	{
> 		MyFLASH_ProgramHalfWord(STORE_START_ADDRESS + i * 2, Store_Data[i]);	//将SRAM数组的数据备份保存到闪存
> 	}
> }
> 
> /**
>   * 函    数：参数存储模块将所有有效数据清0
>   * 参    数：无
>   * 返 回 值：无
>   */
> void Store_Clear(void)
> {
> 	for (uint16_t i = 1; i < STORE_COUNT; i ++)			//循环STORE_COUNT次，除了第一个标志位
> 	{
> 		Store_Data[i] = 0x0000;							//SRAM数组有效数据清0
> 	}
> 	Store_Save();										//保存数据到闪存
> }
> ```
>
> 完整文件：[[源码/15-1 读写内部FLASH/System/Store.c|System/Store.c]]

> [!code]- 视频源码：Flash 存储 main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> #include "Store.h"
> #include "Key.h"
> 
> uint8_t KeyNum;					//定义用于接收按键键码的变量
> 
> int main(void)
> {
> 	/*模块初始化*/
> 	OLED_Init();				//OLED初始化
> 	Key_Init();					//按键初始化
> 	Store_Init();				//参数存储模块初始化，在上电的时候将闪存的数据加载回Store_Data，实现掉电不丢失
> 	
> 	/*显示静态字符串*/
> 	OLED_ShowString(1, 1, "Flag:");
> 	OLED_ShowString(2, 1, "Data:");
> 	
> 	while (1)
> 	{
> 		KeyNum = Key_GetNum();		//获取按键键码
> 		
> 		if (KeyNum == 1)			//按键1按下
> 		{
> 			Store_Data[1] ++;		//变换测试数据
> 			Store_Data[2] += 2;
> 			Store_Data[3] += 3;
> 			Store_Data[4] += 4;
> 			Store_Save();			//将Store_Data的数据备份保存到闪存，实现掉电不丢失
> 		}
> 		
> 		if (KeyNum == 2)			//按键2按下
> 		{
> 			Store_Clear();			//将Store_Data的数据全部清0
> 		}
> 		
> 		OLED_ShowHexNum(1, 6, Store_Data[0], 4);	//显示Store_Data的第一位标志位
> 		OLED_ShowHexNum(3, 1, Store_Data[1], 4);	//显示Store_Data的有效存储数据
> 		OLED_ShowHexNum(3, 6, Store_Data[2], 4);
> 		OLED_ShowHexNum(4, 1, Store_Data[3], 4);
> 		OLED_ShowHexNum(4, 6, Store_Data[4], 4);
> 	}
> }
> ```
>
> 完整文件：[[源码/15-1 读写内部FLASH/User/main.c|User/main.c]]


> [!warning]
> `MyFLASH_EraseAllPages()` 会把包括程序本身在内的 Flash 全部擦掉。课程源码注释也明确提醒这一点。

## 全擦除

![[assets/ppt/s202_程序存储器全擦除.jpg]]

*全擦除会擦掉整个程序存储区，课程源码明确提醒会把当前程序本身一起擦除。*

## 选项字节

![[assets/ppt/s203_选项字节.jpg]]

*选项字节用于写保护、读保护等配置。*

这一部分重点理解：选项字节不是普通用户数据区，它控制 Flash 安全/保护行为，修改时要格外谨慎。

## 芯片 ID

![[assets/ppt/s206_器件电子签名.jpg]]

*器件电子签名：Flash 容量寄存器和 96 位唯一 ID。*

课程直接用固定系统地址读取：

**Flash 容量：`0x1FFFF7E0`，16 位**  
**Unique ID：`0x1FFFF7E8` 起，96 位**

> [!code]- 视频源码：读取芯片 ID main.c
> ```c
> #include "stm32f10x.h"                  // Device header
> #include "Delay.h"
> #include "OLED.h"
> 
> int main(void)
> {
> 	OLED_Init();						//OLED初始化
> 	
> 	OLED_ShowString(1, 1, "F_SIZE:");	//显示静态字符串
> 	OLED_ShowHexNum(1, 8, *((__IO uint16_t *)(0x1FFFF7E0)), 4);		//使用指针读取指定地址下的闪存容量寄存器
> 	
> 	OLED_ShowString(2, 1, "U_ID:");		//显示静态字符串
> 	OLED_ShowHexNum(2, 6, *((__IO uint16_t *)(0x1FFFF7E8)), 4);		//使用指针读取指定地址下的产品唯一身份标识寄存器
> 	OLED_ShowHexNum(2, 11, *((__IO uint16_t *)(0x1FFFF7E8 + 0x02)), 4);
> 	OLED_ShowHexNum(3, 1, *((__IO uint32_t *)(0x1FFFF7E8 + 0x04)), 8);
> 	OLED_ShowHexNum(4, 1, *((__IO uint32_t *)(0x1FFFF7E8 + 0x08)), 8);
> 	
> 	while (1)
> 	{
> 		
> 	}
> }
> ```
>
> 完整文件：[[源码/15-2 读取芯片ID/User/main.c|User/main.c]]


## 速记

**读：直接按地址读。**  
**写：Unlock → Program → Lock。**  
**需要 0→1：先 Erase。**  
**参数保存：平时放 SRAM，需要时整页备份到 Flash。**
