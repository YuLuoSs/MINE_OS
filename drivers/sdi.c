#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <s3c24xx.h>
#include <timer.h>
#include "sdi.h"

/********************************************************
 �궨��
********************************************************/
#define __SD_MMC_DEBUG__

#ifdef __SD_MMC_DEBUG__
#define SD_DEBUG(fmt,args...) printf(fmt,##args)
#else
#define SD_DEBUG(fmt,args...)
#endif

volatile U16 SD_RCA;
volatile U32 LBA_OFFSET;
volatile U32 TOTAL_SECTOR;
volatile U32 TOTAL_SIZE; //(MB)

#define SDCard_BlockSize	9
#define SDCARD_BUFF_SIZE	512
/********************************************************
 ��������
********************************************************/
SD_STRUCT SDCard;
U8 cTxBuffer[SDCARD_BUFF_SIZE * 2];
U8 cRxBuffer[SDCARD_BUFF_SIZE * 2];


void TEST_SD() {
	printf("test begin\n");
	if (Read_Block(1, 2, cRxBuffer)) {
		for (int i = 0; i < SDCARD_BUFF_SIZE * 2; i++) {
			printf("%02X ", cRxBuffer[i] & 0xff);
			if (i % 16 == 15)
				printf("\n");
			if (i == SDCARD_BUFF_SIZE - 1)
				printf("next:\n");
		}
		for (int i = 0; i < SDCARD_BUFF_SIZE * 2; i++)
			cRxBuffer[i] += 1;
		printf("***write:\n");
		if (Write_Block(1, 2, cRxBuffer)) {
			if (Read_Block(1, 2, cRxBuffer)) {
				for (int i = 0; i < SDCARD_BUFF_SIZE * 2; i++) {
					printf("%02X ", cRxBuffer[i] & 0xff);
					if (i % 16 == 15)
						printf("\n");
					if (i == SDCARD_BUFF_SIZE - 1)
						printf("next:\n");
				}
			} else {
				printf("read error!\n");
			}
		} else {
			printf("write error!\n");
		}
	} else {
		printf("read error!\n");
	}
	printf("test end\n");
}
/**********************************************
���ܣ����SDIO����ͣ������Ƿ����
��ڣ�cmd:���� be_resp��=1��Ӧ�� =0��Ӧ��
���ڣ�=0Ӧ��ʱ =1ִ�гɹ�
˵������
**********************************************/
int Chk_CMD_End(int cmd, int be_resp) {
	int finish0;

	if (!be_resp) { // No response
		finish0 = rSDICSTA;
		// Check cmd end
		while (!(finish0 & 0x800))
			finish0 = rSDICSTA;
		// Clear cmd end state
		rSDICSTA = finish0;

		return 1;
	} else {// With response
		finish0 = rSDICSTA;
		// Check cmd/rsp end
		while (!(finish0 & (0x200 + 0x400)))
			finish0 = rSDICSTA;
		//TODO:�еİ汾������ΪCMD9 CRC no check
		if (cmd == 1 | cmd == 41) {// CRC no check, CMD9 is a long Resp. command.
			if ((finish0 & 0xf00) != 0xa00) {// Check error
				// Clear error state
				rSDICSTA = finish0;
				if (finish0 & 0x400)
					return 0;// Timeout error
			}
			// Clear cmd & rsp end state
			rSDICSTA = finish0;
		} else {// CRC check
			if ((finish0 & 0x1f00) != 0xa00) {// Check error
				SD_DEBUG("CMD%d:rSDICSTA=0x%x, rSDIRSP0=0x%x\n", cmd, finish0, rSDIRSP0);
				// Clear error state
				rSDICSTA = finish0;

				if (((finish0 & 0x400) == 0x400))
					return 0;// Timeout error
			}
			rSDICSTA = finish0;
		}
		return 1;
	}
}

