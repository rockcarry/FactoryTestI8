#include <stdlib.h>
#include "stdafx.h"
#include "log.h"
#include "TestNetProtocol.h"


typedef struct {
    struct in_addr addr;
    DWORD  tick;
    char   sn [65];
} DEVICE;

typedef struct {
    HWND    hwnd;
    HANDLE  thread_handle;
    #define TNP_TS_EXIT (1 << 0)
    int     thread_status;
    DEVICE  device_list[256];
    SOCKET  sock;
} TNPCONTEXT;

#define TNP_UDP_PORT      8313
#define TNP_UDP_SENDTIMEO 2000
#define TNP_UDP_RECVTIMEO 2000
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
    servaddr.sin_addr.S_un.S_un_b.s_b4 = 88;

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
//          log_printf("tnp protocal tcp connected !\n");
            Sleep(100);
            continue;
        }

        char msg[256] = {};
        int  ret;
        ret = sendto  (sock, (char*)"uid?", (int)strlen("uid?") + 1, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
        ret = recvfrom(sock, msg, sizeof(msg), 0, (struct sockaddr*)&fromaddr, &socklen);
        if (strstr(msg, "uid:") == msg && msg[ret] == '\0') {
#if 0
            log_printf("receive datagram from %s:%d\n", inet_ntoa(fromaddr.sin_addr), fromaddr.sin_port);
            log_printf("msg: %s\n", msg);
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
            strncpy(ctxt->device_list[b4].sn, msg + 4, sizeof(ctxt->device_list[b4].sn));
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
        Sleep(1000);
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
    if (i == 256) return -1;

    if (context->sock) {
        log_printf("a connection already created !\n");
        log_printf("close the previous connection, and reconnect to %s ...\n", inet_ntoa(context->device_list[i].addr));
        closesocket(context->sock);
    }

    context->sock = socket(AF_INET, SOCK_DGRAM, 0);
    int opt;
    opt = TNP_UDP_SENDTIMEO; setsockopt(context->sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&opt, sizeof(int));
    opt = TNP_UDP_RECVTIMEO; setsockopt(context->sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&opt, sizeof(int));

    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port   = htons(TNP_UDP_PORT);
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

int tnp_send_cmd(void *ctxt, char *cmd, char *rsp, int rlen)
{
    int ret;
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!context || !context->sock) return -1;
    ret = send(context->sock, cmd, (int)strlen(cmd) + 1, 0);
    if (ret == -1) {
        log_printf("tnp_send_cmd send udp data failed !\n");
        return -1;
    }
    ret = recv(context->sock, rsp, rlen, 0);
    if (ret <= 0 || rsp[ret-1] != '\0') {
        log_printf("tnp_send_cmd recv udp data failed !\n");
        return -1;
    }
    return 0;
}

int tnp_get_fwver(void *ctxt, char *ver, int len)
{
    char rsp[256];
    if (tnp_send_cmd(ctxt, "ver?", rsp, sizeof(rsp)) != 0) return -1;
    if (strstr(rsp, "ver:") != rsp) return -1;
    strncpy(ver, rsp + 4, len);
    return 0;
}

int tnp_set_sn(void *ctxt, char *sn)
{
    char cmd[256];
    char rsp[256];
    sprintf(cmd, "uid=%s", sn);
    if (tnp_send_cmd(ctxt, cmd, rsp, sizeof(rsp)) != 0) return -1;
    if (strstr(rsp, "uid.") != rsp) return -1;
    return 0;
}

int tnp_get_sn(void *ctxt, char *sn, int len)
{
    char rsp[256];
    if (tnp_send_cmd(ctxt, "uid?", rsp, sizeof(rsp)) != 0) return -1;
    if (strstr(rsp, "uid:") != rsp) return -1;
    strncpy(sn, rsp + 4, len);
    return 0;
}

int tnp_get_mac(void *ctxt, char *mac, int len)
{
    char rsp[256];
    if (tnp_send_cmd(ctxt, "mac?", rsp, sizeof(rsp)) != 0) return -1;
    if (strstr(rsp, "mac:") != rsp) return -1;
    strncpy(mac, rsp + 4, len);
    return 0;
}

int tnp_get_result(void *ctxt, char *result, int len)
{
    char rsp[256];
    if (tnp_send_cmd(ctxt, "result?", rsp, sizeof(rsp)) != 0) return -1;
    if (strstr(rsp, "result:") != rsp) return -1;
    strncpy(result, rsp + 7, len);
    return 0;
}

int tnp_test_spkmic(void *ctxt)
{
    char rsp[256];
    if (tnp_send_cmd(ctxt, "micspktest!", rsp, sizeof(rsp)) != 0) return -1;
    if (strstr(rsp, "micspktest.") != rsp) return -1;
    return 0;
}

int tnp_test_iperf(void *ctxt)
{
    char rsp[256];
    if (tnp_send_cmd(ctxt, "iperf3=1", rsp, sizeof(rsp)) != 0) return -1;
    if (strstr(rsp, "iperf3.") != rsp) return -1;
    return 0;
}

int tnp_enter_aging(void *ctxt)
{
    return 0;
}
