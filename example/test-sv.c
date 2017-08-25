#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nstdio.h"

int main(int argc, char **argv){
    
    NET *nt;
    ND *nd;
    NHDL *hdl;
    char *str;
    int rc;
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "command error \n");
        fprintf(stderr, "%s [hostname] port \n", argv[0]);
        exit(1);
    }
    if ( argc == 2 ) {
	nt = setnet(NULL, argv[1], NTCP);
    }
    if ( argc == 3 ) {
	nt = setnet(argv[1], argv[2], NTCP);
    }

    str = (char *)malloc(sizeof(char) * 256);
    
    printf("sv: start nopen \n");
    nd = nopen(nt, "s");
    if ( NULL == nd ) {
      printf("sv: nopen failed.\n");
      exit(1);
    }
    printf("sv: finish nopen \n");
    //nsync(nd);
    printf("sv: start nread \n");
    hdl = nread(nd, str, 256);
    printf("sv: finish nread \n");
    
    printf("sv: start nquery \n");
    while(rc = nquery(hdl)) {
	if ( rc == -2 ) {
	    printf("sv: error: disconnect.\n");
	    exit(-1);
	}
    }
    printf("sv: finish nquery \n");
    printf("sv: msize %" PRIu64 "\n", hdl->pp_msize);
    printf("sv: get data  [%s]\n", str);
    
    printf("sv: start nclose \n");
    nclose(nd);
    printf("sv: finish nclose \n");
    
    freenet(nt);    
    printf("sv: finish\n");
    
    return 0;
}
