#include <stdio.h> // debug etc
#include <stdlib.h> // malloc, free
#include <string.h> // strdup
#include <unistd.h> // close
#include <sys/socket.h>  // accept, bind
#include <netinet/in.h> // sockaddr_in
#include <netdb.h> 
#include <arpa/inet.h> // inet_addr
#include <errno.h> 
#include <inttypes.h> // 32/64bit for printf format
#include <time.h> // gettimeofday 
#include <ppstream.h>

/* command type */
#define PPSTREAM_WRITE 1
#define PPSTREAM_READ  2

/* handle type */
#define PPSTREAM_INPUT  1
#define PPSTREAM_OUTPUT 2

/* handle status */
#define PPSTREAM_COMP  0
#define PPSTREAM_START 1
#define PPSTREAM_COMM  2

/* set default environment value */
#define PPSTREAM_DTIMEOUT   10
#define PPSTREAM_DCNTIMEOUT 16
#define PPSTREAM_DSEGMENT 1024 * 1024 * 2
#define PPSTREAM_DNRETRY  100000

/* network status */
#define PPSTREAM_UNCONNECTED 0
#define PPSTREAM_CONNECTED   1

/* resource size */
#define MAX_HANDLE_QSIZE 65536

/* get time for connection timeout */
double gettimeofday_sec(){
    
    struct timeval t;
    
    gettimeofday(&t, NULL);
    
    return (double)t.tv_sec + (double)t.tv_usec * 1e-6;
}

int check_connect( ppstream_networkdescriptor_t *nd ) {
    
    int rc; 
    int dummy = 0;

#ifdef DEBUG
    fprintf( stdout, "check_connect: start.\n" );
    fflush( stdout );
#endif
    
    /* check whether the connection is alive or not */
    rc = (uint64_t)recv(nd->pp_sock, &dummy, 0, MSG_DONTWAIT);

#ifdef DEBUG
    fprintf( stdout, "check_connect: rc %d dummy %d.\n", rc, dummy );
    fflush( stdout );
#endif

    /* if connection is dead */
    if ( rc > 0 ) {
        nd->cerrcode = 0;
    }
    else if ( rc == 0 ) {
        nd->cerrcode++;
        nd->pp_connect_status = PPSTREAM_UNCONNECTED;
    }
    else if ( rc < 0 ) { 
        if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
#ifdef DEBUG
            fprintf( stdout, "check_connect: connect is dead rc = %d.\n", rc );
            fflush( stdout );
            perror("check connect error");
#endif
        }
    }

#ifdef DEBUG
    fprintf( stdout, "check_connect: fin.\n" );
    fflush( stdout );
#endif

    return rc;
}

size_t get_segsize( ppstream_networkdescriptor_t *nd, ppstream_handlequeue_t *hdlq ) {
    
    size_t rdata; // rest of data size. 
    size_t set_segment; // set default segment size by user.
    size_t segment_size; // segment of size. 
    int rcount; // # of the segment. 
    
    /* set the rest data size*/
    rdata = hdlq->pp_size - hdlq->pp_compsize; 
    /* set default segment size */
    set_segment = nd->pp_set_segment;
    /* set # of segment */
    rcount = rdata / set_segment; 
    
    /* set segment size */
    if ( rcount >= 1) {
        segment_size = set_segment;
    }
    else { 
        segment_size = rdata  % set_segment;
    }
    return segment_size;
}

