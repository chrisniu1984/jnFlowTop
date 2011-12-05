#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>

#include "jn_std.h"

#include "session.h"
#include "inode.h"
#include "process.h"
#include "core.h"

static app_t m_apps[10];
static int   m_cnt;

int show(process_t *process, time_t now, int flag)
{
    time_t interval = 0;
    int i = 0;

    switch (flag) {
    case 0:
        m_cnt = 0;
        memset(m_apps, 0x00, sizeof(m_apps));
        break;

    case 1:
        if (m_cnt < 10) {
            interval = now - process->calc_time;
            if (interval > 0) {
                m_apps[m_cnt].pid = process->pinfo.pid;
                strcpy(m_apps[m_cnt].cmd, process->pinfo.cmd);
                m_apps[m_cnt].up = (process->send_byte-process->old_send_byte)/1024/interval;
                m_apps[m_cnt].down = (process->recv_byte-process->old_recv_byte)/1024/interval;
                m_cnt++;
            }
        }
        break;

    case 2:
        quicksort(m_apps, 0, m_cnt-1);
        printf("\33[2J"); //cls
        printf("\33[%d;%dH", 0,0);
        printf("Active TCP/UDP Process\n");
        printf("-------------------------------------------------------------\n");
        for (i=0; i<m_cnt; i++) {
            printf("%20s\tup:%-3uKB/s    down:%-3uKB/s\n", m_apps[i].cmd, m_apps[i].up, m_apps[i].down);
        }
        printf("-------------------------------------------------------------\n");
        break;

    default:
        break;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("usage: %s IFNAME\n", basename(argv[0]));
        return -1;
    }

    if (init(argv[1], show) == -1) {
        return -1;
    }

    while (1) {
        sleep(10);
    }

    return 0;
}
