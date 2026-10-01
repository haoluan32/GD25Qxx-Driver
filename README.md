# GD25Q32C SPI Flash 驱动

一个轻量级的 GD25Q32C（及兼容型号）SPI Flash 驱动库，仅支持 **标准 SPI（Single SPI）模式**。

## 文件说明

| 文件                 | 说明                         |
| -------------------- | ---------------------------- |
| `GD25Q32C.h`         | API 声明、配置宏             |
| `GD25Q32C.c`         | 驱动实现、底层 SPI/GPIO 封装 |
| `GD25Q32C_Command.h` | 命令宏定义、HEX 对照表       |
| `README.md`          | 使用与适配说明               |
| `文档.md`            | 详细的使用与适配说明         |

## 特性

- 支持 4KB 扇区擦除、32KB / 64KB 块擦除
- 支持全片擦除 `GD25Q32C_Chip_Erase()`
- 支持页编程（最大 256 字节/次）
- 支持连续数据读取
- 支持读取芯片 ID（REMS / RDID）
- 支持状态寄存器读写
- 支持易失性状态寄存器写使能
- 支持安全寄存器编程、读取、擦除
- 支持擦写挂起 / 恢复
- 支持深度休眠与唤醒
- 支持软件复位
- 提供底层 SPI 收发封装，便于移植

## 不支持的功能

本库 **不支持** Dual SPI、Quad SPI 和 QPI 模式。使用前请确保：

1. Flash 的 QE 位（状态寄存器 1 的 bit1）为 0  
   
   > 注：GD25Q32C 数据手册中 QE 位位于状态寄存器 1（RDSR1）的 bit1。
2. 芯片未进入 Dual / Quad / QPI 模式
3. SPI 使用标准单线 MOSI/MISO 连接

本库不会使用，也不保证兼容以下命令：

- `0x3B` / `0xBB`：Dual SPI 读
- `0x6B` / `0xEB`：Quad SPI 读
- `0x32`：Quad 页编程
- `0x38`：进入 QPI 模式

若芯片已被配置为 Dual / Quad / QPI 模式，请先退出该模式并恢复标准 SPI，否则读取、写入、擦除等操作将无法正常工作。

## API 一览

### 基础与状态

| 函数 / 宏                            | 说明                                                      |
| ------------------------------------ | --------------------------------------------------------- |
| `GD25Q32C_Write_Enable()`            | 写使能。每次写入/擦除前通常自动调用                       |
| `GD25Q32C_Write_Disable()`           | 写禁用，清除 WEL 位                                       |
| `GD25Q32C_Idle_Check()`              | 检查 Flash 是否空闲，返回 `true` 表示空闲，`false` 表示忙 |
| `GD25Q32C_Reset_flash()`             | 软件复位 Flash                                            |
| `GD25Q32C_Enable_Deep_Power_Down()`  | 进入深度休眠                                              |
| `GD25Q32C_Release_Deep_Power_Down()` | 退出深度休眠                                              |
| `GD25Q32C_NSS_SET()`                 | 片选拉高，释放 Flash                                      |
| `GD25Q32C_NSS_RESET()`               | 片选拉低，选中 Flash                                      |

### 擦除

| 函数 / 宏                              | 说明                                          |
| -------------------------------------- | --------------------------------------------- |
| `GD25Q32C_Sector_Erase(Address)`       | 擦除 4KB 扇区                                 |
| `GD25Q32C_Block_Erase(Address, Is64K)` | 擦除块。`Is64K=false` 为 32KB，`true` 为 64KB |
| `GD25Q32C_Block_Erase_32K(Address)`    | 擦除 32KB 块                                  |
| `GD25Q32C_Block_Erase_64K(Address)`    | 擦除 64KB 块                                  |
| `GD25Q32C_Chip_Erase()`                | 全片擦除。本驱动使用 `CE = 0xC7`              |

### 编程与读取

