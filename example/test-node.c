#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nstdio.h"

int main(int argc, char **argv){
    
    NET *nt;
    ND *nd;
    NHDL *hdl;
    
    /* chars data  */
    char *str;
    
    if (argc != 4) {
        fprintf(stderr, "command error \n");
        fprintf(stderr, "%s [hostname/IP address] port_node [send chars]\n", argv[0]);
        exit(1);
    }
    nt = setnet(argv[1], argv[2], NTCP);
    printf("node: send chars [%s]\n", argv[3]);
    
    printf("node: start nopen \n");
    nd = nopen(nt, "c");
    printf("node: finish nopen \n");
    //nsync(nd);
    printf("node: start nwrite \n");
    hdl = nwrite(nd, argv[3], 256);
    printf("node: finish nwrite \n");
    
    printf("node: start nquery \n");
    while (nquery(hdl));
    printf("node: finish nquery \n");
    
    printf("node: start nclose \n");
    nclose(nd);
    printf("node: finish nclose \n");
    
    freenet(nt);
    printf("node: finish\n");
    
    return 0;
}