void check_comm_status( ppstream_networkdescriptor_t *nd, ppstream_handlequeue_t *hdlq, int flag , int rc) {
    
    int nretry; /* # of fails of sending  */

#ifdef DEBUG
    fprintf(stdout, "check_comm_status: start.\n");
    fflush(stdout);
#endif

    nretry = nd->pp_set_nretry;

#ifdef DEBUG
    fprintf(stdout, "check_comm_status: rc = %d nd->cerrcode %d nretry %d.\n", rc, nd->cerrcode, nretry);
    fflush(stdout);
#endif

    if ( rc > 0 ) {
        /* connet is green, reset # of error counts. */
        nd->pp_chtimeout_stime = gettimeofday_sec();
        nd->cerrcode = 0;
        hdlq->pp_compsize += rc;
        if ( hdlq->pp_compsize == hdlq->pp_size ) {
            if ( PPSTREAM_WRITE == flag ) {
                nd->pp_shqhead++;
            }
            else if ( PPSTREAM_READ == flag ) {
                nd->pp_rhqhead++;
            }
            hdlq->pp_status = PPSTREAM_COMP;
        }
        else {
            hdlq->pp_status == PPSTREAM_COMM;
        }
    }
    else {
        if ( rc == 0 ) {
            if ( nd->pp_set_segment != 0 ) {
                /*  dissconnect on network description. */
                nd->pp_connect_status = PPSTREAM_UNCONNECTED;
#ifdef DEBUG
                fprintf(stdout, "check_comm_status: status unconnected rc %d.\n", rc);
                fflush(stdout);
#endif
            }
        }
        if ( rc < 0  ) {
            /* Error eccurs except stopping non blocking operation. */
            if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
                nd->pp_connect_status = PPSTREAM_UNCONNECTED;
#ifdef DEBUG
                fprintf(stdout, "check_comm_status: status unconnected rc %d.\n", rc);
                fflush(stdout);
#endif
            }
            else {
                nd->cerrcode++;
                if ( nd->cerrcode > nretry ) {
                    nd->pp_connect_status = PPSTREAM_UNCONNECTED;
#ifdef DEBUG
                    fprintf(stdout, "check_comm_status: status unconnected rc %d.\n", rc);
                    fflush(stdout);
#endif
                }

            }
        }
    }

#ifdef DEBUG
    fprintf(stdout, "check_comm_status: fin.\n");
    fflush(stdout);
#endif
    
    return;
}

