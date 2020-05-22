#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<time.h>
#include<unistd.h>

//#pragma warning(disable : 4996)// 用于微软VS标准 // Linux下注释掉此行

typedef struct _MsgInt {
	unsigned int* msgInt;//原始消息字符转化为int指针数组，每4个字符转化为一个int个数
	int intCount;//int数个数
}MsgInt;

typedef struct _ExtendMsgInt {
	unsigned int W[68];
	unsigned int W1[64];
}ExtendMsgInt;

/*
 * 宏函数NOT_BIG_ENDIAN()
 * 用于测试运行环境是否为大端，小端返回true
 */
static const int endianTestNum = 1;
#define NOT_BIG_ENDIAN() ( *(char *)&endianTestNum == 1 )

#define FF_LOW(x,y,z) ( (x) ^ (y) ^ (z))
#define FF_HIGH(x,y,z) (((x) & (y)) | ( (x) & (z)) | ( (y) & (z)))

#define GG_LOW(x,y,z) ( (x) ^ (y) ^ (z))
#define GG_HIGH(x,y,z) (((x) & (y)) | ( (~(x)) & (z)) )

#define ROTATE_LEFT(uint32,shift) ( ( ( (uint32) << (shift) ) | ( (uint32) >> (32 - (shift)) ) ) )

#define P0(x) ((x) ^  ROTATE_LEFT((x),9) ^ ROTATE_LEFT((x),17))
#define P1(x) ((x) ^  ROTATE_LEFT((x),15) ^ ROTATE_LEFT((x),23))

/*
 * 宏函数UCHAR_2_UINT(uchr8,uint32,i,notBigendian)
 * uchr8        -- unsigned char - 8bit
 * uint32       -- unsigned int  - 32bit
 * i            -- int
 * notBigendian -- int/bool
 * 将uchr8接收的字符数组,转换成大端表示的uint32（从底层看二进制按左高位右地位排列）
 * NOT_BIG_ENDIAN()检验环境，非大端时notBigendian为真，启用此宏函数
 */
#define UCHAR_2_UINT(uchr8,uint32,i,notBigendian)				\
{																\
	if(notBigendian){                                           \
		(uint32) = ((unsigned int) (uchr8)[(i)    ] << 24 )		\
				 | ((unsigned int) (uchr8)[(i) + 1] << 16 )		\
				 | ((unsigned int) (uchr8)[(i) + 2] << 8  )		\
				 | ((unsigned int) (uchr8)[(i) + 3]       );	\
	}															\
}

 /*
  * 宏函数UINT_2_UCHAR(uint32,uchr8,i,notBigendian)
  * uchr8        -- unsigned char - 8bit
  * uint32       -- unsigned int  - 32bit
  * i            -- int
  * notBigendian -- int/bool
  * 将大端表示的uint32,转换成uchr8的字符数组
  * NOT_BIG_ENDIAN()检验环境，非大端时notBigendian为真，启用此宏函数
  */
#define UINT_2_UCHAR(uint32,uchr8,i,notBigendian)				\
{																\
	if(notBigendian){                                           \
		(uchr8)[(i)    ] = (unsigned char)((uint32) >> 24);		\
		(uchr8)[(i) + 1] = (unsigned char)((uint32) >> 16);		\
		(uchr8)[(i) + 2] = (unsigned char)((uint32) >> 8 );		\
		(uchr8)[(i) + 3] = (unsigned char)((uint32)      );		\
	}															\
}

