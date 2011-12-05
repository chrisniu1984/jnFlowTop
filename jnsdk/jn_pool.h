/*
 * Memory Pool
 * ------------------------------------
 */

#ifndef __JN_POOL_H__
#define __JN_POOL_H__

typedef struct
{
    spin_t      spin;

    u32         unit_size;
    u32         unit_count;

    u32         *empty_table;
    u32         table_index;
    void        *data;
} pool_t;

ERROR jn_pool_init(pool_t *pool, u32 uint_size, u32 unit_count);
ERROR jn_pool_term(pool_t *pool);

ERROR jn_pool_malloc(pool_t *pool, void **data);

ERROR jn_pool_free(pool_t *pool, void *data);
ERROR jn_pool_free_all(pool_t *pool);

#endif // __JN_POOL_H__