static void *comm_thread_func(void *arnd){
    
    /* rerturn code */
    uint64_t rc;
    
    /* for message handle queue */
    uint64_t sqidx; /* index of send handle Q */
    uint64_t rqidx; /* index of recieve handle Q */ 
    ppstream_networkdescriptor_t *nd; /* pointer of network descriptor */
    
    /* socket file descriptoer for server */
    int socket_accept;
    
    /* measure the timeout */
    double st, et; 
    
    /* segment size */
    size_t segment_size;
    
    /* time for cond_timedwait  */
    struct timespec pctw_t;
    
    /* set network descriptor */
    nd = (ppstream_networkdescriptor_t *) arnd;
    
#ifdef DEBUG
    fprintf(stdout, "comm_thread_func: start.\n");
    fflush(stdout);
#endif
    
    pctw_t.tv_sec = 2;
    pctw_t.tv_nsec = 0;
    
    nd->pp_chtimeout_stime = gettimeofday_sec();
    while ( 1 ) {
#ifdef DEBUG
        fprintf(stdout, "comm_thread_func: pthread_mutex_lock\n");
        fflush(stdout);
#endif
        /* get a mutex lock */
        pthread_mutex_lock(&(nd->pp_mutex));
        
        /* if handle queue is empty, the comm. thread sleeps. */
        if ( nd->pp_finflag != 1 && 
             nd->pp_shqtail - nd->pp_shqhead == 0 && nd->pp_rhqtail - nd->pp_rhqhead == 0 ) {
            pthread_cond_timedwait(&(nd->pp_cond), &(nd->pp_mutex), &pctw_t); 
        }
        nd->pp_chtimeout_stime = gettimeofday_sec();
        
        /* get index of each handl queue. */
        sqidx = nd->pp_shqhead % MAX_HANDLE_QSIZE;
        rqidx = nd->pp_rhqhead % MAX_HANDLE_QSIZE;
        
#ifdef DEBUG_CH_LOOP_CHK
        fprintf(stdout, "comm_thread_func: nd->shdlq[%" PRIu64 "] %p status %d\n", sqidx, & nd->shdlq[sqidx], nd->shdlq[sqidx].pp_status);
        fprintf(stdout, "comm_thread_func: nd->rhdlq[%" PRIu64 "] %p status %d\n", rqidx, & nd->rhdlq[rqidx], nd->rhdlq[rqidx].pp_status);
        fflush(stdout);
#endif
        /* if send/recv handle is NUll, exit */
        if ( &nd->shdlq[sqidx] == NULL ) {
            fprintf(stdout, "comm_thread_func: nd->shdlq[%" PRIu64 "] %p\n", sqidx, & nd->shdlq[sqidx]);
            exit(1);
        }
        if ( &nd->rhdlq[rqidx] == NULL ) {
            fprintf(stdout, "comm_thread_func; nd->rhdlq[%" PRIu64 "] %p\n", rqidx, & nd->rhdlq[rqidx]);
            exit(1);
        }
        
        /* if status is PPSTREAM_START or PPSTREAM_COMM */
        if ( nd->shdlq[sqidx].pp_status == PPSTREAM_START || nd->shdlq[sqidx].pp_status == PPSTREAM_COMM ) {
            switch ( nd->shdlq[sqidx].pp_type ) {
            case PPSTREAM_WRITE:
                nd->pp_chtimeout_stime = gettimeofday_sec();
#ifdef DEBUG
                fprintf( stdout, "comm_thread_func: PPSTREAM_WRITE: shd %" PRIu64 " shd_type %d \n",nd->pp_shqhead, nd->shdlq[sqidx].pp_type );
                fflush( stdout );
#endif
                /* get segment size */
                segment_size = get_segsize ( nd, &(nd->shdlq[sqidx]) );
                
                /* send data  */
                rc = ( size_t )send( nd->pp_sock, nd->shdlq[sqidx].pp_addr + nd->shdlq[sqidx].pp_compsize, segment_size, MSG_DONTWAIT );

#ifdef DEBUG
                perror( "comm_thread_func: send return code" );
                fprintf( stdout, "comm_thread_func: PPSTREAM_WRITE: shd %" PRIu64 " rc %d\n", nd->pp_shqhead, rc );
                fflush( stdout );
#endif

                /* check communication status */
                check_comm_status( nd, &(nd->shdlq[sqidx]), PPSTREAM_WRITE, rc ); 
#ifdef DEBUG
                fprintf( stdout, "comm_thread_func: PPSTREAM_WRITE: shd %" PRIu64 " size %" PRIu64 " compsize %" PRIu64 " rc %d\n", 
                         nd->pp_shqhead, nd->shdlq[sqidx].pp_size, nd->shdlq[sqidx].pp_compsize, rc );
                fflush( stdout );
#endif

                break;
            
            default:
                break;
            }
        }
        if ( nd->rhdlq[rqidx].pp_status == PPSTREAM_START || nd->rhdlq[rqidx].pp_status == PPSTREAM_COMM ) {
            switch ( nd->rhdlq[rqidx].pp_type ) {
            case PPSTREAM_READ:
                nd->pp_chtimeout_stime = gettimeofday_sec();
#ifdef DEBUG
                fprintf(stdout, "comm_thread_func: PPSTREAM_READ: rhd %" PRIu64 " rhd_type %d\n", nd->pp_rhqhead,  nd->rhdlq[rqidx].pp_type);
                fflush(stdout);
#endif
                /* get segment size */
                segment_size = get_segsize ( nd, &(nd->rhdlq[rqidx]) );
                
                /* recv data  */
                rc = (size_t)recv(nd->pp_sock, nd->rhdlq[rqidx].pp_addr + nd->rhdlq[rqidx].pp_compsize, segment_size, MSG_DONTWAIT);
                
#ifdef DEBUG
                perror( "comm_thread_func: recv return code" );
                fprintf( stdout, "comm_thread_func: PPSTREAM_READ: rhd %" PRIu64 " rc %d\n", nd->pp_rhqhead,  rc );                
#endif
                /* check communication status */
                check_comm_status( nd, &( nd->rhdlq[rqidx] ), PPSTREAM_READ, rc );
#ifdef DEBUG 
                fprintf( stdout, "comm_thread_func: PPSTREAM_READ: rhd %" PRIu64 " size %" PRIu64 " compsize %" PRIu64 " rc %d\n", 
                         nd->pp_rhqhead, nd->rhdlq[rqidx].pp_size, nd->rhdlq[rqidx].pp_compsize, rc );                
                fflush( stdout );
#endif            
                break;
            
            default:
                break;
            }
        }
        
#ifdef DEBUG
        fprintf( stdout, "comm_thread_func: pthread_mutex_unlock\n" );
        fflush( stdout );
#endif
        pthread_mutex_unlock( &( nd->pp_mutex ) );
        
        nd->pp_chtimeout_etime = gettimeofday_sec();
        if ( nd->pp_chtimeout_etime - nd->pp_chtimeout_stime > nd->pp_set_cntimeout ) {
            nd->pp_connect_status == PPSTREAM_UNCONNECTED;
        }
        
        /* if server and unconneted,  close socket. */
        if ( nd->pp_scflag == PPSTREAM_SERVER && nd->pp_connect_status == PPSTREAM_UNCONNECTED ) {
            int addrlen;
            int sock_accept;
            fd_set rfds, fds;
            struct timeval timeout;
            
#ifdef DEBUG
            fprintf( stdout, "comm_thread_func: server connect close.\n" );
            fflush( stdout );
#endif
            
            close(nd->pp_sock);
            
            FD_ZERO(&fds);
            FD_SET(nd->pp_socks, &fds);
            
            /* set timeout 1 sec */
            timeout.tv_sec =  1;
            timeout.tv_usec = 0;
            
            select(nd->pp_socks + 1, &fds, NULL, NULL, &timeout);
            
            addrlen = nd->pp_ai->ai_addrlen;
            if ( FD_ISSET( nd->pp_socks, &fds ) ) {
                sock_accept = accept( nd->pp_socks, nd->pp_ai->ai_addr, &addrlen );
                if ( sock_accept > 0 ) {
                    nd->pp_sock = sock_accept;
#ifdef DEBUG
                    fprintf( stdout, "comm_thread_func: the accept of server successes.\n" );
                    fflush( stdout );
#endif
                    //ppstream_sync(nd);
                }
            }
        }
        /* if network descriptor closes, thread is finish. */
        if ( nd->pp_finflag == 1 ) {
#ifdef DEBUG
            fprintf( stdout, "comm_thread_func: fin.\n" );
            fflush( stdout );
#endif
            return 0;
        }
    }
exit:
#ifdef DEBUG
            fprintf( stdout, "comm_thread_func: exit.\n" );
            fflush( stdout );
#endif
    exit(1);
}

