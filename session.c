#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include "jn_std.h"

#include "session.h"
#include "inode.h"

static int _parse_eth(const struct ether_header *pEthHdr, int *jump)
{
    if (NULL == pEthHdr) {
        return -1;
    }

    // 不处理广播
    if (pEthHdr->ether_shost[0]%2 != 0x00 ||
        pEthHdr->ether_dhost[0]%2 != 0x00) {
        return -1; 
    }

    // 只处理IP协议
    if (ntohs(pEthHdr->ether_type) != ETHERTYPE_IP) {
        return -1;
    }

    //printf("%.2X:%.2X:%.2X:%.2X:%.2X:%.2X -> %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n",
    //        pEthHdr->ether_shost[0], pEthHdr->ether_shost[1],
    //        pEthHdr->ether_shost[2], pEthHdr->ether_shost[3],
    //        pEthHdr->ether_shost[4], pEthHdr->ether_shost[5],
    //        pEthHdr->ether_dhost[0], pEthHdr->ether_dhost[1],
    //        pEthHdr->ether_dhost[2], pEthHdr->ether_dhost[3],
    //        pEthHdr->ether_dhost[4], pEthHdr->ether_dhost[5]);
    
    *jump = sizeof(struct ether_header);

    return 0;    
}

static int _parse_ip(const struct iphdr *pIpHdr, int *jump, skey_t *skey)
{
    if (NULL == pIpHdr) {
        return -1;
    }

    skey->src_ip.dw = pIpHdr->saddr;
    skey->dst_ip.dw = pIpHdr->daddr;
    if (pIpHdr->protocol == IPPROTO_TCP) {
        skey->type = TYPE_TCP;
    }
    else if (pIpHdr->protocol == IPPROTO_UDP) {
        skey->type = TYPE_UDP;
    }
    else {
        return -1;
    }

    *jump = pIpHdr->ihl << 2;

    return 0;
}

int ips_init(hashm_t *ips)
{
    return jn_hashm_init(ips, 20, sizeof(ipv4), 100);
}

int ips_refill(hashm_t *ips)
{
    jn_hashm_del_all(ips);

    struct ifaddrs *ifa;
    if (getifaddrs(&ifa) != 0) {
        return -1;
    }
    
    for (; ifa!=NULL; ifa=ifa->ifa_next) {
        struct sockaddr_in *sin = (struct sockaddr_in*)ifa->ifa_addr;
        if (sin->sin_family != AF_INET) {
            continue;
        }

        ipv4 *ip = NULL;
        jn_hashm_add(ips, (void*) &sin->sin_addr.s_addr, sizeof(sin->sin_addr), (void**)&ip);
        ip->dw = sin->sin_addr.s_addr;

        //printf("%s:%u.%u.%u.%u\n", ifa->ifa_name, ip->b[0], ip->b[1], ip->b[2], ip->b[3]);
    }

    return 0;
}

int skey_parse(skey_t *skey, const char *buf, int len)
{
    const char *p = buf;
    int l = len;
    int j = 0;

    // ether
    struct ether_header *pEthHdr = (struct ether_header*) p;
    if (_parse_eth(pEthHdr, &j) != 0) {
        return -1;
    }
    p += j;
    l -= j;

    // ip
    struct iphdr *pIpHdr = (struct iphdr*) p;
    if (_parse_ip(pIpHdr, &j, skey) != 0) {
        return -1;
    }
    p += j;
    l -= j;

    // tcp
    if (skey->type == TYPE_TCP) {
        struct tcphdr *pTcpHdr = (struct tcphdr*) p;
        skey->src_port = ntohs(pTcpHdr->source);
        skey->dst_port = ntohs(pTcpHdr->dest);
    }
    // udp
    else if (skey->type == TYPE_UDP) {
        struct udphdr *pUdpHdr = (struct udphdr*) p;
        skey->src_port = ntohs(pUdpHdr->source);
        skey->dst_port = ntohs(pUdpHdr->dest);
    }
    else {
        return -1;
    }

    return 0;
}

int skey_fmt(skey_t *skey, u8 *dir, hashm_t *ips)
{
    *dir = DIR_UP;

    if (jn_hashm_find(ips, (void*)&skey->src_ip, sizeof(ipv4), NULL) == 0) {
        return 0;
    }
    else if (jn_hashm_find(ips, (void*)&skey->dst_ip, sizeof(ipv4), NULL) == 0) {
        ipv4 ip;
        ip.dw = skey->src_ip.dw;
        skey->src_ip.dw = skey->dst_ip.dw;
        skey->dst_ip.dw = ip.dw;

        u16 port = skey->src_port;
        skey->src_port = skey->dst_port;
        skey->dst_port = port;

        *dir = DIR_DOWN;

        return 0;
    }

    return -1;
}

