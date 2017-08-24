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

void nsync(ND *nd){
    
    ppstream_sync(nd);
    
    return;
}

NHDL *nwrite( ND *nd, void *addr, size_t size){
    
    NHDL *hdl;
    
    if (nd->pp_mode == PPSTREAM_MODE_RO) {
        fprintf(stderr, "ND is read only mode");
        return NULL;
    }

    hdl = (NHDL * )ppstream_input(nd, addr, size);
    
    return hdl;
}

NHDL *nread( ND *nd, void *addr, size_t size){
    
    NHDL *hdl;
    
    if (nd->pp_mode == PPSTREAM_MODE_WO){
        fprintf(stderr, "ND is write only mode");
        return NULL;
    }
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
    
    if (strcmp(mode, "w") == 0) {
        nt->pp_scflag = PPSTREAM_CLIENT;
	nt->pp_mode = PPSTREAM_MODE_WO;
    }
    else if (strcmp(mode, "c") == 0) {
      nt->pp_scflag = PPSTREAM_CLIENT;
      nt->pp_mode = PPSTREAM_MODE_WR;
    }
    else if (strcmp(mode, "r") == 0) {
        nt->pp_scflag = PPSTREAM_SERVER;
	nt->pp_mode = PPSTREAM_MODE_RO;
    }
    else if (strcmp(mode, "s") == 0) {
        nt->pp_scflag = PPSTREAM_SERVER;
	nt->pp_mode = PPSTREAM_MODE_WR;
    }
    else {
        fprintf(stderr, "error: mode of nopen.");
    }
    nd = ppstream_open(nt);
    
    return (ND *)nd;
}

void nclose(ND *nd) {
    
    ppstream_close(nd);

    return ;
}

NET *setnet(char *ipaddr, char* port, uint32_t Dflag) {

    NET *nt;

    nt = ppstream_set_networkinfo(ipaddr, port , UNDEFINE_SCFLAG , Dflag);

    return nt;
}

void freenet(NET *nt) {
    
    ppstream_free_networkinfo(nt);
    
    return;
}

void  settimeout(ND *nd, double timeout) {

    ppstream_set_cntimeout(nd, timeout);
    
    return;
}
