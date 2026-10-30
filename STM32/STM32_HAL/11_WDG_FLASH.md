# WDG + Flash

## IWDG

CubeMX：

**System Core → IWDG**

HAL：

```c
HAL_IWDG_Init(&hiwdg);
HAL_IWDG_Refresh(&hiwdg);
```

标准库原来的：

```c
IWDG_WriteAccessCmd()
IWDG_SetPrescaler()
IWDG_SetReload()
IWDG_ReloadCounter()
IWDG_Enable()
```

在 HAL 中被大幅封装。

## WWDG

CubeMX：

**System Core → WWDG**

```c
HAL_WWDG_Init(&hwwdg);

HAL_WWDG_Refresh(
    &hwwdg,
    counter
);
```

## Flash

内部 Flash 一般不需要 CubeMX 去“启用”。

核心仍然：

**Unlock → Erase → Program → Lock**

```c
HAL_FLASH_Unlock();
```

擦页：

```c
FLASH_EraseInitTypeDef erase = {0};
uint32_t page_error;

erase.TypeErase = FLASH_TYPEERASE_PAGES;
erase.PageAddress = FLASH_ADDR;
erase.NbPages = 1;

HAL_FLASHEx_Erase(
    &erase,
    &page_error
);
```

写半字：

```c
HAL_FLASH_Program(
    FLASH_TYPEPROGRAM_HALFWORD,
    Address,
    Data
);
```

最后：

```c
HAL_FLASH_Lock();
```

读取仍然可以直接使用内存映射地址。
