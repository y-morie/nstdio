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

#define DCOUNT 1024LLU * 1024LLU
#define SIZE (DCOUNT * 8LLU)

int ncomm[21]={
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
};

int main(int argc, char **argv) {
    NET *nt;
    ND *nd;
    NHDL *hdl;
    int i, j, k;
    int count;
    uint64_t ui;
    uint64_t *str;
    double st, et;
    
    if (argc != 3) {
      fprintf(stderr, "command error \n");
      fprintf(stderr, "%s host port \n", argv[0]);
      exit(1);
    }
    
    str = (uint64_t *)malloc(SIZE);
    if(str == NULL){
      printf("malloc error \n");
      return 0;
    }
    
    nt = setnet(argv[1], argv[2], NTCP);
    printf("cl: start nopen \n");
    nd = nopen(nt, "c");
    if ( NULL == nd ) {
      printf("cl: nopen failed. \n");
      exit(1);
    } 
    printf("cl: finish nopen \n");
    
    for (ui = 0; ui < DCOUNT;ui++) {
      str[ui] = 0;
    }
    
    count = 1;
    k = 0;
    for (i = 8; i <= SIZE; i=i*2) {
      nsync(nd);
      st = gettimeofday_sec();
      for (j = 0; j < ncomm[k]; j++){
	hdl = nwrite(nd, str, i);
	while (nquery(hdl));
	hdl = nread(nd, str, i);
	while (nquery(hdl));
      }
      et = gettimeofday_sec();
      printf("cl: size %d time %f msec \n", i, (et-st)/ncomm[k]*1000);
      k++;
    }
    
    nclose(nd);
    freenet(nt);
    printf("cl: finish \n");

    return 0;
}