MsgInt MsgFill512(unsigned char* msg, int notBigendian)
{
	MsgInt filledMsgInt;
	unsigned long long msgLength = strlen((char*)msg);// 这里必须有个强制转换，strlen不支持unsigned char*
	unsigned long long msgbitLength = msgLength * 8; // 求原始消息的比特长度
	int zeroFill = 448 - (msgbitLength + 8) % 512; // +8是补了0x80=0b1000_0000
	unsigned char* zeroChar = (unsigned char*)malloc(zeroFill / 8);

	memset(zeroChar, 0, zeroFill / 8);

	int totalChrLength = msgLength + 1 + zeroFill / 8 + 8;

	filledMsgInt.msgInt = (unsigned int*)malloc(totalChrLength / 4);
	filledMsgInt.intCount = totalChrLength / 4;//totalChrLength是字符个数8bit/个，msgInt为32bit/个

	unsigned char* msgFill = (unsigned char*)malloc(totalChrLength);// 1表示0x80的长度，一个字节
	memcpy(msgFill, msg, msgLength);
	unsigned char one = 0x80;
	memcpy(msgFill + msgLength, &one, 1);
	memcpy(msgFill + msgLength + 1, zeroChar, zeroFill / 8);

	unsigned char msgLenChr[8];
	if (notBigendian) { // 小端系统，long long 都是在内存中颠倒存储的，所以需要转换
		for (int i = 0; i < 8; i++) {
			msgLenChr[i] = msgbitLength >> (56 - 8 * i);
		}
		memcpy(msgFill + msgLength + 1 + zeroFill / 8, msgLenChr, 8);
	}
	else { // 如果是大端系统，直接拷贝msgbitLength内存内容即可
		memcpy(msgFill + msgLength + 1 + zeroFill / 8, &msgbitLength, 8);
	}

	for (int i = 0; i < filledMsgInt.intCount; i++) {
		unsigned char msgSlice[4] = { *(msgFill + i * 4),*(msgFill + i * 4 + 1),*(msgFill + i * 4 + 2),*(msgFill + i * 4 + 3) };
		UCHAR_2_UINT(msgSlice, filledMsgInt.msgInt[i], 0, notBigendian);
	}

	return filledMsgInt;
}

ExtendMsgInt MsgExtend(unsigned int msgInt16[])
{
	ExtendMsgInt etdMsgInt;

	/*for (int i = 0; i < 16; i++) {
		etdMsgInt.W[i] = msgInt16[i];
	}*/
	etdMsgInt.W[0] = msgInt16[0]; // 拆解小循环
	etdMsgInt.W[1] = msgInt16[1];
	etdMsgInt.W[2] = msgInt16[2];
	etdMsgInt.W[3] = msgInt16[3];
	etdMsgInt.W[4] = msgInt16[4];
	etdMsgInt.W[5] = msgInt16[5];
	etdMsgInt.W[6] = msgInt16[6];
	etdMsgInt.W[7] = msgInt16[7];
	etdMsgInt.W[8] = msgInt16[8];
	etdMsgInt.W[9] = msgInt16[9];
	etdMsgInt.W[10] = msgInt16[10];
	etdMsgInt.W[11] = msgInt16[11];
	etdMsgInt.W[12] = msgInt16[12];
	etdMsgInt.W[13] = msgInt16[13];
	etdMsgInt.W[14] = msgInt16[14];
	etdMsgInt.W[15] = msgInt16[15];

	register unsigned int tmp;
	for (int j = 16; j < 68; j++) {
		tmp = etdMsgInt.W[j - 16] ^ etdMsgInt.W[j - 9] ^ ROTATE_LEFT(etdMsgInt.W[j - 3], 15);
		etdMsgInt.W[j] = P1(tmp) ^ ROTATE_LEFT(etdMsgInt.W[j - 13], 7) ^ etdMsgInt.W[j - 6];
	}
	for (int j = 0; j < 64; j++) {
		etdMsgInt.W1[j] = etdMsgInt.W[j] ^ etdMsgInt.W[j + 4];
	}
	return etdMsgInt;
}

