#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nstdio.h"

int main(int argc, char **argv){
    
    NET *nt;
    ND *nd;
    NHDL *hdl;
    
    /* chars data  */
    char *str;
    
    if (argc != 4) {
        fprintf(stderr, "command error \n");
        fprintf(stderr, "%s servername port send_chars\n", argv[0]);
        exit(1);
    }
    nt = setnet(argv[1], argv[2], NTCP);
    printf("cl: send chars [%s]\n", argv[3]);
    
    printf("cl: start nopen \n");
    nd = nopen(nt, "c");
    if ( NULL == nd ) {
      printf("cl: nopen failed.\n");
      exit(1);
    }

    printf("cl: finish nopen \n");
    //nsync(nd);
    printf("cl: start nwrite \n");
    hdl = nwrite(nd, argv[3], 16);
    printf("cl: finish nwrite \n");
    
    printf("cl: start nquery \n");
    while (nquery(hdl));
    printf("cl: finish nquery \n");
    
    printf("cl: start nclose \n");
    nclose(nd);
    printf("cl: finish nclose \n");
    
    freenet(nt);
    printf("cl: finish\n");
    
    return 0;
}

