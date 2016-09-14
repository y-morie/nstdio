#include <stdio.h>
#include <string.h>
#include <unistd.h> // sleep
#include <stdlib.h>
#include "nstdio.h"

#define NCOUNT 1024
#define STR_SIZE 1024 * 1024

#define SERVER 1
#define CLIENT 2

int main(int argc, char **argv) {

    NET *nt;
    ND *nd;
    NHDL *hdl;
    
    int i, count;
    char *str;
    
    int scflag;
    
    str = (char *)malloc(sizeof(char) * STR_SIZE);
    if (NULL == str) {
        fprintf(stderr, "malloc error for str\n");
        exit(1);
    }
    
    if (argc == 4) {
        scflag = CLIENT;
        nt = setnet(argv[1], argv[2], NTCP);
        printf("cl: start nopen \n");
        nd = nopen(nt, "w");
        printf("cl: finish nopen \n");
        sprintf(str, "%s", argv[3]);
    }
    else if (argc == 2) {
        scflag = SERVER;
        nt = setnet(NULL, argv[1], NTCP);
        printf("sv: start nopen \n");
        nd = nopen(nt, "r");
        printf("sv: finish nopen \n");
    }
    else {
        fprintf(stderr, "Command error　\n");
        fprintf(stderr, "cl: %s servername port\n", argv[0]);
        fprintf(stderr, "sv: %s port \n", argv[0]);
        exit(1);
    }
    
    count = NCOUNT;
    for (i = 0; i < count ; i++) {
        if (scflag == CLIENT) {
            sprintf(str, "%s_%d", argv[3], i);
            printf("cl: start nwrite\n"); 
            hdl = nwrite(nd, str, STR_SIZE);
            printf("cl: finish nwrite\n"); 
            printf("cl: start nquery\n"); 
            while (1 == nquery(hdl));
            printf("cl: finish nquery\n"); 
            nsync(nd);
        }        
        else if (scflag == SERVER) {
            printf("sv: start nread\n");
            hdl = nread(nd, str, STR_SIZE);
            printf("sv: finish nread\n"); 
            printf("sv: start nquery\n");
            while (1 == nquery(hdl));
            printf("sv: finish nquery\n"); 
            printf("sv: data [%s] recv\n", str);
            nsync(nd);
        }
    }
    
    printf("start nclose \n");
    nclose(nd);
    printf("finish nclose \n");
    
    freenet(nt);    
    printf("finish\n");
    
    return 0;
}
