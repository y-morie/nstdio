#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h> // strdup
#include <unistd.h> // close
#include <sys/socket.h>  // accept, bind
#include <netinet/in.h> // sockaddr_in
#include <netdb.h>
#include <arpa/inet.h> // inet_addr
#include <errno.h>
#include <inttypes.h> // 32/64bit for printf format
#include <ppstream.h>

#define PPS_WRITE 1
#define PPS_READ  2

#define PPS_COMP  0
#define PPS_START 1

#define MAX_HANDLE_QSIZE 65536

void ppstream_sync(ppstream_networkdescriptor_t *nd){
    char dammy;
        
    dammy = 'x';

#if DEBUG
    fprintf(stdout, "ppstream_sync: nd->sock = %d\n", nd->sock);
    fflush(stdout);
#endif
    
    write(nd->sock, &dammy, sizeof(dammy));
    recv(nd->sock, &dammy, sizeof(dammy), MSG_WAITALL);

    return;
}

static void *comm_thread_func(void *arnd){
    
    /* message handl queue */
    uint64_t qidx = 0;
    ppstream_networkdescriptor_t *nd;
    uint64_t rc;
    
    /* set network descriptor */
    nd = (ppstream_networkdescriptor_t *) arnd;
    
    while (1) {
        pthread_mutex_lock(&(nd->mutex));
        /* if handle queue is empty, the comm. thread sleeps. */
        if (nd->finflag != 1 && nd->hqtail - nd->hqhead == 0) pthread_cond_wait(&(nd->cond), &(nd->mutex));

        qidx = nd->hqhead % MAX_HANDLE_QSIZE;

#if DEBUG_CH_LOOP_CHK        
        fprintf(stdout, "nd->hdlq[%" PRIu64 "] %p st %d\n", qidx, & nd->hdlq[qidx], nd->hdlq[qidx].status);
        fflush(stdout);
#endif
        if (&nd->hdlq[qidx] == NULL) {
            fprintf(stdout, "nd->hdlq[%" PRIu64 "] %p\n", qidx, & nd->hdlq[qidx]);
            exit(1);
        }
        
        if (nd->hdlq[qidx].status == PPS_START) {
            switch (nd->hdlq[qidx].type) {
            case PPS_WRITE:
#if DEBUG
                fprintf(stdout, "PPS_WRITE: hd %" PRIu64 " tp %d \n",nd->hqhead, nd->hdlq[qidx].type);
                fflush(stdout);
#endif
                rc = (uint64_t)write(nd->sock, nd->hdlq[qidx].addr, nd->hdlq[qidx].size);
                if (rc > 0) {
                    nd->hdlq[qidx].msize = rc;
                }
                nd->hqhead++;
                nd->hdlq[qidx].status = PPS_COMP;
#if DEBUG
                fprintf(stdout, "PPS_WRITE: hd %" PRIu64 " mc %" PRIu64 " rc %" PRIu64 "\n",nd->hqhead, nd->hdlq[qidx].msize, rc);
                fflush(stdout);
#endif
                break;
            case PPS_READ:
#if DEBUG
                fprintf(stdout, "PPS_READ: hd %" PRIu64 " tp %d\n", nd->hqhead,  nd->hdlq[qidx].type);
                fflush(stdout);
#endif
                rc = (uint64_t)recv(nd->sock, nd->hdlq[qidx].addr, nd->hdlq[qidx].size, MSG_WAITALL);
#if DEBUG
                perror("recv return code:");
#endif
                if (rc > 0) {
                    nd->hdlq[qidx].msize = rc;
                }
                nd->hqhead++;
                nd->hdlq[qidx].status = PPS_COMP;
#if DEBUG
                fprintf(stdout, "PPS_READ: hd %" PRIu64 " mc %" PRIu64 " rc %" PRIu64 "\n", nd->hqhead, nd->hdlq[qidx].msize, rc);
                fflush(stdout);
#endif
                break;
            default:
                break;
            }
        }
        if (nd->finflag == 1) {
            return 0;
        }
        pthread_mutex_unlock(&(nd->mutex));
    }
}

