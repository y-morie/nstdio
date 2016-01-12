#ifndef __INCLUDE_NSTDIO_H__
#define __INCLUDE_NSTDIO_H__

#include <ppstream.h>

#define NTCP PPSTREAM_TCP
#define NUDP PPSTREAM_UDP
#define NIBRC PPSTREAM_IBRC
#define NIBUD PPSTREAM_IBUD

typedef ppstream_networkinfo_t NET;

typedef ppstream_networkdescriptor_t ND;

typedef ppstream_handle_t NHDL;

/**
 * \brief ネットワークのオープンする。
 *   
 * nopen関数は、ntにより指定されるネットワークをmodeがさすモードでオープンし、
 * ネットワークにストリームを結びつける。
 * 
 * \param nt ネットワーク情報の参照
 * \param mode モード 
*/
ND *nopen(NET *nt, char *mode);

void nclose(ND *nd);

NHDL *nwrite(ND *nd, void *addr, size_t size);

NHDL *nread(ND *nd, void *addr, size_t size);

int nquery(NHDL *hdl);

NET *setnet(char *ip_add, uint16_t port, uint32_t Dflag);

void freenet(NET *nt);

#endif
