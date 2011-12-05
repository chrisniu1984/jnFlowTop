#ifndef __SESSION_H__
#define __SESSION_H__

#include <time.h>

#include "jn_std.h"

#include "process.h"

#define SESSION_TIMEOUT     10

#define DIR_UP              0
#define DIR_DOWN            1

typedef union {
    u8      b[4];
    u32     dw;
} ipv4;

typedef struct {
    #define TYPE_NONE   0
    #define TYPE_TCP    1
    #define TYPE_UDP    2
    u16     type;

    ipv4    src_ip; 
    ipv4    dst_ip;
    u16     src_port;
    u16     dst_port;
} skey_t;

typedef struct {
    skey_t      skey;

    time_t      refresh_time;
    u32         send_byte;
    u32         recv_byte;

    time_t      calc_time;
    u32         old_send_byte;
    u32         old_recv_byte;

    u32         inode;
    pinfo_t     *pinfo;
    process_t   *proc;
} session_t;

int ips_init(hashm_t *ips);
int ips_term(hashm_t *ips);
int ips_refill(hashm_t *ips);

int skey_parse(skey_t *skey, const char *buf, int len);
int skey_fmt(skey_t *skey, u8 *dir, hashm_t *ips);
void skey_print(skey_t *skey, u8 dir);

int session_init(hashm_t *sessions);
int session_add(hashm_t *mhash, skey_t *skey, session_t **session);
int session_bind_inode(session_t *session, hashm_t *m_inode);
int session_bind_pinfo(session_t *session, hashm_t *pinfos);
int session_bind_process(session_t *session, hashm_t *pros);

int session_flow_data(session_t *session, u8 dir, int len);
int session_flow_process(session_t *session, u8 dir, int len);

int session_timeout(hashm_t *mhash);

#endif //__SESSION_H__
