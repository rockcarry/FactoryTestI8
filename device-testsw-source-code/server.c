#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <net/if.h>
#include <arpa/inet.h>

// mips-linux-uclibc-gnu-gcc -muclibc -march=mips32r2 sn.c -o sn.o
char MacBuffer[1566];

#define SIG_MAGIC (('N'<<24)+('S'<<16)+('8'<<8)+('I'<<0))

#pragma pack(4)
typedef struct {
    unsigned long MAGIC;

    char SN[16];
    char MAC[16];
    char VERSION[32];

    char testSN;  // pos 0x34
    char testMAC;
    char testSD;
    char testSPK;
    char testMic;
    char testWifi;

    char testCamera;
    char testIR;
    char testIRCut;
    char testLightSensor;

    char testLED;
    char testKey;
    char testVersion;
    char UpdateSW;

    //-----------------------
    char rtSN;
    char rtMAC;
    char rtSD;  //0x44
    char rtSPK;
    char rtMic;
    char rtWifi;

    char rtCamera;
    char rtIR;
    char rtIRCut;
    char rtLightSensor;

    char rtLED;
    char rtKey;
    char rtVersion;
    char rtUpdateSW;

    char exitTest;
} FACTORYTEST_DATA;
#pragma pack()

#pragma pack(4)
typedef struct {
    char MAG[4];
    char IP[32];
    char Port[12];
} NOTIFY_MSG;
#pragma pack()

extern int AgingPlayTest  ();
extern int TEST_SPK_MIC   (int argc, FACTORYTEST_DATA *plFtD);
extern int TEST_PLAY_MUSIC(int onoff, int vol);

FACTORYTEST_DATA lFtD;
static int KeyL = 0;
static int KeyH = 0;
static int g_IrTestAuto    = 1;
static int g_LedTestAuto   = 1;
static int g_StopHeartBeat = 0;

int DrvSaveToFile(char *cName, char *pData, int len)
{
    int fd;
    int ret = 0;

    fd = open(cName, O_RDWR|O_CREAT|O_TRUNC, 0644);
    if (fd < 0) {
        printf("DrvSaveToFile:open %s error\n", cName);
        return -1;
    }
    lseek(fd, 0, SEEK_SET);
    if (len != write(fd, pData, len)) {
        ret = -2;
        printf("DrvSaveToFile write %s failed!\n", cName);
    }
    printf("DrvSaveToFile %s: %d bytes write ok!\n", cName, len);
    close(fd);
    return ret;
}

int DrvReadFileEx(char *cName, char *pData, int *pLen, int iStartKB)
{
    int fd;
    int iSize = 0;
    int iret  = 0;

    if ( /*(pLen == NULL) ||*/(pData == NULL)) {
        printf("DrvReadFile param error!\n");
        return -3;
    }
    if (pLen) iSize = *pLen;

    fd = open(cName, O_RDWR, 777);
    if (fd < 0) {
        printf("DrvReadFile:%s error\n", cName);
        return -2;
    }
//  printf("DrvReadFile fd=%d: %d bytes!\n", fd, iSize);
    if (iSize==0) {
        iSize = lseek(fd, 0, SEEK_END);
    }

    lseek(fd, (iStartKB<<10), SEEK_SET);
    iret = read(fd, pData, iSize);
    if (iSize != iret) {
        if (pLen) *pLen = iret;
        printf("err: DrvReadFile %s: %d(%d) bytes!\n", cName, iret, iSize);
        close(fd);
        return -1;
    } else {
        if (pLen) *pLen = iSize;
        printf("DrvReadFile %s: %d(%d) bytes read ok!\n", cName, iret, iSize);
    }

    close(fd);
    return 0;
}

int CutoffChar(char *str, char c)
{
    int i = 0;
    int n = strlen(str);
    for (i=0; i<n; i++) {
        if (str[i] == c) {
            str[i] = 0;
            return i;
        }
    }
    return 0;
}

