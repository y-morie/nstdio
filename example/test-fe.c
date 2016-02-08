#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nstdio.h"

int main(int argc, char **argv){
    
    NET *nt1, *nt2;
    ND *nd1, *nd2;
    NHDL *hdl;
    
    /* chars data  */
    char *str;
    
    if (argc != 3) {
        fprintf(stderr, "command error \n");
        fprintf(stderr, "%s  port_node port_pc \n", argv[0]);
        exit(1);
    }
    nt1 = setnet(NULL, argv[1], NTCP);
    nt2 = setnet(NULL, argv[2], NTCP);
    str = (char *)malloc(sizeof(char) * 256);

    printf("fe: start nopen nt2.\n");
    nd2 = nopen(nt2, "s");
    printf("fe: finish nopen nt2.\n");
    //nsync(nd);

    printf("fe: start nopen nt1.\n");
    nd1 = nopen(nt1, "s");
    printf("fe: finish nopen nt1.\n");
    
    printf("fe: start nread nd1.\n");
    hdl = nread(nd1, str, 256);
    printf("fe: finish nread nd1.\n");
    printf("fe: start nquery nd1 handle.\n");
    while (nquery(hdl));
    printf("fe: finish nquery nd1 handle.\n");
    
    printf("fe: start nwrite nd2. \n");
    hdl = nwrite(nd2, str, 256);
    printf("fe: finish nwrite nd2. \n");
    printf("fe: start nquery nd2 handle. \n");
    while (nquery(hdl));
    printf("fe: finish nquery nd2 handle. \n");
    
    printf("fe: start nclose \n");
    nclose(nd1);
    nclose(nd2);
    printf("fe: finish nclose \n");
    
    freenet(nt1);
    freenet(nt2);
    printf("fe: finish\n");
    
    return 0;
}

