#pragma once

#include <windows.h>
#include <winsock2.h>

enum {
    WM_TNP_UPDATE_UI = WM_APP + 100,
    WM_TNP_DEVICE_FOUND,
    WM_TNP_DEVICE_LOST ,
};

typedef struct {
    unsigned short magic; // magic (0x8d50)
    char fw_version[128]; // FW Version
    char uid[128]; // UID [暂无]
    char sn [64];   // SN
    char mac[12];  // mac
} udpcmd_hd_t;

typedef struct {
    unsigned short magic;  // magic (0x8D5C)
    unsigned int cmd_id;   // 命令号
    unsigned int data_len; // 数据长度 pdata 的长度
    char resivered[16];    // 预留
} cmd_hd_t;

typedef struct {
    unsigned short magic;  // magic (0x8D5D)
    unsigned int cmd_id;   // 命令号
    unsigned int data_len; // 数据长度 pdata 的长度
    int  result_value;     // 返回状态
    char result_msg[256];  // 返回消息 为字符串 log
} rsp_hd_t;

void* tnp_init(HWND hwnd);
void  tnp_free(void *ctxt);

int   tnp_connect    (void *ctxt, char *sn, struct in_addr *addr);
void  tnp_disconnect (void *ctxt);
int   tnp_send_cmd   (void *ctxt, cmd_hd_t *cmd, rsp_hd_t *rsp);

int   tnp_dev_alive  (void *ctxt);
int   tnp_get_fwver  (void *ctxt, char *ver, int vlen);
int   tnp_set_snmac  (void *ctxt, char *sn, char *mac);
int   tnp_get_snmac  (void *ctxt, char *sn, int slen, char *mac, int mlen);

int   tnp_test_spkmic(void *ctxt);
int   tnp_test_irc   (void *ctxt, int onoff);
int   tnp_test_auto  (void *ctxt, int *btn, int *lsensor, int *spkmic, int *bat);

int   tnp_enter_aging(void *ctxt);


