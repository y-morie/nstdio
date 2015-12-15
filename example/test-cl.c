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
    str = malloc(sizeof(char)*256);
    
    str = strdup(argv[3]);
    sprintf(str, "%s%s%s", str, ch, str2);
    printf("%s\n", str);

    printf("cl start nopen \n");
    nd = nopen(nt, "w");
    printf("cl finish nopen \n");
    
    printf("cl start nread \n");
    hdl = nwrite(nd, str, 256);
    printf("cl finish nread \n");

    printf("cl start nquery \n");
    while(nquery(hdl));
    printf("cl finish nquery \n");
    
    printf("get data: %s\n", str);
    
    printf("cl start nclose \n");
    nclose(nd);
    printf("cl finish nclose \n");
    
    freenet(nt);
    printf("cl finish\n");
    
    return 0;
}

