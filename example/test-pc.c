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
    
    if (argc != 3) {
        fprintf(stderr, "command error \n");
        fprintf(stderr, "%s [hostname/IP address] port [send chars]\n", argv[0]);
        exit(1);
    }
    nt = setnet(argv[1], argv[2], NTCP);
    str = (char *)malloc(sizeof(char) * 256);
    
    printf("pc: start nopen nt1.\n");
    nd = nopen(nt, "c");
    printf("pc: finish nopen nt1.\n");
    
    //nsync(nd);
    printf("pc: start nread.\n");
    hdl = nread(nd, str, 256);
    printf("pc: finish nread.\n");
    printf("pc: start nquery nd2 handle.\n");
    while (nquery(hdl));
    printf("pc: finish nquery nd2 handle.\n");
    
    fprintf(stdout, "pc: %s\n", str); 
    
    printf("pc: start nclose \n");
    nclose(nd);
    printf("pc: finish nclose \n");
    
    freenet(nt);
    printf("pc: finish\n");
    
    return 0;
}