void CF(unsigned int Vi[], unsigned int msgInt16[], unsigned int W[], unsigned int W1[])
{
	//unsigned int regA2H[8]; // A~H 8个寄存器
	register unsigned int A, B, C, D, E, F, G, H;// A~H 8个寄存器
	register unsigned int SS1, SS2, TT1, TT2; // 中间变量

	/*for (int i = 0; i < 8; i++) {
		regA2H[i] = Vi[i];
	}*/
	A = Vi[0];// 拆解小循环
	B = Vi[1];
	C = Vi[2];
	D = Vi[3];
	E = Vi[4];
	F = Vi[5];
	G = Vi[6];
	H = Vi[7];

	register unsigned int T = 0x79cc4519; // 文档中的常量Tj，此处用T
	for (int j = 0; j < 64; j++) {
		if (j >= 16) {
			T = 0x7a879d8a;
		}
		SS1 = ROTATE_LEFT(ROTATE_LEFT(A, 12) + E + ROTATE_LEFT(T, j), 7);
		SS2 = SS1 ^ ROTATE_LEFT(A, 12);
		if (j < 16) {
			TT1 = FF_LOW(A, B, C) + D + SS2 + W1[j];
			TT2 = GG_LOW(E, F, G) + H + SS1 + W[j];
		}
		else {
			TT1 = FF_HIGH(A, B, C) + D + SS2 + W1[j];
			TT2 = GG_HIGH(E, F, G) + H + SS1 + W[j];
		}
		D = C;
		C = ROTATE_LEFT(B, 9);
		B = A;
		A = TT1;
		H = G;
		G = ROTATE_LEFT(F, 19);
		F = E;
		E = P0(TT2);
	}
	//for (int i = 0; i < 8; i++) { // 计算 ABCDEFH ^ Vi
	//	regA2H[i] ^= Vi[i];
	//	Vi[i] = regA2H[i];
	//}
	A ^= Vi[0];// 拆解小循环
	B ^= Vi[1];
	C ^= Vi[2];
	D ^= Vi[3];
	E ^= Vi[4];
	F ^= Vi[5];
	G ^= Vi[6];
	H ^= Vi[7];

	Vi[0] = A;// 拆解小循环
	Vi[1] = B;
	Vi[2] = C;
	Vi[3] = D;
	Vi[4] = E;
	Vi[5] = F;
	Vi[6] = G;
	Vi[7] = H;

}

void SM3Hash(unsigned char* msgText, int notBigendian, unsigned char sm3HashChr32[])
{
	MsgInt filledMsgInt = MsgFill512(msgText, notBigendian);
	// 对填充好的消息按512bit进行分组，即每16个int一组
	register int groupAmount = filledMsgInt.intCount / 16; // 计算分组个数

	unsigned int V[8] = {
	0x7380166F,0x4914B2B9,0x172442D7,0xDA8A0600,
	0xA96F30BC,0x163138AA,0xE38DEE4D,0xB0FB0E4E
	};

	for (int i = 0; i < groupAmount; i++) {
		unsigned int* bi = (i << 4) + filledMsgInt.msgInt;
		ExtendMsgInt etdMsgInt = MsgExtend(bi);
		CF(V, bi, etdMsgInt.W, etdMsgInt.W1); // 每一轮压缩更新V
	}
	// 直接输出int型的杂凑值测试
	/*for (int i = 0; i < 8; i++) {
		printf("%08x ", V[i]);
	}*/
	//return V;

	/*for (int i = 0; i < 8; i++) { // 将杂凑值换成char类型
		UINT_2_UCHAR(V[i], sm3HashChr32, 4 * i, notBigendian);
	}*/

	UINT_2_UCHAR(V[0], sm3HashChr32, 4 * 0, notBigendian); // 拆解小循环
	UINT_2_UCHAR(V[1], sm3HashChr32, 4 * 1, notBigendian);
	UINT_2_UCHAR(V[2], sm3HashChr32, 4 * 2, notBigendian);
	UINT_2_UCHAR(V[3], sm3HashChr32, 4 * 3, notBigendian);
	UINT_2_UCHAR(V[4], sm3HashChr32, 4 * 4, notBigendian);
	UINT_2_UCHAR(V[5], sm3HashChr32, 4 * 5, notBigendian);
	UINT_2_UCHAR(V[6], sm3HashChr32, 4 * 6, notBigendian);
	UINT_2_UCHAR(V[7], sm3HashChr32, 4 * 7, notBigendian);

}

