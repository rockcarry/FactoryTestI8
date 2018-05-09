#pragma once

#include <windows.h>

enum {
    WM_TNP_UPDATE_UI = WM_APP + 1,
    WM_TNP_DEVICE_FOUND,
    WM_TNP_DEVICE_LOST,
};

void* tnp_init(char *version, HWND hwnd);
void  tnp_free(void *ctxt);
int   tnp_connect    (void *ctxt, struct in_addr addr);
void  tnp_disconnect (void *ctxt);
int   tnp_burn_snmac (void *ctxt, char *sn, char *mac);
int   tnp_test_spkmic(void *ctxt);
void  tnp_test_cancel(void *ctxt, int cancel);