/**********************************************
���ܣ�ʹ������IDEL״̬
��ڣ���
���ڣ���
˵������
**********************************************/
void CMD0(void) {
	rSDICARG = 0x0;
	rSDICCON = (1 << 8) | 0x40;		// No_resp, start
	Chk_CMD_End(0, 0);
}
/**********************************************
���ܣ�MMC�����
��ڣ���
���ڣ�=1:MMC�� =0:��MMC��
˵������
**********************************************/
U8 CMD1(void) {
	rSDICARG = 0xff8000;					//(SD OCR:2.7V~3.6V)
	rSDICCON = (0x1 << 9) | (0x1 << 8) | 0x41; 		//sht_resp, wait_resp, start,

	if (Chk_CMD_End(1, 1)) {	//[31]:Card Power up status bit (busy)
		if ((rSDIRSP0 >> 16) == 0x80ff) {
			return 1;			// Success
		} else
			return 0;
	}
	return 0;
}
/**********************************************
���ܣ���⿨���͡��̼��汾��������ѹ״��
��ڣ���
���ڣ�
 =1��SD V1.X��MMC
 =2����׼SD����SDHC V2.0
 =0����Ч��
˵������
**********************************************/
U8 CMD8(void) {
	rSDICARG = 0x000001AA;
	rSDICCON = (0x1 << 9) | (0x1 << 8) | 0x48;	//sht_resp, wait_resp, start

	if (!Chk_CMD_End(8, 1))
		return 1;
	if ((rSDIRSP0 & 0x1aa) == 0x1aa)
		return 2;
	else
		return 0;
}
/**********************************************
���ܣ���⿨�Ƿ����
��ڣ�iRCA:RCA
���ڣ�=0 ʧ�� =1 �ɹ�
˵������
**********************************************/
U8 CMD55(U16 iRCA) {
	rSDICARG = iRCA << 16;
	rSDICCON = (0x1 << 9) | (0x1 << 8) | 0x77;	//sht_resp, wait_resp, start
	return Chk_CMD_End(55, 1);
}
/**********************************************
���ܣ����SD���ϵ�״̬
��ڣ�iRCA:RCA
���ڣ�
 =0Ӧ�������߿���æ
 =1��׼SD��
 =2SDHC V2.0
˵������
**********************************************/
U8 ACMD41(U16 iRCA) {
	U8 cReturn;

	if (!CMD55(iRCA))
		return 0;

	rSDICARG = 0x40ff8000;	//ACMD41(SD OCR:2.7V~3.6V CCS:1)
	//rSDICARG=0xffc000;	//ACMD41(MMC OCR:2.6V~3.6V)
	rSDICCON = (0x1 << 9) | (0x1 << 8) | 0x69; //sht_resp, wait_resp, start, ACMD41

	if (Chk_CMD_End(41, 1)) {
		if (rSDIRSP0 == 0xc0ff8000)
			cReturn = 2;	//SDHC
		else if (rSDIRSP0 == 0x80ff8000)
			cReturn = 1;	//��׼SD
		else
			cReturn = 0;	//Ӧ�����
		return cReturn;	// Success
	}
	return 0;
}
/**********************************************
���ܣ���ȡCID��ʶ��Ĵ���������
��ڣ���
���ڣ�=0ʧ�� =1�ɹ�
˵������
**********************************************/
U8 CMD2(U8 *cCID_Info) {
	rSDICARG = 0x0;
	rSDICCON = (0x1 << 10) | (0x1 << 9) | (0x1 << 8) | 0x42; //lng_resp, wait_resp, start

	if (!Chk_CMD_End(2, 1))
		return 0;

	*(cCID_Info + 0) = rSDIRSP0 >> 24;
	*(cCID_Info + 1) = rSDIRSP0 >> 16;
	*(cCID_Info + 2) = rSDIRSP0 >> 8;
	*(cCID_Info + 3) = rSDIRSP0;
	*(cCID_Info + 4) = rSDIRSP1 >> 24;
	*(cCID_Info + 5) = rSDIRSP1 >> 16;
	*(cCID_Info + 6) = rSDIRSP1 >> 8;
	*(cCID_Info + 7) = rSDIRSP1;
	*(cCID_Info + 8) = rSDIRSP2 >> 24;
	*(cCID_Info + 9) = rSDIRSP2 >> 16;
	*(cCID_Info + 10) = rSDIRSP2 >> 8;
	*(cCID_Info + 11) = rSDIRSP2;
	*(cCID_Info + 12) = rSDIRSP3 >> 24;
	*(cCID_Info + 13) = rSDIRSP3 >> 16;
	*(cCID_Info + 14) = rSDIRSP3 >> 8;
	*(cCID_Info + 15) = rSDIRSP3;

	return 1;
}
/**********************************************
���ܣ���SD���趨һ����ַ(RCA)
��ڣ�
 iCardType = 0:SD����=1:MMC��
���ڣ�=0 ʧ�� =1 �ɹ�
˵������
**********************************************/
U8 CMD3(U16 iCardType, U16 *iRCA) {
	rSDICARG = iCardType << 16;					// (MMC:Set RCA, SD:Ask RCA-->SBZ)
	rSDICCON = (0x1 << 9) | (0x1 << 8) | 0x43;	// sht_resp, wait_resp, start

	if (!Chk_CMD_End(3, 1))
		return 0;

	if (iCardType) {
		*iRCA = 1;
	} else {
		*iRCA = (rSDIRSP0 & 0xffff0000) >> 16;
	}

	if (rSDIRSP0 & 0x1e00 != 0x600)	// CURRENT_STATE check
		return 0;
	else
		return 1;
}
/**********************************************
���ܣ��ÿ�����ѡ��״̬
��ڣ�cSorD = 0:����ҪӦ�� = 1��ҪӦ��
���ڣ���
˵������
**********************************************/
U8 CMD7(U8 cSorD, U16 iRCA) {
	if (cSorD) {
		rSDICARG = iRCA << 16;				// (RCA,stuff bit)
		rSDICCON = (0x1 << 9) | (0x1 << 8) | 0x47; // sht_resp, wait_resp, start

		if (!Chk_CMD_End(7, 1))
			return 0;
		//--State(transfer) check
		if (rSDIRSP0 & 0x1e00 != 0x800)
			return 0;
		else
			return 1;
	} else {
		rSDICARG = 0 << 16;		//(RCA,stuff bit)
		rSDICCON = (0x1 << 8) | 0x47;	//no_resp, start
		if (!Chk_CMD_End(7, 0))
			return 0;
		return 1;
	}
}
/**********************************************
���ܣ���ȡ����״̬
��ڣ���
���ڣ���״̬��0ֵ
˵������
**********************************************/
U16 CMD13(U16 iRCA) {
	rSDICARG = iRCA << 16;				// (RCA,stuff bit)
	rSDICCON = (0x1 << 9) | (0x1 << 8) | 0x4d;	// sht_resp, wait_resp, start

	if (!Chk_CMD_End(13, 1))
		return 0;
	return rSDIRSP0;
}
/**********************************************
���ܣ��趨��������λ��
��ڣ�
 BusWidth =0��1bit =1��4bit
 iRCA:RCA
���ڣ�=0��ʧ�� =1���ɹ�
˵������
**********************************************/
U8 ACMD6(U8 BusWidth, U16 iRCA) {
	if (!CMD55(iRCA))
		return 0;
	rSDICARG = BusWidth << 1;					//Wide 0: 1bit, 1: 4bit
	rSDICCON = (0x1 << 9) | (0x1 << 8) | 0x46;	//sht_resp, wait_resp, start
	return Chk_CMD_End(6, 1);
}
/**********************************************
���ܣ���ȡ����CSD�Ĵ�����ֵ
��ڣ�
 iRCA:����RCA
 lCSD����ȡ��CSD����
���ڣ�=0ʧ�� =1�ɹ�
˵������
**********************************************/
U8 CMD9(U16 iRCA, U32 *lCSD) {
	rSDICARG = iRCA << 16;			// (RCA,stuff bit)
	rSDICCON = (0x1 << 10) | (0x1 << 9) | (0x1 << 8) | 0x49;	// long_resp, wait_resp, start

	if (!Chk_CMD_End(9, 1))
		return 0;

	*(lCSD + 0) = rSDIRSP0;
	*(lCSD + 1) = rSDIRSP1;
	*(lCSD + 2) = rSDIRSP2;
	*(lCSD + 3) = rSDIRSP3;
	return 1;
}
/**********************************************
���ܣ���ȡһ�����ݿ�
��ڣ���ʼ��ַ
���ڣ�=1���ɹ� =0��ʧ��
˵������
**********************************************/
U8 CMD17(U32 Addr) {
	//STEP1:����ָ��
	rSDICARG = Addr;				//�趨ָ�����
	rSDICCON = (1 << 9) | (1 << 8) | 0X51;	//����CMD17ָ��
	return Chk_CMD_End(17, 1);
}
/**********************************************
���ܣ���ȡ������ݿ�
��ڣ���ʼ��ַ
���ڣ�=1���ɹ� =0��ʧ��
˵������
**********************************************/
U8 CMD18(U32 Addr) {
	//STEP1:����ָ��
	rSDICARG = Addr;				//�趨ָ�����
	rSDICCON = (1 << 9) | (1 << 8) | 0X52;	//����CMD17ָ��
	return Chk_CMD_End(18, 1);
}
/**********************************************
���ܣ�ֹͣ���ݴ���
��ڣ���
���ڣ�=1���ɹ� =0��ʧ��
˵������
**********************************************/
U8 CMD12(void) {
	rSDICARG = 0x0;
	rSDICCON = (0x1 << 9) | (0x1 << 8) | 0x4c;	//sht_resp, wait_resp, start,
	return Chk_CMD_End(12, 1);
}
/**********************************************
���ܣ�д��һ�����ݿ�
��ڣ���
���ڣ�=1���ɹ� =0��ʧ��
˵������
**********************************************/
U8 CMD24(U32 Addr) {
	//STEP1:����ָ��
	rSDICARG = Addr;				//�趨ָ�����
	rSDICCON = (1 << 9) | (1 << 8) | 0x58;	//����CMD24ָ��
	return Chk_CMD_End(24, 1);
}
/**********************************************
���ܣ�д�������ݿ�
��ڣ���
���ڣ�=1���ɹ� =0��ʧ��
˵������
**********************************************/
U8 CMD25(U32 Addr) {
	//STEP1:����ָ��
	rSDICARG = Addr;				//�趨ָ�����
	rSDICCON = (1 << 9) | (1 << 8) | 0x59;	//����CMD25ָ��
	return Chk_CMD_End(25, 1);
}
/**********************************************
���ܣ����ò�����ʼ��ַ
��ڣ���
���ڣ�=1���ɹ� =0��ʧ��
˵������
**********************************************/
U8 CMD32(U32 Addr) {
	//STEP1:����ָ��
	rSDICARG = Addr;				//�趨ָ�����
	rSDICCON = (1 << 9) | (1 << 8) | 0x60;	//����CMD32ָ��
	return Chk_CMD_End(32, 1);
}
/**********************************************
���ܣ����ò�����ֹ��ַ
��ڣ���
���ڣ�=1���ɹ� =0��ʧ��
˵������
**********************************************/
U8 CMD33(U32 Addr) {
	//STEP1:����ָ��
	rSDICARG = Addr;				//�趨ָ�����
	rSDICCON = (1 << 9) | (1 << 8) | 0x61;	//����CMD33ָ��
	return Chk_CMD_End(33, 1);
}
/**********************************************
���ܣ���������ָ�����������
��ڣ���
���ڣ�=1���ɹ� =0��ʧ��
˵������
**********************************************/
U8 CMD38(void) {
	//STEP1:����ָ��
	rSDICARG = 0;					//�趨ָ�����
	rSDICCON = (1 << 9) | (1 << 8) | 0x66;	//����CMD38ָ��
	return Chk_CMD_End(38, 1);
}

