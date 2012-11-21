#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>

typedef uint8_t     U8;
typedef int8_t      S8;
typedef uint16_t    U16;
typedef int16_t     S16;
typedef uint32_t    U32;
typedef int32_t     S32;
typedef uint64_t    U64;
typedef int64_t     S64;

typedef float       F32;
typedef double      F64;

typedef S32         Fix24_8;

#define RESTRICT __restrict

union Pixel {
	struct {
		U8 b, g, r, a;
	} p;
	U8 c[4];
	U32 u32;
};

void panic(const char *fmt, ...);

#endif