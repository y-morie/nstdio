#include <stdio.h>
#include <string.h>
#include <unistd.h> // sleep
#include <stdlib.h>
#include "nstdio.h"


#define DCOUNT 1024LLU * 1024LLU * 128LLU
#define SIZE (DCOUNT * 8LLU)

int main(int argc, char **argv) {
    NET *nt;
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

    nt = setnet(argv[1], atoi(argv[2]), NTCP);
    printf("sv start nopen \n");
    nd = nopen(nt, "r");
    printf("sv finish nopen \n");
        
    for(i = 0; i < DCOUNT;i++){
        str[i] = 0.0;
    }
    count = 1;
    for(i = 0; i < count ; i++){
        printf("sv start nread\n");
        hdl = nread(nd, str, SIZE);
        printf("sv finish nread\n"); 
        printf("sv start nquery\n"); 
        while(nquery(hdl));
        printf("sv finish nquery\n"); 
    }

    printf("sv %lf %lf\n", str[0], str[DCOUNT-1]);
    
    printf("sv start nclose \n");
    nclose(nd);
    printf("sv finish nclose \n");

#if 0
    for(i= 256 * 1024 * 1024 - 1024 ;i <  256 * 1024 * 1024;i++){
        printf("%lf\n", str[i]);
    }
#endif

    freenet(nt);
    printf("sv finish\n");

    return 0;
}
