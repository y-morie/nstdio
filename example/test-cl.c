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
    str = malloc(sizeof(char)*256);
    printf("cl start nopen \n");
    nd = nopen(nt, "r");
    printf("cl finish nopen \n");
    
    printf("cl start nread \n");
    hdl = nread(nd, str, 256);
    printf("cl finish nread \n");

    printf("cl start nquery \n");
    while(nquery(hdl));
    printf("cl finish nquery \n");
    printf("get data: %s\n", str);
    
    printf("cl start nclose \n");
    nclose(nd);
    printf("cl finish nclose \n");
    freenet(nt);
    printf("cl finish\n");
    
    return 0;
}