ppstream_handle_t *ppstream_input( ppstream_networkdescriptor_t *nd, void *ptr, size_t size){
    
    ppstream_handle_t *hdl;
    uint64_t id, qidx;
    
    hdl = (ppstream_handle_t *)malloc(sizeof(ppstream_handle_t));
    if (NULL == hdl) {
        return NULL;
    }
    memset(hdl, 0, sizeof(ppstream_handle_t));
    id = nd->hqtail;
    qidx = id % MAX_HANDLE_QSIZE;

#if DEBUG
    fprintf(stdout, "ppsteram_input: qidx %" PRIu64 " size %zu  hdlq %p\n", qidx ,size, nd->hdlq);
    fflush(stdout);
#endif
    /* access handle queue */
    pthread_mutex_lock(&(nd->mutex));
    nd->hdlq[qidx].addr = ptr;
    nd->hdlq[qidx].size = size;   
    nd->hdlq[qidx].type = PPS_WRITE;
    nd->hdlq[qidx].status = PPS_START;
    nd->hqtail++;
    pthread_cond_signal(&(nd->cond));
    pthread_mutex_unlock(&(nd->mutex));
    
    hdl->id = id;
    hdl->nd = nd;
    hdl->msize = 0;
    
    return hdl;
}

ppstream_handle_t *ppstream_output( ppstream_networkdescriptor_t *nd, void *ptr, size_t size){
    
    ppstream_handle_t *hdl;
    uint64_t id, qidx;
    
    hdl = (ppstream_handle_t *)malloc(sizeof(ppstream_handle_t));
    if (NULL == hdl) {
        return NULL;
    }
    memset(hdl, 0, sizeof(ppstream_handle_t));
    id = nd->hqtail;
    qidx = id % MAX_HANDLE_QSIZE;

#if DEBUG
    fprintf(stdout, "ppstream_output: qidx %" PRIu64 " size %zu hdlq %p\n", qidx ,size, nd->hdlq);
    fflush(stdout);
#endif
    /* access handle queue */
    pthread_mutex_lock(&(nd->mutex));
    nd->hdlq[qidx].addr = ptr;
    nd->hdlq[qidx].size = size;
    nd->hdlq[qidx].type = PPS_READ;
    nd->hdlq[qidx].status = PPS_START;
    nd->hqtail++;
    pthread_cond_signal(&(nd->cond));
    pthread_mutex_unlock(&(nd->mutex));
    
    hdl->id = id;
    hdl->nd = nd;
    hdl->msize = 0;
    
    return hdl;
}

int ppstream_test(ppstream_handle_t *hdl){
    /*
    fprintf(stderr, "%d\n", hdl->nd->mutex);
    exit(1);
    */
    pthread_mutex_lock(&(hdl->nd->mutex));
    if (hdl->id < hdl->nd->hqhead) {
        hdl->msize =  hdl->nd->hdlq[hdl->id].msize ;
        pthread_mutex_unlock(&(hdl->nd->mutex));
        return 0;
    }
    else{
        pthread_mutex_unlock(&(hdl->nd->mutex));
        return 1;
    }
}

