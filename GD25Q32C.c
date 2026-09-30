/**
 * @file GD25Q32C.c
 * @brief GD25Q32C SPI Flash 驱动实现
 * @author haoluan
 * @date 2026年8月19日
 */

#include "GD25Q32C.h"			// GD25Q32C Flash 驱动头文件
#include "cmsis_os.h"			// CMSIS-RTOS 操作系统头文件
#include "main.h"				// 主程序头文件（包含硬件配置）
#include "mem.h"				// 内存管理头文件
#include "spi.h"				// SPI 通信头文件
#include "stm32f4xx_hal.h"		// STM32F4 HAL 库主头文件
#include "stm32f4xx_hal_gpio.h" // GPIO 操作头文件
#include "stm32f4xx_hal_spi.h"	// SPI 操作头文件
#include <stdbool.h>			//标准布尔类型定义
#include <stdint.h>				// 标准整数类型定义

// 自定义类型定义（与嵌入式常用命名一致）
#define u8 unsigned char   // 8位无符号整数
#define u16 unsigned short // 16位无符号整数
#define u32 unsigned int   // 32位无符号整数
#define i8 char			   // 8位有符号整数
#define i16 short		   // 16位有符号整数
#define i32 int			   // 32位有符号整数

u8 spi_RX[264]; // SPI 接收缓冲区（256字节数据 + 4字节命令地址 + 4字节备用）
u8 spi_TX[264]; // SPI 发送缓冲区（256字节数据 + 4字节命令地址 + 4字节备用）

/**
 * @brief 片选信号拉低，选中 Flash 芯片
 * @note  NSS (Not Slave Select) 低电平有效
 */
void NSS_RESET()
{
	HAL_GPIO_WritePin(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_PIN_RESET); // PA4 输出低电平
}

/**
 * @brief 片选信号拉高，释放 Flash 芯片
 * @note  操作完成后必须调用此函数释放 Flash
 */
void NSS_SET()
{
	HAL_GPIO_WritePin(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_PIN_SET); // PA4 输出高电平
}

/**
 * @brief 通过 SPI 向 Flash 发送数据
 * @param pdata 待发送的数据缓冲区指针
 * @param len 发送的字节数
 */
void send(u8 *pdata, u16 len)
{
	HAL_SPI_Transmit(&HSPI, pdata, len, 50);
}

/**
 * @brief 通过 SPI 全双工发送数据并同时接收数据
 * @param pdata_TX 待发送的数据缓冲区指针
 * @param pdata_RX 接收数据的缓冲区指针
 * @param len 传输的字节数
 */
void send_recv(u8 *pdata_TX, u8 *pdata_RX, u16 len)
{
	HAL_SPI_TransmitReceive(&HSPI, pdata_TX, pdata_RX, len, 50);
}

/**
 * @brief 异步 SPI 收发（先发送后接收，非全双工）
 * @param pdata_TX 待发送的数据缓冲区指针（命令+地址）
 * @param pdata_RX 接收数据的缓冲区指针
 * @param TX_len 发送的字节数
 * @param RX_len 接收的字节数
 * @note  用于读取操作：先发送命令和地址，再接收数据
 */
void send_recv_Async(u8 *pdata_TX, u8 *pdata_RX, u16 TX_len, u16 RX_len)
{
	HAL_SPI_Transmit(&HSPI, pdata_TX, TX_len, 50); // 发送命令和地址
	HAL_SPI_Receive(&HSPI, pdata_RX, RX_len, 50);  // 接收数据

	return;
}

/**
 * @brief 发送写使能命令 (WREN)，使 Flash 进入可写/可擦除状态
 * @note  每次写入或擦除操作前必须调用此函数
 *        WREN 命令字节：0x06
 */
void Write_Enable()
{
	spi_TX[0] = WREN;	 // 写使能命令 (0x06)
	NSS_RESET();		 // 选中 Flash
	send(&spi_TX[0], 1); // 发送1字节命令
	NSS_SET();			 // 释放 Flash
}

