#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h> // strdup
#include <unistd.h> // close
#include <sys/socket.h>  // accept, bind
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // inet_addr
#include <errno.h>
#include <ppstream.h>

#define SERVER 1
#define CLIENT 2

#define WRITE 1
#define READ  2

#define START 1

#define MAX_HANDLE_QSIZE 65536

static void ppstream_sync(ppstream_networkdescriptor_t *nd){
    char dammy;
    
    dammy = 'x';

#if DEBUG
    fprintf(stdout, "nd->sock = %d\n", nd->sock);
    fflush(stdout);
#endif
    
    write(nd->sock, &dammy, sizeof(dammy));
    recv(nd->sock, &dammy, sizeof(dammy), MSG_WAITALL);
}

static void *comm_thread_func(void *arnd){
    
    /* message handl queue */
    uint64_t qidx = 0;
    volatile ppstream_networkdescriptor_t *nd;
    uint64_t rc;
    
    nd = (ppstream_networkdescriptor_t *) arnd;
    
    while (1) {
        qidx = nd->hqhead % MAX_HANDLE_QSIZE;
#if DEBUG_CH_LOOP_CHK        
        fprintf(stdout, "nd->hdlq[%lu] %p st %d\n", qidx, & nd->hdlq[qidx], nd->hdlq[qidx].status);
        fflush(stdout);
#endif
        if(&nd->hdlq[qidx] == NULL){
            fprintf(stdout, "nd->hdlq[%lu] %p\n", qidx, & nd->hdlq[qidx]);
            exit(1);
        }
        if (nd->hdlq[qidx].status == START){
            switch(nd->hdlq[qidx].type){
            case WRITE:
#if DEBUG
                fprintf(stdout, "WRITE: hd %lu tp %d \n",nd->hqhead, nd->hdlq[qidx].type);
                fflush(stdout);
#endif
                rc = (uint64_t)write(nd->sock, nd->hdlq[qidx].addr, nd->hdlq[qidx].size);
                if (rc > 0) {
                    nd->hdlq[qidx].msize = rc;
                }
                nd->hqhead++;
#if DEBUG
                fprintf(stdout, "WRITE: hd %lu mc %lu rc %lu\n",nd->hqhead, nd->hdlq[qidx].msize, rc);
                fflush(stdout);
#endif
                break;
            case READ:
#if DEBUG
                fprintf(stdout, "READ: hd %lu tp %d\n", nd->hqhead,  nd->hdlq[qidx].type);
                fflush(stdout);
#endif
                rc = (uint64_t)recv(nd->sock, nd->hdlq[qidx].addr, nd->hdlq[qidx].size, MSG_WAITALL);
                if (rc > 0) {
                    nd->hdlq[qidx].msize = rc;
                }
                nd->hqhead++;
#if DEBUG
                fprintf(stdout, "READ: hd %lu mc %lu rc %lu\n", nd->hqhead, nd->hdlq[qidx].msize, rc);
                fflush(stdout);
#endif
                break;
            default:
                break;
            }
        }
        if (nd->finflag == 1){
            return 0;
        }
    }
}

ppstream_handle_t *ppstream_input( ppstream_networkdescriptor_t *nd, void *addr, size_t size){
    
    ppstream_handle_t *hdl;
    uint64_t id, qidx;
    
    hdl = (ppstream_handle_t *)malloc(sizeof(ppstream_handle_t));
    
    id = nd->hqtail;
    qidx = id % MAX_HANDLE_QSIZE;

#if DEBUG
    fprintf(stdout, "qidx %lu size %lu hdlq %p\n", qidx ,size, nd->hdlq);
    fflush(stdout);
#endif

    nd->hdlq[qidx].addr = addr;
    nd->hdlq[qidx].size = size;   
    nd->hdlq[qidx].type = WRITE;
    nd->hdlq[qidx].status = START;

    nd->hqtail++;
    
    hdl->id = id;
    hdl->nd = nd;
    
    return hdl;
}

ppstream_handle_t *ppstream_output( ppstream_networkdescriptor_t *nd, void *addr, size_t size){
    
    ppstream_handle_t *hdl;
    uint64_t id, qidx;
    
    hdl = (ppstream_handle_t *)malloc(sizeof(ppstream_handle_t));
    
    id = nd->hqtail;
    qidx = id % MAX_HANDLE_QSIZE;

#if DEBUG
    fprintf(stdout, "qidx %lu size %lu hdlq %p\n", qidx ,size, nd->hdlq);
    fflush(stdout);
#endif
    
    nd->hdlq[qidx].addr = addr;
    nd->hdlq[qidx].size = size;
    nd->hdlq[qidx].type = READ;
    nd->hdlq[qidx].status = START;
    
    nd->hqtail++;
    
    hdl->id = id;
    hdl->nd = nd;
    
    return hdl;
}