ppstream_handle_t *ppstream_input( ppstream_networkdescriptor_t *nd, void *ptr, size_t size){
    
    ppstream_handle_t *hdl;
    uint64_t id, qidx;
    
#ifdef DEBUG
    fprintf( stdout, "ppstream_input: start.\n" );
    fflush( stdout );
#endif

    hdl = (ppstream_handle_t *)malloc(sizeof(ppstream_handle_t));
    if ( NULL == hdl ) {
        return NULL;
    }
    memset(hdl, 0, sizeof(ppstream_handle_t));
    id = nd->pp_shqtail;
    qidx = id % MAX_HANDLE_QSIZE;
    
#ifdef DEBUG
    fprintf(stdout, "ppstream_input: qidx %" PRIu64 " size %zu rhdlq %p\n", qidx ,size, nd->shdlq);
    fflush(stdout);
#endif
    
    /* access handle queue */
    pthread_mutex_lock( &( nd->pp_mutex ) );
    nd->shdlq[qidx].pp_addr = ptr;
    nd->shdlq[qidx].pp_size = size; 
    nd->shdlq[qidx].pp_compsize = 0;  
    nd->shdlq[qidx].pp_type = PPSTREAM_WRITE;
    nd->shdlq[qidx].pp_status = PPSTREAM_START;
    nd->pp_shqtail++;
    pthread_cond_signal( &( nd->pp_cond ) );
    pthread_mutex_unlock( &( nd->pp_mutex ) );
    
    hdl->pp_id = id;
    hdl->nd = nd;
    hdl->pp_msize = 0;
    hdl->pp_hdltype = PPSTREAM_INPUT;

#ifdef DEBUG
    fprintf( stdout, "ppstream_input: fin.\n" );
    fflush( stdout );
#endif

    return hdl;
}