/**
 * @brief 发送写禁用命令 (WRDI)，禁止 Flash 写入/擦除操作
 * @note  WRDI 命令字节：0x04
 *        执行后 WEL 位被清除，阻止任何写入/擦除操作
 *        用于在不需要写操作时保护数据安全
 */
void Write_Disable()
{
	spi_TX[0] = WRDI;	 // 写禁用命令 (0x04)
	NSS_RESET();		 // 选中 Flash
	send(&spi_TX[0], 1); // 发送1字节命令
	NSS_SET();			 // 释放 Flash
}

/**
 * @brief 读取状态寄存器检查 Flash 是否忙（正在写入或擦除）
 * @return true 表示 Flash 空闲可操作，false 表示 Flash 忙
 * @note  状态寄存器0 (RDSR0) 的 bit0 是 WIP (Write In Progress) 位
 *        WIP=1 表示 Flash 正在执行写入/擦除操作
 *        WIP=0 表示 Flash 空闲，可以进行新的操作
 */
bool Idle_Check()
{
	spi_TX[0] = RDSR0;							   // 读状态寄存器0命令 (0x05)
	NSS_RESET();								   // 选中 Flash
	send_recv_Async(&spi_TX[0], &spi_RX[0], 1, 1); // 发送命令，读取1字节状态
	NSS_SET();									   // 释放 Flash
	if ((spi_RX[0] & 0x01) == 0)				   // 检查 WIP 位 (bit0)
	{
		return true; // Flash 空闲
	}
	return false; // Flash 忙
}

/**
 * @brief 擦除指定地址所在的扇区
 * @param Address 目标扇区内的任意地址（24 位地址）
 * @note  扇区大小为 4KB (4096 字节)
 *        擦除操作会将扇区内所有数据置为 0xFF
 *        擦除前会自动调用 Idle_Check() 检查 Flash 是否空闲（忙则直接返回，不做等待）
 *        SE 命令字节：0x20
 */
void Sector_Erase(u32 Address)
{
	if (Idle_Check() == false) // 检查 Flash 是否空闲，忙则放弃本次操作
	{
		return;
	}
	Write_Enable();
	spi_TX[0] = SE;						  // 扇区擦除命令 (0x20)
	spi_TX[1] = ((Address >> 16) & 0xff); // 地址高字节 (A23-A16)
	spi_TX[2] = ((Address >> 8) & 0xff);  // 地址中字节 (A15-A8)
	spi_TX[3] = (Address & 0xff);		  // 地址低字节 (A7-A0)
	NSS_RESET();						  // 选中 Flash
	send(&spi_TX[0], 4);				  // 发送命令 + 3字节地址
	NSS_SET();							  // 释放 Flash
}

/**
 * @brief 擦除指定地址所在的块（32KB 或 64KB）
 * @param Address 目标块内的任意地址（24 位）
 * @param Is64K   true=64KB 块擦除(BE64K)，false=32KB 块擦除(BE32K)
 * @note  32KB 块擦除命令 BE32K：0x52
 *        64KB 块擦除命令 BE64K：0xD8
 *        擦除前会自动调用 Idle_Check() 检查 Flash 是否空闲（忙则直接返回，不做等待）
 *        擦除后块内所有数据置为 0xFF
 */
void Block_Erase(u32 Address, bool Is64K)
{
	if (Idle_Check() == false) // 检查 Flash 是否空闲，忙则放弃本次操作
	{
		return;
	}

	spi_TX[0] = BE32K; // 32KB 块擦除命令 (0x52)，Is64K 为 false 时使用
	if (Is64K == true)
	{
		spi_TX[0] = BE64K; // 64KB 块擦除命令 (0xD8)
	}

	Write_Enable();
	spi_TX[1] = ((Address >> 16) & 0xff); // 地址高字节 (A23-A16)
	spi_TX[2] = ((Address >> 8) & 0xff);  // 地址中字节 (A15-A8)
	spi_TX[3] = (Address & 0xff);		  // 地址低字节 (A7-A0)
	NSS_RESET();						  // 选中 Flash
	send(&spi_TX[0], 4);				  // 发送命令 + 3字节地址
	NSS_SET();							  // 释放 Flash
}

