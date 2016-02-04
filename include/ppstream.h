#ifndef __INCLUDE_PPSTREAM_H__
#define __INCLUDE_PPSTREAM_H__

#include <stdint.h>
#include <pthread.h>

#define PPSTREAM_TCP    1
#define PPSTREAM_UDP    2
#define PPSTREAM_IBRC  10
#define PPSTREAM_IBUD  11

#define PPSTREAM_SERVER 1
#define PPSTREAM_CLIENT 2

typedef struct ppstream_networkinfo {
    char *ip_addr;
    char *port;
    uint32_t Dflag;
    uint32_t scflag;
} ppstream_networkinfo_t;

typedef struct ppstearm_handlequeue {
    uint64_t id;
    void *addr;
    int type;
    size_t size;
    int status;
    uint64_t msize;
} ppstream_handlequeue_t;

typedef struct ppstream_networkdescriptor{
    char *ip;
    char *port;
    uint32_t scflag;
    uint32_t Dflag;
    pthread_t comm_thread_id;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int finflag;
    int sock;
    uint64_t hqhead;
    uint64_t hqtail;
    ppstream_handlequeue_t *hdlq;
} ppstream_networkdescriptor_t;

typedef struct ppstream_handle {
    uint64_t id;
    uint64_t msize;
    ppstream_networkdescriptor_t *nd;
} ppstream_handle_t;

void ppstream_sync(ppstream_networkdescriptor_t *nd);

ppstream_networkdescriptor_t *ppstream_open(ppstream_networkinfo_t *nt);

void ppstream_close(ppstream_networkdescriptor_t *nd);

ppstream_handle_t *ppstream_input(ppstream_networkdescriptor_t *nd, void *addr, size_t size);

ppstream_handle_t *ppstream_output(ppstream_networkdescriptor_t *nd, void *addr, size_t size);

int ppstream_test(ppstream_handle_t *hdl);

ppstream_networkinfo_t *ppstream_set_networkinfo(char *ip_addr, char *port, uint32_t scflag, uint32_t Dflag);

void ppstream_free_networkinfo(ppstream_networkinfo_t *nt);

#endif
