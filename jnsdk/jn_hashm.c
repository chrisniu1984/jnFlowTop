#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jn_std.h"

static u64 _md5(void *key, u32 len, u8 md5[16])
{
    MD5_CTX ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, key, len);
    MD5Final(md5, &ctx);

    return *(u64*)md5;
}

static int _find_by_hkey(hashm_t *hashm, u8 *md5, u64 hkey, u32 idx, void **data)
{
    hashm_node_t *node = hashm->hash_idxes[idx];
    if (NULL == node)
        return -1;

    while (node != NULL) {
        if (memcmp(node->md5, md5, 16) == 0) {
            if (data) {
                *data = node->data;
            }
            return 0;
        }

        node = node->next;
    }

    return -1;

}

ERROR jn_hashm_init(hashm_t *hashm, u32 hash_size_s, u32 unit_size, u32 unit_num)
{
    if (NULL == hashm) {
        return ERROR_NULL;
    }
    memset(hashm, 0x00, sizeof(hashm_t));

    // hash index table
    hashm->hash_size = (1 << hash_size_s);
    hashm->hash_idxes = (hashm_node_t**) malloc(sizeof(hashm_node_t*) * hashm->hash_size);
    if (NULL == hashm->hash_idxes) {
        return ERROR_MALLOC;
    }
    memset(hashm->hash_idxes, 0x00, sizeof(hashm_node_t*) * hashm->hash_size);

    // unit
    hashm->unit_size = unit_size;
    hashm->unit_num = unit_num;
    if (jn_pool_init(&hashm->unit_pool, unit_size+sizeof(hashm_node_t), unit_num) != ERROR_NONE) {
        free(hashm->hash_idxes);
        return ERROR_HASHM_POOL;
    }

    return ERROR_NONE;
}

ERROR jn_hashm_term(hashm_t *hashm)
{
    //FIXME: 未实现
    return ERROR_IMPLEMENT;
}

ERROR jn_hashm_add(hashm_t *hashm, void *key, u32 len, void **data)
{
    if (NULL == hashm || NULL == key) {
        return ERROR_NULL;
    }

    u8 md5[16];
    u64 hkey = _md5(key, len, md5);
    u32 idx = hkey % hashm->hash_size;

    if (data != NULL) {
        *data = NULL;
    }

    // 查找是否已经存在
    if (_find_by_hkey(hashm, md5, hkey, idx, data) == 0) {
        return ERROR_HASHM_EXIST;
    }

    hashm_node_t *node;
    ERROR err = jn_pool_malloc(&hashm->unit_pool, (void**) &node);
    if (err != ERROR_NONE) {
        return err;
    }

    // 设置md5
    memcpy(node->md5, md5, 16);

    // 挂载
    node->prev = NULL;
    node->next = hashm->hash_idxes[idx]; 
    hashm->hash_idxes[idx] = node;

    // 初始化数据部分
    *data = node->data;
    memset(*data, 0x00, hashm->unit_size);

    return ERROR_NONE;
}

ERROR jn_hashm_find(hashm_t *hashm, void *key, u32 len, void **data)
{
    if (NULL == hashm) {
        return ERROR_NULL;
    }

    u8 md5[16];
    u64 hkey = _md5(key, len, md5);
    u32 idx = hkey % hashm->hash_size;

    if (_find_by_hkey(hashm, md5, hkey, idx, data) == 0) {
        return ERROR_NONE;
    }

    return ERROR_HASHM_NOTEXIST;
}

ERROR jn_hashm_del(hashm_t *hashm, void *key, u32 len)
{
    if (NULL == hashm) {
        return ERROR_NULL;
    }

    u8 md5[16];
    u64 hkey = _md5(key, len, md5);
    u32 idx = hkey % hashm->hash_size;

    hashm_node_t *node = hashm->hash_idxes[idx];
    if (NULL == node)
        return ERROR_NULL;

    while (node != NULL) {
        if (memcmp(node->md5, md5, sizeof(md5)) == 0) {
            break;
        }

        node = node->next;
    }

    if (node != NULL) {
        // 首节点
        if (node->prev == NULL) {
            hashm->hash_idxes[idx] = node->next;
        }
        else {
            ((hashm_node_t*)(node->prev))->next = node->next;
        }

        jn_pool_free(&hashm->unit_pool, node);

        return ERROR_NONE;
    }

    return ERROR_HASHM_NOTEXIST;
}

ERROR jn_hashm_del_all(hashm_t *hashm)
{
    if (NULL == hashm) {
        return ERROR_NULL;
    }

    memset(hashm->hash_idxes, 0x00, sizeof(hashm_node_t*) * hashm->hash_size);

    return jn_pool_free_all(&hashm->unit_pool);
}

ERROR jn_hashm_first(hashm_t *hashm, hashm_iter_t *iter, void **data)
{
    if (NULL == hashm || NULL == iter || NULL == data) {
        return ERROR_NULL;
    }

    *data = NULL;

    iter->idx = 0;
    while (iter->idx < hashm->hash_size &&
        hashm->hash_idxes[iter->idx] == NULL) {
        iter->idx++;
    }

    if (iter->idx >= hashm->hash_size) {
        return ERROR_HASHM_NOTEXIST;
    }

    *data = hashm->hash_idxes[iter->idx]->data;
    iter->next = (void*) hashm->hash_idxes[iter->idx]->next;

    return ERROR_NONE;
}

ERROR jn_hashm_next(hashm_t *hashm, hashm_iter_t *iter, void **data)
{
    *data = NULL;

    if (iter->idx >= hashm->hash_size) {
        return ERROR_HASHM_NOTEXIST;
    }

    hashm_node_t* node = (hashm_node_t*) iter->next;

    if (node == NULL) {
        iter->idx++;
        while (iter->idx < hashm->hash_size &&
            hashm->hash_idxes[iter->idx] == NULL) {
            iter->idx++;
        }

        if (iter->idx >= hashm->hash_size) {
            return ERROR_HASHM_NOTEXIST;
        }

        *data = hashm->hash_idxes[iter->idx]->data;
        iter->next = (void*) hashm->hash_idxes[iter->idx]->next;
    }
    else {
        *data = node->data;
        iter->next = node->next;
    }

    return ERROR_NONE;
}
