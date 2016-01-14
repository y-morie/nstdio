#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nstdio.h"

int main(int argc, char **argv){
    NET *nt;
    ND *nd;
    NHDL *hdl;
    char *str;
        
    nt = setnet(argv[1], atoi(argv[2]), NTCP);
    str = (char *)malloc(sizeof(char) * 256);
    
    printf("sv start nopen \n");
    nd = nopen(nt, "r");
    printf("sv finish nopen \n");
    nsync(nd);
    printf("sv start nread \n");
    hdl = nread(nd, str, 256);
    printf("sv finish nread \n");

    printf("sv start nquery \n");
    while(nquery(hdl));
    printf("sv finish nquery \n");
    printf("sv msize %lu\n", hdl->msize);
    printf("get data: %s\n", str);
    
    printf("sv start nclose \n");
    nclose(nd);
    printf("sv finish nclose \n");
 
    freenet(nt);    
    printf("sv finish\n");
    
    return 0;
}