int ReplanceMac(char *map, char *MAC, int cnt)
{
    char idx = 0;
    int  i   = 0;

    if (MAC[0] == 'I') MAC[0] = 'F';

    for (idx=0; idx<6; idx++) {
        if (MAC[idx*2] == 0) MAC[idx*2] = '0';
        if (MAC[idx*2+1] == 0) MAC[idx*2+1] = '0';

        map[i++] = MAC[idx*2];
        map[i++] = MAC[idx*2+1];
        i++;
    }

    MAC[12] = MAC[13] = MAC[14] = MAC[15] = 0;

    i = 0;
    printf("After map mac: ");
    for (idx=0; idx<6; idx++) {
        printf("%c%c ", map[idx*3], map[idx*3+1]);
    }
    printf("\r\n");
    return 0;
}

int mymemcmp(char *src, char *dst, int cnt)
{
    int i = 0;
    for (i=0; i<cnt; i++) {
        if (src[i] != dst[i]) {
            printf("mymemcmp mismatch @%d (%02x != %02x)\r\n", i, src[i], dst[i]);
            return 1;
        }
    }
    return 0;
}

int dumpChar(char *name, char *src, int cnt)
{
    int i = 0;
    printf("%s: \r\n", name);
    for (i=0; i<cnt; i++) {
        printf("%c", src[i]);
    }
    return 0;
}

int CheckSD(FACTORYTEST_DATA *plFtD)
{
    // because this APP run from sdcard, so sd is ok here
    plFtD->rtSD = 1;
    return 0;
}

int CheckSNAndMAC(FACTORYTEST_DATA *plFtD)
{
    char pData[1566];
    char SN [32];
    char MAC[32];
    int  iLen = 0;
    int  res  = 0;
    int  i    = 0;

    printf("\r\n\r\nstart CheckSNAndMAC: \r\n");
    CheckSD(plFtD);

    memset(pData, 0 ,sizeof(pData));
    iLen = 32;
    DrvReadFileEx("/dev/mtd5", pData, &iLen, 0);
    memset(SN,0,sizeof(SN));
    memcpy(SN, &pData[13], 15);
    printf("read SN:%s ?= %s \r\n", SN, plFtD->SN);
    if (memcmp(SN, plFtD->SN, 15)) {
        printf(">>> check SN failed!\r\n");
        res |= (1 << 1);
        plFtD->rtSN = 0;
    } else {
        printf(">>> check SN OK!\r\n");
        plFtD->rtSN = 1;
    }
    memcpy(plFtD->SN, SN, 15);

    // 2 check MAC
    for (i=0; i<6; i++) {
        MAC[i*3+0] = plFtD->MAC[i*2+0];
        MAC[i*3+1] = plFtD->MAC[i*2+1];
        MAC[i*3+2] = ' ';
    }
    system("iwpriv wlan0 efuse_get realmap > /tmp/wifi_rel_efuse.map");  // read to file
    iLen = sizeof(MacBuffer);
    DrvReadFileEx("/tmp/wifi_rel_efuse.map", pData, &iLen, 0);
    if (mymemcmp(&pData[0x3e2], MAC, 17)) {
        printf(">>> check MAC failed!\r\n");
        res |= (1 << 0);
        plFtD->rtMAC = 0;
    } else {
        printf(">>> check MAC OK!\r\n");
        plFtD->rtMAC = 1;
    }
    for (i=0; i<6; i++) {
        plFtD->MAC[i*2+0] = MAC[i*3+0];
        plFtD->MAC[i*2+1] = MAC[i*3+1];
    }

    return res;
}