void SM3hmac(unsigned char msgText[], unsigned int keyInt16[], int notBigendian, unsigned char sm3hmacChr32[])
{
	// 因为默认使用自己随机出来的key，随机的时候规定生成的key就是64Byte，就不需要填充key至64Byte
	unsigned int tempInt16[32]; // ipad opad一起算了，tempInt前16个元素存储和ipad异或结果，后16个元素存储opad异或结果
	for (int i = 0; i < 16; i++) {
		tempInt16[i] = keyInt16[i] ^ 0x36363636;//ipad[i];
		tempInt16[i + 16] = keyInt16[i] ^ 0x5c5c5c5c;// opad[i];
	}
	unsigned char keyChr64[128]; // 前64存储ipad异或结果(int)转成char，后64存储opad
	for (int i = 0; i < 16; i++) {
		UINT_2_UCHAR(tempInt16[i], keyChr64, i << 2, notBigendian);
		UINT_2_UCHAR(tempInt16[i + 16], keyChr64, (i + 16) << 2, notBigendian);
	}
	unsigned char* jointChr = (unsigned char*)malloc(strlen(msgText) + 64);// 第一次hash前的拼接长度
	//unsigned char* jointChr = (unsigned char*)malloc((strlen(msgText) + 64) * sizeof(unsigned char));// 第一次hash前的拼接长度
	memcpy(jointChr, keyChr64, 64);
	memcpy(jointChr + 64, msgText, strlen(msgText));
	memset(jointChr + 64 + strlen(msgText), 0, 1);
	// 一定要在末尾添上\0标志字符串结尾，否则后续调用msgFill512时候读取msgLength会有错误，

	SM3Hash(jointChr, notBigendian, sm3hmacChr32);
	//free(jointChr);// 释放内存
	//unsigned char* jointChr1 = (unsigned char*)malloc((64 + 32) * sizeof(unsigned char)); // 第二次hash前的拼接长度
	unsigned char jointChr1[97];//要多加1用来存结束符号0
	memcpy(jointChr1, keyChr64 + 64, 64);
	memcpy(jointChr1 + 64, sm3hmacChr32, 32);
	memset(jointChr1 + 96, 0, 1);
	SM3Hash(jointChr1, notBigendian, sm3hmacChr32);

}

void HmacPrint(unsigned char hmac32[])
{
	for (int i = 0; i < 32; i++) {
		printf("%02x", hmac32[i]);
		if (i != 0 && i % 4 == 0) {
			printf(" ");
		}
	}
	printf("\n");
}

void Key16Generate(unsigned int keyInt16[], int notbigendian)
{
	srand((unsigned int)time(0));
	unsigned char keyChr64[64];
	for (int i = 0; i < 64; i++) {
		while (1)
		{
			keyChr64[i] = rand() % (0xff - 0x00 + 0x01); //rand（） % （b - a + 1）+ a 
			if (keyChr64[i] != 0x36 && keyChr64[i] != 0x5c) {
				break;
			}
		}
	}
	for (int i = 0; i < 16; i++) {
		UCHAR_2_UINT(keyChr64, keyInt16[i], 4 * i, notbigendian);
	}

}

