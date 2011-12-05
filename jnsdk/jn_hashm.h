/*
 * 非线程安全
 */

#ifndef __JN_HASHM_H__
#define __JN_HASHM_H__

typedef struct {
    u8          md5[16];
    void        *prev;
    void        *next;

    u8          data[0];
} hashm_node_t;

typedef struct {
    u32         idx;
    void        *next;
} hashm_iter_t;

typedef struct {
    u32             hash_size;
    hashm_node_t    **hash_idxes;

    u32             unit_size;
    u32             unit_num;
    pool_t          unit_pool; 
} hashm_t;

ERROR jn_hashm_init(hashm_t *hashm, u32 hash_size_s, u32 unit_size, u32 unit_num);

ERROR jn_hashm_term(hashm_t *hashm);

ERROR jn_hashm_find(hashm_t *hashm, void *key, u32 len, void **data);

ERROR jn_hashm_add(hashm_t *hashm, void *key, u32 len, void **data);

ERROR jn_hashm_del(hashm_t *hashm, void *key, u32 len);

ERROR jn_hashm_del_all(hashm_t *hashm);

ERROR jn_hashm_first(hashm_t *hashm, hashm_iter_t *iter, void **data);

ERROR jn_hashm_next(hashm_t *hashm, hashm_iter_t *iter, void **data);

#endif //__JN_HASHM_H__
