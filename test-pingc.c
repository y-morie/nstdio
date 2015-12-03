#include <stdio.h>
#include <string.h>
#include <unistd.h> // sleep
#include <stdlib.h>
#include "nstdio.h"

#define DCOUNT 1024LLU * 1024LLU * 256LLU
#define SIZE (DCOUNT * 8LLU)

int main(int argc, char **argv) {
    NET nt;
    ND *nd;
    HNDL *hdl;
    int i;
    int count;
    //char str[SIZE];
    double *str;
    
    str = malloc(sizeof(double) * SIZE);
    if(str == NULL){
        printf("malloc error\n");
        return 0;
    }
    nt.Dflag = TCP;
    nt.lip_addr = strdup(argv[1]);
    nt.lport = atoi(argv[2]);
    nt.rip_addr = strdup(argv[3]);
    nt.rport = atoi(argv[4]);
    //sprintf(str, "%s", argv[5]);
    //fprintf(stdout, "%s\n", str);

    for(i=0;i<DCOUNT;i++){
        str[i]=i;
    }
    printf("start nopen \n");
    nd = nopen(&nt);
    printf("finish nopen \n");
    
    count = 4;     
    for(i = 0; i < count ; i++){
        printf("cl start nwrite\n"); 
        //sprintf(str, "%s_%d", str, i);
        hdl = nwrite(nd, str, SIZE);
        while(nquery(hdl));
        printf("cl data recv\n");
    }
    printf("start nclose \n");
    nclose(nd);
    printf("finish nclose \n");
    
    printf("finish\n");
    
    return 0;
}