void SM3hmacWithFile(char* filename, int fileChrAmount)
{
	FILE* fp;
	int bigendFlag = NOT_BIG_ENDIAN();
	unsigned char* msg = (unsigned char*)malloc((fileChrAmount + 1) * sizeof(unsigned char));
	unsigned int keyInt16[16];
	Key16Generate(keyInt16, bigendFlag);
	puts("使用的随机密钥：");
	for (int i = 0; i < 16; i++) {
		printf("%08x ", keyInt16[i]);
	}
	printf("\n--------------------------------------------------------------------------\n");
	unsigned char sm3hmacValue[32];

	clock_t start, end;
	double excuteTime;
	fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("Cannot open  %s ! \n", filename);
		exit(0);
	}
	start = clock();
	fgets(msg, fileChrAmount + 1, fp); // +1原因如下
	//char *fgets(char *str,int n,FILE *fp)
	//从由fp指出的文件中读取n-1个字符，并把它们存放到由str指出的字符数组中去，最后加上一个字符串结束符'\0'
	end = clock();
	excuteTime = ((double)end - (double)start) / CLOCKS_PER_SEC;
	printf("消息读取完成，共 %d 字节，用时: %f seconds.\n", fileChrAmount, excuteTime);
	//printf("Your msg show as below:\n%s\n", msg);
	//printf("\n------------------------------------\nCharacter Amount: %d\n", strlen(msg));
	fclose(fp);

	start = clock();
	SM3hmac(msg, keyInt16, bigendFlag, sm3hmacValue);
	end = clock();
	excuteTime = ((double)end - (double)start) / CLOCKS_PER_SEC;

	HmacPrint(sm3hmacValue);
	printf("完成SM3hmac所用时间: % f seconds.\n", excuteTime);
	double speed = fileChrAmount / excuteTime;
	speed /= pow(2, 20);
	printf("SM3hmac运行速度: %f MBps(%f Mbps)\n", speed, speed * 8);

}


void SM3Interface()
{
	puts("      RGRGRGRGR     RG    RG    RG  RGRGRGRGRG      ");
	puts("    RG              RG    RG    RG            RG    ");
	puts("    RG              RG    RG    RG            RG    ");
	puts("      RGRGRGRGR     RG    RG    RG    RGRGRGRG      ");
	puts("               RG   RG          RG            RG    ");
	puts("               RG   RG          RG            RG    ");
	puts("      RGRGRGRGR     RG          RG  RGRGRGRGRG      ");
	puts("                                                    ");
	puts("    RG      RG  RG  RG  RG     RGRG       RGRGRGR   ");
	puts("    RG      RG  RG  RG  RG  RG      RG  RG          ");
	puts("    RG RGRG RG  RG  RG  RG  RG  AG  RG  RG          ");
	puts("    RG      RG  RG      RG  RG      RG  RG       G  ");
	puts("    RG      RG  RG      RG  RG      RG    RGRGRGR   ");
	puts("");
	puts("  ================================================= ");
	puts(" |                                                 |");
	puts(" |               Welcome to SM3hamc !              |");
	puts(" |                 欢迎使用SM3hmac ！              |");
	puts(" |                                                 |");
	puts(" |          请输入序号, 选择相应功能(按q退出)：    |");
	puts(" |                                                 |");
	puts(" |        1.SM3文档示例   2.样例测试   3.自定义    |");
	puts(" |                                                 |");
	puts("  ================================================= ");

}

void SampleInterface()
{
	puts("  ================================================= ");
	puts(" |                                                 |");
	puts(" |     本程序提供以下8个样例用于测试程序执行效率   |");
	puts(" |                                                 |");
	puts(" |   请输入序号，选择相应量级的示例文件(按q退出)： |");
	puts(" |                                                 |");
	puts(" |      (1) 1KB (2) 10KB (3) 50KB (4) 100KB        |");
	puts(" |      (5) 1MB (6) 10MB (7) 50MB (8) 100MB        |");
	puts(" |                                                 |");
	puts("  ================================================= ");
}

void CustomInterface()
{
	puts("  ================================================= ");
	puts(" |                                                 |");
	puts(" |   请输入序号，选择相应功能计算HMAC(按q退出)：   |");
	puts(" |                                                 |");
	puts(" |      (1) 直接输入消息 (2) 指定消息文件          |");
	puts(" |                                                 |");
	puts("  ================================================= ");
}