ppstream_networkdescriptor_t *ppstream_open(ppstream_networkinfo_t *nt){
    
    /* socket descriptor */
    int sock, sock_accept;
    /* for accept */
    socklen_t addrlen;
    /* for getaddrinfo */
    struct addrinfo *res = NULL;
    struct addrinfo *ai;
    struct addrinfo hints;
    /* flag of server or client */
    int scflag;
    /* socket option */
    int on; 
    
    uint64_t hqtail = 0;
    uint64_t hqhead = 0;
    
    ppstream_networkdescriptor_t *nd;
    
    int rc = 0;
    int errno;

#ifdef DEBUG    
    fprintf(stdout, "ppstream_open: address %s port %s\n", nt->ip_addr, nt->port);
    fflush(stdout);
#endif 

    /* malloc networkdescriptor */
    nd = (ppstream_networkdescriptor_t *)malloc(sizeof(ppstream_networkdescriptor_t));
    
    /* set ip address and port */
    nd->ip = nt->ip_addr;
    nd->port = nt->port;
    nd->sock = -1;
    
    /* set flag which is define server or client */
    nd->scflag = nt->scflag;
    if (nt->scflag != PPSTREAM_CLIENT && nt->scflag != PPSTREAM_SERVER) {
        fprintf(stderr, "error: not set PPSTREAM_CLIENT or PPSTREAM_SERVER.\n");
        exit(1);
    }
    
    /* set Device flag ex) TCP or UDP, IB RC, IB UD etc. */
    nd->Dflag = nt->Dflag;
    /* finalization flag of comm. thread */
    nd->finflag = 0;
    
    /* set handle queue poiner */
    nd->hqhead = hqhead;
    nd->hqtail = hqtail;
    nd->hdlq = (ppstream_handlequeue_t *)malloc(sizeof(ppstream_handlequeue_t) * MAX_HANDLE_QSIZE);
    
    /* clear handle queue by 0 */
    memset((char *)nd->hdlq, 0, sizeof(ppstream_handlequeue_t) * MAX_HANDLE_QSIZE);
    
#if DEBUG
    fprintf(stdout, "ppstream_open: server or client flagcheck section\n");
    fflush(stdout);
#endif
    
    /* generate address infomaition */
    memset(&hints, 0, sizeof(hints));
        
    scflag = nd->scflag;
    if (scflag == PPSTREAM_SERVER) { 
        /* set hints for getaddrinfo on server */
        memset(&hints, 0, sizeof(hints)); 
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        
        rc = getaddrinfo(nd->ip, nd->port, &hints, &res);
        ai = res;

        /* generates socket for server and client */
        if ((sock = socket (ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0) {
            perror("socket() failed:");
            rc = -1;
            goto exit;
        }

        /* bind socket */
        rc = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (const void*)&on, sizeof(on) );    
        if (rc == -1) {
            fprintf(stderr, "setsockopt() failed: rc %d\n", rc);
            goto exit;
        }
        
        while (bind(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
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
        
        addrlen = ai->ai_addrlen;
        while ((sock_accept = accept(sock, ai->ai_addr, &addrlen)) < 0) {
            perror("accept() failed:");
            goto exit;
        }
        close(sock);
        
        nd->sock = sock_accept;
    }
    else if (scflag == PPSTREAM_CLIENT) {
        /* set hints for getaddrinfo on server */
        memset(&hints, 0, sizeof(hints)); 
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
        rc = getaddrinfo(nd->ip, nd->port, &hints, &res);
        
        /* set address info form getaddrinfo */
        while(1) {
            for (ai = res; ai; ai = ai->ai_next) {
                
                /* generates socket for server and client */
                if ((sock = socket (ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0) {
                    perror("socket() failed:");
                    rc = -1;
                    goto exit;
                }
                /* connect the ai->addr */
                if (connect(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
                    if (errno != EINTR && errno != EAGAIN && errno != ECONNREFUSED) {
                        perror("connect() failed:");
                        goto exit;
                    }
                }
                else {
                    nd->sock = sock;
                    goto connect_success;
                }
                sleep(1);
            }
        }
    }
    else{
        fprintf(stderr, "error: not set PPSTREAM_CLIENT or PPSTREAM_SERVER.\n");
        goto exit;
    }
    
connect_success:
    
    freeaddrinfo(res);
    
    rc = pthread_cond_init(&(nd->cond), NULL);
    pthread_mutex_init(&(nd->mutex), NULL);
    
    /* initialize communication thread */
    rc = pthread_create((pthread_t *)&(nd->comm_thread_id), NULL, comm_thread_func, (void *)nd);
    
    ppstream_sync((ppstream_networkdescriptor_t *)nd);
    
    return (ppstream_networkdescriptor_t *)nd;

exit:
    return NULL;
}

void ppstream_close(ppstream_networkdescriptor_t *nd){
    
    nd->finflag = 1;
    pthread_cond_signal(&(nd->cond));
    pthread_join(nd->comm_thread_id, NULL);
    
    ppstream_sync(nd);
    
    close(nd->sock);
    if (NULL != nd) {
        free(nd);
    }    

    return ;
}

ppstream_networkinfo_t *ppstream_set_networkinfo(char *hostname, char *servname, uint32_t scflag, uint32_t Dflag){
    ppstream_networkinfo_t *nt;
    
    nt = (ppstream_networkinfo_t *)malloc(sizeof(ppstream_networkinfo_t));
    if (NULL == nt) {
        return NULL;
    }
#ifdef DEBUG
    fprintf(stdout, "ppstream_set_networkinfo: start hostname %s servname %s nt->ip_addr %s np->port %s\n", hostname, servname, nt->ip_addr, nt->port);
#endif
    if ( hostname != NULL ) {
        nt->ip_addr = strdup(hostname);
    }
    else {
        nt->ip_addr = NULL;
    }
    if ( servname != NULL ) {
        nt->port = strdup(servname);
    }
    else {
        nt->port = NULL;
    }
    nt->Dflag = Dflag;
    nt->scflag = scflag;
#ifdef DEBUG
    fprintf(stdout, "ppsteram_set_networkinfo: fin hostname %s servname %s nt->ip_addr %s np->port %s\n", hostname, servname, nt->ip_addr, nt->port);
    fflush(stdout);
#endif 
  
    return nt;
}

void ppstream_free_networkinfo(ppstream_networkinfo_t *nt){
    
    if (nt != NULL) {
        free(nt);
    }
    
    return;
}