/**
 * @brief 擦除整个 Flash 芯片（全片擦除）
 * @note  CE 命令字节：0x60 或 0xC7（本驱动使用 0xC7）
 *        擦除前会自动调用 Idle_Check() 检查 Flash 是否空闲（忙则直接返回，不做等待）
 *        全片擦除耗时较长（约 20~100 秒），期间 Flash 处于忙状态
 *        擦除后所有数据置为 0xFF
 */
void Chip_Erase()
{
	if (Idle_Check() == false) // 检查 Flash 是否空闲，忙则放弃本次操作
	{
		return;
	}
	Write_Enable();
	spi_TX[0] = CE;		 // 全片擦除命令 (0xC7)
	NSS_RESET();		 // 选中 Flash
	send(&spi_TX[0], 1); // 发送命令
	NSS_SET();			 // 释放 Flash
}

/**
 * @brief 暂停 Flash 的写入/擦除操作
 * @note  PES 命令字节：0x75
 *        暂停后可执行阵列读取操作（Read Array）
 *        适用于需要在长时间擦除期间读取其他数据的场景
 *        暂停后 WIP 位变为 0，PS 位（状态寄存器2 bit7）变为 1
 */
void Suspend_Writing_Cycle()
{
	spi_TX[0] = PES;
	NSS_RESET();		 // 选中 Flash
	send(&spi_TX[0], 1); // 发送命令
	NSS_SET();			 // 释放 Flash
}

/**
 * @brief 恢复之前被暂停的写入/擦除操作
 * @note  PER 命令字节：0x7A
 *        必须在 Suspend_Writing_Cycle() 之后调用
 *        恢复后 Flash 继续执行之前的写入/擦除操作
 *        PS 位（状态寄存器2 bit7）恢复为 0
 */
void Resume_Writing_Cycle()
{
	spi_TX[0] = PER;
	NSS_RESET();		 // 选中 Flash
	send(&spi_TX[0], 1); // 发送命令
	NSS_SET();			 // 释放 Flash
}

/**
 * @brief 复位 Flash 芯片
 * @note  先发送使能复位命令 ERST，再发送复位命令 RST
 *        ERST 命令字节：0x66
 *        RST 命令字节：0x99
 *        复位后 Flash 恢复到默认状态
 *        延迟 1ms 等待 ERST 命令生效
 *        延迟 20ms 等待 RST 命令完成
 */
void Reset_flash()
{
	spi_TX[0] = ERST; // 使能复位命令 (0x66)

	send(&spi_TX[0], 1); // 发送命令
	NSS_RESET();
	HAL_Delay(1); // 等待 1ms
	NSS_SET();
	spi_TX[0] = RST; // 复位命令 (0x99)
	NSS_RESET();
	send(&spi_TX[0], 1); // 发送命令
	NSS_SET();
	HAL_Delay(20); // 等待 20ms 复位完成
}

/**
 * @brief 向 Flash 写入数据（页编程，单次最多 256 字节）
 * @param Address 目标写入地址（24 位）
 * @param pdata 待写入的数据缓冲区指针
 * @param len 待写入的字节数，超过 256 时函数直接返回
 * @note  页大小为 256 字节，写入不能跨越页边界
 *        写入前会自动调用 Idle_Check() 检查 Flash 是否空闲（忙则直接返回，不做等待）
 *        PP 命令字节：0x02
 */
