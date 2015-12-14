#include <stdio.h>
#include <string.h>
#include <unistd.h> // sleep
#include <stdlib.h>
#include "nstdio.h"


int main(int argc, char **argv) {
    NET *nt;
    ND *nd;
    NHDL *hdl;
    int i;
    int count;
    char str[256];
 
    if(argc == 3){
        nt = setnet(argv[1], atoi(argv[2]), NTCP);
        printf("cl start nopen \n");
        nd = nopen(nt, "r");
        printf("cl finish nopen \n");
    }
    else if (argc == 4){
        nt = setnet(argv[1], atoi(argv[2]), NTCP);
        printf("sv start nopen \n");
        nd = nopen(nt, "w");
        printf("sv finish nopen \n");
        sprintf(str, "%s", argv[3]);
    }
    else {
        fprintf(stderr, "command error\n");
        exit(1);
    }
    
    count = 4;
    for(i = 0; i < count ; i++){
        if (argc == 3){
            printf("cl start nread\n"); 
            hdl = nread(nd, str, 256);            
            while(nquery(hdl));
            printf("cl finish nquery\n"); 
            fprintf(stdout, "%s\n", str);
            printf("cl data recv\n");
        }        
        else if (argc == 4){
            printf("sv start nwrite\n");
            sprintf(str, "%s_%d", str, i);
            hdl = nwrite(nd, str, 256);
            while(nquery(hdl));
            printf("sv finish nquery\n"); 
        }
    }
    printf("start nclose \n");
    nclose(nd);
    printf("finish nclose \n");
    freenet(nt);    
    printf("finish\n");
    
    return 0;
}