int WriteAndCheckSN(FACTORYTEST_DATA *plFtD)
{
    char pData[1566];
    char SN[32];
    int iLen=0;
    int res=0;

    printf("\r\n\r\nstart WriteAndCheckSN: \r\n");
    CheckSD(plFtD);

    // 1 write SN
    if (plFtD->rtSN == 0) {
        memset(SN, 0, sizeof(SN));
        printf("Write SN:%s \r\n", plFtD->SN);
        sprintf(SN, "000000000000\n%s\n", plFtD->SN);
        printf("rel Write SN:<<%s>> \r\n", SN);
        printf("format SN part!\r\n");
        system("flash_eraseall /dev/mtd5");
        DrvSaveToFile("/dev/mtd5", SN, sizeof(SN));
        sync();
        memset(pData, 0 ,sizeof(pData));
        iLen = 32;
        DrvReadFileEx("/dev/mtd5", pData, &iLen, 0);
        memset(SN,0,sizeof(SN));
        memcpy(SN, &pData[13], 15);
        printf("read SN:%s ?= %s \r\n", SN, plFtD->SN);
        if (memcmp(SN, plFtD->SN, 15)) {
            printf(">>> Write SN failed!\r\n");
            res |= (1 << 1);
            plFtD->rtSN = 0;
        } else {
            printf(">>> Write SN OK!\r\n");
            plFtD->rtSN = 1;
        }
    }

    // 2 write MAC
    if (plFtD->rtMAC == 0) {
        if (plFtD->rtSD) {
            printf("Write MAC (SD):%s \r\n", plFtD->MAC);
            iLen = 0; // all file bytes
            DrvReadFileEx("./wifi_efuse_8189fs.map", MacBuffer, &iLen, 0);
            ReplanceMac((char*)&MacBuffer[0x35f], (char*)&plFtD->MAC, 6);
//          dumpChar("MacBuffer1", MacBuffer, 1566);
            DrvSaveToFile("/tmp/wifi_efuse_8189fs_new.map", MacBuffer, sizeof(MacBuffer));
            sync();
            system("iwpriv wlan0 efuse_file /tmp/wifi_efuse_8189fs_new.map");
        }

        system("iwpriv wlan0 efuse_set wlfk2map");  //write
        system("iwpriv wlan0 efuse_get realmap > /tmp/wifi_rel_efuse.map");  //read to file
        iLen = sizeof(MacBuffer);
        DrvReadFileEx("/tmp/wifi_rel_efuse.map", pData, &iLen, 0);
//      dumpChar("MacBuffer", MacBuffer, 1566);
//      dumpChar("\r\npData", pData, 1566);
        if (mymemcmp(&pData[0x3e2], &MacBuffer[0x35f], 17)) {
            printf(">>> Write MAC failed!\r\n");
            res |= (1 << 0);
            plFtD->rtMAC = 0;
        } else {
            printf(">>> Write MAC OK!\r\n");
            plFtD->rtMAC = 1;
        }
    }

    return res;
}

int ChechVersion(FACTORYTEST_DATA *plFtD)
{
    char ver[64];
    int  iLen = 64;

//  printf("\r\n\r\nstart ChechVersion: \r\n");
    memset(ver, 0, sizeof(ver));
    DrvReadFileEx("/etc/version", ver, &iLen, 0);

    ver[63] = 0;
    CutoffChar(plFtD->VERSION, '\r');
    CutoffChar(plFtD->VERSION, '\n');
    CutoffChar(ver, '\n');
    if (strcmp(plFtD->VERSION, ver)) {
        printf("ver:%s != %s\r\n", ver, plFtD->VERSION);
        plFtD->rtVersion = 0;
        strcpy(plFtD->VERSION, ver);
        return -1;
    } else {
        printf("ver:%s == %s\r\n", ver, plFtD->VERSION);
        strcpy(plFtD->VERSION, ver);
        plFtD->rtVersion = 1;
    }
    return 0;
}

int TestIRCut(FACTORYTEST_DATA *plFtD)
{
//  printf("\r\n\r\nstart TestIRCut: %d\r\n", plFtD->testIRCut);
    if (plFtD->testIRCut == '1') {
        system("echo 0 > /sys/class/gpio/gpio81/value");
        system("echo 1 > /sys/class/gpio/gpio82/value");
    } else if (plFtD->testIRCut == '2') {
        system("echo 1 > /sys/class/gpio/gpio81/value");
        system("echo 0 > /sys/class/gpio/gpio82/value");
    }

    return 0;
}

