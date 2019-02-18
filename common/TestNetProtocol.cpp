#include <stdlib.h>
#include "stdafx.h"
#include "log.h"
#include "TestNetProtocol.h"


typedef struct {
    struct in_addr addr;
    DWORD  tick;
    char   sn [65 ];
    char   mac[13 ];
    char   ver[128];
} DEVICE;

typedef struct {
    HWND    hwnd;
    HANDLE  thread_handle;
    #define TNP_TS_EXIT (1 << 0)
    int     thread_status;
    DEVICE  device_list[256];
    SOCKET  sock;
} TNPCONTEXT;

#define TNP_UDP_PORT      34677
#define TNP_TCP_PORT      34678
#define TNP_UDP_SENDTIMEO 1000
#define TNP_UDP_RECVTIMEO 1000
#define TNP_TCP_SENDTIMEO 1000
#define TNP_TCP_RECVTIMEO 1000
#define TND_DEVLOST_TIMEO 3000

static DWORD get_localhost_ip(void)
{
    char     name[MAXBYTE];
    hostent *host = NULL;
    gethostname(name, MAXBYTE);
    host = gethostbyname(name);
    return host->h_addr_list[0] ? *(u_long*)host->h_addr_list[0] : 0;
}

static DWORD WINAPI DeviceDetectThreadProc(LPVOID pParam)
{
    TNPCONTEXT *ctxt = (TNPCONTEXT*)pParam;
    SOCKET      sock = NULL;
    int     socklen  = sizeof(struct sockaddr_in);
    struct  sockaddr_in servaddr = {};
    struct  sockaddr_in fromaddr = {};

    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = get_localhost_ip();
    servaddr.sin_port        = htons(TNP_UDP_PORT);
    servaddr.sin_addr.S_un.S_un_b.s_b4 = 255;

    // open & config socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    int opt;
    opt = TNP_UDP_SENDTIMEO; setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO , (char*)&opt, sizeof(int));
    opt = TNP_UDP_RECVTIMEO; setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO , (char*)&opt, sizeof(int));
    opt = 1;                 setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(int));
    opt = 1;                 setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(int));

    // start device detection
    while (!(ctxt->thread_status & (TNP_TS_EXIT))) {
        if (ctxt->sock) {
            log_printf("tnp protocal tcp connected !\n");
            Sleep(100);
            continue;
        }

        msg_hd_t msg = {};
        int      ret;
        ret = sendto  (sock, (char*)"dh", 2, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
        ret = recvfrom(sock, (char*)&msg, sizeof(msg), 0, (struct sockaddr*)&fromaddr, &socklen);
        if (ret == sizeof(msg)) {
#if 0
            log_printf("receive datagram from %s:%d\n", inet_ntoa(fromaddr.sin_addr), fromaddr.sin_port);
            log_printf("msg.sn  : %s\n", msg.sn );
            log_printf("msg.mac : %s\n", msg.mac);
#endif

            char *ip = inet_ntoa(fromaddr.sin_addr);
            int   b4 = fromaddr.sin_addr.S_un.S_un_b.s_b4;
            if (ctxt->device_list[b4].addr.S_un.S_addr != fromaddr.sin_addr.S_un.S_addr) {
                // new device added
                log_printf("new device found, ip is: %s\n", ip);
                ctxt->device_list[b4].addr.S_un.S_addr = fromaddr.sin_addr.S_un.S_addr;
                PostMessage(ctxt->hwnd, WM_TNP_DEVICE_FOUND, 0, fromaddr.sin_addr.S_un.S_addr);
            }
            ctxt->device_list[b4].tick = GetTickCount();
            strncpy(ctxt->device_list[b4].sn, msg.sn, sizeof(ctxt->device_list[b4].sn));
        } else {
//          log_printf("receive datagram error or timeout !\n");
        }

        DWORD curtick = GetTickCount();
        for (int i=0; i<256; i++) {
            if (  ctxt->device_list[i].tick + TND_DEVLOST_TIMEO < curtick
               && ctxt->device_list[i].addr.S_un.S_addr != 0) {
                log_printf("device %s lost !\n", inet_ntoa(ctxt->device_list[i].addr));
                log_printf("remove it from device list !\n");
                memset(&(ctxt->device_list[i]), 0, sizeof(DEVICE));
            }
        }

#if 0
        log_printf("++ dump device list:\n");
        for (int i=0; i<256; i++) {
            if (ctxt->device_list[i].addr.S_un.S_addr) {
                log_printf("%s %s\n", inet_ntoa(ctxt->device_list[i].addr), ctxt->device_list[i].sn);
            }
        }
        log_printf("-- dump device list:\n");
#endif
    }

    closesocket(sock);
    return 0;
}

