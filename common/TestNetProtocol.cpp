#include <winsock2.h>
#include <stdlib.h>
#include "stdafx.h"
#include "log.h"
#include "TestNetProtocol.h"

typedef struct {
    struct in_addr addr;
    DWORD  tick;
    char   sn[32];
    int    type;
} DEVICE;

typedef struct {
    HWND    hwnd;
    HANDLE  thread_handle;
    #define TNP_TS_EXIT (1 << 0)
    int     thread_status;
    DEVICE  device_list[256];
    int     devlost_timeout;

    struct in_addr tcp_conn_addr;
    SOCKET  sock;
    #define TNP_TEST_CANCEL (1 << 0)
    int     test_status;
} TNPCONTEXT;

#pragma pack(4)
typedef struct
{
    char mag [4 ];
    char sn  [16];
    char type[8 ];
} NOTIFY_MSG;
#pragma pack()

#define TNP_UDP_PORT      8989
#define TNP_UDP_SENDTIMEO 1000
#define TNP_UDP_RECVTIMEO 1000
#define TNP_TCP_SENDTIMEO 1000
#define TNP_TCP_RECVTIMEO 1000
#define TND_DEVLOST_TIMEO 3000
static DWORD WINAPI DeviceDetectThreadProc(LPVOID pParam)
{
    TNPCONTEXT *ctxt = (TNPCONTEXT*)pParam;
    SOCKET     sock;
    NOTIFY_MSG msg;

    // open & config socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    int opt;
    opt = TNP_UDP_SENDTIMEO; setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO , (char*)&opt, sizeof(int));
    opt = TNP_UDP_RECVTIMEO; setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO , (char*)&opt, sizeof(int));
    opt = 1;                 setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(int));

    // bind socket
    struct sockaddr_in local;
    local.sin_family      = AF_INET;
    local.sin_port        = htons(TNP_UDP_PORT);
    local.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0) {
        log_printf("DeviceDetectThreadProc socket bind failed !\n");
        return 0;
    }

    // start device detection
    while (!(ctxt->thread_status & (TNP_TS_EXIT))) {
        struct sockaddr_in from;
        int fromlen = sizeof(from);
        if (recvfrom(sock, (char*)&msg, sizeof(msg), 0, (struct sockaddr*)&from, &fromlen) != SOCKET_ERROR) {
            /*
            log_printf("receive datagram from %s:%d\n", inet_ntoa(from.sin_addr), from.sin_port);
            log_printf("msg.mag : %c%c%c%c\n", msg.mag[0], msg.mag[1], msg.mag[2], msg.mag[3]);
            log_printf("msg.sn  : %s\n", msg.sn  );
            log_printf("msg.port: %s\n", msg.port);
            */
            char *ip = inet_ntoa(from.sin_addr);
            int   b4 = from.sin_addr.S_un.S_un_b.s_b4;
            if (ctxt->device_list[b4].addr.S_un.S_addr != from.sin_addr.S_un.S_addr) {
                // new device added
                log_printf("new device found, ip is: %s\n", ip);
                ctxt->device_list[b4].addr.S_un.S_addr = from.sin_addr.S_un.S_addr;
                PostMessage(ctxt->hwnd, WM_TNP_DEVICE_FOUND, 0, from.sin_addr.S_un.S_addr);
            }
            ctxt->device_list[b4].tick = GetTickCount();
            ctxt->device_list[b4].type = (msg.type[0] << 0) | (msg.type[1] << 1) | (msg.type[2] << 2);
            if (ctxt->tcp_conn_addr.S_un.S_addr == from.sin_addr.S_un.S_addr) {
                PostMessage(ctxt->hwnd, WM_TNP_TYPE_CHANGED, ctxt->device_list[b4].type, ctxt->device_list[b4].addr.S_un.S_addr);
            }
            strncpy(ctxt->device_list[b4].sn, msg.sn, 32);
        } else {
//          log_printf("receive datagram error or timeout !\n");
        }

        DWORD curtick = GetTickCount();
        for (int i=0; i<256; i++) {
            if (  ctxt->device_list[i].tick + ctxt->devlost_timeout < curtick
               && ctxt->device_list[i].addr.S_un.S_addr != 0) {
                log_printf("device %s lost !\n", inet_ntoa(ctxt->device_list[i].addr));
                log_printf("remove it from device list !\n");
                PostMessage(ctxt->hwnd, WM_TNP_DEVICE_LOST, 0, ctxt->device_list[i].addr.S_un.S_addr);
                memset(&(ctxt->device_list[i]), 0, sizeof(DEVICE));
                strncpy(ctxt->device_list[i].sn, "", 32);
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

#define TNP_TCP_PORT 7838
#define SIG_MAGIC  (('N' << 24) | ('S' << 16) | ('8' << 8) | ('I' << 0))
#pragma pack(4)
typedef struct {
    DWORD MAGIC;

    char SN [16];
    char MAC[16];
    char VER[32];

    char testSN;
    char testMic;

    char testIR;
    char testIRCut;
    char testLightSensor;

    char testKey;
    char testVersion;

    char rtSN;
    char rtMAC;
    char rtMic;
    char rtLightSensor;
    char rtKey;
    char rtVersion;

    char exitTest;
} FACTORYTEST_DATA;
#pragma pack()

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
    ctxt->hwnd            = hwnd;
    ctxt->devlost_timeout = TND_DEVLOST_TIMEO;

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

int tnp_connect(void *ctxt, struct in_addr addr)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return -1;

    if (context->sock) {
        log_printf("a connection already created !\n");
        log_printf("close the previous connection, and reconnect to %s ...\n", inet_ntoa(addr));
        closesocket(context->sock);
    }

    context->sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt;
    opt = TNP_TCP_SENDTIMEO; setsockopt(context->sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&opt, sizeof(int));
    opt = TNP_TCP_RECVTIMEO; setsockopt(context->sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&opt, sizeof(int));

    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port   = htons(TNP_TCP_PORT);;
    sockaddr.sin_addr   = addr;
    if (connect(context->sock, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1) {
        memset(&(context->device_list[addr.S_un.S_un_b.s_b4]), 0, sizeof(DEVICE));
        return -1;
    } else {
        context->tcp_conn_addr = addr;
        return  0;
    }
}

void tnp_disconnect(void *ctxt)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return;

    if (context->sock) {
        closesocket(context->sock);
        context->tcp_conn_addr.S_un.S_addr = 0;
        context->sock = NULL;
    }
}

int tnp_connect_by_sn(void *ctxt, char *sn, struct in_addr *paddr)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return -1;

    struct in_addr addr = {0};
    int            find =  0;
    for (int i=0; i<256; i++) {
        if (context->device_list[i].addr.S_un.S_addr && strcmp(context->device_list[i].sn, sn) == 0) {
            find = 1;
            *paddr = addr = context->device_list[i].addr;
            break;
        }
    }
    return find ? tnp_connect(ctxt, addr) : -1;
}

int tnp_disconnect_by_sn(void *ctxt, char *sn)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return -1;

    struct in_addr addr = {0};
    int            find =  0;
    for (int i=0; i<256; i++) {
        if (context->device_list[i].addr.S_un.S_addr && strcmp(context->device_list[i].sn, sn) == 0) {
            find = 1;
            break;
        }
    }
    if (find) return -1;

    tnp_disconnect(ctxt);
    return 0;
}

