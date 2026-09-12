/*
 * GD25Q32C.h
 *
 *  Created on: 2026年8月19日
 *      Author: haoluan
 */

/* ============================================================================
 * GD25Q32C 驱动模式支持声明
 * ----------------------------------------------------------------------------
 * 本库仅支持标准 SPI（Standard SPI / Single SPI）模式。
 *
 * 不支持：
 *   - Dual SPI（双线 SPI）
 *   - Quad SPI（四线 SPI）
 *   - QPI（Quad Peripheral Interface）模式
 *
 * 本库所有操作均基于标准 SPI 命令，例如：
 *   0x03 READ、0x02 PP、0x20 SE、0x06 WREN、0x05 RDSR0、0x90 REMS 等。
 *
 * 本库不会使用，也不保证兼容以下命令：
 *   0x3B / 0xBB  - Dual SPI 读
 *   0x6B / 0xEB  - Quad SPI 读
 *   0x32         - Quad 页编程
 *   0x38         - 进入 QPI 模式
 *
 * 使用前请确保：
 *   1. Flash 的 QE 位（状态寄存器2 的 bit1）为 0；
 *   2. 芯片未进入 Dual / Quad / QPI 模式；
 *   3. SPI 使用标准单线 MOSI/MISO 连接。
 *
 * 若芯片已被配置为 Dual / Quad / QPI 模式，请先退出该模式并恢复标准 SPI，
 * 否则本库的读取、写入、擦除等操作将无法正常工作。
 *
 * 如需 Dual SPI 或 Quad SPI 支持，请另行实现或使用支持该模式的驱动。
 * ============================================================================
 */

#ifndef INC_GD25Q32C_H_
#define INC_GD25Q32C_H_

#include <stdbool.h>
#include <stdint.h>

// Configure
#define HSPI hspi1

// commands
#define WREN 0x06
#define WRDI 0x04

#define SE 0x20
#define BE32K 0x52
#define BE64K 0xd8

#define PP 0x02
#define READ 0x03

#define ERST 0x66
#define RST 0x99

#define RDSR0 0x05
#define RDSR1 0x35
#define RDSR2 0x15
#define WRSR0 0x01
#define WRSR1 0x31
#define WRSR2 0x11

#define REMS 0x90
#define RDID 0x9f

#define DP 0xb9
#define RDI 0xab

// function
void Write_Enable();
void Write_Disable();
bool Idle_Check();
void Reset_flash();
void Sector_Erase(unsigned int Address);
void Block_Erase(unsigned int Address, bool Is64K);
void Page_Program(unsigned int Address, unsigned char *pdata, unsigned short len);
void Read_Data(unsigned int Address, unsigned char *data_buf, unsigned int len);
unsigned short Get_ID();
void NSS_RESET();
void NSS_SET();
unsigned int Get_Identification();
void Enable_Deep_PowerDown();
void Release_Deep_PowerDown();
unsigned char Read_Status_Register(unsigned char Command);
void Write_Status_Register(unsigned char Command, unsigned char Data);

#define Read_Status_Register_0() Read_Status_Register(RDSR0)
#define Read_Status_Register_1() Read_Status_Register(RDSR1)
#define Read_Status_Register_2() Read_Status_Register(RDSR2)
#define Write_Status_Register_0(RAWDATA) Write_Status_Register(WRSR0, RAWDATA)
#define Write_Status_Register_1(RAWDATA) Write_Status_Register(WRSR1, RAWDATA)
#define Write_Status_Register_2(RAWDATA) Write_Status_Register(WRSR2, RAWDATA)

#define Block_Erase_32K(ADDRESS) Block_Erase(ADDRESS, false)
#define Block_Erase_64K(ADDRESS) Block_Erase(ADDRESS, true)
#endif /* INC_GD25Q32C_H_ */

/*
DESC.		|BLOCK|SECTOR|PAGE|ADD. IN PAGE|
HEX 		|F   F|		F|	 F|F          F|
*/

/*
 ** Command-HEX-Describe Table **
Cmd     HEX     Desc.
WREN    0x06    Write Enable
WRDI	0x04	Write Disable
SE      0x20    Sector Erase
BE32K	0x52	Block Erase(32K)
BE64K	0xD8	Block Erase(64K)
PP      0x02    Page Program
READ    0x03    Read Data Bytes
ERST    0x66    Enable Reset
RST     0x99    Reset
RDSR0   0x05    Read Status Register 0 (s0~s7)
RDSR1   0x35    Read Status Register 1 (s8~s15)
RDSR2   0x15    Read Status Register 2 (s16~s23)
WRSR0 	0x01	Write Status Register 0 (s0~s7)
WRSR1 	0x31	Write Status Register 1 (s8~s15)
WRSR2 	0x11	Write Status Register 2 (s16~s23)
REMS    0x90    Read Manufacture ID / Device ID
RDID	0x9F	Read Identification
DP		0xB9	Deep Power-Down
RDI		0xAB	Release from Deep Power-Down or High Performance Mode(This library don't feature Read Device ID)
*/