void* tnp_init(HWND hwnd)
{
    // wsa startup
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa)) {
         log_printf("init winsock failed !");
         return 0;
    }

    // allocate context
    TNPCONTEXT *ctxt = (TNPCONTEXT*)calloc(1, sizeof(TNPCONTEXT));
    if (!ctxt) {
        log_printf("failed to allocate memory for TNPCONTEXT !");
        return ctxt;
    }

    // init context variables
    ctxt->hwnd = hwnd;

    // create thread for device detection
    ctxt->thread_handle = CreateThread(NULL, 0, DeviceDetectThreadProc, ctxt, 0, NULL);
    return ctxt;
}

void tnp_free(void *ctxt)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return;

    // wait until thread save exit
    context->thread_status |= TNP_TS_EXIT;
    WaitForSingleObject(context->thread_handle, -1);
    CloseHandle(context->thread_handle);

    // free context
    free(context);

    // wsa cleanup
    WSACleanup();
}

int tnp_connect(void *ctxt, char *sn, struct in_addr *addr)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return -1;
    int i;
    
    for (i=0; i<256; i++) {
        if (sn) {
            if (strcmp(sn, context->device_list[i].sn) == 0) break;
        } else {
            if (strlen(context->device_list[i].sn)) break;
        }
    }

    if (context->sock) {
        log_printf("a connection already created !\n");
        log_printf("close the previous connection, and reconnect to %s ...\n", inet_ntoa(context->device_list[i].addr));
        closesocket(context->sock);
    }

    context->sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt;
    opt = TNP_TCP_SENDTIMEO; setsockopt(context->sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&opt, sizeof(int));
    opt = TNP_TCP_RECVTIMEO; setsockopt(context->sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&opt, sizeof(int));

    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port   = htons(TNP_TCP_PORT);
    sockaddr.sin_addr   = context->device_list[i].addr;
    if (connect(context->sock, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1) {
        memset(&(context->device_list[i]), 0, sizeof(DEVICE));
        return -1;
    } else {
        *addr = context->device_list[i].addr;
        return  0;
    }
}

void tnp_disconnect(void *ctxt)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return;
    if (context->sock) {
        closesocket(context->sock);
        context->sock = NULL;
    }
}

int tnp_send_cmd(void *ctxt, cmd_hd_t *cmd, rsp_hd_t *rsp, int rlen)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return -1;

    char recvbuf[1024];
    int  ret;

    if (send(context->sock, (const char*)cmd, sizeof(cmd_hd_t) + cmd->data_len, 0) == -1) {
        log_printf("tnp_send_cmd send tcp data failed !\n");
        return -1;
    }

    ret = recv(context->sock, recvbuf, sizeof(recvbuf), 0);
    if (ret > 0) {
        rsp_hd_t *r = (rsp_hd_t*)recvbuf;
        if (r->magic != 0x8D5D || ret != sizeof(cmd_hd_t) + r->data_len || cmd->cmd_id != rsp->cmd_id) {
            return -1;
        }
        memcpy(rsp, recvbuf, sizeof(recvbuf) < rlen ? sizeof(recvbuf) : rlen);
        return 0;
    }
    return -1;
}