void Page_Program(u32 Address, u8 *pdata, u16 len)
{
	if (len > 256) // 页编程最大 256 字节
	{
		return;
	}
	if (Idle_Check() == false) // 检查 Flash 是否空闲，忙则放弃本次操作
	{
		return;
	}
	Write_Enable();
	spi_TX[0] = PP;						  // 页编程命令 (0x02)
	spi_TX[1] = ((Address >> 16) & 0xff); // 地址高字节 (A23-A16)
	spi_TX[2] = ((Address >> 8) & 0xff);  // 地址中字节 (A15-A8)
	spi_TX[3] = (Address & 0xff);		  // 地址低字节 (A7-A0)
	NSS_RESET();						  // 选中 Flash
	send(&spi_TX[0], 4);				  // 发送命令 + 地址
	send(pdata, len);					  // 发送数据
	NSS_SET();							  // 释放 Flash
}

/**
 * @brief 从 Flash 指定地址读取数据
 * @param Address 起始读取地址（24 位）
 * @param data_buf 用于存放读出数据的缓冲区指针
 * @param len 读取的字节数
 * @note  读取操作可以读取任意长度的数据（无页边界限制）
 *        读取前会自动调用 Idle_Check() 检查 Flash 是否空闲（忙则直接返回，不做等待）
 *        READ 命令字节：0x03
 *        注意：Flash 忙时函数直接返回，data_buf 内容保持不变（未定义数据）
 */
void Read_Data(u32 Address, u8 *data_buf, u32 len)
{
	if (Idle_Check() == false) // 检查 Flash 是否空闲，忙则放弃本次操作
	{
		return;
	}
	spi_TX[0] = READ;							   // 读数据命令 (0x03)
	spi_TX[1] = ((Address >> 16) & 0xff);		   // 地址高字节 (A23-A16)
	spi_TX[2] = ((Address >> 8) & 0xff);		   // 地址中字节 (A15-A8)
	spi_TX[3] = (Address & 0xff);				   // 地址低字节 (A7-A0)
	NSS_RESET();								   // 选中 Flash
	send_recv_Async(&spi_TX[0], data_buf, 4, len); // 发送命令+地址，接收数据
	NSS_SET();									   // 释放 Flash
	return;
}

/**
 * @brief 读取 Flash 的厂商 ID 和设备 ID
 * @return 16位 ID 值（高字节为厂商 ID，低字节为设备 ID）
 * @note  使用 REMS (0x90) 命令读取，地址固定为 0x000000
 *        GD25Q32C 的 ID 通常为 0xC840（厂商 GigaDevice）
 */
u16 Get_ID()
{
	u16 ID = 0xffff;							   // 初始化为无效值
	u32 Address = 0x000000;						   // 固定地址（REMS 命令要求）
	spi_TX[0] = REMS;							   // 发送读取厂商 ID 命令 (0x90)
	spi_TX[1] = ((Address >> 16) & 0xff);		   // 地址高字节
	spi_TX[2] = ((Address >> 8) & 0xff);		   // 地址中字节
	spi_TX[3] = (Address & 0xff);				   // 地址低字节
	NSS_RESET();								   // 选中 Flash
	send_recv_Async(&spi_TX[0], &spi_RX[0], 4, 2); // 发送4字节，接收2字节
	NSS_SET();									   // 释放 Flash

	ID = (spi_RX[0] << 8) | spi_RX[1]; // 组合厂商ID和设备ID
	return ID;
}

/**
 * @brief 读取芯片完整识别信息（厂商 ID + 存储类型 + 容量）
 * @return 32 位 ID 值，格式：[厂商ID][存储类型][容量][0x00]
 * @note  使用 RDID (0x9F) 命令读取 3 字节
 *        GD25Q32C 典型返回值：0xC8401600
 *        C8 = GigaDevice, 40 = SPI Flash, 16 = 32Mbit (4MB)
 */
u32 Get_Identification()
{
	u32 DID = 0xffffff00;											// 初始化为无效值
	spi_TX[0] = RDID;												// 读 JEDEC ID 命令 (0x9F)
	NSS_RESET();													// 选中 Flash
	send_recv_Async(&spi_TX[0], &spi_RX[0], 1, 3);					// 发送1字节命令，接收3字节 ID
	NSS_SET();														// 释放 Flash
	DID = (spi_RX[0] << 24) | (spi_RX[1] << 16) | (spi_RX[2] << 8); // 组合为32位 ID
	return DID;
}

