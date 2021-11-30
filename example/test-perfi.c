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
  NET *nt_bnport, *nt_clport2;
  ND *nd_bnport, *nd_clport2;
    NHDL *hdl;
    int j, k;
    int count;
    uint64_t ui, i;
    uint64_t *str;
    double st, et;
    
    if (argc != 3) {
        fprintf(stderr, "command error \n");
        fprintf(stderr, "%s bn_port cl_port \n", argv[0]);
        exit(1);
    }
    
    str = (uint64_t *)malloc(SIZE);
    if(str == NULL){
        printf("malloc error \n");
        return 0;
    }
    
    nt_bnport = setnet(NULL, argv[1], NTCP);
    printf("fe: start nopen bn_port1\n");
    nd_bnport = nopen(nt_bnport, "s");
    if ( NULL == nd ) {
      printf("fe: nopen failed. \n");
      exit(1);
    } 
    printf("fe: finish nopen bn_port\n");

    
    nt_clport= setnet(NULL, argv[2], NTCP);
    printf("fe: start nopen cl_port\n");
    nd_clport = nopen(nt_clport, "s");
    if ( NULL == nd_clport ) {
      printf("fe: nopen failed. \n");
      exit(1);
    }
    printf("fe: finish nopen cl_port\n");
    
    for (ui = 0; ui < DCOUNT;ui++) {
        str[ui] = 0;
    }
    
    count = 1;
    k = 0;
    for (i = 1; i <= SIZE; i=i*2) {
      nsync(nd);
      for (j = 0; j < 20/*ncomm[k]*/; j++){
	hdl = nread(nd_clport, str, i);
	while (nquery(hdl));
	hdl = nwrite(nd_bnport, str, i);
	while (nquery(hdl));
	hdl = nread(nd_bnport, str, i);
	while (nquery(hdl));
	hdl = nwrite(nd_clport, str, i);
	while (nquery(hdl));
      }
      k++;
    }
    
    nclose(nd);
    freenet(nt);
    printf("fe: finish \n");

    return 0;
}