int ppstream_test(ppstream_handle_t *hdl){

    if (hdl->id < hdl->nd->hqhead){
        hdl->msize =  hdl->nd->hdlq[hdl->id].msize ;
        return 0;
    }
    else{
        return 1;
    }
}

ppstream_networkdescriptor_t *ppstream_open(ppstream_networkinfo_t *nt){
    
    int sock, sock_accept;
    struct sockaddr_in addr;
    socklen_t addrlen;
    int scflag;
    int on; /* socket option */
        
    uint64_t hqtail = 0;
    uint64_t hqhead = 0;
    
    volatile ppstream_networkdescriptor_t *nd;
    
    int rc = 0;
    int errno;
    
    nd = (ppstream_networkdescriptor_t *)malloc(sizeof(ppstream_networkdescriptor_t));
    
    nd->ip = inet_addr(nt->ip_addr);
    nd->port = htons(nt->port);
    nd->sock = -1;
    
    nd->scflag = nt->scflag;
    if(nt->scflag != PPSTREAM_CLIENT && nt->scflag != PPSTREAM_SERVER){
        fprintf(stderr, "error: not set PPSTREAM_CLIENT or PPSTREAM_SERVER.\n");
        exit(1);
    }
        
    nd->Dflag = nt->Dflag;
    nd->finflag = 0;
    
    scflag = nd->scflag;
    nd->hqhead = hqhead;
    nd->hqtail = hqtail;
    nd->hdlq = (ppstream_handlequeue_t *)malloc(sizeof(ppstream_handlequeue_t) * MAX_HANDLE_QSIZE);
    
    memset((char *)nd->hdlq, 0, sizeof(ppstream_handlequeue_t) * MAX_HANDLE_QSIZE);
    
    /* generates socket for server and client */
    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket() failed \n");
        rc = -1;
        goto exit;
    }
#if DEBUG
    fprintf(stdout, "s or c flagcheck section\n");
    fflush(stdout);
#endif
    /* generate address infomaition */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = nd->ip;
    addr.sin_port = htons(nd->port);
#if DEBUG
    fprintf(stdout, "ip %u p %u %u", nd->ip, nd->port, nd->scflag);
    fflush(stdout);
#endif
    
    if(scflag == PPSTREAM_SERVER){ 
        /* bind socket */
        rc = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (const void*)&on, sizeof(on) );    
        if(rc == -1){
            fprintf(stderr, "setsockopt rc %d\n", rc);
            goto exit;
        }
        while (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            if (errno != EADDRINUSE) {
                perror("bind() failed:");
                rc = -1;
                goto exit;
            }
        }
        
        /* listen socket */
        if (listen(sock, 10) < 0) {
            perror("listen() failed:");
            rc = -1;
            goto exit;
        }
        
        addrlen = sizeof(addr);
        while ((sock_accept = accept(sock, (struct sockaddr *)&addr, &addrlen)) < 0) {
            perror("accept() failed:");
            goto exit;
        }
        close(sock);
        
        nd->sock = sock_accept;
    }
    else if (scflag == PPSTREAM_CLIENT){
        while (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0){
            if (errno != EINTR && errno != EAGAIN && errno != ECONNREFUSED) {
                perror("connect() failed:");
                goto exit;
            }
            else{
                sleep(1);
            }
        }
        nd->sock = sock;
    }
    else{
        fprintf(stderr, "error: not set PPSTREAM_CLIENT or PPSTREAM_SERVER.\n");
        goto exit;
    }
    
    pthread_create((pthread_t *)&(nd->comm_thread_id), NULL, comm_thread_func, (void *)nd);
    
    ppstream_sync((ppstream_networkdescriptor_t *)nd);
    
    return (ppstream_networkdescriptor_t *)nd;

exit:
    exit(1);
}

void ppstream_close(ppstream_networkdescriptor_t *nd){
    
    nd->finflag = 1;
    pthread_join(nd->comm_thread_id, NULL);
    
    ppstream_sync(nd);
    
    close(nd->sock);
    if (NULL != nd) {
        free(nd);
    }    

    return ;
}

ppstream_networkinfo_t *ppstream_set_networkinfo(char *ip_addr, uint16_t port, uint32_t scflag, uint32_t Dflag){
    ppstream_networkinfo_t *nt;
    
    nt = (ppstream_networkinfo_t *)malloc(sizeof(ppstream_networkinfo_t));

    nt->ip_addr = ip_addr;
    nt->port = port;
    nt->Dflag = Dflag;
    nt->scflag = scflag;
    
    return nt;
}

void ppstream_free_networkinfo(ppstream_networkinfo_t *nt){
    
    if(nt != NULL){
        free(nt);
    }
    
    return;
}