| 函数                                        | 说明                                 |
| ------------------------------------------- | ------------------------------------ |
| `GD25Q32C_Page_Program(Address, data, len)` | 页编程，单次最多 256 字节            |
| `GD25Q32C_Read_Data(Address, buf, len)`     | 从指定地址读取数据，长度无页边界限制 |
| `GD25Q32C_Suspend_Writing_Cycle()`          | 暂停当前写入/擦除操作                |
| `GD25Q32C_Resume_Writing_Cycle()`           | 恢复之前被暂停的写入/擦除操作        |

### ID 读取

| 函数                            | 说明                                                                                           |
| ------------------------------- | ---------------------------------------------------------------------------------------------- |
| `GD25Q32C_Get_ID()`             | 读取厂商 ID + 设备 ID，返回 16 位。GD25Q32C 通常为 `0xC840`                                    |
| `GD25Q32C_Get_Identification()` | 读取完整芯片识别信息，返回 32 位。典型值 `0xC8401600`，格式为 `[厂商ID][存储类型][容量][0x00]` |

### 状态寄存器

| 函数 / 宏                                              | 说明                                                     |
| ------------------------------------------------------ | -------------------------------------------------------- |
| `GD25Q32C_Read_Status_Register(Command)`               | 按命令读取状态寄存器                                     |
| `GD25Q32C_Write_Status_Register(Command, Data)`        | 按命令写入状态寄存器，内部自动 `GD25Q32C_Write_Enable()` |
| `GD25Q32C_Read_Status_Register_0()`                    | 读取状态寄存器 0，命令 `RDSR0 = 0x05`                    |
| `GD25Q32C_Read_Status_Register_1()`                    | 读取状态寄存器 1，命令 `RDSR1 = 0x35`                    |
| `GD25Q32C_Read_Status_Register_2()`                    | 读取状态寄存器 2，命令 `RDSR2 = 0x15`                    |
| `GD25Q32C_Write_Status_Register_0(data)`               | 写状态寄存器 0，命令 `WRSR0 = 0x01`                      |
| `GD25Q32C_Write_Status_Register_1(data)`               | 写状态寄存器 1，命令 `WRSR1 = 0x31`                      |
| `GD25Q32C_Write_Status_Register_2(data)`               | 写状态寄存器 2，命令 `WRSR2 = 0x11`                      |
| `GD25Q32C_Write_Enable_for_Volatile_Status_Register()` | 易失性状态寄存器写使能，命令 `VSR_WREN = 0x50`           |

### 安全寄存器

| 函数                                                                | 说明                                     |
| ------------------------------------------------------------------- | ---------------------------------------- |
| `GD25Q32C_Program_Security_Registers(Register, Address, data, len)` | 向安全寄存器写入数据，命令 `PSER = 0x42` |
| `GD25Q32C_Read_Security_Registers(Register, Address, buf, len)`     | 从安全寄存器读取数据，命令 `RSER = 0x48` |
| `GD25Q32C_Erase_Security_Registers(Register)`                       | 擦除指定安全寄存器，命令 `ESER = 0x44`   |

说明：

- 安全寄存器共 3 个，编号通常为 `1 / 2 / 3`，**每个 1024 字节**，总容量 **3072 字节**。
- 每个安全寄存器的地址偏移为 **10 位**，即 `0x000 ~ 0x3FF`。
- `GD25Q32C_Program_Security_Registers()` 中 `len > 1024` 会直接返回，因为单次最多写入一个安全寄存器的 1024 字节。
- 安全寄存器写/擦前会自动调用 `GD25Q32C_Idle_Check()`，忙则直接返回。
- 读取安全寄存器时，会先发送命令、3 字节地址和额外 dummy，再接收数据。
- 地址格式：`spi_TX[1]` 为地址高字节 `A23~A16`（固定 `0x00`）；`spi_TX[2]` 的高 4 位为寄存器编号，低 2 位为地址高位 `A9~A8`；`spi_TX[3]` 为地址低字节 `A7~A0`。

## 命令宏一览

