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
 *   1. Flash 的 QE 位（状态寄存器1 的 bit1）为 0；
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
#define GD25Q32C_HSPI hspi1
#define GD25Q32C_FLASH_CS_PORT GPIOA
#define GD25Q32C_FLASH_CS_PIN GPIO_PIN_4

// function
void GD25Q32C_Write_Enable(void);
void GD25Q32C_Write_Disable(void);
bool GD25Q32C_Idle_Check(void);
void GD25Q32C_Reset_flash(void);
void GD25Q32C_Sector_Erase(unsigned int Address);
void GD25Q32C_Block_Erase(unsigned int Address, bool Is64K);
void GD25Q32C_Chip_Erase(void);
void GD25Q32C_Page_Program(unsigned int Address, unsigned char *pdata, unsigned short len);
void GD25Q32C_Read_Data(unsigned int Address, unsigned char *data_buf, unsigned int len);
void GD25Q32C_Suspend_Writing_Cycle(void);
void GD25Q32C_Resume_Writing_Cycle(void);
unsigned short GD25Q32C_Get_ID(void);
void GD25Q32C_NSS_RESET(void);
void GD25Q32C_NSS_SET(void);
unsigned int GD25Q32C_Get_Identification(void);
void GD25Q32C_Enable_Deep_Power_Down(void);
void GD25Q32C_Release_Deep_Power_Down(void);
unsigned char GD25Q32C_Read_Status_Register(unsigned char Command);
void GD25Q32C_Write_Status_Register(unsigned char Command, unsigned char Data);
void GD25Q32C_Program_Security_Registers(unsigned char Register, unsigned short Address, unsigned char *pdata,
										 unsigned short len);
void GD25Q32C_Read_Security_Registers(unsigned char Register, unsigned short Address, unsigned char *data_buf,
									  unsigned short len);
void GD25Q32C_Erase_Security_Registers(unsigned char Register);
void GD25Q32C_Write_Enable_for_Volatile_Status_Register(void);

#define GD25Q32C_Read_Status_Register_0() GD25Q32C_Read_Status_Register(0x05)
#define GD25Q32C_Read_Status_Register_1() GD25Q32C_Read_Status_Register(0x35)
#define GD25Q32C_Read_Status_Register_2() GD25Q32C_Read_Status_Register(0x15)
#define GD25Q32C_Write_Status_Register_0(RAWDATA) GD25Q32C_Write_Status_Register(0x01, RAWDATA)
#define GD25Q32C_Write_Status_Register_1(RAWDATA) GD25Q32C_Write_Status_Register(0x31, RAWDATA)
#define GD25Q32C_Write_Status_Register_2(RAWDATA) GD25Q32C_Write_Status_Register(0x11, RAWDATA)

#define GD25Q32C_Block_Erase_32K(ADDRESS) GD25Q32C_Block_Erase(ADDRESS, false)
#define GD25Q32C_Block_Erase_64K(ADDRESS) GD25Q32C_Block_Erase(ADDRESS, true)
#endif /* INC_GD25Q32C_H_ */

/*
DESC.		|BLOCK|SECTOR|PAGE|ADD. IN PAGE|
HEX 		|F   F|		F|	 F|F          F|
*/

/*
 ** Command-HEX-Describe Table **
Cmd   		HEX     Desc.
WREN   		0x06    Write Enable
WRDI		0x04	Write Disable
SE      	0x20    Sector Erase
BE32K		0x52	Block Erase(32K)
BE64K		0xD8	Block Erase(64K)
CE			0xC7	Chip Erase
PP      	0x02    Page Program
PES			0x75	Program/Erase Suspend
PER 		0x7A	Program/Erase Resume
READ    	0x03    Read Data Bytes
ERST    	0x66    Enable Reset
RST     	0x99    Reset
RDSR0   	0x05    Read Status Register 0 (s0~s7)
RDSR1   	0x35    Read Status Register 1 (s8~s15)
RDSR2   	0x15    Read Status Register 2 (s16~s23)
WRSR0 		0x01	Write Status Register 0 (s0~s7)
WRSR1 		0x31	Write Status Register 1 (s8~s15)
WRSR2 		0x11	Write Status Register 2 (s16~s23)
VSR_WREN	0x50	Write Enable for Volatile Status Register
REMS   		0x90    Read Manufacture ID / Device ID
RDID		0x9F	Read Identification
DP			0xB9	Deep Power-Down
RDI			0xAB	Release from Deep Power-Down or High Performance Mode(This library don't feature Read Device ID)
ESER		0x44	Erase Security Registers
PSER		0x42	Program Security Registers
RSER		0x48	Read Security Registers
*/