// 文档示例一
void Eg1_test() {
	int bigendFlag = NOT_BIG_ENDIAN();

	unsigned char* msg = "abc";
	//unsigned char* hashChr = SM3Hash_Old(chr, bigendFlag);
	unsigned char hashChr[32];

	SM3Hash(msg, bigendFlag, hashChr);

	puts("\n-------------------- 文档示例1 --------------------\n");
	//Fill_N_extend_test(chr);
	puts("示例1 SM3杂凑值: ");
	for (int i = 0; i < 32; i++) {
		printf("%02x", hashChr[i]);
		if (i != 0 && i % 4 == 0) {
			printf(" ");
		}
	}
	printf("\n");
	printf("\n-------------------------------------------------\n");
	unsigned int keyInt16[16];
	Key16Generate(keyInt16, bigendFlag);
	puts("随机密钥：");
	for (int i = 0; i < 16; i++) {
		printf("%08x ", keyInt16[i]);
	}
	printf("\n-------------------------------------------------\n");
	unsigned char sm3hmacValue[32];
	clock_t start, end;
	double excuteTime;
	start = clock();
	SM3hmac(msg, keyInt16, bigendFlag, sm3hmacValue);
	end = clock();
	excuteTime = ((double)end - (double)start) / CLOCKS_PER_SEC;
	puts("示例1 HMAC：");
	HmacPrint(sm3hmacValue);
	printf("-------------------------------------------------\n");
	printf("完成SM3hmac所用时间: % f seconds.\n", excuteTime);
	double speed = strlen(msg) / excuteTime;
	speed /= pow(2, 20);
	printf("SM3hmac运行速度: %f MBps(%f Mbps)\n", speed, speed * 8);

}

// 文档示例二
void Eg2_test() {
	int bigendFlag = NOT_BIG_ENDIAN();
	unsigned char* msg = "abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcd";

	//unsigned char* hashChr = SM3Hash_Old(chr, bigendFlag);
	// 运算没问题，但是返回给指针之后到下面的puts调用时，会破坏hashChr指向的内存内容
	unsigned char hashChr[32];

	SM3Hash(msg, bigendFlag, hashChr);

	puts("\n-------------------- 文档示例2 --------------------\n");
	//Fill_N_extend_test(chr);
	puts("示例2 SM3杂凑值: ");
	for (int i = 0; i < 32; i++) {
		printf("%02x", hashChr[i]);
		if (i != 0 && i % 4 == 0) {
			printf(" ");
		}
	}
	printf("\n");
	printf("\n-------------------------------------------------\n");
	unsigned int keyInt16[16];
	Key16Generate(keyInt16, bigendFlag);
	puts("随机密钥：");
	for (int i = 0; i < 16; i++) {
		printf("%08x ", keyInt16[i]);
	}
	printf("\n-------------------------------------------------\n");
	unsigned char sm3hmacValue[32];
	clock_t start, end;
	double excuteTime;
	start = clock();
	SM3hmac(msg, keyInt16, bigendFlag, sm3hmacValue);
	end = clock();
	excuteTime = ((double)end - (double)start) / CLOCKS_PER_SEC;
	puts("示例2 HMAC：");
	HmacPrint(sm3hmacValue);
	printf("-------------------------------------------------\n");
	printf("完成SM3hmac所用时间: % f seconds.\n", excuteTime);
	double speed = strlen(msg) / excuteTime;
	speed /= pow(2, 20);
	printf("SM3hmac运行速度: %f MBps(%f Mbps)\n", speed, speed * 8);
}

void StdSM3Test()
{
	puts("------- 下面是国密SM3标准文档所给的两个示例的SM3杂凑值和HMAC -------");
	Eg1_test();
	Eg2_test();

}

