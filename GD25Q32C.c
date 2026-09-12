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
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); // PA4 输出低电平
}

/**
 * @brief 片选信号拉高，释放 Flash 芯片
 * @note  操作完成后必须调用此函数释放 Flash
 */
void NSS_SET()
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); // PA4 输出高电平
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
 *        擦除前会自动调用 Idle_Check() 等待 Flash 空闲
 *        SE 命令字节：0x20
 */
void Sector_Erase(u32 Address)
{
	if (Idle_Check() == false) // 等待 Flash 空闲
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

void Block_Erase(u32 Address, bool Is64K)
{
	if (Idle_Check() == false) // 等待 Flash 空闲
	{
		return;
	}

	spi_TX[0] = BE32K;
	if (Is64K == true)
	{
		spi_TX[0] = BE64K;
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
	spi_TX[0] = ERST;	 // 使能复位命令 (0x66)
	send(&spi_TX[0], 1); // 发送命令
	HAL_Delay(1);		 // 等待 1ms
	spi_TX[0] = RST;	 // 复位命令 (0x99)
	send(&spi_TX[0], 1); // 发送命令
	HAL_Delay(20);		 // 等待 20ms 复位完成
}

/**
 * @brief 向 Flash 写入数据（页编程，单次最多 256 字节）
 * @param Address 目标写入地址（24 位）
 * @param pdata 待写入的数据缓冲区指针
 * @param len 待写入的字节数，超过 256 时函数直接返回
 * @note  页大小为 256 字节，写入不能跨越页边界
 *        写入前会自动调用 Idle_Check() 等待 Flash 空闲
 *        PP 命令字节：0x02
 *        延迟 25ms 等待页编程完成
 */
void Page_Program(u32 Address, u8 *pdata, u16 len)
{
	if (len > 256) // 页编程最大 256 字节
	{
		return;
	}
	if (Idle_Check() == false) // 等待 Flash 空闲
	{
		return;
	}
	Write_Enable();
	spi_TX[0] = PP;						  // 页编程命令 (0x02)
	spi_TX[1] = ((Address >> 16) & 0xff); // 地址高字节 (A23-A16)
	spi_TX[2] = ((Address >> 8) & 0xff);  // 地址中字节 (A15-A8)
	spi_TX[3] = (Address & 0xff);		  // 地址低字节 (A7-A0)
	for (u16 index = 0; index < len; index++)
	{
		spi_TX[4 + index] = *(pdata + index); // 复制数据到发送缓冲区			//TODO:不拷贝数据到缓冲区
	}
	NSS_RESET();			   // 选中 Flash
	send(&spi_TX[0], len + 4); // 发送命令 + 地址 + 数据
	NSS_SET();				   // 释放 Flash
}

/**
 * @brief 从 Flash 指定地址读取数据
 * @param Address 起始读取地址（24 位）
 * @param data_buf 用于存放读出数据的缓冲区指针
 * @param len 读取的字节数
 * @note  读取操作可以读取任意长度的数据（无页边界限制）
 *        读取前会自动调用 Idle_Check() 等待 Flash 空闲
 *        READ 命令字节：0x03
 *        延迟 50ms 等待数据稳定
 */
void Read_Data(u32 Address, u8 *data_buf, u32 len)
{
	if (Idle_Check() == false) // 等待 Flash 空闲
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
	u32 DID = 0xffffff00;
	spi_TX[0] = RDID;
	NSS_RESET();
	send_recv_Async(&spi_TX[0], &spi_RX[0], 1, 3);
	NSS_SET();
	DID = (spi_RX[0] << 24) | (spi_RX[1] << 16) | (spi_RX[2] << 8);
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
	spi_TX[0] = DP;
	NSS_RESET();
	send(&spi_TX[0], 1);
	NSS_SET();
}

/**
 * @brief 退出深度休眠模式
 * @note  使用 RDI (0xAB) 命令
 *        退出后 Flash 恢复正常工作状态
 *        退出后需等待 tRDP（约 3μs）才能进行其他操作
 */
void Release_Deep_PowerDown()
{
	spi_TX[0] = RDI;
	NSS_RESET();
	send(&spi_TX[0], 1);
	NSS_SET();
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
	spi_TX[0] = Command;
	u8 raw_Data = 0xff;
	NSS_RESET();
	send_recv_Async(&spi_TX[0], &raw_Data, 1, 1);
	NSS_SET();
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

	spi_TX[0] = Command;
	spi_TX[1] = Data;
	NSS_RESET();
	send(&spi_TX[0], 2);
	NSS_SET();
}