ppstream_handle_t *ppstream_output( ppstream_networkdescriptor_t *nd, void *ptr, size_t size ){
    
    ppstream_handle_t *hdl;
    uint64_t id, qidx;


#ifdef DEBUG
    fprintf( stdout, "ppstream_output: start.\n" );
    fflush( stdout );
#endif
    
    hdl = (ppstream_handle_t *)malloc( sizeof( ppstream_handle_t ) );
    if ( NULL == hdl ) {
        return NULL;
    }
    memset(hdl, 0, sizeof(ppstream_handle_t));
    
#ifdef DEBUG
    fprintf( stdout, "ppstream_output: qidx %" PRIu64 " size %zu rhdlq %p\n", qidx ,size, nd->rhdlq );
    fflush( stdout );
#endif
    
    /* access handle queue */
    pthread_mutex_lock( &( nd->pp_mutex ) );
    id = nd->pp_rhqtail;
    qidx = id % MAX_HANDLE_QSIZE;
    nd->rhdlq[qidx].pp_addr = ptr;
    nd->rhdlq[qidx].pp_size = size;
    nd->rhdlq[qidx].pp_compsize = 0;
    nd->rhdlq[qidx].pp_type = PPSTREAM_READ;
    nd->rhdlq[qidx].pp_status = PPSTREAM_START;
    nd->pp_rhqtail++;
    pthread_cond_signal( &( nd->pp_cond ) );
    pthread_mutex_unlock( &( nd->pp_mutex ) );
    
    hdl->pp_id = id;
    hdl->nd = nd;
    hdl->pp_msize = 0;
    hdl->pp_hdltype = PPSTREAM_OUTPUT;
#ifdef DEBUG
    fprintf( stdout, "ppstream_output: fin.\n" );
    fflush( stdout );
#endif
    
    return hdl;
}

int ppstream_test( ppstream_handle_t *hdl ){
    
    int rc = 0;

#ifdef DEBUG
    fprintf(stdout, "ppstream_test: start.\n");
    fflush(stdout);
#endif
    
    if ( NULL == hdl ) {
        rc = -1;
        goto exit;
    }
    
    /* get mutex */
    pthread_mutex_lock(&(hdl->nd->pp_mutex));
    
    if ( hdl->pp_hdltype == PPSTREAM_INPUT ) {
        if ( hdl->pp_id < hdl->nd->pp_shqhead ) {
            hdl->pp_msize =  hdl->nd->shdlq[hdl->pp_id].pp_compsize;
            rc = 0;
        }
        else {
            rc = 1;
        }
    }
    else if ( hdl->pp_hdltype == PPSTREAM_OUTPUT ) { 
        if ( hdl->pp_id < hdl->nd->pp_rhqhead ) {
            hdl->pp_msize =  hdl->nd->rhdlq[hdl->pp_id].pp_compsize;
            rc = 0;
        }
        else {
            rc = 1;
        }
    }
    
    /* release mutex */
    pthread_mutex_unlock( &(hdl->nd->pp_mutex ));

exit:

#ifdef DEBUG
    fprintf(stdout, "ppstream_test: fin. \n");
    fflush(stdout);
#endif

    return rc;
}