void tnp_set_timeout(void *ctxt, int timeout)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return;
    context->devlost_timeout = timeout;
}

int tnp_burn_snmac(void *ctxt, char *sn, char *mac, int *snrslt, int *macrslt)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return -1;

    if (!context->sock) {
        log_printf("tnp_burn_snmac failed ! no connection !\n");
        return -1;
    }

    FACTORYTEST_DATA data = {0};
    data.MAGIC = SIG_MAGIC;
    memset(data.SN , '0', sizeof(data.SN ));
    memset(data.MAC, '0', sizeof(data.MAC));
    memcpy(data.SN , sn , strlen(sn ));
    memcpy(data.MAC, mac, strlen(mac));
    data.SN [15] = '\0';
    data.MAC[12] = '\0';
    data.testSN  = '1';

    if (send(context->sock, (const char*)&data, sizeof(data), 0) == -1) {
        log_printf("tnp_burn_snmac send tcp data failed !\n");
        return -1;
    }

    if (recv(context->sock, (char*)&data, sizeof(data), 0) == -1) {
        log_printf("tnp_burn_snmac recv tcp data failed !\n");
        return -1;
    }

    log_printf("tnp_burn_snmac data received:\n");
    log_printf("SN       = %s\n", data.SN      );
    log_printf("MAC      = %s\n", data.MAC     );
    log_printf("rtSN     = %d\n", data.rtSN    );
    log_printf("rtMAC    = %d\n", data.rtMAC   );
    *snrslt  = data.rtSN;
    *macrslt = data.rtMAC;
    return 0;
}

