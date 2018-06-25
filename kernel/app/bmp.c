#include <stdio.h>
#include <assert.h>
#include <framebuffer.h>
#include <s3c24xx.h>
#include "ff.h"


/* λͼ�ļ�ͷ��Ҫ�Ƕ�λͼ�ļ���һЩ���� λͼ��Ϣͷ��Ҫ�Ƕ�λͼͼ������Ϣ������ */
/*************************** λͼ��׼��Ϣ(54�ֽ�) ******************************************/
/* λͼ�ļ�ͷ ��λͼͷ���ֽ��� = λͼ�ļ��ֽ��� - λͼͼ�������ֽ�����*/
typedef struct BMP_FILE_HEADER {
	U16 bType;			// �ļ���ʶ��
	U32 bSize;			// �ļ��Ĵ�С
	U16 bReserved1;		// ����ֵ,��������Ϊ0
	U16 bReserved2;		// ����ֵ,��������Ϊ0
	U32 bOffset;		// �ļ�ͷ�����ͼ������λ��ʼ��ƫ����
} __attribute__((packed)) BMPFILEHEADER;	// 14 �ֽ�

/* λͼ��Ϣͷ */
typedef struct BMP_INFO {
	U32 bInfoSize;			// ��Ϣͷ�Ĵ�С
	U32 bWidth;				// ͼ��Ŀ��
	U32 bHeight;			// ͼ��ĸ߶�
	U16 bPlanes;			// ͼ���λ����
	U16 bBitCount;			// ÿ�����ص�λ��
	U32 bCompression;		// ѹ������
	U32 bmpImageSize;		// ͼ��Ĵ�С,���ֽ�Ϊ��λ
	U32 bXPelsPerMeter;		// ˮƽ�ֱ���
	U32 bYPelsPerMeter;		// ��ֱ�ֱ���
	U32 bClrUsed;			// ʹ�õ�ɫ����
	U32 bClrImportant;		// ��Ҫ����ɫ��
} __attribute__((packed)) BMPINF;	// 40 �ֽ�

/* ��ɫ��:��ɫ�� */
typedef struct RGB {
	U8 b;    // ��ɫǿ��
	U8 g;    // ��ɫǿ��
	U8 r;    // ��ɫǿ��
	U8 rgbReversed; // ����ֵ
} RGB;


int drawImage(char *name) {
	BMPFILEHEADER bmpFileHeader;  // ����һ�� BMP �ļ�ͷ�Ľṹ��
	BMPINF bmpInfo;               // ����һ�� BMP �ļ���Ϣ�ṹ��
	FIL f;
	FRESULT res;
	UINT br;
	if ((res = f_open(&f, name, FA_READ)) != FR_OK) {
		printf("�ټ�\n");
		return 1;
	}

	if ((res = f_read(&f, &bmpFileHeader, sizeof(bmpFileHeader), &br)) != FR_OK)
		goto exit;
	if ((res = f_read(&f, &bmpInfo, sizeof(bmpInfo), &br)) != FR_OK)
		goto exit;
	/*
	// ���BMP�ļ���λͼ�ļ�ͷ��������Ϣ
	printf("λͼ�ļ�ͷ��Ҫ�Ƕ�λͼ�ļ���һЩ����:BMPFileHeader\n\n");
	printf("�ļ���ʶ�� = %#X\n", bmpFileHeader.bType);
	printf("BMP �ļ���С = %d �ֽ�\n", bmpFileHeader.bSize);
	printf("����ֵ1 = %d \n", bmpFileHeader.bReserved1);
	printf("����ֵ2 = %d \n", bmpFileHeader.bReserved2);
	printf("�ļ�ͷ�����ͼ������λ��ʼ��ƫ���� = %d �ֽ�\n", bmpFileHeader.bOffset);

	// ���BMP�ļ���λͼ��Ϣͷ��������Ϣ
	printf("\n\nλͼ��Ϣͷ��Ҫ�Ƕ�λͼͼ������Ϣ������:BMPInfo\n\n");
	printf("��Ϣͷ�Ĵ�С = %d �ֽ�\n", bmpInfo.bInfoSize);
	printf("λͼ�ĸ߶� = %d \n", bmpInfo.bHeight);
	printf("λͼ�Ŀ�� = %d \n", bmpInfo.bWidth);
	printf("ͼ���λ����(λ�����ǵ�ɫ�������,Ĭ��Ϊ1����ɫ��) = %d \n", bmpInfo.bPlanes);
	printf("ÿ�����ص�λ�� = %d λ\n", bmpInfo.bBitCount);
	printf("ѹ������ = %d \n", bmpInfo.bCompression);
	printf("ͼ��Ĵ�С = %d �ֽ�\n", bmpInfo.bmpImageSize);
	printf("ˮƽ�ֱ��� = %d \n", bmpInfo.bXPelsPerMeter);
	printf("��ֱ�ֱ��� = %d \n", bmpInfo.bYPelsPerMeter);
	printf("ʹ�õ�ɫ���� = %d \n", bmpInfo.bClrUsed);
	printf("��Ҫ��ɫ���� = %d \n", bmpInfo.bClrImportant);

	printf("\n\n\nѹ��˵������0����ѹ������1��RLE 8��8λRLEѹ������2��RLE 4��4λRLEѹ����3��Bitfields��λ���ţ�");
	*/
	if (bmpInfo.bBitCount != 24)
		goto exit;
	int totalSize = (bmpInfo.bWidth * bmpInfo.bBitCount / 8 + 3) / 4 * 4 * bmpInfo.bHeight; //�������ֽ�����4�ֽڶ��룩
	RGB rgb;
	char buf[3 * 480];

	for (int y = 0; y < bmpInfo.bHeight; y++) {
		int count;
		for (int x = 0; x < bmpInfo.bWidth; x += count) {
			if (bmpInfo.bWidth - x >= (sizeof(buf) / 3)) {
				count = (sizeof(buf) / 3);
			} else {
				count = bmpInfo.bWidth - x;
			}
			res = f_read(&f, buf, 3 * count, &br);
			if ((res != FR_OK) || (br == 0))
				goto exit;
			for (int i = 0; i < count; i++) {
				assert(x + i < 480);
				rgb.b = buf[3 * i + 0];
				rgb.g = buf[3 * i + 1];
				rgb.r = buf[3 * i + 2];
				PutPixel(x + i, bmpInfo.bHeight - y - 1, convert888_565(*(U32 *)&rgb));
			}
		}
		if (bmpInfo.bWidth % 4 > 0) {
			res = f_read(&f, &rgb, 4 - bmpInfo.bWidth % 4, &br);
			if ((res != FR_OK) || (br == 0))
				goto exit;
		}
	}

exit:
	f_close(&f);
	return 0;
}