/* synchronization for nd */
void ppstream_sync(ppstream_networkdescriptor_t *nd){
    
    char dammy;
    
    dammy = 'x';
    
#ifdef DEBUG
    fprintf(stdout, "ppstream_sync: start.\n");
    fprintf(stdout, "ppstream_sync: nd->sock = %d.\n", nd->pp_sock);
    fflush(stdout);
#endif
    
    write(nd->pp_sock, &dammy, sizeof(dammy));
    recv(nd->pp_sock, &dammy, sizeof(dammy), MSG_WAITALL);

#ifdef DEBUG
    fprintf(stdout, "ppstream_sync: fin.\n");
    fflush(stdout);
#endif
    
    return;
}

ppstream_networkdescriptor_t *ppstream_open(ppstream_networkinfo_t *nt){
    
    /* socket descriptor */
    int sock, sock_accept;
    
    /* address length for accept */
    socklen_t addrlen;
    
    /* for getaddrinfo */
    struct addrinfo *res = NULL;
    struct addrinfo hints;
    struct addrinfo *ai = NULL;
    
    /* flag of server or client */
    int scflag;
    
    /* socket option */
    int on; 
    
    /* head and tail of handle queue */
    uint64_t hqtail = 0;
    uint64_t hqhead = 0;
    
    /* network descriptor */
    ppstream_networkdescriptor_t *nd;
    
    /* return code */
    int rc = 0;
    
    /* error number */
    int errno;
    
#ifdef DEBUG    
    fprintf(stdout, "ppstream_open: start.\n");
    fprintf(stdout, "ppstream_open: address %s port %s.\n", nt->pp_ipaddr, nt->pp_port);
    fflush(stdout);
#endif 
    
    /* set of network descriptor */
    /* malloc network descriptor */
    nd = (ppstream_networkdescriptor_t *)malloc(sizeof(ppstream_networkdescriptor_t));
    
    /* set ip address and port */
    nd->pp_ipaddr = nt->pp_ipaddr;
    nd->pp_port = nt->pp_port;
    nd->pp_sock = -1;
    nd->pp_socks = -1;
    nd->pp_set_timeout = nt->pp_set_timeout;
    nd->pp_set_cntimeout = nt->pp_set_cntimeout;
    nd->pp_set_segment = nt->pp_set_segment;
    nd->pp_set_nretry = nt->pp_set_nretry;
    
    /* set flag which is define server or client */
    nd->pp_scflag = nt->pp_scflag;
    
    if ( nt->pp_scflag != PPSTREAM_CLIENT && nt->pp_scflag != PPSTREAM_SERVER ) {
        fprintf(stderr, "ppstream_open: error occurs, because of not seting PPSTREAM_CLIENT or PPSTREAM_SERVER.\n");
        exit(1);
    }
    
    /* set Device flag ex) TCP or UDP, IB RC, IB UD etc. */
    nd->pp_devflag = nt->pp_devflag;
    /* finalization flag of comm. thread */
    nd->pp_finflag = 0;
    
    /* set initial of handle Q pointer */
    nd->pp_shqhead = hqhead;
    nd->pp_shqtail = hqtail;
    nd->shdlq = (ppstream_handlequeue_t *)malloc(sizeof(ppstream_handlequeue_t) * MAX_HANDLE_QSIZE);
    nd->pp_rhqhead = hqhead;
    nd->pp_rhqtail = hqtail;
    nd->rhdlq = (ppstream_handlequeue_t *)malloc(sizeof(ppstream_handlequeue_t) * MAX_HANDLE_QSIZE);
    
    /* clear send handle queue by 0 */
    memset((char *)nd->shdlq, 0, sizeof(ppstream_handlequeue_t) * MAX_HANDLE_QSIZE);
    /* clear recv handle queue by 0 */
    memset((char *)nd->rhdlq, 0, sizeof(ppstream_handlequeue_t) * MAX_HANDLE_QSIZE);
    
#ifdef DEBUG
    fprintf(stdout, "ppstream_open: server or client flagcheck section.\n");
    fflush(stdout);
#endif
    
    /* generate address infomaition */
    memset(&hints, 0, sizeof(hints));
    
    scflag = nd->pp_scflag;
    /* if server */
    if (scflag == PPSTREAM_SERVER) { 
#ifdef DEBUG
    fprintf(stdout, "ppstream_open: server.\n");
    fflush(stdout);
#endif
        /* set hints for getaddrinfo on server */
        memset(&hints, 0, sizeof(hints)); 
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        
        rc = getaddrinfo(nd->pp_ipaddr, nd->pp_port, &hints, &res);
        nd->pp_ai = res;
        
        /* generates socket for server and client */
        if ((sock = socket (nd->pp_ai->ai_family, nd->pp_ai->ai_socktype, nd->pp_ai->ai_protocol)) < 0) {
            perror("ppstream_open: server: socket() failed");
            rc = -1;
            goto exit;
        }
        
        /* set socket option that can reuse address and port. */
        rc = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (const void*)&on, sizeof(on) );    
        if (rc == -1) {
            fprintf(stderr, "ppstream_open: server: setsockopt() failed: rc %d.\n", rc);
            goto exit;
        }
        /* bind a socket. */
        while ( bind(sock, nd->pp_ai->ai_addr, nd->pp_ai->ai_addrlen) < 0 ) {
            if (errno != EADDRINUSE) {
                perror("ppstream_open: server: bind() failed");
                rc = -1;
                goto exit;
            }
        }
        
        /* listen a socket */
        if (listen(sock, 10) < 0) {
            perror("ppstream_open: server: listen() failed");
            rc = -1;
            goto exit;
        }
        
        addrlen = nd->pp_ai->ai_addrlen;
        while ((sock_accept = accept(sock, nd->pp_ai->ai_addr, &addrlen)) < 0) {
            perror("ppstream_open: server: accept() failed");
            goto exit;
        }
        /* set socket for network descriptor */
        nd->pp_socks = sock;
        nd->pp_sock = sock_accept;
    }
    /* if client */
    else if (scflag == PPSTREAM_CLIENT) {
#ifdef DEBUG
    fprintf(stdout, "ppstream_open: client.\n");
    fflush(stdout);
#endif
        /* set hints for getaddrinfo on server */
        memset(&hints, 0, sizeof(hints)); 
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        rc = getaddrinfo(nd->pp_ipaddr, nd->pp_port, &hints, &res);
        
        /* set address info form getaddrinfo */
        while ( 1 ) {
            for (ai = res; ai; ai = ai->ai_next) {
                /* generates socket for server and client */
                if ( ( sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol ) ) < 0 ) {
                    perror("ppstream_open: client: socket() failed");
                    rc = -1;
                    goto exit;
                }
                /* connect ai->pp_addr */
                if ( connect(sock, ai->ai_addr, ai->ai_addrlen) < 0 ) {
                    if (errno != EINTR && errno != EAGAIN && errno != ECONNREFUSED) {
                        perror("ppsream_open: client: connect() failed");
                        goto exit;
                    }
                }
                else {
                    nd->pp_sock = sock;
                    goto connect_success;
                }
                sleep(1);
            }
        }
    }
    else{
        fprintf(stderr, "ppstream_open: error occurs, because of not seting PPSTREAM_CLIENT or PPSTREAM_SERVER.\n");
        goto exit;
    }
    