int tnp_test_spkmic(void *ctxt)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return -1;

    if (!context->sock) {
        log_printf("tnp_test_spkmic failed ! no connection !\n");
        return -1;
    }

    FACTORYTEST_DATA data = {0};
    data.MAGIC   = SIG_MAGIC;
    data.testMic = '1';

    if (send(context->sock, (const char*)&data, sizeof(data), 0) == -1) {
        log_printf("tnp_test_spkmic send tcp data failed !\n");
        return -1;
    }

    for (int i=0; i<10; i++) {
        if (context->test_status & TNP_TEST_CANCEL) break;
        if (recv(context->sock, (char*)&data, sizeof(data), 0) == -1) {
            log_printf("tnp_test_spkmic recv tcp data failed ! retry %d\n", i);
            continue;
        } else {
            break;
        }
    }

    log_printf("tnp_test_spkmic data received:\n");
    log_printf("rtMic = %d\n", data.rtMic);
    return data.rtMic == 1 ? 0 : -1;
}

int tnp_test_button(void *ctxt, int *btn)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return -1;

    if (!context->sock) {
        log_printf("tnp_test_button failed ! no connection !\n");
        return -1;
    }

    FACTORYTEST_DATA data = {0};
    data.MAGIC   = SIG_MAGIC;
    data.testKey = '1';

    if (send(context->sock, (const char*)&data, sizeof(data), 0) == -1) {
        log_printf("tnp_test_button send tcp data failed !\n");
        return -1;
    }

    if (recv(context->sock, (char*)&data, sizeof(data), 0) == -1) {
        log_printf("tnp_test_button recv tcp data failed !\n");
        return -1;
    }

    log_printf("tnp_test_button data received:\n");
    log_printf("rtKey = %d\n", data.rtKey);
    *btn = data.rtKey;
    return 0;
}

int tnp_test_ir_and_filter(void *ctxt, int onoff)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return -1;

    if (!context->sock) {
        log_printf("tnp_test_ir_and_filter failed ! no connection !\n");
        return -1;
    }

    FACTORYTEST_DATA data = {0};
    data.MAGIC  = SIG_MAGIC;
    data.testIR = data.testIRCut = onoff ? '2' : '1';

    if (send(context->sock, (const char*)&data, sizeof(data), 0) == -1) {
        log_printf("tnp_test_ir_and_filter send tcp data failed !\n");
        return -1;
    }

    if (recv(context->sock, (char*)&data, sizeof(data), 0) == -1) {
        log_printf("tnp_test_ir_and_filter recv tcp data failed !\n");
        return -1;
    }

    return 0;
}

int tnp_test_spkonly_manual(void *ctxt, int onoff)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return -1;

    if (!context->sock) {
        log_printf("tnp_test_spkmic_manual failed ! no connection !\n");
        return -1;
    }

    FACTORYTEST_DATA data = {0};
    data.MAGIC   = SIG_MAGIC;
    data.testMic = onoff ? '2' : '3';

    if (send(context->sock, (const char*)&data, sizeof(data), 0) == -1) {
        log_printf("tnp_test_spkmic_manual send tcp data failed !\n");
        return -1;
    }

    int i;
    for (i=0; i<10; i++) {
        if (context->test_status & TNP_TEST_CANCEL) break;
        if (recv(context->sock, (char*)&data, sizeof(data), 0) == -1) {
            log_printf("tnp_test_spkmic_manual recv tcp data failed ! retry %d\n", i);
            continue;
        } else {
            break;
        }
    }

    return i == 10 ? -1 : 0;
}

