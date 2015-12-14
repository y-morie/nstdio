#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nstdio.h"

int main(int argc, char **argv){
    NET *nt;
    ND *nd;
    NHDL *hdl;
    char *str;
    char *ch='\0';
    char *str2="original";
        
    nt = setnet(argv[1], atoi(argv[2]), NTCP);
    str = strdup(argv[3]);
    sprintf(str, "%s%s%s", str, ch, str2);
    printf("%s\n", str);

    printf("sv start nopen \n");
    nd = nopen(nt, "w");
    printf("sv finish nopen \n");
    
    printf("sv start nread \n");
    hdl = nwrite(nd, str, 256);
    printf("sv finish nread \n");

    printf("sv start nquery \n");
    while(nquery(hdl));
    printf("sv finish nquery \n");
        
    printf("sv start nclose \n");
    nclose(nd);
    printf("sv finish nclose \n");
    freenet(nt);    
    printf("sv finish\n");
    
    return 0;
}
