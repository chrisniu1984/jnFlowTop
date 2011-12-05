#ifndef __CORE_H__
#define __CORE_H__

#define INTERVAL        2
#define TIMEOUT         10

#include "process.h"


typedef struct {
    pid_t   pid;
    char    cmd[256];
    u32     up;
    u32     down;
} app_t;

int init(char *ifname, cb_t cb);
int term();

void quicksort(app_t array[], int begin, int end);

#endif //__CORE_H__
