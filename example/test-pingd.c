#include <stdio.h>
#include <string.h>
#include <unistd.h> // sleep
#include <stdlib.h>
#include "nstdio.h"


#define DCOUNT 1024LLU * 1024LLU * 256LLU
#define SIZE (DCOUNT * 8LLU)

int main(int argc, char **argv) {
    NET *nt;
    ND *nd;
    NHDL *hdl;
    int i;
    int count;
    uint64_t ui;
    uint64_t *str;
    
    if (argc != 2) {
        fprintf(stderr, "command error \n");
        fprintf(stderr, "%s port \n", argv[0]);
        exit(1);
    }
    
    str = (uint64_t *)malloc(SIZE);
    if(str == NULL){
        printf("malloc error \n");
        return 0;
    }
    
    nt = setnet(NULL, argv[1], NTCP);
    printf("sv: start nopen \n");
    nd = nopen(nt, "r");
    if ( NULL == nd ) {
      printf("sv: nopen failed. \n");
      exit(1);
    } 
    printf("sv: finish nopen \n");
    
    for (ui = 0; ui < DCOUNT;ui++) {
        str[ui] = 0;
    }
    
    count = 1;
    for (i = 0; i < count ; i++) {
        printf("sv: start nread \n");
        hdl = nread(nd, str, SIZE);
        printf("sv: finish nread \n"); 
        printf("sv: start nquery \n"); 
        while (nquery(hdl));
        printf("sv: finish nquery \n"); 
    }
    
    printf("sv: data 0:%" PRIu64 " %" PRIu64 ":%" PRIu64" \n",
	   str[0], DCOUNT-1, str[DCOUNT-1]);
    printf("sv: start nclose \n");
    nclose(nd);
    printf("sv: finish nclose \n");
    
#if 0
    for(ui = DCOUNT - 1024; ui < DCOUNT; ui++){
        printf("%" PRIu64 " ", str[ui]);
    }
    printf("\n");
#endif
    for(ui = 0; ui < DCOUNT; ui++){
        if ( ui != str[ui]) {
	  printf("NG exp %llu data %llu\n", ui, str[ui]);
	  break;
      }
    }
    if ( ui == DCOUNT ) printf("OK\n");
    
    freenet(nt);
    printf("sv: finish \n");

    return 0;
}
