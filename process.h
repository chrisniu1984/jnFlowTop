#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <time.h>

#include "jn_std.h"

#define PROCESS_TIMEOUT   30


typedef struct {
    pid_t   pid;
    char    cmd[256];
} pinfo_t;

typedef struct {
    pinfo_t pinfo;

    time_t  refresh_time;
    u32     send_byte;
    u32     recv_byte;

    time_t  calc_time;
    u32     old_send_byte;
    u32     old_recv_byte;
} process_t;

typedef int (*cb_t)(process_t *proc, time_t now, int flag);

int pinfo_init(hashm_t *pinfos);
int pinfo_term(hashm_t *pinfos);

int pinfo_refille(hashm_t *pinfos);

int process_init(hashm_t *pros);
int process_term(hashm_t *pros);

void process_show(hashm_t *pros, cb_t cb);
int process_timeout(hashm_t *mhash);

#endif //__PROCESS_H__
