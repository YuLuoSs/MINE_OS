#ifndef __GLOBAL_CONFIG_H__
#define __GLOBAL_CONFIG_H__

#define CONFIG_S3C2440
#define CONFIG_S3C2410

#define BOOT_FORM_STEPPINGSTONE	0
#define BOOT_FORM_SDRAM			1
#define BOOT_MODE				BOOT_FORM_SDRAM
//TODO
#define UND_STACK_BASE_ADDR	0x33e80000
#define ABT_STACK_BASE_ADDR	0x33e80000
#define IRQ_STACK_BASE_ADDR	0x33d80000
#define FIQ_STACK_BASE_ADDR	0x33d80000
#define SWI_STACK_BASE_ADDR	0x33f00000

#define MUM_TLB_BASE_ADDR	0x30000000

#define PHYSICAL_MEM_ADDR	0x30000000
#define VIRTUAL_MEM_ADDR	0x30000000
#define MEM_MAP_SIZE		0x4000000

#define PHYSICAL_IO_ADDR	0x48000000
#define VIRTUAL_IO_ADDR		0x48000000
#define IO_MAP_SIZE			0x18000000

//#define PHYSICAL_KERNEL_ADDR		0x33f00000
//#define VIRTUAL_KERNEL_ADDR			0xfff00000
//#define KERNEL_SIZE					0x00100000
#endif
