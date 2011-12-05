#ifndef __INODE_H__
#define __INODE_H__

#include "jn_std.h"

int inode_init(hashm_t *inodes);
int inode_term(hashm_t *inodes);

int inode_load(hashm_t *inodes);

#endif //__INODE_H__
