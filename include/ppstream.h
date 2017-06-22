#ifndef __INCLUDE_PPSTREAM_H__
#define __INCLUDE_PPSTREAM_H__

/**
 * @file ppstream.h
 */

#include <stdint.h>
#include <pthread.h>
#include <inttypes.h>

#define PPSTREAM_TCP    1
#define PPSTREAM_UDP    2
#define PPSTREAM_IBRC  10
#define PPSTREAM_IBUD  11

#define PPSTREAM_SERVER 1
#define PPSTREAM_CLIENT 2

#define PPSTREAM_MODE_WR 0
#define PPSTREAM_MODE_WO 1
#define PPSTREAM_MODE_RO 2

typedef struct ppstream_networkinfo {
    char *pp_ipaddr;
    char *pp_port;
    uint32_t pp_devflag;
    uint32_t pp_scflag;
    uint32_t pp_mode;
    size_t pp_set_segment;
    double pp_set_timeout;
    double pp_set_cntimeout;
    int pp_set_nretry;
} ppstream_networkinfo_t;

typedef struct ppstearm_handlequeue {
    uint64_t pp_id;
    void *pp_addr;
    size_t pp_sizeorg;
    size_t pp_size;
    size_t pp_sendsize;
    size_t pp_compsize;
    int pp_type;
    int pp_status;
    double pp_stime;
    double pp_etime;
} ppstream_handlequeue_t;

typedef struct ppstream_networkdescriptor{
    char *pp_ipaddr;
    char *pp_port;
    uint32_t pp_scflag;
    uint32_t pp_mode;
    uint32_t pp_devflag;
    pthread_t pp_comm_thread_id;
    pthread_mutex_t pp_mutex;
    pthread_cond_t pp_cond;
    int pp_finflag;
    int pp_connect_status;
    int pp_sock;
    int pp_socks;
    uint64_t pp_shqhead;
    uint64_t pp_shqtail;
    uint64_t pp_rhqhead;
    uint64_t pp_rhqtail;
    ppstream_handlequeue_t *shdlq;
    ppstream_handlequeue_t *rhdlq;
    struct addrinfo *pp_ai;
    size_t pp_set_segment;
    double pp_set_timeout;
    double pp_set_cntimeout;
    double pp_chtimeout_stime;
    double pp_chtimeout_etime;
    int pp_set_nretry;
    int cerrcode;
} ppstream_networkdescriptor_t;

typedef struct ppstream_handle {
    uint64_t pp_id;
    uint64_t pp_msize;
    ppstream_networkdescriptor_t *nd;
    int pp_hdltype;
} ppstream_handle_t;

void ppstream_sync(ppstream_networkdescriptor_t *nd);

ppstream_networkdescriptor_t *ppstream_open(ppstream_networkinfo_t *nt);

void ppstream_close(ppstream_networkdescriptor_t *nd);

ppstream_handle_t *ppstream_input(ppstream_networkdescriptor_t *nd, void *ptr, size_t size);

ppstream_handle_t *ppstream_output(ppstream_networkdescriptor_t *nd, void *ptr, size_t size);

int ppstream_test(ppstream_handle_t *hdl);

ppstream_networkinfo_t *ppstream_set_networkinfo(char *hostname, char *servname, uint32_t scflag, uint32_t devflag);

void ppstream_free_networkinfo(ppstream_networkinfo_t *nt);

void ppstream_set_cntimeout(ppstream_networkdescriptor_t *nd, double timeout);

#endif