/**********************************************
���ܣ��������߽���SD��
��ڣ�
 cSelDesel = 1:���� =0����
 iCardRCA: CARD RCA
���ڣ�=1���ɹ� =0��ʧ��
˵������
**********************************************/
U8 Card_sel_desel(U8 cSelDesel, U16 iCardRCA) {
	return CMD7(cSelDesel, iCardRCA);
}

/**********************************************
���ܣ����ÿ�ͨ�ſ��
��ڣ�
 cCardType ������
 cBusWidth =0��1bit =1��4bit
���ڣ�
 =1���ɹ� =0��ʧ��
˵������
**********************************************/
U8 Set_bus_Width(U8 cCardType, U8 cBusWidth, U16 iRCA) {
	if (cCardType == MMC_CARD)
		return 0;
	return ACMD6(cBusWidth, iRCA);
}

U8 SDI_CheckDATend(void) {
	int finish;
	finish = rSDIDSTA;
	while (!((finish & 0x10) || (finish & 0x20))) {  // Chek timeout or data end
		finish = rSDIDSTA;
	}//one of the DatFin and DatTout occur
	if ((finish & 0x2) || (finish & 0x1)) {
		udelay(200);
	}
	if ((finish & 0xfc) != 0x10) {
		printf("receive data(or receive busy) time out!!\n\r");
		rSDIDSTA = 0xf8; // Clear all error state
		return 1;
	}
	rSDIDSTA = 0xf8; // Clear all error state
	return 1;
}

