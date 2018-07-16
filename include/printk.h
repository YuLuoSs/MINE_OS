/***************************************************
*		��Ȩ����
*
*	������ϵͳ��Ϊ��MINE
*	�ò���ϵͳδ����Ȩ������ӯ�����ӯ��ΪĿ�Ľ��п�����
*	ֻ�������ѧϰ�Լ���������ʹ��
*
*	������������Ȩ������Ȩ���������У�
*
*	��ģ�����ߣ�	����
*	EMail:		345538255@qq.com
*
*
***************************************************/

#ifndef __PRINTK_H__
#define __PRINTK_H__


#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */

#define is_digit(c)	((c) >= '0' && (c) <= '9')

#define WHITE	0x00ffffff		//��
#define BLACK	0x00000000		//��
#define RED		0x00ff0000		//��
#define ORANGE	0x00ff8000		//��
#define YELLOW	0x00ffff00		//��
#define GREEN	0x0000ff00		//��
#define BLUE	0x000000ff		//��
#define INDIGO	0x0000ffff		//��
#define PURPLE	0x008000ff		//��

#define color_printk(a,b,fmt,arg...) printf(fmt,##arg)

#endif

