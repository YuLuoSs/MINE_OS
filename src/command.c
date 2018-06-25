#include <sys/types.h>
#include <assert.h>
#include <timer.h>
#include <usb/2440usb.h>
#include "command.h"
#include "ff.h"

#define CMD_MAX_CMD_NUM 50
#define CMD_MAXARGS 10
extern cmd_table *ct_list[];

DIR  dirobj;               // current work dir fof cd
void FileAttr(BYTE attr, char *p) {
	if ((attr & 0x10) == 0x10) {
		sprintf(p, "%5s", "dir :");
	} else {
		sprintf(p, "%5s", "file:");
	}

}
CMD_DEFINE(ls, "ls", "ls") {
	char p_cmd[16], p_arg[32];
	char *p_path, *pfname;
	FRESULT read_res, dir_res;
	DIR  tempdir;
	FILINFO tempfinfo;
	char fdesp[8];
	FRESULT res1 = f_opendir(&dirobj, "/");

	if (res1 == FR_OK)
		if (argc == 1) {
			p_path = "/";
		} else if (argc == 2) {
			p_path = argv[1];
		} else {
			return 1;
		}
	dir_res = f_opendir(&tempdir, p_path);
	if (dir_res != FR_OK) {
		printf("f_opendir failed,path:%s does not exist\n\r", p_path);
		return 1;
	}
	for (;;) {
		read_res = f_readdir(&tempdir, &tempfinfo);
		if ((read_res != FR_OK) || (tempfinfo.fname[0] == 0)) {
			break;
		} else if (tempfinfo.fname[0] == '.') {
			continue;
		} else {
			pfname = tempfinfo.fname;
			FileAttr((tempfinfo.fattrib), fdesp);
			printf("%s   %-15s  %8dbyte\n\r", fdesp, pfname, tempfinfo.fsize);
		}
	}
	return 0;
}
CMD_DEFINE(wav, "wav", "wav") {
	if (argc != 2)
		return 1;
	read_wav_file(argv[1]);
	return 0;
}
CMD_DEFINE(usbdebug, "usbdebug", "usbdebug") {
#if USB_DEBUG == 1
	DbgPrintf("show");
#endif
	return 0;
}
CMD_DEFINE(usbtest, "usbtest", "usbtest") {
	printf("USB slave ����\n");
	usb_init_slave();
	return 0;
}
CMD_DEFINE(backtrace, "backtrace", "backtrace") {
	printf("backtrace����\n");
	char s[128];
	for (int i = 0; i < 128 / 3; i++) {
		*(volatile int *)(s + 3 * i) = 0;
	}
	return 0;
}
CMD_DEFINE(ts_test, "ts_test", "ts_test") {
	ts_test_view();
	return 0;
}
CMD_DEFINE(test, "test", "test") {
	ten_test_view();
	return 0;
}
CMD_DEFINE(lcd_test, "lcd_test", "lcd_test") {
	lcd_putstr(0, 0, "11113145623vdfhigaeruirh4uifthv89y9q3ry478h7f@#$%^^!@#$%^&*((*)_+-=\":>?<{}|;'][]\./,./");
	return 0;
}
CMD_DEFINE(usbmouse, "usbmouse", "usbmouse") {
#if 1
	while (1) {
		U8 Buf[4] = {0, 0, 0, 0};
		switch (getc()) {
			case 'a':
				Buf[1] = -1;	//����һ�������ƶ�һ����λ��
				break;
			case 'd':
				Buf[1] = 1;		//����һ�������ƶ�һ����λ��
				break;
			case 'w':
				Buf[2] = -1;	//����һ�������ƶ�һ����λ��
				break;
			case 's':
				Buf[2] = 1;		//����һ�������ƶ�һ����λ��
				break;
			case 'j':
				Buf[0] |= 0x01;	//D0Ϊ������
				break;
			case 'k':
				Buf[0] |= 0x02;	//D1Ϊ����Ҽ�
				break;
			case 'q':
			case 'Q':
				return 0;
				break;
			default:
				break;
		}
		usb_send_init(EP1, Buf, sizeof(Buf));
		usb_send_message(EP1);
	}
#endif
	return 0;
}
CMD_DEFINE(delay_u, "delay_u", "delay_u") {
	if (argc != 2)
		return 1;
	int time = simple_strtoul(argv[1], NULL, 10);
	run_command("RTC");
	for (int i = time; i > 0; i -= 60000)
		delay_u((time > 60000) ? 60000 : time);
	run_command("RTC");
	return 0;
}
CMD_DEFINE(udelay, "udelay", "udelay") {
	run_command("RTC");
	if (argc != 2)
		return 1;
	udelay(simple_strtoul(argv[1], NULL, 10));
	run_command("RTC");
	return 0;
}
CMD_DEFINE(wr_at24xx, "wr_at24xx", "wr_at24xx") {
	if (argc != 3)
		return 1;
	int err;

	/* ��õ�ַ */
	unsigned int addr = simple_strtoul(argv[1], NULL, 10);
	if (addr > 256) {
		printf("address > 256, error!\n");
		return;
	}
	printf("[write:%d][%s]\n", addr, argv[2]);
	err = at24cxx_write(addr, argv[2], strlen(argv[2]) + 1);
	if (err)
		printf("[error]\n");
	return 0;
}
CMD_DEFINE(rd_at24xx, "rd_at24xx", "rd_at24xx addr len") {
	unsigned char data[100];
	unsigned char str[16];
	int err;
	int cnt = 0;

	if (argc != 3)
		return 1;
	/* ��õ�ַ */
	unsigned int addr = simple_strtoul(argv[1], NULL, 10);

	if (addr > 256) {
		printf("address > 256, error!\n");
		return;
	}

	/* ��ó��� */
	int len = simple_strtoul(argv[2], NULL, 10);
	err = at24cxx_read(addr, data, len);
	if (err)
		printf("[error]\n");
	printf("[data view]\n");

	for (int i = 0; i < (len + 15) / 16; i++) {
		/* ÿ�д�ӡ16������ */
		for (int j = 0; j < 16; j++) {
			/* �ȴ�ӡ��ֵ */
			unsigned char c = data[cnt++];
			str[j] = c;
			if (j < len - i * 16)
				printf("%02x ", c);
			else
				printf("   ");
		}

		printf("   ; ");

		for (int j = 0; j < len - i * 16; j++) {
			/* ���ӡ�ַ� */
			if (str[j] < 0x20 || str[j] > 0x7e)  /* �������ַ� */
				putchar('.');
			else
				putchar(str[j]);
		}
		printf("\n");
	}
	return 0;
}
CMD_DEFINE(i2c_init, "i2c_init", "i2c_init") {
	i2c_init();
	return 0;
}
CMD_DEFINE(i2c_test, "i2c_test", "i2c_test") {
	run_command("i2c_init");
	run_command("wr_at24xx 0 helloworld!");
	run_command("rd_at24xx 0 20");
	return 0;
}
CMD_DEFINE(adc_test, "adc_test", "adc_test") {
	int vol0, vol1;
	int t0, t1;
	printf("Measuring the voltage of AIN0 and AIN1, press any key to exit\n");
	while (!serial_getc_async()) {  // ���������룬�򲻶ϲ���
		get_adc(&vol0, &t0, 0);
		get_adc(&vol1, &t1, 2);
		printf("AIN0 = %d.%-3dV    AIN2 = %d.%-3dV\r", (int)vol0, t0, (int)vol1, t1);
	}
	return 0;
}
CMD_DEFINE(res_test, "res_test", "res_test") {
	photoresistor_test();
	return 0;
}
CMD_DEFINE(dh_test, "dh_test", "dh_test") {
	dht11_test();
	return 0;
}
CMD_DEFINE(ds_test, "ds_test", "ds_test") {
	ds18b20_test();
	return 0;
}
CMD_DEFINE(irda_raw, "irda_raw", "irda_raw") {
	irda_raw_test();
	return 0;
}
CMD_DEFINE(irda_nec, "irda_nec", "irda_nec") {
	irda_nec_test();
	return 0;
}
CMD_DEFINE(bmp_test, "bmp_test", "bmp_test") {
	drawImage("xx01.bmp");
	return 0;
}
CMD_DEFINE(RTC, "RTC", "RTC") {
	char data[7] = {0};
	char *week_str[7] = {"һ", "��", "��", "��", "��", "��", "��"};
	char *week;
	if (argc == 1) {
		RTC_Read(&data[0], &data[1], &data[2], &data[3], &data[4], &data[5], &data[6]);

		if (data[3] >= 1 && data[3] <= 7) {
			week = week_str[data[3] - 1];
			printf("%d��,%d��,%d��,����%s,%d��,%d��,%d��\n", 2000 + data[0],
				   data[1], data[2], week, data[4], data[5], data[6]);
		} else {
			printf("error!\n");
			return 1;
		}
	} else if (argc == 8) {
		for (int i = 0; i < 7; i++) {
			data[i] = simple_strtoul(argv[i + 1], NULL, 10);
		}
		//year:0-99
		RTC_Set(data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
		if (data[3] >= 1 && data[3] <= 7) {
			week = week_str[data[3] - 1];
			printf("%d��,%d��,%d��,����%s,%d��,%d��,%d��\n", 2000 + data[0],
				   data[1], data[2], week, data[4], data[5], data[6]);
			printf("���óɹ�\n");
		} else {
			printf("error!\n");
			return 1;
		}
	} else {
		printf("error!���������쳣\n");
		return 1;
	}
	return 0;
}
CMD_DEFINE(spi_test, "spi_test", "spi_test") {
	int vol0, vol1;
	int t0, t1;
	char str[200];
	unsigned int mid, pid;
	SPIInit();
	OLEDInit();
	OLEDPrint(0, 0, "www.100ask.net");

	SPIFlashReadID(&mid, &pid);
	printf("SPI Flash : MID = 0x%02x, PID = 0x%02x\n", mid, pid);

	sprintf(str, "SPI : %02x, %02x", mid, pid);
	OLEDPrint(2, 0, str);

	SPIFlashInit();

	SPIFlashEraseSector(4096);
	SPIFlashProgram(4096, "100ask", 7);
	SPIFlashRead(4096, str, 7);
	printf("SPI Flash read from 4096: %s\n", str);
	OLEDPrint(4, 0, str);

	i2c_init();

	OLEDClearPage(2);
	OLEDClearPage(3);

	printf("Measuring the voltage of AIN0 and AIN1, press any key to exit\n");
	while (!serial_getc_async()) {  // ���������룬�򲻶ϲ���
		get_adc(&vol0, &t0, 0);
		get_adc(&vol1, &t1, 2);
		//printf("AIN0 = %d.%-3dV    AIN2 = %d.%-3dV\r", (int)vol0, t0, (int)vol1, t1);
		sprintf(str, "ADC: %d.%-3d, %d.%-3d", (int)vol0, t0, (int)vol1, t1);
		OLEDPrint(6, 0, str);
	}
	return 0;
}
CMD_DEFINE(usbslave,
		   "usbslave - get file from host(PC)",
		   "[loadAddress] [wait] \n"
		   "\"wait\" is 0 or 1, 0 means for return immediately, not waits for the finish of transferring") {
#if 0
	//TODO:��ý��ļ����ص��ļ�ϵͳ��
	extern int download_run;
	extern volatile U32 dwUSBBufBase;
	extern volatile U32 dwUSBBufSize;

	int wait = 1;
#define BUF_SIZE (1024*1024)
	/* download_runΪ1ʱ��ʾ���ļ�������USB Host���͹���dnwָ����λ��
	 * download_runΪ0ʱ��ʾ���ļ������ڲ���argv[2]ָ����λ��
	 * Ҫ���س����ڴ棬Ȼ��ֱ������ʱ��Ҫ����download_run=1����Ҳ������������ֵ�����
	 */
	//����0x3000000�����ҳ������download_run = 0ȷ�����ص�ַ��ȷ������������λ�����õĵ�ַ
	download_run = 0;//Ĭ������λ��������ַ�ʹ�С
	if (argc == 2) {
		//dwUSBBufBase = kmalloc(BUF_SIZE);
		dwUSBBufBase = 0x30a00000;
		if (!dwUSBBufBase) {
			printf("malloc memory error!\n");
			return 1;
		}
		wait = (int)simple_strtoul(argv[1], NULL, 16);
		dwUSBBufSize = BUF_SIZE;
	} else {
		return 1;
	}
	usb_init_slave();
	int size = usb_receive(dwUSBBufBase, dwUSBBufSize, wait);
	assert(size > 0 && size <= BUF_SIZE);
#endif
	return 0;
}
CMD_DEFINE(help, "help", "help") {
	for (int i = 0; ct_list[i] != NULL; i++) {
		printf("%-20s:\t-%s\n", ct_list[i]->name, ct_list[i]->usage);
	}
	return 0;
}
#define CMD_ENTRY(x) & ct_##x
cmd_table *ct_list[] = {
	CMD_ENTRY(help),
	CMD_ENTRY(ls),
	CMD_ENTRY(wav),
	CMD_ENTRY(usbslave),
	CMD_ENTRY(delay_u),
	CMD_ENTRY(udelay),
	CMD_ENTRY(usbdebug),
	CMD_ENTRY(usbmouse),
	CMD_ENTRY(usbtest),
	CMD_ENTRY(ts_test),
	CMD_ENTRY(test),
	CMD_ENTRY(lcd_test),
	CMD_ENTRY(wr_at24xx),
	CMD_ENTRY(rd_at24xx),
	CMD_ENTRY(i2c_init),
	CMD_ENTRY(i2c_test),
	CMD_ENTRY(adc_test),
	CMD_ENTRY(spi_test),
	CMD_ENTRY(res_test),
	CMD_ENTRY(dh_test),
	CMD_ENTRY(ds_test),
	CMD_ENTRY(RTC),
	CMD_ENTRY(irda_raw),
	CMD_ENTRY(irda_nec),
	CMD_ENTRY(bmp_test),
	CMD_ENTRY(backtrace),
	NULL
};
cmd_table *search_cmd(char *name) {
	for (int i = 0; ct_list[i] != NULL; i++) {
		if (strcmp(ct_list[i]->name, name) == 0) {
			return ct_list[i];
		}
	}
	return NULL;
}
int run_command(char *cmd) {
	char str[256] = {
		[255] = 0
	};
	strncpy(str, cmd, 255);

	char *argv[CMD_MAXARGS + 1] = {0};	/* NULL terminated	*/
	int argc = 0;
	int cmdlen = strlen(cmd);

	for (int i = 0; i < cmdlen; i++) {
		if (str[i] != ' ' && i != 0) {
			continue;
		} else {
			while (str[i] == ' ') {
				str[i] = '\0';
				i++;
			}
			if (i < cmdlen) {
				argv[argc] = &str[i];
				argc++;
				if (argc == CMD_MAXARGS + 1)
					return -1;
			} else
				break;
		}
	}
	cmd_table *pct = search_cmd(argv[0]);
	if (pct) {
		pct->cmd(pct, argc, argv);
	} else {
		printf("%s:command not found\n", argv[0]);
		return 0;
	}
	return 1;
}
static int get_str(char *buf, int len) {
	int i;
	for (i = 0; i < len - 1; i++) {
		char c = getc();
		//xshell �س�����\r\n
		if (c == '\r') {
			getc();

			if (i == 0) {
				return -1;
			} else {
				printf("\n");
				buf[i] = '\0';
				break;
			}
		} else if (c == '\b') {
			if (i > 0) { //ǰ�����ַ�
				putc(c);
				i = i - 2;
			} else { //ǰ��û���ַ�
				i = i - 1;
			}
		} else {
			putc(c);
			buf[i] = c;
		}
	}
	return 1;
}
int cmd_loop() {
	char buf[100] = {0};
	while (1) {
		printf("\nOS>");
		if (get_str(buf, 100) == -1)
			continue;
		run_command(buf);
	}
}
