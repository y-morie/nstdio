#include <stdio.h>
#include <string.h>
#include <unistd.h> // sleep
#include <stdlib.h>
#include <sys/time.h>
#include "nstdio.h"

double gettimeofday_sec(){
    
    struct timeval t;
    
    gettimeofday(&t, NULL);
    
    return (double)t.tv_sec + (double)t.tv_usec * 1e-6;
}

#define DCOUNT 1024LLU * 1024LLU * 32LLU
#define SIZE (DCOUNT * 8LLU)
/*
int ncomm[24]={
	     1000,
	     1000,
	     1000,
	     1000,
	     1000,
	     1000,
	     1000,
	     1000,
	     1000,
	     640,
	     640,
	     320,
	     160,
	     80,
	     40,
	     20,
	     20,
	     20,
	     10,
	     10,
	     10
	     };*/

int ncomm[27]={
	     100,
	     100,
	     100,
	     100,
	     100,
	     100,
	     100,
	     100,
	     100,
	     64,
	     64,
	     32,
	     32,
	     32,
	     20,
	     20,
	     20,
	     20,
	     10,
	     10,
	     10,
	     10,
	     10,
	     10,
	     10,
	     10,
	     10
};

int main(int argc, char **argv) {
    NET *nt;
    ND *nd;
    NHDL *hdl;
    int j, k;
    int count;
    uint64_t ui, i;
    uint64_t *str;
    double st, et;
    
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
    nd = nopen(nt, "s");
    if ( NULL == nd ) {
      printf("sv: nopen failed. \n");
      exit(1);
    } 
    printf("sv: finish nopen \n");
    
    for (ui = 0; ui < DCOUNT;ui++) {
        str[ui] = 0;
    }
    
    count = 1;
    k = 0;
    for (i = 8; i <= SIZE; i=i*2) {
      nsync(nd);
      for (j = 0; j < ncomm[k]; j++){
	hdl = nread(nd, str, i);
	while (nquery(hdl));
	hdl = nwrite(nd, str, i);
	while (nquery(hdl));
      }
      k++;
    }
    
    nclose(nd);
    freenet(nt);
    printf("sv: finish \n");

    return 0;
}
