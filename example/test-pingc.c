#include <stdio.h>
#include <string.h>
#include <unistd.h> // sleep
#include <stdlib.h>
#include <nstdio.h>

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
    
    if (argc != 3) {
        fprintf(stderr, "command error \n");
        fprintf(stderr, "%s servername port \n", argv[0]);
        exit(1);
    }
    
    str = (uint64_t *)malloc(SIZE);
    if(str == NULL){
        printf("malloc error\n");
        return 0;
    }
    
    nt = setnet(argv[1], argv[2], NTCP);
    printf("cl: start nopen \n");
    nd = nopen(nt, "w" );
    if ( NULL == nd ) {
      printf("cl: nopen failed.\n");
      exit(1);
    }
    printf("cl: finish nopen \n");
    
    for (ui = 0; ui < DCOUNT; ui++) {
        str[ui] = ui;
    }

    count = 1;     
    for (i = 0; i < count ; i++) {
        printf("cl: start nwrite\n");
        hdl = nwrite(nd, str, SIZE); 
        printf("cl: finish nwrite\n");
        printf("cl: start nquery\n");  
        while (nquery(hdl));
        printf("cl: finish nquery\n");
    }
    
    printf("cl: start nclose \n");
    nclose(nd);
    printf("cl: finish nclose \n");
    
    freenet(nt);
    printf("cl: finish\n");
    
    return 0;
}