| 宏         | 值     | 说明                                                |
| ---------- | -----: | --------------------------------------------------- |
| `WREN`     | `0x06` | 写使能                                              |
| `WRDI`     | `0x04` | 写禁用                                              |
| `SE`       | `0x20` | 4KB 扇区擦除                                        |
| `BE32K`    | `0x52` | 32KB 块擦除                                         |
| `BE64K`    | `0xD8` | 64KB 块擦除                                         |
| `CE`       | `0xC7` | 全片擦除。本驱动使用 `0xC7`；GD25Q32C 也支持 `0x60` |
| `PP`       | `0x02` | 页编程                                              |
| `READ`     | `0x03` | 读数据                                              |
| `PES`      | `0x75` | 编程/擦除挂起                                       |
| `PER`      | `0x7A` | 编程/擦除恢复                                       |
| `ERST`     | `0x66` | 使能复位                                            |
| `RST`      | `0x99` | 复位                                                |
| `RDSR0`    | `0x05` | 读状态寄存器 0                                      |
| `RDSR1`    | `0x35` | 读状态寄存器 1                                      |
| `RDSR2`    | `0x15` | 读状态寄存器 2                                      |
| `WRSR0`    | `0x01` | 写状态寄存器 0                                      |
| `WRSR1`    | `0x31` | 写状态寄存器 1                                      |
| `WRSR2`    | `0x11` | 写状态寄存器 2                                      |
| `VSR_WREN` | `0x50` | 易失性状态寄存器写使能                              |
| `REMS`     | `0x90` | 读厂商 ID / 设备 ID                                 |
| `RDID`     | `0x9F` | 读 JEDEC ID                                         |
| `DP`       | `0xB9` | 深度掉电                                            |
| `RDI`      | `0xAB` | 退出深度掉电                                        |
| `ESER`     | `0x44` | 擦除安全寄存器                                      |
| `PSER`     | `0x42` | 编程安全寄存器                                      |
| `RSER`     | `0x48` | 读取安全寄存器                                      |

## 状态寄存器与常用位

| 寄存器 / 位        | 说明                                           |
| ------------------ | ---------------------------------------------- |
| `RDSR0` bit0 `WIP` | 写操作进行中。`1=忙`，`0=空闲`                 |
| `RDSR0` bit1 `WEL` | 写使能锁存。`1=已使能`                         |
| `RDSR1` bit1 `QE`  | Quad 使能位。标准 SPI 模式下应为 `0`           |
| `RDSR1` bit7 `PS`  | 编程/擦除挂起状态位                            |
| `RDSR2` bit1       | 不是 QE 位；具体含义请以 GD25Q32C 数据手册为准 |

使用建议：

- 擦除、编程前必须确保 `WIP = 0`。
- 若需确认 QE 位，请以具体型号数据手册为准。
- 修改状态寄存器可能影响保护位、QE 位，请谨慎操作。

## 忙检查与调用注意

本驱动中多数擦除、编程、读取函数会在开始时调用 `GD25Q32C_Idle_Check()`：

- 如果 `GD25Q32C_Idle_Check()` 返回 `false`，函数会直接返回，不会阻塞等待。
- 因此调用者通常需要自行轮询 `GD25Q32C_Idle_Check()`，等待 Flash 空闲后再执行下一步操作。

典型模式：

```c
while (!GD25Q32C_Idle_Check()) {
    // 等待 Flash 空闲，可加入延时或 RTOS 让出
}

GD25Q32C_Sector_Erase(0x000000);

while (!GD25Q32C_Idle_Check()) {
    // 等待擦除完成
}

GD25Q32C_Page_Program(0x000000, buf, 256);

while (!GD25Q32C_Idle_Check()) {
    // 等待写入完成
}
```

注意：