int EnIR(FACTORYTEST_DATA *plFtD)
{
//  printf("\r\n\r\nstart EnIR: %d\r\n", plFtD->testIR);
    if (plFtD->testIR == '1') {
        system("echo 0 > /sys/class/gpio/gpio61/value");
    } else if (plFtD->testIR == '2') {
        system("echo 1 > /sys/class/gpio/gpio61/value");
    }

    return 0;
}

int TestLightSensor(FACTORYTEST_DATA *plFtD)
{
    static int MaxV = 0;
    static int MinV = 0x400;

    int val = 0;
    int fd;

    const char *cName = "/dev/jz_adc_aux_0";

//  printf("\r\n\r\nstart TestLightSensor: %d\r\n", plFtD->rtLightSensor);
    plFtD->rtLightSensor = 0;

    fd = open(cName, O_RDONLY, 0644);
    if (fd < 0) {
//      printf("TestLightSensor:open %s error\n", cName);
        return -1;
    }
    if (32 != read(fd, &val, 32)) {
//      printf("TestLightSensor read %s failed!\n", cName);
    }
    printf("TestLightSensor %s: read ok :%x!\n", cName, val);
    if (val > MaxV) MaxV = val;
    if (val < MinV) MinV = val;

    if (MaxV > 0x400 && MinV < 0x100) {
//      printf("TestLightSensor OK! [%x, %x]\n", MinV, MaxV);
        plFtD->rtLightSensor = 1;
    }
    if (val > 0x350 && val < 0x550) {
        plFtD->rtLightSensor = 1;
    }

    close(fd);
    return 0;
}

int TestKey(FACTORYTEST_DATA *plFtD)
{
    int val = 0;
    int fd  = 0;
    const char *cName = "/sys/class/gpio/gpio60/value";

    ///printf("\r\n\r\nstart TestKey: \r\n");
    plFtD->rtKey = 0;

    fd = open(cName, O_RDONLY, 0644);
    if (fd < 0) {
//      printf("TestKey:open %s error\n", cName);
        return -1;
    }
    if (1 != read(fd, &val, 1)) {
//      printf("TestKey read %s failed!\n", cName);
    }
    printf("TestKey %s: read ok :%x!\n", cName, val);
    if (val == '0') KeyL = 1;
    if (val == '1') KeyH = 1;

    if (KeyL == 1 && KeyH == 1) {
        printf("TestKey OK! [%x, %x]\n", KeyL, KeyH);
        plFtD->rtKey = 1;
    }

    close(fd);
    return 0;
}

int EnIRCutForAuto(int en)
{
//  printf("\r\n\r\nstart EnIRCut: %d\r\n", en);
    if (en == 1) {
        system("echo 0 > /sys/class/gpio/gpio81/value");
        system("echo 1 > /sys/class/gpio/gpio82/value");
    } else {
        system("echo 1 > /sys/class/gpio/gpio81/value");
        system("echo 0 > /sys/class/gpio/gpio82/value");
    }
    return 0;
}

int EnIRForAuto(int en)
{
//  printf("\r\n\r\nstart EnIR: %d\r\n", en);
    if (en == 1) {
        system("echo 0 > /sys/class/gpio/gpio61/value");
    } else {
        system("echo 1 > /sys/class/gpio/gpio61/value");
    }
    return 0;
}

int GetLightSensor()
{
    int val = 0;
    int fd  = 0;
    const char *cName = "/dev/jz_adc_aux_0";

//  printf("\r\n\r\nGettLightSensor value\r\n");
    fd = open(cName, O_RDONLY, 0644);
    if (fd < 0) {
        printf("GetLightSensor:open %s error\n", cName);
        return -1;
    }
    if (32 != read(fd, &val, 32)) {
//      printf("GetLightSensor read %s failed!\n", cName);
    }
//  printf("GetLightSensor %s: read ok :0x%x!\n", cName, val);

    close(fd);

    if (val > 0x350 && val < 0x550) {
        return 1;
    } else {
        return 0;
    }
}

int getBroadcastIP(char *ip)
{
    int inet_sock;
    struct ifreq ifr = {};
    struct in_addr broadcastip = {};

    inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifr.ifr_name, "wlan0");
    ioctl(inet_sock, SIOCGIFADDR, &ifr);
    broadcastip = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr;
    broadcastip.s_addr |= (255 << 24);
    strcpy(ip , inet_ntoa(broadcastip));