int tnp_get_fwver(void *ctxt, char *ver, int vlen)
{
    char cmd_buf[1024];
    char rsp_buf[1024];
    int   ret;
    cmd_hd_t *cmd = (cmd_hd_t*)cmd_buf;
    rsp_hd_t *rsp = (rsp_hd_t*)rsp_buf;
    cmd->magic  = 0x8d5d;
    cmd->cmd_id = 0x64;
    ret = tnp_send_cmd(ctxt, cmd, rsp, sizeof(rsp_buf));
    if (ret != 0) return ret;
    strncmp(ver , rsp_buf + sizeof(rsp_hd_t), vlen < (int)rsp->data_len ? vlen : (int)rsp->data_len);
    return 0;
}

int tnp_set_snmac(void *ctxt, char *sn, char *mac)
{
    char cmd_buf[1024];
    char rsp_buf[1024];
    cmd_hd_t *cmd = (cmd_hd_t*)cmd_buf;
    rsp_hd_t *rsp = (rsp_hd_t*)rsp_buf;
    int       ret = 0;

    if (sn) {
        cmd->magic    = 0x8d5d;
        cmd->cmd_id   = 0x60;
        cmd->data_len = (unsigned)strlen(sn);
        strncmp(cmd_buf + sizeof(cmd_hd_t), sn , sizeof(cmd_buf) - sizeof(cmd_hd_t));
        ret = tnp_send_cmd(ctxt, cmd, rsp, sizeof(rsp_buf));
    }
    if (ret != 0) return ret;

    if (mac) {
        cmd->magic    = 0x8d5d;
        cmd->cmd_id   = 0x64;
        cmd->data_len = (unsigned)strlen(mac);
        strncmp(cmd_buf + sizeof(cmd_hd_t), mac, sizeof(cmd_buf) - sizeof(cmd_hd_t));
        ret = tnp_send_cmd(ctxt, cmd, rsp, sizeof(rsp_buf));
    }
    return ret;
}

int tnp_get_snmac(void *ctxt, char *sn, int slen, char *mac, int mlen)
{
    char cmd_buf[1024];
    char rsp_buf[1024];
    cmd_hd_t *cmd = (cmd_hd_t*)cmd_buf;
    rsp_hd_t *rsp = (rsp_hd_t*)rsp_buf;
    int       ret = 0;

    if (sn) {
        cmd->magic    = 0x8d5d;
        cmd->cmd_id   = 0x62;
        ret = tnp_send_cmd(ctxt, cmd, rsp, sizeof(rsp_buf));
        if (ret != 0) return ret;
        strncmp(sn , rsp_buf + sizeof(rsp_hd_t), slen < (int)rsp->data_len ? slen : (int)rsp->data_len);
    }

    if (mac) {
        cmd->magic    = 0x8d5d;
        cmd->cmd_id   = 0x63;
        ret = tnp_send_cmd(ctxt, cmd, rsp, sizeof(rsp_buf));
        if (ret != 0) return ret;
        strncmp(mac, rsp_buf + sizeof(rsp_hd_t), mlen < (int)rsp->data_len ? mlen : (int)rsp->data_len);
    }
    return ret;
}

int tnp_test_spkmic(void *ctxt)
{
    return 0;
}

int tnp_test_irc(void *ctxt, int onoff)
{
    cmd_hd_t cmd = {};
    rsp_hd_t rsp = {};
    cmd.magic  = 0x8d5d;
    cmd.cmd_id = onoff ? 0x58 : 0x59;
    return tnp_send_cmd(ctxt, &cmd, &rsp, sizeof(rsp));
}

int tnp_test_auto(void *ctxt, int *btn, int *lsensor, int *micspk, int *battery)
{
    return -1;
}

int tnp_enter_aging(void *ctxt)
{
    cmd_hd_t cmd = {};
    rsp_hd_t rsp = {};
    cmd.magic  = 0x8d5d;
    cmd.cmd_id = 0x77;
    return tnp_send_cmd(ctxt, &cmd, &rsp, sizeof(rsp));
}