/**
 * @brief 进入深度休眠模式
 * @note  使用 DP (0xB9) 命令
 *        进入后功耗降至 1μA 以下
 *        所有操作停止，不响应除了 RDI 之外的命令
 *        适用于低功耗待机场景
 */
void Enable_Deep_PowerDown()
{
	spi_TX[0] = DP;		 // 深度掉电命令 (0xB9)
	NSS_RESET();		 // 选中 Flash
	send(&spi_TX[0], 1); // 发送1字节命令
	NSS_SET();			 // 释放 Flash
}

/**
 * @brief 退出深度休眠模式
 * @note  使用 RDI (0xAB) 命令
 *        退出后 Flash 恢复正常工作状态
 *        退出后需等待 tRDP（约 3μs）才能进行其他操作
 */
void Release_Deep_PowerDown()
{
	spi_TX[0] = RDI;	 // 退出深度掉电命令 (0xAB)
	NSS_RESET();		 // 选中 Flash
	send(&spi_TX[0], 1); // 发送1字节命令
	NSS_SET();			 // 释放 Flash
}

/**
 * @brief 读取指定状态寄存器的值
 * @param Command 寄存器读取命令（RDSR0=0x05 / RDSR1=0x35 / RDSR2=0x15）
 * @return 状态寄存器的 8 位值
 * @note  常用检查位：
 *        RDSR0 bit0 (WIP): 写操作进行中标志，1=忙, 0=空闲
 *        RDSR0 bit1 (WEL): 写使能锁存标志，1=已使能
 *        RDSR1 bit1 (QE):  Quad 使能标志
 */
u8 Read_Status_Register(u8 Command)
{
	spi_TX[0] = Command;						  // 状态寄存器读取命令
	u8 raw_Data = 0xff;							  // 初始化为全 1（无效值）
	NSS_RESET();								  // 选中 Flash
	send_recv_Async(&spi_TX[0], &raw_Data, 1, 1); // 发送命令，读取1字节状态
	NSS_SET();									  // 释放 Flash
	return raw_Data;
}

/**
 * @brief 写入指定状态寄存器的值
 * @param Command 寄存器写入命令（WRSR0=0x01 / WRSR1=0x31 / WRSR2=0x11）
 * @param Data    要写入的 8 位数据
 * @note  写入前需先调用 Write_Enable() 使能写操作
 *        修改状态寄存器可能影响保护位和 QE 位，请谨慎操作
 */
void Write_Status_Register(u8 Command, u8 Data)
{
	Write_Enable();

	spi_TX[0] = Command; // 状态寄存器写入命令 (WRSR0/1/2)
	spi_TX[1] = Data;	 // 待写入的 8 位数据
	NSS_RESET();		 // 选中 Flash
	send(&spi_TX[0], 2); // 发送命令 + 数据
	NSS_SET();			 // 释放 Flash
}

/**
 * @brief 发送易失性状态寄存器写使能命令
 * @note  VSR_WREN 命令字节：0x50
 *        与普通 WREN (0x06) 不同，此命令允许直接写入状态寄存器
 *        写入后状态寄存器值在断电后丢失，适合临时修改
 *        常用于修改保护位或 QE 位而不想永久改变 Flash 配置
 */
void Write_Enable_for_Volatile_Status_Register()
{
	spi_TX[0] = VSR_WREN; // 写使能命令 (0x50)
	NSS_RESET();		  // 选中 Flash
	send(&spi_TX[0], 1);  // 发送1字节命令
	NSS_SET();			  // 释放 Flash
}

/**
 * @brief 向指定安全寄存器写入数据
 * @param Register 目标安全寄存器编号（1 / 2 / 3）
 * @param Address  目标起始偏移地址（12 位：高 4 位为寄存器编号，低 8 位为寄存器内偏移）
 * @param pdata 待写入的数据缓冲区指针
 * @param len 待写入的字节数，超过 1024 时函数直接返回
 * @note  PSER 命令字节：0x42
 *        安全寄存器共 3 个，每个 1024 字节
 *        写入前会自动调用 Idle_Check() 检查 Flash 是否空闲
 */