connect_success:
    
    /* if client, free resources */
    if ( scflag == PPSTREAM_CLIENT ) {
        freeaddrinfo(res);
    }
    
    rc = pthread_cond_init(&(nd->pp_cond), NULL);
    pthread_mutex_init(&(nd->pp_mutex), NULL);
    
    /* initialize communication thread. */
    rc = pthread_create( (pthread_t *)&( nd->pp_comm_thread_id ), NULL, comm_thread_func, (void *)nd );
    nd->pp_connect_status = PPSTREAM_CONNECTED;
    //ppstream_sync( (ppstream_networkdescriptor_t *)nd );

#ifdef DEBUG
    fprintf( stdout, "ppstream_open: fin.\n");
    fflush( stdout );
#endif 

    return (ppstream_networkdescriptor_t *)nd;

exit:

#ifdef DEBUG
    fprintf( stdout, "ppstream_open: exit.\n");
    fflush( stdout );
#endif 
    
    return NULL;
}

void ppstream_close(ppstream_networkdescriptor_t *nd){

#ifdef DEBUG
    fprintf( stdout, "ppstream_close: start.\n");
    fflush( stdout );
#endif 
    
    /* set flag which comm. thread finish. */
    nd->pp_finflag = 1;
    
    /* kick comm. thread. */
    pthread_cond_signal(&(nd->pp_cond));
    /* wait comm. thread finish. */
    pthread_join(nd->pp_comm_thread_id, NULL);
    
    /* free socket for accept() and connect() */
    close( nd->pp_sock );
    if ( nd->pp_scflag == PPSTREAM_SERVER ) {
        /* free socket for server */
        close( nd->pp_socks );
    }
    if ( NULL != nd ) {
        free(nd);
    }

#ifdef DEBUG
    fprintf( stdout, "ppstream_close: fin.\n");
    fflush( stdout );
#endif 

    return ;
}