int tnp_test_sensor_snmac_version(void *ctxt, char *sn, char *mac, char *version, int *rsltkey, int *rsltsensor, int *rsltsn, int *rsltmac, int *rsltver)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return -1;

    if (!context->sock) {
        log_printf("tnp_test_sensor_snmac_version failed ! no connection !\n");
        return -1;
    }

    FACTORYTEST_DATA data = {0};
    data.MAGIC = SIG_MAGIC;
    data.testLightSensor = '1';
    data.testVersion     = '1';
    data.testSN          = '2';
    data.testKey         = '1';
    memset(data.SN , '0', sizeof(data.SN ));
    memset(data.MAC, '0', sizeof(data.MAC));
    memcpy(data.SN , sn , strlen(sn ));
    memcpy(data.MAC, mac, strlen(mac));
    data.SN [15] = '\0';
    data.MAC[12] = '\0';
    strcpy(data.VER, version);

    if (send(context->sock, (const char*)&data, sizeof(data), 0) == -1) {
        log_printf("tnp_test_sensor_snmac_version send tcp data failed !\n");
        return -1;
    }

    if (recv(context->sock, (char*)&data, sizeof(data), 0) == -1) {
        log_printf("tnp_test_sensor_snmac_version recv tcp data failed !\n");
        return -1;
    }

    log_printf("tnp_test_sensor_snmac_version data received:\n");
	log_printf("rtKey         = %d\n", data.rtKey        );
    log_printf("rtLightSensor = %d\n", data.rtLightSensor);
    log_printf("rtSN          = %d\n", data.rtSN         );
    log_printf("rtMAC         = %d\n", data.rtMAC        );
    log_printf("rtVersion     = %d\n", data.rtVersion    );
	*rsltkey    = data.rtKey;
    *rsltsensor = data.rtLightSensor;
    *rsltsn     = data.rtSN;
    *rsltmac    = data.rtMAC;
    *rsltver    = data.rtVersion;
    strcpy(sn     , data.SN );
    strcpy(mac    , data.MAC);
    strcpy(version, data.VER);
    return 0;
}

int tnp_test_smt(void *ctxt, char *version, int *rsltsensor, int *rslspkmic, int *rsltver)
{
	TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
	if (!ctxt) return -1;

	if (!context->sock) {
		log_printf("tnp_test_smt failed ! no connection !\n");
		return -1;
	}

	FACTORYTEST_DATA data = {0};
	data.MAGIC		      = SIG_MAGIC;
	data.testLightSensor  = '1';
	data.testMic		  = '1';
	data.testVersion      = '1';
    strcpy(data.VER, version);

	if (send(context->sock, (const char*)&data, sizeof(data), 0) == -1) {
		log_printf("tnp_test_smt send tcp data failed !\n");
		return -1;
	}

	int i;
	for (i=0; i<10; i++) {
		if (context->test_status & TNP_TEST_CANCEL) break;
		if (recv(context->sock, (char*)&data, sizeof(data), 0) == -1) {
			log_printf("tnp_test_smt recv tcp data failed ! retry %d\n", i);
			continue;
		} else {
			break;
		}
	}

	log_printf("tnp_test_smt data received:\n");
	log_printf("rtLightSensor = %d\n", data.rtLightSensor);
	log_printf("rtMic         = %d\n", data.rtMic        );
	log_printf("rtVersion     = %d\n", data.rtVersion    );
	*rsltsensor = data.rtLightSensor;
	*rslspkmic  = data.rtMic;
	*rsltver    = data.rtVersion;
	strcpy(version, data.VER);
	return i == 10 ? -1 : 0;
}

int tnp_test_done(void *ctxt)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return -1;

    if (!context->sock) {
        log_printf("tnp_test_done failed ! no connection !\n");
        return -1;
    }

    FACTORYTEST_DATA data = {0};
    data.MAGIC    = SIG_MAGIC;
    data.exitTest = 'a';

    if (send(context->sock, (const char*)&data, sizeof(data), 0) == -1) {
        log_printf("tnp_test_done send tcp data failed !\n");
        return -1;
    }

    if (recv(context->sock, (char*)&data, sizeof(data), 0) == -1) {
        log_printf("tnp_test_done recv tcp data failed !\n");
        return -1;
    }

    return 0;
}

void tnp_test_cancel(void *ctxt, int cancel)
{
    TNPCONTEXT *context = (TNPCONTEXT*)ctxt;
    if (!ctxt) return;
    if (cancel) {
        context->test_status |= TNP_TEST_CANCEL;
    } else {
        context->test_status &=~TNP_TEST_CANCEL;
    }
}
