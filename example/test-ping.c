#include <stdio.h>
#include <string.h>
#include <unistd.h> // sleep
#include <stdlib.h>
#include "nstdio.h"


int main(int argc, char **argv) {
    NET nt;
    ND *nd;
    NHDL *hdl;
    int i;
    int count;
    char str[256];
 
    nt.Dflag = TCP;
    if(argc == 6){
        nt.lip_addr = strdup(argv[1]);
        nt.lport = atoi(argv[2]);
        nt.rip_addr = strdup(argv[3]);
        nt.rport = atoi(argv[4]);
        sprintf(str, "%s", argv[5]);
        fprintf(stdout, "%s\n", str);
        printf("start nopen \n");
        nd = nopen(&nt);
        printf("finish nopen \n");
    }
    else if (argc == 5){
        nt.lip_addr = strdup(argv[1]);
        nt.lport = atoi(argv[2]);
        nt.rip_addr = strdup(argv[3]);
        nt.rport = atoi(argv[4]);
        printf("start nopen \n");
        nd = nopen(&nt);
        printf("finish nopen \n");
    }
    else {
        fprintf(stderr, "command error\n");
        exit(1);
    }
    count = 4;
         
    for(i = 0; i < count ; i++){
        if (argc == 6){
            printf("cl start nwrite\n"); 
            sprintf(str, "%s_%d", str, i);
            hdl = nwrite(nd, str, 256);
            while(nquery(hdl));
            printf("cl data recv\n");
        }        
        else if (argc == 5){
            printf("sv start nread\n");
            hdl = nread(nd, str, 256);
            while(nquery(hdl));
            fprintf(stdout, "%s\n", str);
            printf("sv finish nquery\n"); 
        }
    }
    printf("start nclose \n");
    nclose(nd);
    printf("finish nclose \n");
    
    printf("finish\n");
    
    return 0;
}
