#ifndef __JN_STD_H__
#define __JN_STD_H__

#include <linux/types.h>

typedef __u8    u8;
typedef __u16   u16;
typedef __u32   u32;
typedef __u64   u64;

typedef __s8    s8;
typedef __s16   s16;
typedef __s32   s32;
typedef __s64   s64;

typedef u8      bool;
typedef bool    BOOL;

#ifndef true
#define true    1
#endif

#ifndef false
#define false   0
#endif

#ifndef TRUE
#define TRUE    true
#endif

#ifndef FALSE
#define FALSE   false
#endif

#ifndef MIN
#define MIN(a,b)    do {(a)>(b)?(b):(a)} while(0)
#endif

#ifndef MAX
#define MAX(a,b)    do {(a)>(b)?(a):(b)} while(0)
#endif

/* 3rd */
#include "3rd/Md5.h"

/* sdk */
#include "jn_error.h"
#include "jn_spin.h"
#include "jn_pool.h"
#include "jn_hashm.h"

#endif //__JN_STD_H__
