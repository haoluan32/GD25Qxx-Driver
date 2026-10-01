#ifndef INC_GD25Q32C_COMMAND_H_
#define INC_GD25Q32C_COMMAND_H_

#define WREN 0x06
#define WRDI 0x04

#define SE 0x20
#define BE32K 0x52
#define BE64K 0xd8
#define CE 0xc7

#define PP 0x02
#define READ 0x03
#define PES 0x75
#define PER 0x7a

#define ESER 0x44
#define PSER 0x42
#define RSER 0x48

#define ERST 0x66
#define RST 0x99

#define RDSR0 0x05
#define RDSR1 0x35
#define RDSR2 0x15
#define WRSR0 0x01
#define WRSR1 0x31
#define WRSR2 0x11
#define VSR_WREN 0x50

#define REMS 0x90
#define RDID 0x9f

#define DP 0xb9
#define RDI 0xab

#endif
/*
 ** Flash Address-Describe Table**
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