void Program_Security_Registers(u8 Register, u16 Address, u8 *pdata, u16 len)
{
	if (len > 1024) // 单次写入上限 1024 字节（3 个寄存器总容量）
	{
		return;
	}
	if (Idle_Check() == false) // 检查 Flash 是否空闲，忙则放弃本次操作
	{
		return;
	}
	Write_Enable();
	spi_TX[0] = PSER;									   // 安全寄存器编程命令 (0x42)
	spi_TX[1] = 0x00;									   // 字节 0 为 dummy（固定填 0）
	spi_TX[2] = (Register << 4) | ((Address >> 8) & 0b11); // 高 4 位为寄存器编号，低 2 位为地址高位
	spi_TX[3] = Address & 0xff;							   // 地址低字节 (A7-A0)
	NSS_RESET();										   // 选中 Flash
	send(&spi_TX[0], 4);								   // 发送命令 + 地址
	send(pdata, len);									   // 发送数据
	NSS_SET();											   // 释放 Flash
}

/**
 * @brief 读取指定安全寄存器中的数据
 * @param Register 目标安全寄存器编号（1 / 2 / 3）
 * @param Address  目标起始偏移地址（12 位：高 4 位为寄存器编号，低 8 位为寄存器内偏移）
 * @param data_buf 用于存放读出数据的缓冲区指针
 * @param len 读取的字节数
 * @note  RSER 命令字节：0x48
 *        地址字段后需额外发送 1 字节 dummy (0xFF) 再开始接收数据
 *        安全寄存器共 3 个，每个 1024 字节
 *        读取前会自动调用 Idle_Check() 检查 Flash 是否空闲
 */
void Read_Security_Registers(u8 Register, u16 Address, u8 *data_buf, u16 len)
{
	if (Idle_Check() == false) // 检查 Flash 是否空闲，忙则放弃本次操作
	{
		return;
	}
	spi_TX[0] = RSER;									   // 安全寄存器读取命令 (0x48)
	spi_TX[1] = 0x00;									   // 字节 0 为 dummy（固定填 0）
	spi_TX[2] = (Register << 4) | ((Address >> 8) & 0b11); // 高 4 位为寄存器编号，低 2 位为地址高位
	spi_TX[3] = Address & 0xff;							   // 地址低字节 (A7-A0)
	spi_TX[4] = 0xff;									   // 接收阶段的 dummy 字节
	NSS_RESET();										   // 选中 Flash
	send_recv_Async(&spi_TX[0], data_buf, 5, len);		   // 发送命令+地址+dummy，接收数据
	NSS_SET();											   // 释放 Flash
	return;
}

/**
 * @brief 擦除指定安全寄存器
 * @param Register 目标安全寄存器编号（1 / 2 / 3）
 * @note  ESER 命令字节：0x44
 *        擦除后该寄存器内所有数据置为 0xFF
 *        安全寄存器共 3 个，每个 1024 字节
 *        擦除前会自动调用 Idle_Check() 检查 Flash 是否空闲
 */
void Erase_Security_Registers(u8 Register)
{
	if (Idle_Check() == false) // 检查 Flash 是否空闲，忙则放弃本次操作
	{
		return;
	}
	Write_Enable();
	spi_TX[0] = ESER;			 // 安全寄存器擦除命令 (0x44)
	spi_TX[1] = 0x00;			 // 字节 0 为 dummy（固定填 0）
	spi_TX[2] = (Register << 4); // 高 4 位为寄存器编号，低 4 位填 0（整寄存器擦除）
	spi_TX[3] = 0x00;			 // 地址低字节填 0
	NSS_RESET();				 // 选中 Flash
	send(&spi_TX[0], 4);		 // 发送命令 + 地址
	NSS_SET();					 // 释放 Flash
	return;
}
