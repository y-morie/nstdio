#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h> // strdup
#include <unistd.h> // close
#include <sys/socket.h>  // accept, bind
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // inet_addr
#include <errno.h>
#include <nstdio.h>

#define UNDEFINE_SCFLAG  0

NHDL *nwrite( ND *nd, void *addr, size_t size){
    
    NHDL *hdl;
    
    hdl = (NHDL * )ppstream_input(nd, addr, size);
    
    return hdl;
}

NHDL *nread( ND *nd, void *addr, size_t size){
    
    NHDL *hdl;
    
    hdl = ppstream_output(nd, addr, size);
    
    return hdl;
}

int nquery(NHDL *hdl){

    int rc;
    
    rc = ppstream_test(hdl);

    return rc;
}

ND *nopen(NET *nt, char *mode){
    
    ND *nd;
    int scflag;
    
    int rc = 0;
    int errno;
    
    
    if(strcmp(mode, "w") == 0){
        nt->scflag = PPSTREAM_CLIENT;
    }
    else if(strcmp(mode, "r") == 0){
        nt->scflag = PPSTREAM_SERVER;
    }
    else{
        fprintf(stderr, "error: mode of nopen.");
    }
    nd = ppstream_open(nt);
    
    return (ND *)nd;
}

void nclose(ND *nd){
    
    ppstream_close(nd);

    return ;
}

NET *setnet(char *ipaddr, uint16_t port, uint32_t Dflag){

    NET *nt;

    nt = ppstream_set_networkinfo(ipaddr, port , UNDEFINE_SCFLAG , Dflag);

    return nt;
}

void freenet(NET *nt){
    
    ppstream_free_networkinfo(nt);
    
    return;
}

