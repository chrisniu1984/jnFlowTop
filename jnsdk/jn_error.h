#ifndef __JN_ERROR_H__
#define __JN_ERROR_H__

#define ERROR                   u32

/* common */
#define ERROR_NONE              0x00000000  // 无错误
#define ERROR_NULL              0x00000001  // 空指针错误
#define ERROR_MALLOC            0x00000002  // 内存分配失败，一般由malloc引起
#define ERROR_IMPLEMENT         0x00FFFFFE  // 函数未实现
#define ERROR_UNKNOWN           0x00FFFFFF  // 未知错误

/* jn_spin */
//#define ERROR_POOL_XXX        0x01000000

/* jn_pool */
#define ERROR_POOL_NOTMINE      0x02000000  // 此数据块不属于此内存池
#define ERROR_POOL_EMPTY        0x02000001  // 内存块已用尽

/* jn_hashm */
#define ERROR_HASHM_POOL        0x03000000  // 内存池申请失败
#define ERROR_HASHM_EXIST       0x03000001  // 已经存在
#define ERROR_HASHM_NOTEXIST    0x03000002  // 不存在

#endif //__JN_ERROR_H__
