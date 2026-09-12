# GD25Q32C SPI Flash 驱动

一个轻量级的 GD25Q32C（及兼容型号）SPI Flash 驱动库，仅支持 **标准 SPI（Single SPI）模式**。

## 特性

- 支持 4KB 扇区擦除、32KB / 64KB 块擦除
- 支持页编程（最大 256 字节/次）
- 支持连续数据读取
- 支持读取芯片 ID（REMS / RDID）
- 支持状态寄存器读写
- 支持深度休眠与唤醒
- 支持软件复位

## 不支持的功能

本库 **不支持** Dual SPI、Quad SPI 和 QPI 模式。使用前请确保：

1. Flash 的 QE 位（状态寄存器 2 的 bit1）为 0
2. 芯片未进入 Dual / Quad / QPI 模式
3. SPI 使用标准单线 MOSI/MISO 连接

## API 一览

| 函数                                  | 说明                         |
| ----------------------------------- | -------------------------- |
| `Write_Enable()`                    | 写使能（每次写入/擦除前必须调用，通常是自动调用的） |
| `Write_Disable()`                   | 写禁用                        |
| `Sector_Erase(Address)`             | 擦除 4KB 扇区                  |
| `Block_Erase_32K(Address)`          | 擦除 32KB 块                  |
| `Block_Erase_64K(Address)`          | 擦除 64KB 块                  |
| `Page_Program(Address, data, len)`  | 页编程，单次最多 256 字节            |
| `Read_Data(Address, buf, len)`      | 从指定地址读取数据                  |
| `Get_ID()`                          | 读取厂商 ID + 设备 ID（16 位）      |
| `Get_Identification()`              | 读取完整芯片识别信息（32 位）           |
| `Read_Status_Register_0/1/2()`      | 读取状态寄存器 0/1/2              |
| `Write_Status_Register_0/1/2(data)` | 写入状态寄存器 0/1/2              |
| `Reset_flash()`                     | 软件复位 Flash                 |
| `Enable_Deep_PowerDown()`           | 进入深度休眠                     |
| `Release_Deep_PowerDown()`          | 退出深度休眠                     |
| `Idle_Check()`                      | 检查 Flash 是否空闲              |
| `NSS_SET()`                         | 片选拉高（释放）                   |
| `NSS_RESET()`                       | 片选拉低（选中）                   |

## 适配指南

本库依赖 STM32 HAL 库。在非 STM32 平台或其他 HAL 版本上使用时，需要适配以下部分：

### 需要替换的 HAL 函数调用

| 原始调用                                        | 所在函数                           | 用途        |
| ------------------------------------------- | ------------------------------ | --------- |
| `HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, ...)` | `NSS_RESET()` / `NSS_SET()`    | 片选引脚控制    |
| `HAL_SPI_Transmit(&HSPI, ...)`              | `send()` / `send_recv_Async()` | SPI 发送    |
| `HAL_SPI_Receive(&HSPI, ...)`               | `send_recv_Async()`            | SPI 接收    |
| `HAL_SPI_TransmitReceive(&HSPI, ...)`       | `send_recv()`                  | SPI 全双工收发 |

> **注意：** `HAL_Delay()` 仅在 `Reset_flash()` 中使用，可替换为任意毫秒级延时函数，不影响主要功能。

### 需要修改的头文件

```c
// 替换为你的平台头文件，或直接删除以下 include
#include "cmsis_os.h"
#include "main.h"
#include "mem.h"
#include "spi.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_spi.h"
```

### 需要修改的宏定义

在 `GD25Q32C.h` 中修改 SPI 句柄：

```c
#define HSPI hspi1  // 改为你实际的 SPI 句柄名
```

### 需要修改的硬件引脚

`NSS_RESET()` 和 `NSS_SET()` 中硬编码了 **PA4** 作为片选引脚，需根据实际硬件修改：

```c
// GD25Q32C.c 第 36 行 / 第 45 行
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); // 改为你的 CS 引脚
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);    // 改为你的 CS 引脚
```

### 适配清单

- [ ] 将 `HAL_SPI_Transmit` / `HAL_SPI_Receive` / `HAL_SPI_TransmitReceive` 替换为你的平台 SPI 驱动
- [ ] 将 `HAL_GPIO_WritePin` 替换为你的平台 GPIO 驱动
- [ ] 修改 `HSPI` 宏为你的 SPI 句柄
- [ ] 修改 `NSS_RESET()` / `NSS_SET()` 中的 GPIO 端口和引脚号
- [ ] 移除或替换不需要的头文件
- [ ] 如使用 RTOS，保留 `cmsis_os.h`；否则可移除

## 许可证

本项目采用 [GNU Lesser General Public License v3.0 (LGPL-3.0)](LICENSE) 许可证。

你可以将本库以动态链接的方式集成到闭源商业项目中，但对本库本身的修改必须以 LGPL-3.0 许可证开源。