/**********************************************************************************
�� �ܣ��ú������ڴ� SD ���ж���ָ������ʼ��ַ�����ݿ���Ŀ�Ķ���������ݿ飬��Ҫ��ȡ��
����������ʱ��ֹͣ��ȡ��
�� ����
 U32 Addr ���������ʼ��ַ
 U32 count �ڴ��������Ŀ���Ŀ
 U32* RxBuffer ���ڽ��ն������ݵĻ�����
����ֵ��
 0 ����������ɹ�
 1 ��������ɹ�
�� ����
�����������ж���һ��������Ϊ���ջ��������� U32 Rx_buffer[BufferSize];
Ȼ��ʼ���� Read_Mult_Block(addr,BufferSize,Rx_buffer);
**********************************************************************************/
U8 Read_Mult_Block(U32 Addr, U32 count, U8* RxBuffer) {
	U32 i = 0;
	U32 status = 0;
	rSDIFSTA = rSDIFSTA | (1 << 16); // FIFO reset
	rSDIDCON = (count) | (2 << 12) | (1 << 14) | (1 << 16) | (1 << 17) | (1 << 19) | (2 << 22);
	//TODO:sd v2.0 �� AddrΪ�����ţ���˴��벻����sd v1.0
	if (count == 1)
		while (CMD17(Addr) != 1);		//���Ͷ�������ָ��
	else
		while (CMD18(Addr) != 1); //���Ͷ�������ָ��
	rSDIDSTA = 0xf8;// Clear all flags
	for (int i = 0; i < count; i++) {
		for (int j = 0; j < 128;) {
			//�������ݵ�������
			if (rSDIDSTA & 0x20) {
				//����Ƿ�ʱ
				rSDIDSTA = (0x1 << 0x5); //�����ʱ��־
				return 0;
			}
			status = rSDIFSTA;
			if ((status & 0x1000) == 0x1000) {
				//������� FIFO ��������
				U32 temp = rSDIDAT;
				RxBuffer[0] = ((U8 *)&temp)[0];
				RxBuffer[1] = ((U8 *)&temp)[1];
				RxBuffer[2] = ((U8 *)&temp)[2];
				RxBuffer[3] = ((U8 *)&temp)[3];
				RxBuffer += 4;
				j++;
			}
		}
		if ((rSDIDSTA & (1 << 6))) {  // Check data-reccive CRC for every Block
			printf("SDI_ReadBlock failed:data-receive CRC error\n\r");
			rSDIDSTA = (1 << 6);	// Clear data CRC error flag
			return 0;
		}
	}
	if (!SDI_CheckDATend()) {	//*** remain count is not zero
		rSDIDCON  =  0x0;		//DataCon clear
		rSDIFSTA  = (1 << 9);	//clear Rx FIFO Last data Ready
		if (count > 1)			//CMD12 is needed after CMD18 to change SD states to tran
			while (CMD12() != 1); //���ͽ���ָ��
		return 0;
	} else {					//*** remain count is zero
		rSDIDCON  =  0x0;		//DataCon clear
		rSDIFSTA  = (1 << 9);	//clear Rx FIFO Last data Ready
		//CMD17 is now in tran(opertion complete) while CMD18 in data
		if (count > 1)			//CMD12 is needed after CMD18 to change SD states to tran
			while (CMD12() != 1); //���ͽ���ָ��
		return 1;
	}
}

