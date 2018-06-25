#include <stdio.h>
#include <s3c24xx.h>
#include <interrupt.h>
#include <serial.h>
#include <assert.h>
#include "command.h"

//lcd driver
#include "lcddrv.h"
#include "framebuffer.h"

//fs
#include "ff.h"
FATFS fatworkarea;         // Work area (file system object) for logical drives 

void show_bss_info(){
	extern char __bss_start, __bss_end;
    unsigned int size = &__bss_end - &__bss_start;
	printf("BSS��СΪ��%uKB, %uMB\n", size/1024, size/1024/1024);
}
static void initer(void (*init)(), char *msg){
	assert(init);
	if(init){
		if(msg)
			printf(msg);
		init();
	}
}

int main() {
	irq_init();
	Port_Init();
	uart0_init();
	//uart0_interrupt_init();
	printf("\n\n************************************************\n");
	show_bss_info();

	printf("��ʼ��MMU...\n");
	mmu_init();

	printf("��ʼ��TIMER...\n");
	timer_init();

	printf("��ʼ��TICK...\n");
	init_tick(1000, NULL);

	printf("��ʼ��LCD...\n");
	Lcd_Port_Init();						// ����LCD����
	Tft_Lcd_Init(MODE_TFT_16BIT_480272);	// ��ʼ��LCD����
	Lcd_PowerEnable(0, 1);					// ����LCD_PWREN��Ч�������ڴ�LCD�ĵ�Դ
	Lcd_EnvidOnOff(1);						// ʹ��LCD����������ź�
	ClearScr(0x0);							// ����

	printf("��ʼ��SD������...\n");
	SDI_init();

	printf("��ʼ��fatfs...\n");
	f_mount(0,&fatworkarea);

	printf("ʹ��IRQ...\n");
	enable_irq();

	cmd_loop();
	while (1);
}