- `GD25Q32C_Page_Program()` 单次最大 256 字节，`len > 256` 会直接返回。
- `GD25Q32C_Page_Program()` 不检查是否跨页，调用者需确保 `(Address & 0xFF) + len <= 256`。
- `GD25Q32C_Read_Data()` 读取长度无页限制。
- `GD25Q32C_Chip_Erase()` 全片擦除耗时较长，通常约 20~100 秒，期间 Flash 处于忙状态。
- `GD25Q32C_Reset_flash()` 内部使用 `HAL_Delay()`，分别等待约 1ms 和 20ms。

#### 全局缓冲区与线程安全

驱动内部使用全局缓冲区：

```c
u8 spi_RX[264];
u8 spi_TX[264];
```

注意：

- 这些缓冲区为全局变量，非静态。
- 驱动本身不是线程安全的。
- 多任务、RTOS 或多 Flash 设备并发访问时，需要外部加互斥锁或串行化调用。
- 中断中不建议直接调用本驱动。

### 缓冲区大小说明

这两个缓冲区默认各 264 字节，合计 528 字节，对于 RAM 较小的 MCU 可能偏大。实际上，如果只使用本驱动现有 API，缓冲区并不需要这么大：

- `spi_TX`：主要用于存放“命令 + 3 字节地址 + dummy”等短数据，现有函数中最多用到约 5 字节。
- `spi_RX`：主要用于读取短状态 / ID 数据，现有函数中最多用到约 3 字节。
- 大块读写数据（页编程、连续读、安全寄存器读写）都直接使用调用者传入的 `pdata` / `data_buf`，不经过 `spi_TX` / `spi_RX`。

因此，可根据实际使用情况把缓冲区调小，例如：

```c
u8 spi_RX[8];
u8 spi_TX[8];
```

或更保守一些：

```c
u8 spi_RX[16];
u8 spi_TX[16];
```

> 注意：如果你自行扩展了驱动，或直接在外部使用 `spi_TX` / `spi_RX` 暂存大块数据，请保留足够大小，避免越界。

## 使用示例

```c
#include "GD25Q32C.h"

uint8_t tx_buf[256];
uint8_t rx_buf[256];

void flash_demo(void)
{
    uint16_t id16;
    uint32_t id32;

    GD25Q32C_Release_Deep_Power_Down();
    GD25Q32C_Reset_flash();

    id16 = GD25Q32C_Get_ID();              // 典型 0xC840
    id32 = GD25Q32C_Get_Identification();  // 典型 0xC8401600

    while (!GD25Q32C_Idle_Check()) {
        // 等待空闲
    }

    GD25Q32C_Sector_Erase(0x000000);

    while (!GD25Q32C_Idle_Check()) {
        // 等待扇区擦除完成
    }

    for (int i = 0; i < 256; i++) {
        tx_buf[i] = (uint8_t)i;
    }

    GD25Q32C_Page_Program(0x000000, tx_buf, 256);

    while (!GD25Q32C_Idle_Check()) {
        // 等待页编程完成
    }

    GD25Q32C_Read_Data(0x000000, rx_buf, 256);
}
```

## 适配指南

> 📘 **更详细的适配说明**（包含 ESP32、Linux spidev、STM32 LL 库、裸机寄存器版等多种平台的 **伪代码** 示例，以及函数手册与命令对照表）请参见 [文档.md](./文档.md)。

本库依赖 STM32 HAL 库。在非 STM32 平台或其他 HAL 版本上使用时，需要适配以下部分。

### 需要替换的 HAL 函数调用

| 原始调用                                       | 所在函数                                      | 用途           |
| ---------------------------------------------- | --------------------------------------------- | -------------- |
| `HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, ...)`    | `GD25Q32C_NSS_RESET()` / `GD25Q32C_NSS_SET()` | 片选引脚控制   |
| `HAL_SPI_Transmit(&GD25Q32C_HSPI, ...)`        | `send()` / `send_recv_Async()`                | SPI 发送       |
| `HAL_SPI_Receive(&GD25Q32C_HSPI, ...)`         | `send_recv_Async()`                           | SPI 接收       |
| `HAL_SPI_TransmitReceive(&GD25Q32C_HSPI, ...)` | `send_recv()`                                 | SPI 全双工收发 |
| `HAL_Delay(1)` / `HAL_Delay(20)`               | `GD25Q32C_Reset_flash()`                      | 毫秒级延时     |

