#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jn_std.h"

ERROR jn_pool_init(pool_t *pool, u32 unit_size, u32 unit_count)
{
    if (NULL == pool)
        return ERROR_NULL;

    memset(pool, 0x00, sizeof(pool_t));
    
    jn_spin_init(&pool->spin);
    pool->unit_size = unit_size;
    pool->unit_count = unit_count;

    // 用于分配空间
    pool->empty_table = malloc(sizeof(u32)*unit_count);
    if (pool->empty_table == NULL) {
        return ERROR_MALLOC;
    }

    pool->table_index = unit_count - 1;
    int i;
    for (i=pool->table_index; i>=0; i--) {
        pool->empty_table[i] = i;
    }

    pool->data = malloc(unit_size*unit_count);

    return ERROR_NONE;
}

ERROR jn_pool_term(pool_t *pool)
{
    if (NULL == pool)
        return ERROR_NULL;

    free(pool->data);
    free(pool->empty_table);
    
    memset(pool, 0x00, sizeof(pool_t));
    return jn_spin_init(&pool->spin);
}

ERROR jn_pool_malloc(pool_t *pool, void **data)
{
    if (NULL == pool || NULL == data)
        return ERROR_NULL;

    *data = NULL;

    // 加锁
    jn_spin_lock(&pool->spin);

    // 分配
    if (pool->table_index != 0xFFFFFFFF) {
        u32 id = pool->empty_table[pool->table_index];
        *data = pool->data + (id * pool->unit_size);
        pool->table_index--;
    }
    else {
        jn_spin_unlock(&pool->spin);
        return ERROR_POOL_EMPTY;
    }

    // 解锁
    return jn_spin_unlock(&pool->spin);
}

ERROR jn_pool_free(pool_t *pool, void *data)
{
    if (NULL == pool || NULL == data)
        return ERROR_NULL;

    // 加锁
    jn_spin_lock(&pool->spin);

    if (data < pool->data ||   // 越界
        ((data-pool->data)/pool->unit_size >= pool->unit_count) ||   // 越界
        ((data-pool->data)%pool->unit_size != 0) ||                   // 不是unit_size的整数倍
        pool->table_index >= (pool->unit_count-1))                    // 空闲列表已满
    {
        return ERROR_POOL_NOTMINE;
    }
        
    pool->table_index++;
    pool->empty_table[pool->table_index] = (data-pool->data)/pool->unit_size;

    // 解锁
    return jn_spin_unlock(&pool->spin);
}

ERROR jn_pool_free_all(pool_t *pool)
{
    int i;

    if (NULL == pool)
        return ERROR_NULL;

    // 加锁
    jn_spin_lock(&pool->spin);

    pool->table_index = pool->unit_count-1;
    for (i=pool->table_index; i>=0; i--) {
        pool->empty_table[i] = i;
    }

    // 解锁
    return jn_spin_unlock(&pool->spin);
}

