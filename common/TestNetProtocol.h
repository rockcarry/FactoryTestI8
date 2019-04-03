#pragma once

#include <windows.h>
#include <winsock2.h>

enum {
    WM_TNP_UPDATE_UI = WM_APP + 100,
    WM_TNP_DEVICE_FOUND,
    WM_TNP_DEVICE_LOST ,
};

void* tnp_init(HWND hwnd);
void  tnp_free(void *ctxt);

int   tnp_connect    (void *ctxt, char *sn, struct in_addr *addr);
void  tnp_disconnect (void *ctxt);
int   tnp_send_cmd   (void *ctxt, char *cmd, char *rsp, int rlen);

int   tnp_get_fwver  (void *ctxt, char *ver, int len);
int   tnp_set_sn     (void *ctxt, char *sn);
int   tnp_get_sn     (void *ctxt, char *sn , int len);
int   tnp_get_mac    (void *ctxt, char *mac, int len);
int   tnp_get_result (void *ctxt, char *result, int len);

int   tnp_test_spkmic(void *ctxt);
int   tnp_test_iperf (void *ctxt);
int   tnp_enter_aging(void *ctxt);


