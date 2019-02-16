#include <stdlib.h>
#include "stdafx.h"
#include "log.h"
#include "TestNetProtocol.h"

void* tnp_init(HWND hwnd)
{
    return NULL;
}

void tnp_free(void *ctxt)
{
}

int tnp_connect(void *ctxt, char *sn, struct in_addr *addr)
{
    return -1;
}

void tnp_disconnect(void *ctxt)
{
}

int tnp_send_cmd(void *ctxt, cmd_hd_t *cmd, rsp_hd_t *rsp)
{
    return -1;
}

int tnp_dev_alive(void *ctxt)
{
    return -1;
}

int tnp_get_fwver(void *ctxt, char *ver, int vlen)
{
    return -1;
}

int tnp_set_snmac(void *ctxt, char *sn, char *mac)
{
    return -1;
}

int tnp_get_snmac(void *ctxt, char *sn, int slen, char *mac, int mlen)
{
    return -1;
}

int tnp_test_spkmic(void *ctxt)
{
    return -1;
}

int tnp_test_irc(void *ctxt, int onoff)
{
    return -1;
}

int tnp_test_auto(void *ctxt, int *btn, int *lsensor, int *micspk, int *battery)
{
    return -1;
}

int tnp_enter_aging(void *ctxt)
{
    return -1;
}

