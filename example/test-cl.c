#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nstdio.h"

int main(int argc, char **argv){
    NET nt;
    ND *nd;
    NHDL *hdl;
    char *str;
    char *ch='\0';
    char *str2="aaaaaa";
    
    nt.lip_addr = strdup(argv[1]);
    nt.lport = atoi(argv[2]);
    nt.rip_addr = strdup(argv[3]);
    nt.rport = atoi(argv[4]);
    nt.Dflag = TCP;
    str = strdup(argv[5]);
    sprintf(str, "%s%s%s", str, ch, str2);
        

    printf("sv start nopen \n");
    nd = nopen(&nt);
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
    
    printf("sv finish\n");
    
    return 0;
}

