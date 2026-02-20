#pragma once

typedef unsigned char   uint8;
#define _BYTE  uint8

#define LOBYTE(x)   (*((_BYTE*)&(x)))   // low byte
#define BYTEn(x, n)   (*((_BYTE*)&(x)+n))
#define BYTE1(x)   BYTEn(x,  1)         // byte 1 (counting from 0)
#define BYTE2(x)   BYTEn(x,  2)
#define BYTE3(x)   BYTEn(x,  3)
#define BYTE4(x)   BYTEn(x,  4)
#define HIBYTE(x)   (*((_BYTE*)&(x)+1))

// use the R2 namespace for game funcs
namespace R2
{
	typedef void* (*GetBaseLocalClientType)();
	extern GetBaseLocalClientType GetBaseLocalClient;
} // namespace R2