#ifndef __INCLUDE_NSTDIO_H__
#define __INCLUDE_NSTDIO_H__

#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h> // strdup
#include <unistd.h> // close
#include <sys/socket.h>  // accept, bind
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // inet_addr
#include <errno.h>
#include "nstdio.h"

#define SERVER 1
#define CLIENT 2

#define WRITE 1
#define READ  2

#define START 1

#define MAX_HANDLE_QSIZE 65536

static void ppsync(ND *nd){
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
    volatile ND *nd;
    uint64_t rc;
    nd = (ND *)arnd;
    
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

NHDL *nwrite( ND *nd, void *addr, size_t size){
    
    NHDL *hdl;
    uint64_t id, qidx;
    
    hdl = (NHDL *)malloc(sizeof(NHDL));
    
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

NHDL *nread( ND *nd, void *addr, size_t size){
    
    NHDL *hdl;
    uint64_t id, qidx;
    
    hdl = (NHDL *)malloc(sizeof(NHDL));
    
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


int nquery(NHDL *hdl){

    if (hdl->id < hdl->nd->hqhead){
        return 0;
    }
    else{
        return 1;
    }
}

ND *nopen(NET *nt){
        
    int sock, sock_accept;
    struct sockaddr_in addr;
    socklen_t addrlen;
    int on; /* socket option */
    
    int32_t ipdiff, portdiff;
    int svflag;
    
    uint64_t hqtail = 0;
    uint64_t hqhead = 0;
    
    volatile ND *nd;
    
    int rc = 0;
    int errno;
    
    nd = (ND *)malloc(sizeof(ND));
    
    nd->lip = inet_addr(nt->lip_addr);
    nd->rip = inet_addr(nt->rip_addr);
    nd->lport = htons(nt->lport);
    nd->rport = htons(nt->rport);
    nd->sock = -1;
    
    ipdiff = (int32_t)(nd->rip - nd->lip) ;
    if (ipdiff > 0){
        nd->svflag = SERVER;
    }
    else if (ipdiff < 0 ){
        nd->svflag = CLIENT;
    }
    else {
        portdiff = (int32_t)(nd->rport - nd->lport);
        if (portdiff > 0){
            nd->svflag = SERVER;
        }
        else if (portdiff < 0){
            nd->svflag = CLIENT;
        }
        else {
            fprintf(stderr, "error: address and port is same remote ones.\n");
            exit(1);
        }
    }
    
    nd->Dflag = nt->Dflag;
    nd->finflag = 0;
    
    svflag = nd->svflag;
    nd->hqhead = hqhead;
    nd->hqtail = hqtail;
    nd->hdlq = (HQ *)malloc(sizeof(HQ) * MAX_HANDLE_QSIZE);
    
    memset((char *)nd->hdlq, 0, sizeof(HQ) * MAX_HANDLE_QSIZE);
    
    /* generate server socket */
    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket() failed \n");
        rc = -1;
        goto exit;
    }
#if DEBUG
    fprintf(stdout, "sv flagcheck section\n");
    fflush(stdout);
#endif
    
    if(svflag == SERVER){ 
        /* generate destination address */
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = nd->lip;
        addr.sin_port = htons(nd->lport);
#if DEBUG
        fprintf(stdout, "lip %u lp %u rip %u rp %u\n", nd->lip, nd->lport, nd->rip, nd->rport);
        fflush(stdout);
#endif
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
    else if (svflag == CLIENT){
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = nd->rip;
        addr.sin_port = htons(nd->rport);
        
#if DEBUG
        fprintf(stdout, "lip %u lp %u rip %u rp %u\n", nd->lip, nd->lport, nd->rip, nd->rport);
        fflush(stdout);
#endif
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
        fprintf(stderr, "server flag error\n");
        goto exit;
    }
    
    pthread_create((pthread_t *)&(nd->comm_thread_id), NULL, comm_thread_func, (void *)nd);
    
    ppsync((ND *)nd);
    
    return (ND *)nd;

exit:
    exit(1);
}

void nclose(ND *nd){
    
    nd->finflag = 1;
    pthread_join(nd->comm_thread_id, NULL);
    
    close(nd->sock);
    if (NULL != nd) {
        free(nd);
    }    

    return ;
}

#endif
