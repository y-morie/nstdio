#include <stdio.h>
#include <string.h>
#include <unistd.h> // sleep
#include <stdlib.h>
#include "nstdio.h"

#define DCOUNT 1024LLU * 1024LLU * 128LLU
#define SIZE (DCOUNT * 8LLU)

int main(int argc, char **argv) {
    NET nt;
    ND *nd;
    NHDL *hdl;
    uint64_t i;
    int count;
    double *str;
    
    str = (double *)malloc(SIZE);
    if(str == NULL){
        printf("malloc error\n");
        return 0;
    }
    nt.Dflag = TCP;
    nt.lip_addr = strdup(argv[1]);
    nt.lport = atoi(argv[2]);
    nt.rip_addr = strdup(argv[3]);
    nt.rport = atoi(argv[4]);
    
    for(i = 0; i < DCOUNT; i++){
        str[i] = (double)i;
    }
    printf("start nopen \n");
    nd = nopen(&nt);
    printf("finish nopen \n");
    
    count = 1;     
    for(i = 0; i < count ; i++){
        printf("cl start nwrite\n"); 
        hdl = nwrite(nd, str, SIZE);
        printf("cl finish nwrite\n");
        printf("cl start nquery\n");  
        while(nquery(hdl));
        printf("cl finish nquery\n");
    }
    printf("start nclose \n");
    nclose(nd);
    printf("finish nclose \n");
    
    printf("finish\n");
    
    return 0;
}
