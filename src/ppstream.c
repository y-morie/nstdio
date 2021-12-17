#include <stdio.h> // debug etc
#include <signal.h>
#include <stdlib.h> // malloc, free
#include <string.h> // strdup
#include <unistd.h> // close
#include <sys/socket.h>  // accept, bind
#include <sys/ioctl.h> // ioctl
#include <netinet/in.h> // ioctl
#include <netinet/tcp.h> // ioctl
#include <netdb.h> 
#include <errno.h> 
#include <inttypes.h> // 32/64bit for printf format
#include <sys/time.h> // gettimeofday 
#include <ppstream.h>

/* command type */
#define PPSTREAM_WRITE 1
#define PPSTREAM_READ  2

/* handle type */
#define PPSTREAM_INPUT  1
#define PPSTREAM_OUTPUT 2

/* handle status */
#define PPSTREAM_COMP     0
#define PPSTREAM_START    1
#define PPSTREAM_COMM     2
#define PPSTREAM_OVERREAD 3

/* set default environment value */
#define PPSTREAM_DTIMEOUT    8
#define PPSTREAM_DCNTIMEOUT 32
#define PPSTREAM_DSEGMENT 1024 * 1024 * 2

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

/* get segment size */

size_t get_segsize( ppstream_networkdescriptor_t *nd,
		    ppstream_handlequeue_t *hdlq ) {
    
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
    /* if # of segment is more than 2, set default */
    if ( rcount >= 1) {
        segment_size = set_segment;
    }
    /* otherwise, set rest size */
    else { 
        segment_size = rdata % set_segment;
    }
    
    return segment_size;
}