//  printf("BroadcastIP: %s\r\n", ip);
    return ip[0] == '1' ? 0 : -1;
}

static void *heart_beat_thread(void *argv)
{
    struct sockaddr_in servaddr = {0};
    int    sockfd   = 0 ;
    NOTIFY_MSG msg  = {};
    char pData [64] = {};
    char SN    [16] = {};
    char servip[16] = {};
    struct timeval t;
    int    led = 0;
    int    opt = 0;

    // read sn from mtd5
    int len = 32;
    DrvReadFileEx("/dev/mtd5", pData, &len, 0);
    memcpy(SN, &pData[13], 15);

    // init for msg, set magic code, and set SN
    // note !!, the msg.IP is used for store SN
    memcpy(msg.MAG, "NMSG", 4);
    strcpy(msg.IP , SN);

    // open socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // set non-block io
    opt = 1; ioctl(sockfd, FIONBIO, &opt);

    // set udp send & recv timeout
    t.tv_sec  = 1;
    t.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&t, sizeof(t));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&t, sizeof(t));

    // set udp broadcast
    if (argv == NULL) {
        opt = 1; setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));
    }

    // for server addr
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(8989);

    while (!g_StopHeartBeat) {
        if (servaddr.sin_addr.s_addr == 0) {
            if (getBroadcastIP(servip) == 0) {
                if (argv != NULL) {
                    strcpy(servip, argv);
                }
                inet_pton(AF_INET, servip, &servaddr.sin_addr);
                system("echo 1 > /sys/class/gpio/gpio49/value"); // turn on blue led
            }
        }
        if (servaddr.sin_addr.s_addr != 0) {
            if (sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
                printf("sendto failed !\n");
            } else {
                printf("heartbeat: %s\r\n", servip);
            }
        }

        // auto led
        if (g_LedTestAuto) {
            if (servaddr.sin_addr.s_addr) {
                led++; led %= 3;
            } else {
                led++; led %= 2;
            }
            switch (led) {
            case 0:
                system("echo 1 > /sys/class/gpio/gpio50/value");
                system("echo 0 > /sys/class/gpio/gpio72/value");
                system("echo 0 > /sys/class/gpio/gpio49/value");
                break;
            case 1:
                system("echo 0 > /sys/class/gpio/gpio50/value");
                system("echo 1 > /sys/class/gpio/gpio72/value");
                system("echo 0 > /sys/class/gpio/gpio49/value");
                break;
            case 2:
                system("echo 0 > /sys/class/gpio/gpio50/value");
                system("echo 0 > /sys/class/gpio/gpio72/value");
                system("echo 1 > /sys/class/gpio/gpio49/value");
                break;
            }
        }

        // auto IRCut
        if (g_IrTestAuto) {
            if (GetLightSensor() == 1) {
                EnIRForAuto   (1);
                EnIRCutForAuto(1);
            } else {
                EnIRForAuto   (0);
                EnIRCutForAuto(0);
            }
        }

        sleep(1);
    }

    close(sockfd);
    return NULL;
}

static int g_button_test = 0;
static int g_stop_button_monitor = 0;
static void* button_monitor_thread(void *argv)
{
    int fd    = 0;
    int key   = 0;
    int ir    = 0;
    int press = 0;
    const char *cName = "/sys/class/gpio/gpio60/value";

    while (!g_stop_button_monitor) {
        fd = open(cName, O_RDONLY, 0644);
        if (fd) {
            int val = 0;
            if (1 == read(fd, &val, 1)) {
                key = val == '0' ? 0 : 1;
            }
            close(fd);
        }
        if (key == 0) g_button_test |= (1 << 0);
        if (key == 1) g_button_test |= (1 << 1);
        if (key == 0) {
            if (++press == 3) {
                TEST_PLAY_MUSIC(0, 0);
                g_IrTestAuto = 0;
                ir = !ir;
                if (ir) {
                    system("echo 1 > /sys/class/gpio/gpio61/value");
                    system("echo 1 > /sys/class/gpio/gpio81/value");
                    system("echo 0 > /sys/class/gpio/gpio82/value");
                } else {
                    system("echo 0 > /sys/class/gpio/gpio61/value");
                    system("echo 0 > /sys/class/gpio/gpio81/value");
                    system("echo 1 > /sys/class/gpio/gpio82/value");
                }
            }
        } else {
            press = 0;
        }
        usleep(100000);
    }
    return NULL;
}

