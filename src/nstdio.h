#include <stdint.h>
#include <pthread.h>

#define TCP    0
#define UDP    1
#define IB_RC 10

typedef struct NetworkInfo {
    char *lip_addr;
    char *rip_addr;
    uint16_t lport;
    uint16_t rport;
    uint32_t Dflag;
} NET;

typedef struct HandleQueue {
    uint64_t id;
    void *addr;
    int type;
    size_t size;
    int status;
    uint64_t msize;
} HQ;

typedef struct NetworkDiscrptor {
    uint32_t lip;
    uint32_t rip;
    uint16_t lport;
    uint16_t rport;
    uint32_t svflag;
    uint32_t Dflag;
    pthread_t comm_thread_id;
    int finflag;
    int sock;
    uint64_t hqhead;
    uint64_t hqtail;
    volatile HQ *hdlq;
} ND;

typedef struct Handle {
    uint64_t id;
    ND *nd;
} NHDL;

ND *nopen(NET *nt);

void nclose(ND *nd);

NHDL *nwrite(ND *nd, void *addr, size_t size);

NHDL *nread(ND *nd, void *addr, size_t size);

int nquery(NHDL *hdl);