void check_comm_status( ppstream_networkdescriptor_t *nd,
			ppstream_handlequeue_t *hdlq, int flag , ssize_t rc) {
    
#ifdef DEBUG
    fprintf(stdout, "check_comm_status: start.\n");
    fflush(stdout);
#endif
    
    /* check send/recv header  */
    if ( rc == sizeof(size_t) && hdlq->pp_status ==  PPSTREAM_START ) {
	nd->pp_chtimeout_stime = gettimeofday_sec();
	/* if flag is read */
	if ( PPSTREAM_READ == flag ) {
	    /* if recvsize > sendsize, set sizesize */
	    if ( hdlq->pp_size >= hdlq->pp_sendsize ) {
		hdlq->pp_size = hdlq->pp_sendsize;
	    }
	}
	/* next status */
    	hdlq->pp_status = PPSTREAM_COMM;
    }
    else {
	if ( rc > 0 ) {
	    /* connet is green, reset start time of connection timeout. */
	    nd->pp_chtimeout_stime = gettimeofday_sec();
	    /* update complete message size. */
	    hdlq->pp_compsize += rc;
	    
	    if ( PPSTREAM_WRITE == flag ) {
		if ( hdlq->pp_compsize == hdlq->pp_size ) {
		    hdlq->pp_status = PPSTREAM_COMP;
		    nd->pp_shqhead++;
		}
		else {
		    hdlq->pp_status = PPSTREAM_COMM;
		}
	    }
	    else if ( PPSTREAM_READ == flag ) {
		if (hdlq->pp_size < hdlq->pp_sendsize ) {
		    if ( hdlq->pp_compsize >= hdlq->pp_size ) {
			hdlq->pp_status = PPSTREAM_OVERREAD;
		    }
		    else {
			hdlq->pp_status = PPSTREAM_COMM;
		    }
		    if ( hdlq->pp_compsize >= hdlq->pp_sendsize ) {
			hdlq->pp_status = PPSTREAM_COMP;
			nd->pp_rhqhead++;
		    }
		}		
		else {
		    if ( hdlq->pp_compsize == hdlq->pp_size ) {
			hdlq->pp_status = PPSTREAM_COMP;
			nd->pp_rhqhead++;
		    }
		    else {
			hdlq->pp_status = PPSTREAM_COMM;
		    }
		}
	    }
	}
	/* check whether connection is dead or not. */
	else {
	    if ( rc == 0 ) {
		if ( nd->pp_set_segment != 0 ) {
		    /*  disconnect on network description. */
		    nd->pp_connect_status = PPSTREAM_UNCONNECTED;
#ifdef DEBUG
		    fprintf(stdout, "check_comm_status: status unconnected rc %d.\n", rc);
		    fflush(stdout);
#endif
		}
	    }
	    if ( rc < 0  ) {
		/* Error occurs except stopping non blocking operation. */
		if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
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
    ssize_t rc;
    
    /* for message handle queue */
    uint64_t sqidx; /* index of send handle Q */
    uint64_t rqidx; /* index of recieve handle Q */ 
    ppstream_networkdescriptor_t *nd; /* pointer of network descriptor */
    
    /* socket file descriptoer for server */
    int socket_accept;
    
    /* measure the timeout */
    double st, et; 
    
    /* segment size */
    size_t segment_size, size;
    
    /* time for cond_timedwait  */
    struct timespec pctw_t;
    
    /* set network descriptor */
    nd = (ppstream_networkdescriptor_t *) arnd;
    
#ifdef DEBUG
    fprintf(stdout, "comm_thread_func: start.\n");
    fflush(stdout);
#endif
    /* if have SIGPIPE, ignore. */
    signal( SIGPIPE , SIG_IGN );
    
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
             nd->pp_shqtail - nd->pp_shqhead == 0 && 
	     nd->pp_rhqtail - nd->pp_rhqhead == 0 ) {
            pthread_cond_timedwait(&(nd->pp_cond), &(nd->pp_mutex), &pctw_t); 
        }
        nd->pp_chtimeout_stime = gettimeofday_sec();
        
        /* get index of each handl queue. */
        sqidx = nd->pp_shqhead % MAX_HANDLE_QSIZE;
        rqidx = nd->pp_rhqhead % MAX_HANDLE_QSIZE;
        
#ifdef DEBUG_CH_LOOP_CHK
        fprintf(stdout, "comm_thread_func: nd->shdlq[%" PRIu64 "] %p status %d\n",
		sqidx, & nd->shdlq[sqidx], nd->shdlq[sqidx].pp_status);
        fprintf(stdout, "comm_thread_func: nd->rhdlq[%" PRIu64 "] %p status %d\n",
		rqidx, & nd->rhdlq[rqidx], nd->rhdlq[rqidx].pp_status);
        fflush(stdout);
#endif
        /* if send/recv handle is NUll, exit */
        if ( &nd->shdlq[sqidx] == NULL ) {
            fprintf(stderr, "comm_thread_func: nd->shdlq[%" PRIu64 "] %p\n",
		    sqidx, & nd->shdlq[sqidx]);
	    goto exit;
        }
        if ( &nd->rhdlq[rqidx] == NULL ) {
            fprintf(stderr, "comm_thread_func; nd->rhdlq[%" PRIu64 "] %p\n",
		    rqidx, & nd->rhdlq[rqidx]);
            goto exit;
        }
	
	/* handle is READ */
        if ( nd->rhdlq[rqidx].pp_type == PPSTREAM_READ) {
	    switch (nd->rhdlq[rqidx].pp_status) {
	    case PPSTREAM_START:
		nd->pp_chtimeout_stime = gettimeofday_sec();
#ifdef DEBUG
		fprintf( stdout, "comm_thread_func: PPSTREAM_READ: rhd %" PRIu64 " shd_type %d \n",
			 nd->pp_rhqhead, nd->rhdlq[rqidx].pp_type );
		fflush( stdout );
#endif
		/* get segment size */
		size = -1;
		
		/* send data  */
		rc = ( ssize_t )recv( nd->pp_sock, &size, sizeof(size), MSG_DONTWAIT );
		
#ifdef DEBUG
		perror( "comm_thread_func: send return code" );
		fprintf( stdout, "comm_thread_func: PPSTREAM_READ: rhd %" PRIu64 " rc %zd\n",
			 nd->pp_rhqhead, rc );
		fprintf( stdout, "comm_thread_func: PPSTREAM_READ: request: size %zd rc %zd\n",
			 size, rc );
		fflush( stdout );
#endif
		if ( rc == sizeof(size) ) {
		    nd->rhdlq[rqidx].pp_sendsize = size;
		}
		
		/* check communication status */
		check_comm_status( nd, &(nd->rhdlq[rqidx]), PPSTREAM_READ, rc ); 
#ifdef DEBUG
		fprintf( stdout,
			 "comm_thread_func: PPSTREAM_READ: rhd %" PRIu64 " size %zd compsize %zu rc %zd\n", 
			 nd->pp_rhqhead, nd->rhdlq[rqidx].pp_size, nd->rhdlq[rqidx].pp_compsize, rc );
		fflush( stdout );
#endif
		break;
		
	    case PPSTREAM_COMM:
                nd->pp_chtimeout_stime = gettimeofday_sec();
#ifdef DEBUG
                fprintf(stdout, "comm_thread_func: PPSTREAM_READ: rhd %" PRIu64 " rhd_type %d\n",
			nd->pp_rhqhead,  nd->rhdlq[rqidx].pp_type);
                fflush(stdout);
#endif
                /* get segment size */
                segment_size = get_segsize ( nd, &(nd->rhdlq[rqidx]) );
		
                /* recv data  */
                rc = (ssize_t)recv(nd->pp_sock,
				   nd->rhdlq[rqidx].pp_addr + nd->rhdlq[rqidx].pp_compsize,
				   segment_size, MSG_DONTWAIT);
#ifdef DEBUG
                perror( "comm_thread_func: recv return code" );
                fprintf( stdout, "comm_thread_func: PPSTREAM_READ: rhd %" PRIu64 " rc %zd\n",
			 nd->pp_rhqhead,  rc );
#endif
                /* check communication status */
                check_comm_status( nd, &( nd->rhdlq[rqidx] ), PPSTREAM_READ, rc );
		
#ifdef DEBUG 
                fprintf( stdout,
			 "comm_thread_func: PPSTREAM_READ: rhd %" PRIu64 " size %zd compsize %zu rc %zd\n", 
                         nd->pp_rhqhead, nd->rhdlq[rqidx].pp_size, nd->rhdlq[rqidx].pp_compsize, rc );                
                fflush( stdout );
#endif            
		break;
	    case PPSTREAM_OVERREAD:
                nd->pp_chtimeout_stime = gettimeofday_sec();
#ifdef DEBUG
                fprintf( stdout,
			 "comm_thread_func: PPSTREAM_READ: PPSTREAM_OVERREAD: rhd %" PRIu64 " rhd_type %d\n",
			 nd->pp_rhqhead,  nd->rhdlq[rqidx].pp_type );
                fflush(stdout);
#endif
                /* get segment size */
                segment_size = get_segsize ( nd, &(nd->rhdlq[rqidx]) );
                
                /* recv data  */
                rc = (ssize_t)recv(nd->pp_sock, NULL, segment_size, MSG_DONTWAIT);
                
#ifdef DEBUG
                perror( "comm_thread_func: recv return code" );
                fprintf( stdout,
			 "comm_thread_func: PPSTREAM_READ: PPSTREAM_OVERREAD: rhd %" PRIu64 " rc %d\n",
			 nd->pp_rhqhead,  rc );
#endif
                /* check communication status */
                check_comm_status( nd, &( nd->rhdlq[rqidx] ), PPSTREAM_READ, rc );
#ifdef DEBUG 
                fprintf( stdout,
			 "comm_thread_func: PPSTREAM_READ: PPSTREAM_OVERREAD: rhd %" PRIu64 " size %zd compsize %zu rc %d\n",
			 nd->pp_rhqhead, nd->rhdlq[rqidx].pp_size, nd->rhdlq[rqidx].pp_compsize, rc );                
                fflush( stdout );
#endif            
		break;
	    default:
		break;
	    }
	}

	/* handle type is WRITE */
	if ( nd->shdlq[sqidx].pp_type == PPSTREAM_WRITE ) {
	    switch ( nd->shdlq[sqidx].pp_status ) {
	    case PPSTREAM_START:
                nd->pp_chtimeout_stime = gettimeofday_sec();
#ifdef DEBUG
                fprintf( stdout, "comm_thread_func: PPSTREAM_WRITE: PPSTREAM_START: shd %" PRIu64 " shd_type %d \n", 
			 nd->pp_shqhead, nd->shdlq[sqidx].pp_type );
                fflush( stdout );
#endif
                /* send size info  */
                size = nd->shdlq[sqidx].pp_size;
                
                /* send size info  */
		rc = ( ssize_t )send( nd->pp_sock, &size, sizeof(size), MSG_DONTWAIT );
#ifdef DEBUG
                perror( "comm_thread_func: send return code" );
                fprintf( stdout, "comm_thread_func: PPSTREAM_WRITE: shd %" PRIu64 " rc %zd\n",
			 nd->pp_shqhead, rc );
                fprintf( stdout, "comm_thread_func: PPSTREAM_WRITE: reqest: size %zd rc %zd \n",
			 size, rc );
                fflush( stdout );
#endif
		/* check communication status */
                check_comm_status( nd, &(nd->shdlq[sqidx]), PPSTREAM_WRITE, rc ); 
#ifdef DEBUG
                fprintf( stdout,
			 "comm_thread_func: PPSTREAM_WRITE: shd %" PRIu64 " size %zd compsize %zu rc %zd\n", 
                         nd->pp_shqhead, nd->shdlq[sqidx].pp_size, nd->shdlq[sqidx].pp_compsize, rc );
                fflush( stdout );
#endif
		break;
	    case PPSTREAM_COMM:
                nd->pp_chtimeout_stime = gettimeofday_sec();
#ifdef DEBUG
                fprintf( stdout, "comm_thread_func: PPSTREAM_WRITE: PPSTREAM_COMM shd %" PRIu64 " shd_type %d \n", 
			 nd->pp_shqhead, nd->shdlq[sqidx].pp_type );
                fflush( stdout );
#endif
                /* get segment size */
                segment_size = get_segsize ( nd, &(nd->shdlq[sqidx]) );
		
                /* send data  */
                rc = ( ssize_t )send( nd->pp_sock,
				      nd->shdlq[sqidx].pp_addr + nd->shdlq[sqidx].pp_compsize,
				      segment_size, MSG_DONTWAIT );

#ifdef DEBUG
                perror( "comm_thread_func: send return code" );
                fprintf( stdout, "comm_thread_func: PPSTREAM_WRITE: shd %" PRIu64 " rc %d\n", nd->pp_shqhead, rc );
                fflush( stdout );
#endif
		
                /* check communication status */
                check_comm_status( nd, &(nd->shdlq[sqidx]), PPSTREAM_WRITE, rc ); 
#ifdef DEBUG
                fprintf( stdout,
			 "comm_thread_func: PPSTREAM_WRITE: shd %" PRIu64 " size %"
			 PRIu64 " compsize %" PRIu64 " rc %d\n", 
                         nd->pp_shqhead, nd->shdlq[sqidx].pp_size, nd->shdlq[sqidx].pp_compsize, rc );
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
        
	/* check the timout of connection. */
        nd->pp_chtimeout_etime = gettimeofday_sec();
        if ( nd->pp_chtimeout_etime - nd->pp_chtimeout_stime > nd->pp_set_cntimeout ) {
            nd->pp_connect_status = PPSTREAM_UNCONNECTED;
        }
	
        /* if server and unconneted, close socket. */
        if ( nd->pp_scflag == PPSTREAM_SERVER && nd->pp_connect_status == PPSTREAM_UNCONNECTED ) {
	    int addrlen;
            int sock_accept;
            fd_set fds;
            struct timeval timeout;
            
#ifdef DEBUG
            fprintf( stdout, "comm_thread_func: server connect close.\n" );
            fflush( stdout );
#endif

	    FD_ZERO(&fds);
            FD_SET(nd->pp_socks, &fds);
            
            /* set timeout 1 sec */
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            /* check to be able to connetion or not */
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
    
    fprintf( stderr, "comm_thread_func: exception error occurs, and exit.\n" );
    exit(1);
}

ppstream_handle_t *ppstream_input( ppstream_networkdescriptor_t *nd,
				   void *ptr, size_t size){
    
    ppstream_handle_t *hdl;
    uint64_t id, qidx, hdlsize;
    
#ifdef DEBUG
    fprintf( stdout, "ppstream_input: start.\n" );
    fflush( stdout );
#endif

    if ( nd->pp_connect_status == PPSTREAM_UNCONNECTED ) {
        return NULL;
    }
    
    hdlsize = nd->pp_shqtail - nd->pp_shqhead;
    if ( hdlsize > MAX_HANDLE_QSIZE){
        fprintf(stderr, "The INPUT handle queue DO NOT have any space.");
        return NULL;
    }
    
    hdl = (ppstream_handle_t *)malloc(sizeof(ppstream_handle_t));
    if ( NULL == hdl ) {
        return NULL;
    }
    memset(hdl, 0, sizeof(ppstream_handle_t));
    id = nd->pp_shqtail;
    qidx = id % MAX_HANDLE_QSIZE;
    
#ifdef DEBUG
    fprintf(stdout, "ppstream_input: qidx %" PRIu64 " size %zd rhdlq %p\n", qidx ,size, nd->shdlq);
    fflush(stdout);
#endif
    
    /* access handle queue */
    pthread_mutex_lock( &( nd->pp_mutex ) );
    nd->shdlq[qidx].pp_addr = ptr;
    nd->shdlq[qidx].pp_size = size; 
    nd->shdlq[qidx].pp_compsize = 0;  
    nd->shdlq[qidx].pp_rsize = 0;  
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

ppstream_handle_t *ppstream_output( ppstream_networkdescriptor_t *nd,
				    void *ptr, size_t size ){
    
    ppstream_handle_t *hdl;
    uint64_t id, qidx, hdlsize;
    
#ifdef DEBUG
    fprintf( stdout, "ppstream_output: start.\n" );
    fflush( stdout );
#endif
    
    if ( nd->pp_connect_status == PPSTREAM_UNCONNECTED ) {
        return NULL;
    }
    hdlsize = nd->pp_rhqtail - nd->pp_rhqhead;
    if ( hdlsize > MAX_HANDLE_QSIZE){
        fprintf(stderr, "The OUTPUT handle queue DO NOT have any space.");
        return NULL;
    }
    
    hdl = (ppstream_handle_t *)malloc( sizeof( ppstream_handle_t ) );
    if ( NULL == hdl ) {
        return NULL;
    }
    memset(hdl, 0, sizeof(ppstream_handle_t));
    
    /* access handle queue */
    id = nd->pp_rhqtail;
    qidx = id % MAX_HANDLE_QSIZE;

#ifdef DEBUG
    fprintf( stdout, "ppstream_output: qidx %" PRIu64 " size %zd rhdlq %p\n", qidx ,size, nd->rhdlq );
    fflush( stdout );
#endif

    pthread_mutex_lock( &( nd->pp_mutex ) );
    nd->rhdlq[qidx].pp_addr = ptr;
    nd->rhdlq[qidx].pp_size = size;
    nd->rhdlq[qidx].pp_compsize = 0;
    nd->rhdlq[qidx].pp_rsize = 0;
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
    
    if ( hdl->nd->pp_connect_status == PPSTREAM_UNCONNECTED ) {
        rc = -2;
        goto exit;
    }
    
    /* get mutex */
    pthread_mutex_lock(&(hdl->nd->pp_mutex));
    
    if ( hdl->pp_hdltype == PPSTREAM_INPUT ) {
        if ( hdl->pp_id < hdl->nd->pp_shqhead ) {
            hdl->pp_msize =  hdl->nd->shdlq[hdl->pp_id % MAX_HANDLE_QSIZE].pp_compsize;
            rc = 0;
        }
        else {
            rc = 1;
        }
    }
    else if ( hdl->pp_hdltype == PPSTREAM_OUTPUT ) { 
        if ( hdl->pp_id < hdl->nd->pp_rhqhead ) {
            hdl->pp_msize =  hdl->nd->rhdlq[hdl->pp_id % MAX_HANDLE_QSIZE].pp_compsize;
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

ppstream_networkdescriptor_t *ppstream_open(ppstream_networkinfo_t *nt){
    
    /* socket descriptor */
    int sock, sock_accept, *sockary, maxsock;
    
    /* address length for accept */
    socklen_t addrlen;
    
    /* for getaddrinfo */
    struct addrinfo *res = NULL;
    struct addrinfo hints;
    struct addrinfo *ai = NULL;
    
    /* flag of server or client */
    int scflag;
    /* sock close flag */
    int *flag;
    
    /* socket option */
    int on; 
    
    /* head and tail of handle queue */
    uint64_t hqtail = 0;
    uint64_t hqhead = 0;
    
    /* network descriptor */
    ppstream_networkdescriptor_t *nd;

    /* timeout starttime and endtime */
    double st, et;
    
    /* return code */
    int rc = 0;
    
    /* error number */
    int errno;
    
    /* set timeout in connect */
    fd_set rfds;
    struct timeval timeout;
    int rselct = 0;
    ssize_t rrcv = 0;
    int dummy = 0;
    int iai = 0, nai, ai_id;
    
    signal( SIGPIPE , SIG_IGN );
    
#ifdef DEBUG    
    fprintf(stdout, "ppstream_open: start.\n");
    fprintf(stdout, "ppstream_open: address %s port %s.\n",
	    nt->pp_ipaddr, nt->pp_port);
    fflush(stdout);
#endif 
    
    /* set of network descriptor */
    /* malloc network descriptor */
    nd = (ppstream_networkdescriptor_t *)malloc(sizeof(ppstream_networkdescriptor_t));
    memset( nd, 0, sizeof(ppstream_networkdescriptor_t) );
    
    /* set ip address and port */
    nd->pp_ipaddr = nt->pp_ipaddr;
    nd->pp_port = nt->pp_port;
    nd->pp_mode = nt->pp_mode;
    nd->pp_sock = -1;
    nd->pp_socks = -1;
    nd->pp_set_timeout = nt->pp_set_timeout;
    nd->pp_set_cntimeout = nt->pp_set_cntimeout;
    nd->pp_set_segment = nt->pp_set_segment;
    
    /* set flag which is define server or client */
    nd->pp_scflag = nt->pp_scflag;
    
    if ( nt->pp_scflag != PPSTREAM_CLIENT && nt->pp_scflag != PPSTREAM_SERVER ) {
        fprintf(stderr,
		"ppstream_open: error occurs, because of not seting PPSTREAM_CLIENT or PPSTREAM_SERVER.\n");
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
        
        /* generates socket for server */
        if ((sock = socket (nd->pp_ai->ai_family, nd->pp_ai->ai_socktype, nd->pp_ai->ai_protocol)) < 0) {
            perror("ppstream_open: server: socket() failed");
            rc = -1;
            goto exit;
        }
        
        /* set socket option that can reuse address and port. */
	on = 1;
	rc = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (const void*)&on, sizeof(on) );    
        if (rc == -1) {
            fprintf(stderr, "ppstream_open: server: setsockopt() failed: rc %d.\n", rc);
            goto exit;
        }
	
	on = 1;
	rc = setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (const void*)&on, sizeof(on) );    
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
	
	/* accpet a socket */
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
        /* set hints for getaddrinfo on client */
	memset(&hints, 0, sizeof(hints)); 
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        rc = getaddrinfo(nd->pp_ipaddr, nd->pp_port, &hints, &res);
	
	nai = 0;
	for ( ai = res; ai; ai = ai->ai_next ) {
	    nai++;
	}
	
#ifdef DEBUG
	printf("nai %d\n", nai);
	fflush(stdout);
#endif
	sockary = (int *)malloc( sizeof(int) * nai );
	if ( sockary == NULL ) {
	    fprintf(stderr, "ppstream_open: client: malloc error.\n");
	    rc = -1;
	    goto exit;
	}
	memset(sockary, 0, sizeof(int) * nai);
	flag = (int *) malloc ( sizeof(int) * nai );
	if ( flag == NULL ) {
	    fprintf(stderr, "ppstream_open: client: malloc error.\n");
	    rc = -1;
	    goto exit;
	}
	memset(flag, 0, sizeof(int) * nai);

       	st = gettimeofday_sec();
	while ( 1 ) {
	    et = gettimeofday_sec();
	    if ( et - st > nd->pp_set_cntimeout ) {
#ifdef DEBUG
		fprintf(stdout, "ppstream_open: client: timeout.\n");
		fflush( stdout );
#endif
		for ( iai = 0; iai < nai; iai++ ) {
		    close( sockary[iai] );
		}
		goto exit;
	    }
	    iai = 0;
	    /* set address info form getaddrinfo */
	    for ( ai = res; ai; ai = ai->ai_next ) {
	        /* generates socket for server and client */
	        if ( flag[iai] == 0 ) {
#ifdef DEBUG
		    printf("create socket\n");
		    fflush(stdout);
#endif
		    if ( ( sockary[iai] = socket( ai->ai_family, ai->ai_socktype, ai->ai_protocol ) ) < 0 ) {
			perror("ppstream_open: client: socket() failed");
			rc = -1;
			goto exit;
		    }
		    on = 1;
		    rc = setsockopt( sockary[iai], IPPROTO_TCP, TCP_NODELAY, (const void*)&on, sizeof(on) );    
		    if ( rc == -1 ) {
			fprintf(stderr, "ppstream_open: client: setsockopt() failed: rc %d.\n", rc);
			goto exit;
		    }
		    
		    flag[iai] = 1;
		    iai++;
		}
	    }

	    /* make socket non-blocking */
	    on = 1;
	    for( iai = 0; iai < nai; iai++ ) {
		rc = ioctl(sockary[iai], FIONBIO, &on);
		if ( rc != 0 ) {
		    fprintf(stderr, "ppstream_open: client: ioctl() failed: rc %d.\n", rc);
		    goto exit;
		}
	    }
	    
	    /* connect ai->pp_addr */
	    iai = 0;
	    for ( ai = res; ai; ai = ai->ai_next ) {
	        if ( connect(sockary[iai], ai->ai_addr, ai->ai_addrlen) < 0 ) {
		    if ( errno == EISCONN ) {
			nd->pp_sock = sockary[iai];
			ai_id = iai;
#ifdef DEBUG		      
			perror("0 connect()");
#endif
			goto connect_success;
		    }
		    else {
		        if ( errno != EINPROGRESS ) {
			    if ( errno != EINTR && errno != EAGAIN
				 && errno != ECONNREFUSED && errno != ECONNABORTED ) {
#ifdef DEBUG
			        perror("ppsream_open: client: connect() failed");
#endif
				goto exit;
			    }
			    else {
#ifdef DEBUG
			        perror("2 connect()");
#endif
			        sleep(1);
			    }
			}
			else {
#ifdef DEBUG
			    perror("3 connect()");
#endif
			}
		    }
		}
		else {
#ifdef DEBUG
		    perror("4 connect()");
#endif
		    nd->pp_sock = sockary[iai];
		    ai_id = iai;
		    goto connect_success;
		}
		iai++;
	    }
	    
	    /* set a file descriptor */
	    FD_ZERO(&rfds);
	    for( iai = 0; iai < nai; iai++ ) {
		FD_SET(sockary[iai], &rfds);
		if ( FD_ISSET( sockary[iai], &rfds ) ) {
#ifdef DEBUG
		    printf("sockary %d in rfds\n", sockary[iai]);
		    fflush(stdout);
#endif
		}
	    }
	    
	    maxsock = -1;
	    for( iai = 0; iai < nai; iai++ ) {
		if ( maxsock < sockary[iai] ) {
		    maxsock = sockary[iai];
#ifdef DEBUG
		    printf("sockary %d\n", sockary[iai]);
		    fflush(stdout);
#endif
		}
	    }
#ifdef DEBUG
	    printf("maxsock %d\n", maxsock);
	    fflush(stdout);
#endif
	    /* set timeout 1 sec */
	    timeout.tv_sec = 1;
	    timeout.tv_usec = 0;
	    
	    if ( ( rselct = select(maxsock + 1, &rfds, &rfds, NULL, &timeout) ) >= 0 ) {
#ifdef DEBUG
		printf("ppstream_open: cl: select: ret %d\n", rselct);
		fflush(stdout);
#endif
		for( iai = 0; iai < nai; iai++ ) {
		    if ( FD_ISSET( sockary[iai], &rfds ) ) {
		        rrcv = recv(sockary[iai], &dummy, sizeof(dummy), MSG_DONTWAIT);
#ifdef DEBUG
			perror("ppstream_open: recv retrun code");
			printf("ppstream_open: iai %d cl: recv: ret %zd\n", iai, rrcv);
			fflush(stdout);
#endif
			if ( rrcv == -1 ) {
			    if  ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			        nd->pp_sock = sockary[iai];
				ai_id = iai;
#ifdef DEBUG
				printf("ppstream_open: cl: recv: ret %zd\n", rrcv);
				fflush(stdout);
#endif
				goto connect_success;
			    }
			    else {
				flag[iai] = 0;
				close(sockary[iai]);
#ifdef DEBUG
				printf("socket close and set flag %d %d\n", sockary[iai], flag[iai]);
				fflush(stdout);
#endif
			    }
			}
		    }
		}
		sleep(1);
	    }
       	    else {
#ifdef DEBUG
		printf("select time out\n");
		fflush(stdout);
#endif
	    }
	}
    }
    else {
	fprintf( stderr,
		 "ppstream_open: error occurs, "
		 "because of not seting PPSTREAM_CLIENT or PPSTREAM_SERVER. \n" );
	goto exit;
    }
    
 connect_success:
    
    /* if client, free resources */
    if ( scflag == PPSTREAM_CLIENT ) {
	for ( iai = 0; iai < nai; iai++ ) {
	    if ( iai != ai_id ) {
		close(sockary[iai]);
	    }
	}
        freeaddrinfo( res );
	free( sockary );
	free( flag );
    }
    
    /* initialize variables of the communication thread. */
    rc = pthread_cond_init(&(nd->pp_cond), NULL);
    pthread_mutex_init(&(nd->pp_mutex), NULL);
    
    /* create communication thread */
    rc = pthread_create( (pthread_t *)&( nd->pp_comm_thread_id ), NULL, comm_thread_func, (void *)nd );
    nd->pp_connect_status = PPSTREAM_CONNECTED;
    
#ifdef DEBUG
    fprintf( stdout, "ppstream_open: fin.\n");
    fflush( stdout );
#endif 
    
    return (ppstream_networkdescriptor_t *)nd;
    
 exit:
    
#ifdef DEBUG
    fprintf( stdout, "ppstream_open: failed open ppstream: exit.\n");
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
    /* free the network descriptor */
    if ( NULL != nd ) {
        free ( nd );
    }
    
#ifdef DEBUG
    fprintf( stdout, "ppstream_close: fin.\n");
    fflush( stdout );
#endif 

    return ;
}


/* synchronization for nd */
void ppstream_sync( ppstream_networkdescriptor_t *nd ){
    
    char dummy;
    ppstream_handle_t *ihdl, *ohdl;
    
    dummy = 'x';
    
#ifdef DEBUG
    fprintf(stdout, "ppstream_sync: start.\n");
    fprintf(stdout, "ppstream_sync: nd->sock = %d.\n", nd->pp_sock);
    fflush(stdout);
#endif
    
    ihdl = ppstream_input(nd, &dummy, sizeof(dummy));
    ohdl = ppstream_output(nd, &dummy, sizeof(dummy));
    while( ppstream_test(ihdl) );
    while( ppstream_test(ohdl) );
    
#ifdef DEBUG
    fprintf(stdout, "ppstream_sync: fin.\n");
    fflush(stdout);
#endif
    
    free(ihdl);
    free(ohdl);
    
    return;
}


ppstream_networkinfo_t *ppstream_set_networkinfo(char *hostname,
						 char *servname,
						 uint32_t scflag,
						 uint32_t devflag){

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
    memset( nt, 0, sizeof(ppstream_networkinfo_t) );
#ifdef DEBUG
    fprintf( stdout, 
	     "ppstream_set_networkinfo: "
	     "hostname %s servname %s nt->pp_ipaddr %s nt->pp_port %s.\n",
	     hostname, servname, nt->pp_ipaddr, nt->pp_port );
    fflush( stdout );
#endif
    
    /* set hostname on network info */
    if ( hostname != NULL ) {
        nt->pp_ipaddr = strdup( hostname );
    }
    else {
        nt->pp_ipaddr = NULL;
    }
    
    /* set service name on network info */
    if ( servname != NULL ) {
        nt->pp_port = strdup( servname );
    }
    else {
        nt->pp_port = NULL;
    }
    
    /* set device flag */
    nt->pp_devflag = devflag;
    /* set server or client flag */
    nt->pp_scflag = scflag;
    
    /* set default value. */
    nt->pp_set_timeout = PPSTREAM_DTIMEOUT;
    nt->pp_set_cntimeout = PPSTREAM_DCNTIMEOUT;
    nt->pp_set_segment = PPSTREAM_DSEGMENT;
    
#ifdef DEBUG
    fprintf( stdout, 
	     "ppstream_set_networkinfo: "
	     "hname %s sname %s ipaddr %s pp_port %s pp_set_timeout %f pp_set_segment %zu\n", 
             hostname, servname, nt->pp_ipaddr, nt->pp_port, nt->pp_set_timeout, nt->pp_set_segment );
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

void ppstream_set_cntimeout(ppstream_networkdescriptor_t *nd,
			    double timeout) {
    
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