void StartAgainTest()
{
    int i   = 0;
    int cnt = 0;
    int ir  = 0;
    int led = 0;

    int val = 0;
    int fd  = 0;
    const char *cName = "/sys/class/gpio/gpio60/value";

    while (1) {
        // switch ir & ircut
        ir = !ir;
        EnIRForAuto(ir);
        EnIRCutForAuto(ir);

        // check destory key
        for (cnt=0,i=0; i<20; i++) {
            if (i % 5 == 0) {
                led = !led;
                if (led) {
                    system("echo 0 > /sys/class/gpio/gpio72/value");
                    system("echo 1 > /sys/class/gpio/gpio50/value");
                } else {
                    system("echo 1 > /sys/class/gpio/gpio72/value");
                    system("echo 0 > /sys/class/gpio/gpio50/value");
                }
            }

            fd = open(cName, O_RDONLY, 0644);
            if (fd) {
                if (1 != read(fd, &val, 1)) {
//                  printf("TestKey read %s failed!\n", cName);
                }
                close(fd);
            }
            printf("KEY:0x%x\r\n", val);
            if (val == '0') {
                cnt++;
            }
            if (cnt > 5) {
                // 销毁程序
                printf("test over, destory!!!\r\n");
                system("mv /etc/init.d/rcS.dh /etc/init.d/rcS");
                system("rm -f /etc/apkft/stage");
                system("rm -f /etc/apkft/apkft.sh");
                system("rm -rf /etc/apkft");
                system("sync");
                printf("\r\n server:remove test app & exit.\n\r\n");

                system("echo 0 > /sys/class/gpio/gpio50/value");
                system("echo 0 > /sys/class/gpio/gpio72/value");

                while (1) {
                    // 蓝灯闪烁
                    system("echo 1 > /sys/class/gpio/gpio49/value");
                    usleep(300000); // 300ms
                    system("echo 0 > /sys/class/gpio/gpio49/value");
                    usleep(300000); // 300ms
                }
            }

            sleep(1);
        }
    }
}

