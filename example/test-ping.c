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
 
    if(argc == 4){
        nt = setnet(argv[1], atoi(argv[2]), NTCP);
        printf("cl start nopen \n");
        nd = nopen(nt, "w");
        printf("cl finish nopen \n");
        sprintf(str, "%s", argv[3]);
    }
    else if (argc == 3){
        nt = setnet(argv[1], atoi(argv[2]), NTCP);
        printf("sv start nopen \n");
        nd = nopen(nt, "r");
        printf("sv finish nopen \n");
    }
    else {
        fprintf(stderr, "command error\n");
        exit(1);
    }
    
    count = 4;
    for(i = 0; i < count ; i++){
        if (argc == 4){
            printf("cl start nwrite\n"); 
            sprintf(str, "%s_%d", str, i);
            hdl = nwrite(nd, str, 256);            
            while(nquery(hdl));
            printf("cl finish nquery\n"); 
          }        
        else if (argc == 3){
            printf("sv start nread\n");
            hdl = nread(nd, str, 256);
            while(nquery(hdl));
            printf("sv finish nquery\n"); 
            fprintf(stdout, "sv %s\n", str);
            printf("sv data recv\n");
        
        }
    }
    printf("start nclose \n");
    nclose(nd);
    printf("finish nclose \n");
    freenet(nt);    
    printf("finish\n");
    
    return 0;
}