/**********************************************************************************
�� �ܣ��ú��������� SD ���Ķ�����ݿ�д������
�� ����
 U32 Addr ��д�����ʼ��ַ
 U32 count �ڴ���д�����Ŀ
 U32* TxBuffer ���������ݵĻ�����
����ֵ��
 0 ����д�����ʧ��
 1 ����д������ɹ�
�� ����
�����������ж���һ��������Ϊ���ͻ��������� U32 Tx_buffer[BlockSize];
Ȼ��ʼ���� Write_Mult_Block(addr,DatSize,Tx_buffer);
**********************************************************************************/
U8 Write_Mult_Block(U32 Addr, U32 count, U8* TxBuffer) {
	U16 i = 0;
	U32 status = 0;
	rSDIFSTA = rSDIFSTA | (1 << 16);	// FIFO reset
	rSDIDCON = (count) | (3 << 12) | (1 << 14) | (1 << 16) | (1 << 17) | (1 << 20) | (2 << 22);
	//TODO:sd v2.0 �� AddrΪ�����ţ���˴��벻����sd v1.0
	if (count == 1)
		while (CMD24(Addr) != 1);	//����д�������ָ��
	else
		while (CMD25(Addr) != 1);	//����д������ָ��
	for (int i = 0; i < count; i++) {
		for (int j = 0; j < 128;) {
			//��ʼ�������ݵ�������
			status = rSDIFSTA;
			if ((status & 0x2000)) {
				//������� FIFO ���ã��� FIFO δ��
				U32 temp;
				((U8 *)&temp)[0] = TxBuffer[0];
				((U8 *)&temp)[1] = TxBuffer[1];
				((U8 *)&temp)[2] = TxBuffer[2];
				((U8 *)&temp)[3] = TxBuffer[3];
				rSDIDAT = temp;
				TxBuffer += 4;
				j++;
			}
		}
		do {
			while (!CMD13(SDCard.iCardRCA));	//Poll the state of SD card
		} while ((rSDIRSP0 & (1 << 8)) == 0);			//if SD card is not ready for new data
		//wait until SD card is ready for new data

		if (rSDIDSTA & (1 << 7)) { //Check send-data CRC for every block
			printf("SDI_WriteBlock failed:data-send CRC error\n\r");
			rSDIDSTA = (1 << 7);    //Clear send-data CRC error flag
			return 0;               //if Block-CRC error has occured,state of CMD24 is tran.
			//but state of CMD25 is rcv
		}
	}
	if (!SDI_CheckDATend()) {	//*** remain counter is not zero
		rSDIDCON  =  0x0;		//DataCon clear
		if (count > 1)			//CMD12 is needed after CMD25 to change SD states to tran
			while (CMD12() != 1); //���ͽ���ָ��
		return 0;
	} else {					//*** remain counter became zero
		rSDIDCON  =  0x0; 		//DataCon clear

		if (count > 1)			//CMD12 is needed after CMD25 to change SD states to tran
			while (CMD12() != 1);	//���ͽ���ָ��
		do {
			while (!CMD13(SDCard.iCardRCA));	//Poll the state of SD card
		} while ((rSDIRSP0 & (0x1f00)) != 0x900);	//wait for programing flash
		//SD card return to tran
		return 1;
	}
}