void skey_print(skey_t *skey, u8 dir)
{
    if (dir == DIR_UP) {
        printf("[%s] %u.%u.%u.%u:%u -> %u.%u.%u.%u:%u\n",
                skey->type==TYPE_TCP?"TCP":"UDP",
                skey->src_ip.b[0], skey->src_ip.b[1], skey->src_ip.b[2], skey->src_ip.b[3],
                skey->src_port,
                skey->dst_ip.b[0], skey->dst_ip.b[1], skey->dst_ip.b[2], skey->dst_ip.b[3],
                skey->dst_port);
    }
    else {
        printf("[%s] %u.%u.%u.%u:%u <- %u.%u.%u.%u:%u\n",
                skey->type==TYPE_TCP?"TCP":"UDP",
                skey->src_ip.b[0], skey->src_ip.b[1], skey->src_ip.b[2], skey->src_ip.b[3],
                skey->src_port,
                skey->dst_ip.b[0], skey->dst_ip.b[1], skey->dst_ip.b[2], skey->dst_ip.b[3],
                skey->dst_port);
    }
}

int session_init(hashm_t *sessions)
{
    return jn_hashm_init(sessions, 20, sizeof(session_t), 256);
}

int session_add(hashm_t *mhash, skey_t *skey, session_t **session)
{
    if (jn_hashm_add(mhash, (void*)skey, sizeof(skey_t), (void**)session) != 0) {
        return -1;
    }
    if (session != NULL) {
        memcpy(&((*session)->skey), skey, sizeof(skey_t));
        (*session)->refresh_time = time(NULL);
        (*session)->send_byte = 0;
        (*session)->recv_byte = 0;
        (*session)->calc_time = time(NULL);
        (*session)->old_send_byte = 0;
        (*session)->old_recv_byte = 0;

        (*session)->inode = 0;

        return 0;
    }

    return -1;
}

int session_bind_inode(session_t *session, hashm_t *m_inode)
{
    u32 *inode = NULL;
    if (jn_hashm_find(m_inode, (void*)&session->skey, sizeof(skey_t), (void**)&inode) != 0) {
        inode_load(m_inode);
    }

    if (jn_hashm_find(m_inode, (void*)&session->skey, sizeof(skey_t), (void**)&inode) != 0) {
        return -1;
    }

    session->inode = *inode;

    return 0;
}

int session_bind_pinfo(session_t *session, hashm_t *pinfos)
{
    pinfo_t *pinfo = NULL;
    if (jn_hashm_find(pinfos, (void*)&session->inode, sizeof(session->inode), (void**)&pinfo) != 0) {
        pinfo_refille(pinfos);
    }

    if (jn_hashm_find(pinfos, (void*)&session->inode, sizeof(session->inode), (void**)&pinfo) != 0) {
        return -1;
    }

    session->pinfo = pinfo;

    return 0;
}

int session_bind_process(session_t *session, hashm_t *pros)
{
    process_t *proc = NULL;
    if (jn_hashm_find(pros, (void*)&session->pinfo->pid, sizeof(session->pinfo->pid), (void**)&proc) != 0) {
        jn_hashm_add(pros, (void*)&session->pinfo->pid, sizeof(session->pinfo->pid), (void**)&proc);
        if (proc == NULL) {
            return -1;
        }

        memset(proc, 0x00, sizeof(process_t));
        memcpy(&proc->pinfo, session->pinfo, sizeof(pinfo_t));
    }

    if (proc == NULL) {
        return -1;
    }

    session->proc = proc;

    return 0;
}



int session_flow_data(session_t *session, u8 dir, int len)
{
    session->refresh_time = time(NULL);

    if (dir == DIR_UP) {
        session->send_byte += len;
    }
    else {
        session->recv_byte += len;
    }

    return 0;
}

int session_flow_process(session_t *session, u8 dir, int len)
{
    if (session->proc == NULL) {
        return -1;
    }

    session->proc->refresh_time = time(NULL);

    if (dir == DIR_UP) {
        session->proc->send_byte += len;
    }
    else {
        session->proc->recv_byte += len;
    }

    return 0;
}

int session_timeout(hashm_t *mhash)
{
    time_t now = time(NULL);

    session_t *session = NULL;

    hashm_iter_t iter;
    jn_hashm_first(mhash, &iter, (void**)&session);
    while (session != NULL) {
        if (now - session->refresh_time >= SESSION_TIMEOUT) {
            jn_hashm_del(mhash, (void*)&session->skey, sizeof(skey_t));
        }

        jn_hashm_next(mhash, &iter, (void**)&session);
    }

    return 0;
}
