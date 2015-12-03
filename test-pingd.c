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
    //char str[256];
    //double str[DCOUNT];
    double *str;
    //exit(1);
    
    str = malloc(sizeof(SIZE));
    if(str == NULL){
        printf("malloc error\n");
        return 0;
    }
    nt.Dflag = TCP;
    nt.lip_addr = strdup(argv[1]);
    nt.lport = atoi(argv[2]);
    nt.rip_addr = strdup(argv[3]);
    nt.rport = atoi(argv[4]);
    printf("start nopen \n");
    nd = nopen(&nt);
    printf("finish nopen \n");
    
    for(i=0;i < DCOUNT;i++){
        str[i]=0;
    }
    count = 1;
    for(i = 0; i < count ; i++){
        //printf("sv start nwrite\n");
        hdl = nread(nd, str, SIZE);
        while(nquery(hdl));
        //fprintf(stdout, "%s\n", str);
        printf("sv finish nquery\n"); 
    }
    printf("start nclose \n");
    nclose(nd);
    printf("finish nclose \n");
    
    //for(i=0;i<DCOUNT;i++){
    printf("%f %f\n", str[0], str[DCOUNT-1]);
    //}
    printf("finish\n");
    
    return 0;
}