/**********************************************************************************
�� �ܣ��ú������ڲ���ָ����ַ���������
�� ����
 U32 StartAddr ��������ʼ��ַ
 U32 EndAddr �����Ľ�����ַ
����ֵ��
 0 ���������ɹ�
 1 ��������ʧ��
ע �⣺
 ��ʼ�ͽ�����ַ������������룬��������С��λ�������������ʼ�ڽ�����ַ��һ�������Ĵ�
 С��������û�����������룬�Ӷ�ʹ��ʼ��ַ�ͽ�����ַ���Ϊ������������ô��������������
 ��������
**********************************************************************************/
U8 Erase_Block(U32 StartAddr, U32 EndAddr) {
	if (CMD32(StartAddr) != 1)
		return 0;
	if (CMD33(EndAddr) != 1)
		return 0;
	if (CMD38() != 1)
		return 0;
	return 1;
}

/**********************************************
���ܣ�SD����ʼ��
��ڣ���
���ڣ�=0ʧ�� =1�ɹ�
˵������
**********************************************/
U8 SDI_init(void) {
	int i;
	U8  MBR[512];

	udelay(500000);	//�����������ϵ�ʱ��Ҫ��ʱ
	//printf("GPEUP=%X,GPECON=%X,GPGDAT=%X,GPGCON=%X,GPGUP=%X\n",rGPEUP,rGPECON,rGPGDAT,rGPGCON,rGPGUP);
	rGPEUP  &= ~((1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));   // SDCMD ,SDDATA[3:0] =>PULL UP
	rGPECON = ((rGPECON & ~(0xfff << 10)) | (0xaaa << 10));  //GPE5 GPE6 GPE7 GPE8 GPE9 GPE 10 => SDI

	rSDIPRE = PCLK / (INICLK) - 1;	// 400KHz
	rSDICON = (0 << 4) | 1;			// Type A, clk enable
	rSDIFSTA = rSDIFSTA | (1 << 16);	// FIFO reset
	rSDIBSIZE = 0x200;		// 512byte(128word)
	rSDIDTIMER = 0x7fffff;

	for (i = 0; i < 0x1000; i++);  	// Wait 74SDCLK for card

	CMD0();
	if (CMD1()) {
		SDCard.cCardType = MMC_CARD;
	} else {
		switch (CMD8()) {
			case 0://���̼���Ч
				SDCard.cCardType = INVALID_CARD;
				SD_DEBUG("���̼���Ч\n");
				break;
			case 1://�� SD2.0 ��
				SDCard.cCardType = SD_V1X_CARD;
				SD_DEBUG("�� SD2.0 ��n");
				break;
			case 2://SD2.0 ��
				SDCard.cCardType = SDHC_V20_CARD;
				SD_DEBUG("SD2.0 ��\n");
				break;
		}
	}

	SDCard.iCardRCA = 0;
	for (int j = 0; j < 20; j++) {
		for (i = 0; i < 100; i++) {
			if (ACMD41(SDCard.iCardRCA))
				break;
			udelay(2000);
		}
		if (i < 100)
			break;
		CMD1();
	}


	if (i == 100) {
		printf("Initialize fail\nNo Card assertion\n");
		return 0;
	} else {
		SD_DEBUG("SD is ready\n");
	}

	if (CMD2(SDCard.cCardCID)) {
		SD_DEBUG("CID\n");
		SD_DEBUG("MID = %d\n", SDCard.cCardCID[0]);
		SD_DEBUG("OLD = %d\n", (SDCard.cCardCID[1] * 0X100) + SDCard.cCardCID[2]);
		SD_DEBUG("��������:%s\n", (SDCard.cCardCID + 3));
		SD_DEBUG("��������:20%d,%d\n", ((SDCard.cCardCID[13] & 0x0f) << 4) + ((SDCard.cCardCID[14] & 0xf0) >> 4), (SDCard.cCardCID[14] & 0x0f));
	} else {
		printf("Read Card CID is fail!\n");
		return 0;
	}

	//RCA
	if (SDCard.cCardType == MMC_CARD) {
		if (CMD3(1, &SDCard.iCardRCA)) {
			SDCard.iCardRCA = 1;
			rSDIPRE = (PCLK / MMCCLK) - 1;
			SD_DEBUG("MMC Frequency is %dHz\n", (PCLK / (rSDIPRE + 1)));
		} else {
			printf("Read MMC RCA is fail!\n");
			return 0;
		}
	} else {
		if (CMD3(0, &SDCard.iCardRCA)) {
			rSDIPRE = PCLK / (SDCLK) - 1;	// Normal clock=25MHz
			SD_DEBUG("SD Card RCA = 0x%x\n", SDCard.iCardRCA);
			SD_DEBUG("SD Frequency is %dHz\n", (PCLK / (rSDIPRE + 1)));
		} else {
			printf("Read SD RCA is fail!\n");
			return 0;
		}
	}

	//CSD
	if (CMD9(SDCard.iCardRCA, SDCard.lCardCSD)) {
		SDCard.lCardSize = (((SDCard.lCardCSD[1] & 0x0000003f) << 16) + ((SDCard.lCardCSD[2] & 0xffff0000) >> 16) + 1) * 512;
		SDCard.lSectorSize = ((SDCard.lCardCSD[2] >> 6) & 0x0000007f) + 1;

		SD_DEBUG("Read Card CSD OK!\n");
		SD_DEBUG("0x%08x\n", SDCard.lCardCSD[0]);
		SD_DEBUG("0x%08x\n", SDCard.lCardCSD[1]);
		SD_DEBUG("0x%08x\n", SDCard.lCardCSD[2]);
		SD_DEBUG("0x%08x\n", SDCard.lCardCSD[3]);
		printf("������Ϊ:%dGB,%dMB\n", SDCard.lCardSize / 1024 / 1024, SDCard.lCardSize / 1024);
	} else {
		printf("Read Card CSD Fail!\n");
		return 0;
	}


	if (Card_sel_desel(1, SDCard.iCardRCA)) {
		SD_DEBUG("Card sel desel OK!\n");
	} else {
		printf("Card sel desel fail!\n");
		return 0;
	}

	//cmd13
	//iTemp = CMD13(SDCard.iCardRCA);
	//if (iTemp) {
	//	printf("Card Status is 0x%x\n", iTemp);
	//}

	if (Set_bus_Width(SDCard.cCardType, 1, SDCard.iCardRCA)) {
		SD_DEBUG("Bus Width is 4bit\n");
	} else {
		SD_DEBUG("Bus Width is 1bit\n");
		return 0;
	}

	if (Read_Block(0, 1, MBR)) {
		LBA_OFFSET   = ((MBR[457] << 24) + (MBR[456] << 16) + (MBR[455] << 8) + MBR[454]);
		TOTAL_SECTOR = ((MBR[461] << 24) + (MBR[460] << 16) + (MBR[459] << 8) + MBR[458]);
		TOTAL_SIZE   = (TOTAL_SECTOR / 2 / 1024);
		SD_DEBUG("MBR Boot OK!LBA_OFFSET=%4d,SD_TOTAL_SIZE=%4dMB\n\r", LBA_OFFSET, TOTAL_SIZE);
		SD_DEBUG("SDI Init OK\n\r");
		return 1;
	} else {
		printf("SDI Read MBR error!!Cannot boot for SD card!!\r\n");
		return 0;
	}
	return 1;
}



U8 Read_Block(U32 Addr, U32 count, U8* RxBuffer) {
	assert((count) && (RxBuffer));
	if (count == 0) {
		return 0;
	} else if (count < 4096) {
		SD_DEBUG("read: addr:%X size:%d buffer:%X\n", Addr, count, RxBuffer);
		return Read_Mult_Block(Addr, count, RxBuffer);
	} else {
		//TODO
		assert(0);
		return 0;
	}
}
U8 Write_Block(U32 Addr, U32 count, U8* TxBuffer) {
	assert((count) && (TxBuffer));
	if (count == 0) {
		return 0;
	} else if (count < 4096) {
		SD_DEBUG("write: addr:%X size:%d buffer:%X\n", Addr, count, TxBuffer);
		return Write_Mult_Block(Addr, count, TxBuffer);
	} else {
		//TODO
		assert(0);
		return 0;
	}
}