ppstream_networkinfo_t *ppstream_set_networkinfo(char *hostname, char *servname, uint32_t scflag, uint32_t devflag){

    ppstream_networkinfo_t *nt;

#ifdef DEBUG
    fprintf( stdout, "ppstream_set_networkinfo: start.\n");
    fflush( stdout );
#endif 
    
    /* allocates network info */
    nt = (ppstream_networkinfo_t *)malloc(sizeof(ppstream_networkinfo_t));
    if ( NULL == nt ) {
        return NULL;
    }

#ifdef DEBUG
    fprintf( stdout, "ppstream_set_networkinfo: hostname %s servname %s nt->pp_ipaddr %s np->pp_port %s.\n", hostname, servname, nt->pp_ipaddr, nt->pp_port );
    fflush( stdout );
#endif
    /* set hostname on network info */
    if ( hostname != NULL ) {
        nt->pp_ipaddr = strdup(hostname);
    }
    else {
        nt->pp_ipaddr = NULL;
    }
    
    /* set service name on network info */
    if ( servname != NULL ) {
        nt->pp_port = strdup(servname);
    }
    else {
        nt->pp_port = NULL;
    }
    
    /* set device flag */
    nt->pp_devflag = devflag;
    /* set server or client flag */
    nt->pp_scflag = scflag;
    
    nt->pp_set_timeout = PPSTREAM_DTIMEOUT;
    nt->pp_set_cntimeout = PPSTREAM_DCNTIMEOUT;
    nt->pp_set_segment = PPSTREAM_DSEGMENT;
    nt->pp_set_nretry = PPSTREAM_DNRETRY;
    
#ifdef DEBUG
    fprintf( stdout, "ppstream_set_networkinfo: hname %s sname %s ipaddr %s pp_port %s pp_set_timeout %f, pp_set_segment %zu, pp_set_nretry %d\n", 
             hostname, servname, nt->pp_ipaddr, nt->pp_port, nt->pp_set_timeout, nt->pp_set_segment, nt->pp_set_nretry );
    fprintf( stdout, "ppstream_set_networkinfo: fin.\n");
    fflush( stdout );
#endif 
  
    return nt;
}

void ppstream_free_networkinfo(ppstream_networkinfo_t *nt){
    
#ifdef DEBUG
    fprintf( stdout, "ppstream_free_networkinfo: start.\n");
    fflush( stdout );
#endif 

    if ( nt != NULL ) {
        free( nt );
    }

#ifdef DEBUG
    fprintf( stdout, "ppstream_free_networkinfo: fin.\n");
    fflush( stdout );
#endif 
    
    return;
}

void ppstream_set_cntimeout(ppstream_networkdescriptor_t *nd, double timeout) {
    
#ifdef DEBUG
    fprintf( stdout, "ppstream_set_cntimeout: start.\n");
    fflush( stdout );
#endif 
    
    nd->pp_set_cntimeout = timeout;
    
#ifdef DEBUG
    fprintf( stdout, "ppstream_set_cntimeout: fin.\n");
    fflush( stdout );
#endif 
    return;
}
