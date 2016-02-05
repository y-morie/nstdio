#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nstdio.h"

int main(int argc, char **argv){
    
    NET *nt;
    ND *nd;
    NHDL *hdl;
    char *str;
    
    if (argc != 2) {
        fprintf(stderr, "command error \n");
        fprintf(stderr, "%s port \n", argv[0]);
        exit(1);
    }
    
    nt = setnet(NULL, argv[1], NTCP);
    str = (char *)malloc(sizeof(char) * 256);
    
    printf("sv: start nopen \n");
    nd = nopen(nt, "s");
    printf("sv: finish nopen \n");
    //nsync(nd);
    printf("sv: start nread \n");
    hdl = nread(nd, str, 256);
    printf("sv: finish nread \n");
    
    printf("sv: start nquery \n");
    while(nquery(hdl));
    printf("sv: finish nquery \n");
    printf("sv: msize %" PRIu64 "\n", hdl->msize);
    printf("sv: get data  [%s]\n", str);
    
    printf("sv: start nclose \n");
    nclose(nd);
    printf("sv: finish nclose \n");
    
    freenet(nt);    
    printf("sv: finish\n");
    
    return 0;
}
