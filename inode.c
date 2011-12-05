#include <stdio.h>
#include <string.h>

#include "inode.h"
#include "session.h"

static void add_to_mhash(hashm_t *mhash, char *buffer)
{
    skey_t skey;
    memset(&skey, 0x00, sizeof(skey_t));
    skey.type = TYPE_TCP;

    char stSrc[128];
    char stDst[128];
    u32 src_port;
    u32 dst_port;
    u32 inode;

    int matches = sscanf(buffer, "%*d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X "
                                 "%*X %*X:%*X %*X:%*X %*X %*d %*d "
                                 "%u %*512s\n",
                                 stSrc, &src_port,
                                 stDst, &dst_port,
                                 &inode);

    if (matches != 5) {
        return;
    }
    
    if (inode == 0) {
        return;
    }

    if (strlen(stSrc) == 8) {
        sscanf(stSrc, "%X", &skey.src_ip.dw);
        sscanf(stDst, "%X", &skey.dst_ip.dw);
        skey.src_port = (u16) src_port;
        skey.dst_port = (u16) dst_port;
    }

    //print_skey(&skey, DIR_UP);

    u32 *tmp;
    jn_hashm_add(mhash, (void*)&skey, sizeof(skey_t), (void**) &tmp);
    if (tmp != NULL) {
        *tmp = inode;
    }
}

int inode_init(hashm_t *inodes)
{
    return jn_hashm_init(inodes, 20, sizeof(u32), 65535);
}

int inode_load(hashm_t *inodes)
{
    FILE *fp;
    char buffer[8192];

    memset(buffer, 0x00, sizeof(buffer));
    fp = fopen ("/proc/net/tcp", "r");
    if (fp != NULL) {
        fgets(buffer, sizeof(buffer), fp);
        while (!feof(fp)) {
            if (fgets(buffer, sizeof(buffer), fp) > 0) {
                add_to_mhash(inodes, buffer);
            }
        }

        fclose(fp);
    }

    memset(buffer, 0x00, sizeof(buffer));
    fp = fopen ("/proc/net/udp", "r");
    if (fp != NULL) {
        fgets(buffer, sizeof(buffer), fp);
        while (!feof(fp)) {
            if (fgets(buffer, sizeof(buffer), fp) > 0) {
                add_to_mhash(inodes, buffer);
            }
        }

        fclose(fp);
    }

    return 0;
}
