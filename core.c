#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/ethernet.h>

#include "jn_std.h"
#include "session.h"
#include "inode.h"
#include "process.h"
#include "core.h"

/* 接收缓冲区 */
#define RCV_BUF_SIZE     1024 * 5

static int m_recv_len = RCV_BUF_SIZE; 
static char m_recv_buf[RCV_BUF_SIZE] = {0};

static int              m_run;
static int              m_sock;

static pthread_t        m_thdshow;
static pthread_t        m_thdtimeout;
static pthread_t        m_thdcapture;

static pthread_mutex_t  m_mutex;
static hashm_t          m_ips;       // 本地IP地址池，用于判断方向
static hashm_t          m_session;   // skey->session
static hashm_t          m_inode;     // skey->inode
static hashm_t          m_pinfo;     // inode->(pid+cmd)
static hashm_t          m_process;   // pid->process

static int init_capture(const char *ifname)
{
    int iRet = -1;
    int fd = -1;
    
    /* 创建SOCKET */
    fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (0 > fd) {
        perror("");
        return -1;
    }
    
    /* 设置SOCKET选项 */
    iRet = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &m_recv_len, sizeof(int));
    if (0 > iRet) {
        perror("");
        close(fd);
        return -1; 
    }
    
    return fd;    
}

static void *thd_show(void *p)
{
    while (m_run) {
        process_show(&m_process, (cb_t)(p));
        sleep(INTERVAL);
    }

    return NULL;
}

static void *thd_timeout(void *p)
{
    while (m_run) {
        sleep(INTERVAL);

        pthread_mutex_lock(&m_mutex);
        process_timeout(&m_process);
        session_timeout(&m_session);
        pthread_mutex_unlock(&m_mutex);

        sleep(TIMEOUT);
    }

    return NULL;
}

/* 捕获网卡数据帧 */
static void *thd_capture(void *p)
{
    int len = -1;
    
    /* 循环监听 */
    while(m_run)
    {
        /* 接收数据帧 */
        len = recv(m_sock, m_recv_buf, m_recv_len, 0);
        if (len < 0) {
            continue;
        }
        
        u8 dir;
        skey_t skey;
        memset(&skey, 0x00, sizeof(skey_t));

        // 解析数据获得session key
        if (skey_parse(&skey, m_recv_buf, len) != 0) {
            continue;
        }

        pthread_mutex_lock(&m_mutex);

        // 使用本地地址池调整方向
        if (skey_fmt(&skey, &dir, &m_ips) != 0) {
            continue;
        }

        // 使用session key 查找session，准备刷入数据
        session_t *session = NULL;
        if (jn_hashm_find(&m_session, (void*) &skey, sizeof(skey_t),
                (void**)&session) != 0) {

            // 创建新的session
            session_add(&m_session, &skey, &session);
        }

        // 刷入数据和刷新时间
        if (session != NULL) {
            session_flow_process(session, dir, len);
        }

        // session需要帮定inode,pinfo,process
        if (session->inode == 0 && session_bind_inode(session, &m_inode) != 0) {
            pthread_mutex_unlock(&m_mutex);
            continue;
        }
        if (session->pinfo == NULL && session_bind_pinfo(session, &m_pinfo) != 0) {
            pthread_mutex_unlock(&m_mutex);
            continue;
        }
        if (session->proc == NULL && session_bind_process(session, &m_process) != 0) {
            pthread_mutex_unlock(&m_mutex);
            continue;
        }

        pthread_mutex_unlock(&m_mutex);
    }

    close(m_sock);

    return NULL;
}   

int init(char *ifname, cb_t cb)
{
    m_run = 1;

    /* 初始化SOCKET */
    m_sock = init_capture(ifname);
    if(0 > m_sock) {
        return -1;
    }

    // 初始化线程
    pthread_mutex_init(&m_mutex, NULL);

    // 本地地址池初始化
    ips_init(&m_ips);
    ips_refill(&m_ips);
    
    // session表
    session_init(&m_session);

    // inode表
    inode_init(&m_inode);
    inode_load(&m_inode);

    // process info表
    pinfo_init(&m_pinfo);
    pinfo_refille(&m_pinfo);

    // process
    process_init(&m_process);

    /* 显示回调 */
    pthread_create(&m_thdshow, NULL, thd_show, (void*) cb);

    /* 超时扫描 */
    pthread_create(&m_thdtimeout, NULL, thd_timeout, NULL);

    /* 捕获数据包 */
    pthread_create(&m_thdcapture, NULL, thd_capture, NULL);

    return 0;
}

int term()
{
    m_run = 0;

    return 0;
}

static int swap(app_t *i, app_t *j)
{
   app_t temp;

   memcpy(&temp, i, sizeof(app_t));
   memcpy(i, j, sizeof(app_t));
   memcpy(j, &temp, sizeof(app_t));

   return 0;
}

void quicksort(app_t array[], int begin, int end)
{
    int i, j;

    if(begin < end) {
        /* 因为开始值作为基准值不用移动，所以需要比较的值是从 begin+1 开始 */
        i = begin + 1;
        j = end;

        while(i < j) {
            /* 如果当前值小于 array[begin]，把当前值换到 j 指向的位置,换位后j下标前移 */
            if(array[i].down < array[begin].down) {
                swap(&array[i], &array[j]);
                j--; 
            }
            /* 否则 i 下标向后移动,准备比较下一个数。*/
            else {
                i++;
            }
        }

        /* 在此时: i=j, array[i]还没有进行判断处理 */
        if (array[i].down < array[begin].down) {
            i--;
        }

        /* 将基准值换到分界位置i */
        swap(&array[begin], &array[i]);

        /* 对从分割线位置 i 两侧分别排序 */
        quicksort(array, begin, i-1);
        quicksort(array, i+1, end);
    }
}
