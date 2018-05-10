#pragma once

#include <windows.h>

enum {
    WM_TNP_UPDATE_UI = WM_APP + 100,
    WM_TNP_DEVICE_FOUND,
    WM_TNP_DEVICE_LOST,
};

void* tnp_init(HWND hwnd);
void  tnp_free(void *ctxt);
int   tnp_connect    (void *ctxt, struct in_addr addr);
void  tnp_disconnect (void *ctxt);
void  tnp_set_timeout(void *ctxt, int timeout);
int   tnp_burn_snmac (void *ctxt, char *sn, char *mac, int *snrslt, int *macrslt);
int   tnp_test_spkmic(void *ctxt);
int   tnp_test_button(void *ctxt, int *btn);
int   tnp_test_ir_and_filter(void *ctxt, int onoff);
int   tnp_test_spkmic_manual(void *ctxt);
int   tnp_test_sensor_snmac_version(void *ctxt, char *sn, char *mac, char *version, int *rsltsensor, int *rsltsn, int *rsltmac, int *rsltver);
void  tnp_test_cancel(void *ctxt, int cancel);