> 注意：`HAL_Delay()` 仅在 `GD25Q32C_Reset_flash()` 中使用，可替换为任意毫秒级延时函数，不影响主要功能。

`GD25Q32C.c` 中实现了以下底层函数：

```c
void send(u8 *pdata, u16 len);
void send_recv(u8 *pdata_TX, u8 *pdata_RX, u16 len);
void send_recv_Async(u8 *pdata_TX, u8 *pdata_RX, u16 TX_len, u16 RX_len);
```

这些函数未在 `GD25Q32C.h` 中声明，主要用于内部收发和移植适配。若外部需要调用，请自行声明或加入头文件。

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
#define GD25Q32C_HSPI hspi1  // 改为你实际的 SPI 句柄名
```

### 需要修改的硬件引脚

`GD25Q32C.h` 顶部已把片选引脚抽象为宏，默认使用 **PA4**：

```c
#define GD25Q32C_FLASH_CS_PORT  GPIOA
#define GD25Q32C_FLASH_CS_PIN   GPIO_PIN_4
```

`GD25Q32C_NSS_RESET()` 和 `GD25Q32C_NSS_SET()` 通过这两个宏控制 CS，无需改动函数体：

```c
void GD25Q32C_NSS_RESET(void)
{
    HAL_GPIO_WritePin(GD25Q32C_FLASH_CS_PORT, GD25Q32C_FLASH_CS_PIN, GPIO_PIN_RESET);  // 拉低 CS，选中 Flash
}

void GD25Q32C_NSS_SET(void)
{
    HAL_GPIO_WritePin(GD25Q32C_FLASH_CS_PORT, GD25Q32C_FLASH_CS_PIN, GPIO_PIN_SET);    // 拉高 CS，释放 Flash
}
```

根据实际硬件修改宏即可。例如 CS 接在 **PB12**，改为：

```diff
- #define GD25Q32C_FLASH_CS_PORT  GPIOA
- #define GD25Q32C_FLASH_CS_PIN   GPIO_PIN_4
+ #define GD25Q32C_FLASH_CS_PORT  GPIOB
+ #define GD25Q32C_FLASH_CS_PIN   GPIO_PIN_12
```

其他代码无需改动。更详细的引脚配置说明（含 CubeMX 设置、常见引脚示例、跨平台移植建议）请参见 [文档.md](./文档.md) 的 [1.4 修改 CS 引脚](文档.md#part1-4) 一节。

### 适配清单

- [ ] 将 `HAL_SPI_Transmit` / `HAL_SPI_Receive` / `HAL_SPI_TransmitReceive` 替换为你的平台 SPI 驱动
- [ ] 将 `HAL_GPIO_WritePin` 替换为你的平台 GPIO 驱动
- [ ] 将 `HAL_Delay` 替换为你的平台毫秒级延时函数
- [ ] 修改 `GD25Q32C_HSPI` 宏为你的 SPI 句柄
- [ ] 修改 `GD25Q32C_NSS_RESET()` / `GD25Q32C_NSS_SET()` 中的 GPIO 端口和引脚号
- [ ] 移除或替换不需要的头文件
- [ ] 如使用 RTOS，保留 `cmsis_os.h`；否则可移除
- [ ] 如多任务并发访问 Flash，增加互斥锁或串行化保护
- [ ] 确认 Flash 的 QE 位为 0，且未进入 Dual / Quad / QPI 模式

## 许可证

代码（`GD25Q32C.c` 等）采用 [Eclipse Public License 2.0 (EPL-2.0)](LICENSE) 许可证。

你可以在 EPL-2.0 条款下使用、修改和分发本库；对代码的修改需按 EPL-2.0 要求处理。

> 附带的文档使用 CC BY-SA 4.0 许可