#define MAXDATASIZE 100 /* 每次最大数据传输量 */
int main(int argc, char *argv[]) {
    int    sockfd, new_fd; /* 监听 socket: sock_fd, 数据传输 socket: new_fd */
    struct sockaddr_in my_addr   = {0}; /* 本机地址信息 */
    struct sockaddr_in their_addr= {0}; /* 客户地址信息 */
    unsigned int myport, lisnum;
    pthread_t tid_heart_beat;
    pthread_t tid_btn_monitor;
    int  cnt = 0;
    int  iRecvBytes;
    char buf[MAXDATASIZE];

    //++ init gpios
    // led gpio rgb
    system("echo 50 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio50/direction");
    system("echo 72 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio72/direction");
    system("echo 49 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio49/direction");
    system("echo 0 > /sys/class/gpio/gpio49/value"); // turn off blue led

    // button gpio
    system("echo 60 > /sys/class/gpio/export");
    system("echo in > /sys/class/gpio/gpio60/direction");

    // ir gpio
    system("echo 61 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio61/direction");

    // filter gpio
    system("echo 81 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio81/direction");
    system("echo 82 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio82/direction");
    //-- init gpios

    myport = 7838;
    lisnum = 5;
    if (argv[1]) {
        if (strcmp(argv[1], "aging") == 0) {
            TEST_PLAY_MUSIC(1, 20);
            StartAgainTest();
            return 0;
        } else if (strcmp(argv[1], "all") == 0) {
            TEST_PLAY_MUSIC(1, 0);
            pthread_create(&tid_heart_beat , NULL, heart_beat_thread    , NULL);
            pthread_create(&tid_btn_monitor, NULL, button_monitor_thread, NULL);
        } else if (strstr(argv[1], "192.168.") == argv[1]) {
            pthread_create(&tid_heart_beat , NULL, heart_beat_thread    , argv[1]);
            pthread_create(&tid_btn_monitor, NULL, button_monitor_thread, NULL);
        } else {
            pthread_create(&tid_heart_beat , NULL, heart_beat_thread    , NULL);
        }
    }

    memset(&lFtD, 0, sizeof(lFtD));

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    my_addr.sin_family = PF_INET;
    my_addr.sin_port   = htons(myport);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    printf("server:binding...\n");
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    printf("server:listening...\n");
    if (listen(sockfd, lisnum) == -1) {
        perror("listen");
        exit(1);
    }

    while (1) {
        unsigned sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
            printf("accept failed !");
            continue;
        }

        printf("\r\n>>[%d] server: got connection from %s\n", cnt++, inet_ntoa(their_addr.sin_addr));
        while (1) {
            iRecvBytes = recv(new_fd, &lFtD, sizeof(lFtD), 0);
            if (iRecvBytes <= 0) {
                printf("\r\n server: recv connect losed! \r\n");
                close(new_fd);
                break;
            }

            memset(buf, 0, sizeof(buf));
            memcpy(buf, &lFtD.MAGIC, 4);

            // iperf
            if (lFtD.testWifi) {
                lFtD.rtWifi = 1;
            }

            // SN & MAC
            if (lFtD.testSN=='1') {
                WriteAndCheckSN(&lFtD);
            }
            if (lFtD.testSN=='2') {
                CheckSNAndMAC(&lFtD);
            }

            // SPK & MIC
            if (lFtD.testMic) {
                switch (lFtD.testMic) {
                case '1': // auto test spk & mic
                    TEST_SPK_MIC(2, &lFtD);
                    if (lFtD.rtMic) {
                        // 写号 + wifi 吞吐量 + 喇叭咪头，测试通过，下一步功能测试
                        printf("change stage to all\r\n");
                        system("echo all > /etc/apkft/stage");
                    }
                    break;
                case '2':
                    TEST_PLAY_MUSIC(1, 0);
                    break;
                case '3':
                    TEST_PLAY_MUSIC(0, 0);
                    break;
                }
            }

            // LightSensor
            if (lFtD.testLightSensor) {
                TestLightSensor(&lFtD);
            }

            // IRCut
            if (lFtD.testIRCut) {
                TestIRCut(&lFtD);
            }

            // KEY
            if (lFtD.testKey) {
                lFtD.rtKey = (g_button_test == 3);
            }

            // IR LED
            if (lFtD.testIR) {
                g_IrTestAuto = 0;
                printf("\r\nserver:testIR:%d dis auto IR test\n\r\n", lFtD.testIR);
                EnIR(&lFtD);
            }

            // Version
            if (lFtD.testVersion) {
                ChechVersion(&lFtD);
            }

            printf("\r\nserver:sending...mic:%d, key=%d, exitTest=%d\n\r\n", lFtD.rtMic, lFtD.rtKey, lFtD.exitTest);
            if (send(new_fd, &lFtD, sizeof(lFtD), 0) <= 0) {
                printf("\r\nserver:send connect losed.\n\r\n");
                close(new_fd);
                break;
            }

            if (lFtD.exitTest == 'f') {
                // 下一步进入功能测试
                system("echo all > /etc/apkft/stage");
                system("sync");
            } else if (lFtD.exitTest == 'a') {
                // 下一步进入老化测试
                system("echo aging > /etc/apkft/stage");
                system("sync");
            }
        }
        close(new_fd);
    }

    g_StopHeartBeat       = 1;
    g_stop_button_monitor = 1;
    pthread_join(tid_heart_beat , NULL);
    pthread_join(tid_btn_monitor, NULL);
    TEST_PLAY_MUSIC(0, 0);
    printf("\r\nserver:EXIT.\n\r\n");
    return 0;
}