void SampleFileTest()
{
	while (1) {
		int outFlag = 0;
		char option;
		char* filename;
		int fileChrAmount;
		SampleInterface();
		puts("请选择序号：");
		option = getchar();
		getchar();//接收回车
		switch (option)
		{
		case '1':
			system("clear");
			filename = ".//TestSample//msg1k.txt";
			fileChrAmount = 1389;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '2':
			system("clear");
			filename = ".//TestSample//msg10k.txt";
			fileChrAmount = 13748;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '3':
			system("clear");
			filename = ".//TestSample//msg50k.txt";
			fileChrAmount = 68956;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '4':
			system("clear");
			filename = ".//TestSample//msg100k.txt";
			fileChrAmount = 110433;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '5':
			system("clear");
			filename = ".//TestSample//msg1M.txt";
			fileChrAmount = 1411509;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '6':
			system("clear");
			filename = ".//TestSample//msg10M.txt";
			fileChrAmount = 11291574;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '7':
			system("clear");
			filename = ".//TestSample//msg50M.txt";
			fileChrAmount = 59351213;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '8':
			system("clear");
			filename = ".//TestSample//msg100M.txt";
			fileChrAmount = 108683394;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case 'q':
			outFlag = 1;
			break;
		default:
			system("clear");
			puts("Unsupported Input!\nPlease rechoose!");
			//system("pause");
			//system("cls");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		}
		if (outFlag)
			break;
	}
}

void CustomIput()
{
	int bigendFlag = NOT_BIG_ENDIAN();
	while (1) {
		int outFlag = 0;
		char option;
		clock_t start, end;
		double excuteTime;
		CustomInterface();
		puts("请选择序号：");
		option = getchar();
		getchar();//接收回车
		unsigned int keyInt16[16];
		unsigned char sm3hmacValue[32];
		switch (option)
		{
		case '1':
			system("clear");
			puts("请输入消息字符：");
			unsigned char msg[1024];
			scanf("%s", msg);
			getchar();//回收回车
			printf("\n-------------------------------------------------\n");
			Key16Generate(keyInt16, bigendFlag);
			puts("随机密钥：");
			for (int i = 0; i < 16; i++) {
				printf("%08x ", keyInt16[i]);
			}
			printf("\n-------------------------------------------------\n");
			start = clock();
			SM3hmac(msg, keyInt16, bigendFlag, sm3hmacValue);
			end = clock();
			excuteTime = ((double)end - (double)start) / CLOCKS_PER_SEC;
			puts("所输入的消息的 HMAC：");
			HmacPrint(sm3hmacValue);
			printf("-------------------------------------------------\n");
			printf("完成SM3hmac所用时间: % f seconds.\n", excuteTime);
			double speed = strlen(msg) / excuteTime;
			speed /= pow(2, 20);
			printf("SM3hmac运行速度: %f MBps(%f Mbps)\n", speed, speed * 8);

			//system("pause");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '2':
			system("clear");
			puts("请指定消息文件的绝对路径(若不确定，可将文件拖入本程序所在文件夹，然后输入文件名即可，含文件后缀.txt)：");
			char filename[1024];
			scanf("%s", filename);
			getchar();//回收回车
			int fileChrAmount;
			puts("请输入消息文件字节数(可查阅文件属性得知)：");
			scanf("%d", &fileChrAmount);
			getchar();//回收回车
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case 'q':
			outFlag = 1;
			break;
		default:
			system("clear");
			puts("Unsupported Input!\nPlease rechoose!");
			//system("pause");
			//system("cls");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		}
		if (outFlag)
			break;
	}
}

void SM3hmacEntry()
{
	while (1) {
		int outFlag = 0;
		char option;
		SM3Interface();
		puts("请选择序号：");
		option = getchar();
		getchar();//接收回车
		switch (option)
		{
		case '1':
			system("clear");
			StdSM3Test();
			//system("pause");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '2':
			system("clear");
			SampleFileTest();
			//system("pause");
			//system("cls");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '3':
			system("clear");
			CustomIput();
			//system("pause");
			//system("cls");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case 'q':
			outFlag = 1;
			break;
		default:
			system("clear");
			puts("Unsupported Input!\nPlease rechoose!");
			//system("pause");
			//system("cls");
			puts("按任意键继续...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		}
		if (outFlag)
			break;
	}
}

int main()
{
	SM3hmacEntry();

	return 0;
}