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
    
    printf("fe: start nopen for pc.\n");
    nd2 = nopen(nt2, "s");
    if ( NULL == nd2 ) {
      printf("fe: nopen failed.\n");
      exit(1);
    }
    printf("fe: finish nopen for pc.\n");
    //nsync(nd);
    
    printf("fe: start nopen for node.\n");
    nd1 = nopen(nt1, "s");
    if ( NULL == nd1 ) {
      printf("fe: nopen failed.\n");
      exit(1);
    }
    printf("fe: finish nopen for node\n");
    
    printf("fe: start nread from node.\n");
    hdl = nread(nd1, str, 256);
    printf("fe: finish nread from node.\n");
    printf("fe: start nquery handle for node.\n");
    while (nquery(hdl));
    printf("fe: finish nquery handle for node.\n");
    
    printf("fe: start nwrite to pc. \n");
    hdl = nwrite(nd2, str, 256);
    printf("fe: finish nwrite to pc. \n");
    printf("fe: start nquery handle for pc. \n");
    while (nquery(hdl));
    printf("fe: finish nquery handle for pc. \n");
    
    printf("fe: start nclose \n");
    nclose(nd1);
    nclose(nd2);
    printf("fe: finish nclose \n");
    
    freenet(nt1);
    freenet(nt2);
    printf("fe: finish\n");
    
    return 0;
}

