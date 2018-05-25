#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <netdb.h>

#include <net/if.h>
#include <arpa/inet.h>

//mips-linux-uclibc-gnu-gcc -muclibc -march=mips32r2 sn.c -o sn.o
char MacBuffer[1566];
char host[32];
char LocalIP[32];

#define SIG_MAGIC ('N'<<24)+('S'<<16)+('8'<<8)+('I'<<0)

#pragma pack(4)
typedef struct _FACTORYTEST_DATA
{
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
typedef struct _NOTIFY_MSG
{
    char MAG[4];
    char IP[32];
    char Port[12];
} NOTIFY_MSG;
#pragma pack()

FACTORYTEST_DATA lFtD;
static int KeyL=0;
static int KeyH=0;
static int g_IrTestAuto=1;
static int g_LedTestAuto=1;

int DrvSaveToFile(char* cName, char* pData, int len)
{
    int ret = 0;
    int fd;

    fd = open(cName, O_RDWR|O_CREAT|O_TRUNC, 0644);
    if (fd < 0)
    {
        printf("DrvSaveToFile:open %s error\n", cName);
        return -1;
    }
    lseek(fd, 0, SEEK_SET);
    if (len != write(fd, pData, len))
    {
        ret = -2;
        printf("DrvSaveToFile write %s failed!\n", cName);
    }
    printf("DrvSaveToFile %s: %d bytes write ok!\n", cName, len);
    close(fd);

    return ret;
}

int DrvReadFileEx(char* cName, char* pData, int* pLen, int iStartKB)
{
    int fd;
    int iSize = 0;
    int iret=0;

    if ( /*(pLen == NULL) ||*/(pData == NULL))
    {
        printf("DrvReadFile param error!\n");
        return -3;
    }
    if (pLen)    iSize=*pLen;

    fd = open(cName, O_RDWR, 777);
    if (fd < 0)
    {
        printf("DrvReadFile:%s error\n", cName);
        return -2;
    }
    printf("DrvReadFile fd=%d: %d bytes!\n", fd, iSize);
    if (iSize==0)
        iSize = lseek(fd, 0, SEEK_END);

    lseek(fd, (iStartKB<<10), SEEK_SET);
    iret = read(fd, pData, iSize);
    if (iSize != iret)
    {
        if (pLen) *pLen = iret;
        printf("err: DrvReadFile %s: %d(%d) bytes!\n", cName, iret, iSize);
        close(fd);
        return -1;
    }
    else
    {
        if (pLen) *pLen = iSize;
        printf("DrvReadFile %s: %d(%d) bytes read ok!\n", cName, iret, iSize);
    }

    close(fd);
    return 0;
}

int NetSetup()
{
    //setup use shell
    /*
    system("echo connect wifi...");
    system("/mnt/sdcard/opt/network/wifi_cmd.sh connect Apical001_2.4G apicalgood WPA2");
    system("sleep 5");
    system("ifconfig wlan0 192.168.2.199 up");
    */

    return 0;
}

int CheckSD(FACTORYTEST_DATA* plFtD)
{
    //because this APP run from sdcard, so sd is ok here
    plFtD->rtSD = 1;

    return 0;
}

char MAP_CHAR_TO_HEX(char c)
{
    switch(c)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return c-'0';

    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
        return c-'A';

    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
        return c-'a';
    }

    return 'X';
}

int ReplanceMac(char* map, char* MAC, int cnt)
{
    char idx=0;
    int i=0;

    if (MAC[0] == 'I') MAC[0] = 'F';

    for (idx=0; idx<6; idx++)
    {
        if (MAC[idx*2] == 0) MAC[idx*2] = '0';
        if (MAC[idx*2+1] == 0) MAC[idx*2+1] = '0';

        map[i++] = MAC[idx*2];
        map[i++] = MAC[idx*2+1];
        i++;
    }

    MAC[12] = MAC[13] = MAC[14] = MAC[15] = 0;

    i=0;
    printf("After map mac: ");
    for (idx=0; idx<6; idx++)
    {
        printf("%c%c ", map[idx*3], map[idx*3+1]);
    }
    printf("\r\n");

    return 0;
}

int mymemcmp(char* src, char* dst, int cnt)
{
    int i=0;
    for (i=0; i<cnt; i++)
    {
        if (src[i] != dst[i])
        {
            printf("mymemcmp mismatch @%d (%02x != %02x)\r\n", i, src[i], dst[i]);
            return 1;
        }
    }

    return 0;
}

int dumpChar(char* name, char* src, int cnt)
{
    int i=0;

    printf("%s: \r\n", name);
    for (i=0; i<cnt; i++)
    {
        printf("%c", src[i]);
    }

    return 0;
}


int CheckSNAndMAC(FACTORYTEST_DATA* plFtD)
{
    char pData[1566];
    char SN[32];
    char MAC[32];
    int iLen=0;
    int res=0;
    int i=0;

    printf("\r\n\r\nstart CheckSNAndMAC: \r\n");
    CheckSD(plFtD);

    memset(pData, 0 ,sizeof(pData));
    iLen = 32;
    DrvReadFileEx("/dev/mtd5", pData, &iLen, 0);
    memset(SN,0,sizeof(SN));
    memcpy(SN, &pData[13], 15);
    printf("read SN:%s ?= %s \r\n", SN, plFtD->SN);
    if (memcmp(SN, plFtD->SN, 15))
    {
        printf(">>> check SN failed!\r\n");
        res += 2;
        plFtD->rtSN = 0;
    }
    else
    {
        printf(">>> check SN OK!\r\n");
        plFtD->rtSN = 1;
    }

    //2 check MAC
    for (i=0; i<6; i++)
    {
        MAC[i*3] = plFtD->MAC[i*2];
        MAC[i*3+1] = plFtD->MAC[i*2+1];
        MAC[i*3+2] = ' ';
    }
    system("iwpriv wlan0 efuse_get realmap > /tmp/wifi_rel_efuse.map");  //read to file
    iLen = sizeof(MacBuffer);
    DrvReadFileEx("/tmp/wifi_rel_efuse.map", pData, &iLen, 0);
    if (mymemcmp(&pData[0x3e2], &MAC[0], 17))
    {
        printf(">>> check MAC failed!\r\n");
        res += 1;
        plFtD->rtMAC = 0;
    }
    else
    {
        printf(">>> check MAC OK!\r\n");
        plFtD->rtMAC = 1;
    }

    return res;
}

int WriteAndCheckSN(FACTORYTEST_DATA* plFtD)
{
    char pData[1566];
    char SN[32];
    int iLen=0;
    int res=0;

    printf("\r\n\r\nstart WriteAndCheckSN: \r\n");
    CheckSD(plFtD);

    //1 write SN
    if (plFtD->rtSN == 0)
    {
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
        if (memcmp(SN, plFtD->SN, 15))
        {
            printf(">>> Write SN failed!\r\n");
            res += 2;
            plFtD->rtSN = 0;
        }
        else
        {
            printf(">>> Write SN OK!\r\n");
            plFtD->rtSN = 1;
        }
    }

    //2 write MAC
    if (plFtD->rtMAC == 0)
    {
        if (plFtD->rtSD)
        {
            printf("Write MAC (SD):%s \r\n", plFtD->MAC);
            iLen=0; //all file bytes
            DrvReadFileEx("./wifi_efuse_8189fs.map", MacBuffer, &iLen, 0);
            ReplanceMac((char*)&MacBuffer[0x35f], (char*)&plFtD->MAC, 6);
            //dumpChar("MacBuffer1", MacBuffer, 1566);
            DrvSaveToFile("/tmp/wifi_efuse_8189fs_new.map", MacBuffer, sizeof(MacBuffer));
            sync();
            system("iwpriv wlan0 efuse_file /tmp/wifi_efuse_8189fs_new.map");
        }

        system("iwpriv wlan0 efuse_set wlfk2map");  //write
        system("iwpriv wlan0 efuse_get realmap > /tmp/wifi_rel_efuse.map");  //read to file
        iLen = sizeof(MacBuffer);
        DrvReadFileEx("/tmp/wifi_rel_efuse.map", pData, &iLen, 0);
        //dumpChar("MacBuffer", MacBuffer, 1566);
        //dumpChar("\r\npData", pData, 1566);
        if (mymemcmp(&pData[0x3e2], &MacBuffer[0x35f], 17))
        {
            printf(">>> Write MAC failed!\r\n");
            res += 1;
            plFtD->rtMAC = 0;
        }
        else
        {
            printf(">>> Write MAC OK!\r\n");
            plFtD->rtMAC = 1;
        }
    }

    return res;
}

int CutoffChar(char* str, char c)
{
    int i = 0;
    int n=strlen(str);

    for (i=0; i<n; i++)
    {
        if (str[i] == c)
        {
            str[i] = 0;
            return i;
        }
    }

    return 0;
}

int ChechVersion(FACTORYTEST_DATA* plFtD)
{
    char ver[64];
    int iLen=64;

    ///printf("\r\n\r\nstart ChechVersion: \r\n");
    memset(ver, 0, sizeof(ver));
    DrvReadFileEx("/etc/version", ver, &iLen, 0);

    ver[63] = 0;

    CutoffChar(plFtD->VERSION, '\r');
    CutoffChar(plFtD->VERSION, '\n');
    CutoffChar(ver, '\n');
    if (strcmp(plFtD->VERSION, ver))
    {
        printf("ver:%s != %s\r\n", ver, plFtD->VERSION);
        plFtD->rtVersion = 0;
        return -1;
    }
    else
    {
        printf("ver:%s == %s\r\n", ver, plFtD->VERSION);
        strcpy(plFtD->VERSION, ver);
        plFtD->rtVersion = 1;
    }

    return 0;
}

int TestIRCut(FACTORYTEST_DATA* plFtD)
{
    ///printf("\r\n\r\nstart TestIRCut: %d\r\n", plFtD->testIRCut);

    system("echo 81 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio81/direction");
    system("echo 82 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio82/direction");
    if (plFtD->testIRCut == '1')
    {
        system("echo 0 > /sys/class/gpio/gpio81/value");
        system("echo 1 > /sys/class/gpio/gpio82/value");
    }
    else if (plFtD->testIRCut == '2')
    {
        system("echo 1 > /sys/class/gpio/gpio81/value");
        system("echo 0 > /sys/class/gpio/gpio82/value");
    }

    return 0;
}

int EnIR(FACTORYTEST_DATA* plFtD)
{
    ///printf("\r\n\r\nstart EnIR: %d\r\n", plFtD->testIR);

    system("echo 61 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio61/direction");
    if (plFtD->testIR == '1')
    {
        system("echo 0 > /sys/class/gpio/gpio61/value");
    }
    else if (plFtD->testIR == '2')
    {
        system("echo 1 > /sys/class/gpio/gpio61/value");
    }

    return 0;
}

int EnLED(FACTORYTEST_DATA* plFtD)
{
    ///printf("\r\n\r\nstart EnLED: %d\r\n", plFtD->testLED);
    system("echo 49 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio49/direction");
    system("echo 50 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio50/direction");
    if (plFtD->testLED == '1')
    {
        //BLUE  ON
        system("echo 1 > /sys/class/gpio/gpio49/value");

        // RED OFF
        system("echo 0 > /sys/class/gpio/gpio50/value");
    }
    else if (plFtD->testLED == '2')
    {
        //RED   ON
        system("echo 1 > /sys/class/gpio/gpio50/value");

        //BLUE OFF
        system("echo 0 > /sys/class/gpio/gpio49/value");
    }
    else if (plFtD->testLED == '3')
    {
        //RED & BLUE  = purple
        system("echo 1 > /sys/class/gpio/gpio49/value");
        system("echo 1 > /sys/class/gpio/gpio50/value");
    }
    else if (plFtD->testLED == '4')
    {
        //GREEN ON

        //BLUE OFF
        system("echo 0 > /sys/class/gpio/gpio50/value");
        system("echo 0 > /sys/class/gpio/gpio49/value");
    }
    else if (plFtD->testLED == '8')
    {
        system("echo 1 > /sys/class/gpio/gpio49/value");
        system("echo 1 > /sys/class/gpio/gpio50/value");
    }
    else if (plFtD->testLED == '9')
    {
        system("echo 0 > /sys/class/gpio/gpio49/value");
        system("echo 0 > /sys/class/gpio/gpio50/value");
    }
    plFtD->rtLED = 1;

    return 0;
}

int EnLEDAuto(int on)
{
    ///printf("\r\n\r\nstart EnLEDAuto: %d\r\n", on);

    if (on == 1)
    {
        system("echo 0 > /sys/class/gpio/gpio72/value");
        system("echo 1 > /sys/class/gpio/gpio50/value");
    }
    else
    {
        system("echo 1 > /sys/class/gpio/gpio72/value");
        system("echo 0 > /sys/class/gpio/gpio50/value");
    }

    return 0;
}

int TestLightSensor(FACTORYTEST_DATA* plFtD)
{
    static int MaxV=0;
    static int MinV=0x400;

    int val = 0;
    int fd;

    const char* cName="/dev/jz_adc_aux_0";

    ///printf("\r\n\r\nstart TestLightSensor: %d\r\n", plFtD->rtLightSensor);
    plFtD->rtLightSensor = 0;

    fd = open(cName, O_RDONLY, 0644);
    if (fd < 0)
    {
        ///printf("TestLightSensor:open %s error\n", cName);
        return -1;
    }
    if (32 != read(fd, &val, 32))
    {
        ///printf("TestLightSensor read %s failed!\n", cName);
    }
    printf("TestLightSensor %s: read ok :%x!\n", cName, val);
    if (val>MaxV)
        MaxV=val;
    if (val<MinV)
        MinV=val;

    if ((MaxV>0x400) && (MinV<0x100))
    {
        //printf("TestLightSensor OK! [%x, %x]\n", MinV, MaxV);
        plFtD->rtLightSensor = 1;
    }
    if ((val>0x350) && (val<0x550))
        plFtD->rtLightSensor = 1;

    close(fd);

    return 0;
}

int TestKey(FACTORYTEST_DATA* plFtD)
{
    int val = 0;
    int fd;

    const char* cName="/sys/class/gpio/gpio60/value";

    system("echo 60 > /sys/class/gpio/export");
    system("echo in > /sys/class/gpio/gpio60/direction");

    ///printf("\r\n\r\nstart TestKey: \r\n");
    plFtD->rtKey = 0;

    fd = open(cName, O_RDONLY, 0644);
    if (fd < 0)
    {
        ///printf("TestKey:open %s error\n", cName);
        return -1;
    }
    if (1 != read(fd, &val, 1))
    {
        ///printf("TestKey read %s failed!\n", cName);
    }
    printf("TestKey %s: read ok :%x!\n", cName, val);
    if (val=='0')
        KeyL=1;
    if (val=='1')
        KeyH=1;

    if ((KeyL==1) && (KeyH==1))
    {
        printf("TestKey OK! [%x, %x]\n", KeyL, KeyH);
        plFtD->rtKey = 1;
    }

    close(fd);

    return 0;
}

int EnIRCutForAuto(int en)
{
    printf("\r\n\r\nstart EnIRCut: %d\r\n", en);

    if (en == 1)
    {
        system("echo 0 > /sys/class/gpio/gpio81/value");
        system("echo 1 > /sys/class/gpio/gpio82/value");
    }
    else
    {
        system("echo 1 > /sys/class/gpio/gpio81/value");
        system("echo 0 > /sys/class/gpio/gpio82/value");
    }

    return 0;
}

int EnIRForAuto(int en)
{
    printf("\r\n\r\nstart EnIR: %d\r\n", en);

    if (en == 1)
    {
        system("echo 0 > /sys/class/gpio/gpio61/value");
    }
    else
    {
        system("echo 1 > /sys/class/gpio/gpio61/value");
    }

    return 0;
}

int GetLightSensor()
{
    int val = 0;
    int fd;

    const char* cName="/dev/jz_adc_aux_0";

    printf("\r\n\r\nGettLightSensor value\r\n");

    fd = open(cName, O_RDONLY, 0644);
    if (fd < 0)
    {
        printf("GetLightSensor:open %s error\n", cName);
        return -1;
    }
    if (32 != read(fd, &val, 32))
    {
        printf("GetLightSensor read %s failed!\n", cName);
    }
    printf("GetLightSensor %s: read ok :0x%x!\n", cName, val);

    close(fd);

    if ((val>0x350) && (val<0x550))
        return 1;

    return 0;
}

int getLocalIP(char* pLocIP)
{
    int inet_sock;
    struct ifreq ifr;
    //char pLocIP[32]={NULL};

    inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifr.ifr_name, "wlan0");
    //strcpy(ifr.ifr_name, "eth0");
    ioctl(inet_sock, SIOCGIFADDR, &ifr);
    strcpy(pLocIP, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    printf("LocIP: %s\r\n", pLocIP);
    if (pLocIP[0] == '1')
        return 0;

    printf("getLocalIP error!\r\n");
    return -1;
}

int NotifyHost(char* pHostIP, char* pLocIP)
{
    struct sockaddr_in servaddr;
    int sockfd, n;
    NOTIFY_MSG rmsg;
    NOTIFY_MSG msg;
    int cnt=0;
    struct timeval t;
    int Led=0;
    int nb=1;

    char pData[64];
    char SN[16];
    int iLen=0;
    memset(pData, 0 ,sizeof(pData));
    iLen = 32;
    DrvReadFileEx("/dev/mtd5", pData, &iLen, 0);
    memset(SN,0,sizeof(SN));
    memcpy(SN, &pData[13], 15);

    memset(&rmsg, 0, sizeof(rmsg));
    memset(&msg, 0, sizeof(msg));
    memcpy(msg.MAG, "NMSG", 4);
    strcpy(msg.IP, SN);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
#if 1
    inet_pton(AF_INET, pHostIP, &servaddr.sin_addr);
    servaddr.sin_port = htons(8989);
#else
    inet_pton(AF_INET, "192.168.0.201", &servaddr.sin_addr);
    servaddr.sin_port = htons(8989);
#endif

    ioctl(sockfd, FIONBIO, &nb);
    t.tv_sec = 1;   //设置2秒
    t.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET,SO_SNDTIMEO, (char *)&t,sizeof(t));
    setsockopt(sockfd, SOL_SOCKET,SO_RCVTIMEO, (char *)&t,sizeof(t));
    setsockopt(sockfd, SOL_SOCKET,SO_BROADCAST, (char *)&nb,sizeof(nb));

    system("echo 72 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio72/direction");
    system("echo 50 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio50/direction");
    system("echo 49 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio49/direction");
    system("echo 0 > /sys/class/gpio/gpio49/value");

    system("echo 81 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio81/direction");
    system("echo 82 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio82/direction");

    system("echo 61 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio61/direction");

    while(1)
    {
        printf("notify [%d]: host:%s, Loc:%s\r\n", cnt, pHostIP, pLocIP);
        n = sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
        if (n == -1) {
            printf("sendto failed !\n");
        }
#if 0
        /////printf("notify recv: host:%s\r\n", pHostIP);
        //n = recvfrom(sockfd, &rmsg, sizeof(rmsg), 0, NULL, 0);
        if (n == -1)
        {
            ///printf("recvfrom error!\n");
        }
        else
        {
            /////printf("recv:%s ?= %s\r\n", rmsg.MAG, msg.MAG);
            if (memcmp(rmsg.MAG, msg.MAG, 4)==0)
            {
                ///printf("notify ok: host:%s\r\n", rmsg.IP);
                //break;
            }
        }
        memset(&rmsg, 0, sizeof(rmsg));
        //TestKey(&lFtD);
#endif

        if (g_LedTestAuto)
        {
            Led = !Led;
            EnLEDAuto(Led);
        }

        //auto IRCut
        printf("\r\n server:testIR:%d \n\r\n", lFtD.testIR);
        if (g_IrTestAuto)
        {
            if (GetLightSensor()==1)
            {
                EnIRForAuto(1);
                EnIRCutForAuto(1);
                //changeSensorDayNightMode(1);
            }
            else
            {
                EnIRForAuto(0);
                EnIRCutForAuto(0);
                //changeSensorDayNightMode(0);
            }
        }

        sleep(1);
    }
    close(sockfd);
    return 0;
}

void printld(FACTORYTEST_DATA lFtD, char* str)
{
    char buf[256];
    int cnt = 0;

    memcpy(buf, &lFtD.MAGIC, 64+32);
    printf("%s:\r\n", str);
    for (cnt=0; cnt<4; cnt++)
        printf("%02x ", buf[cnt]);
    printf("\r\n");
    for (; cnt<(4+16); cnt++)
        printf("%02x ", buf[cnt]);
    printf("\r\n");
    for (; cnt<(4+16+16); cnt++)
        printf("%02x ", buf[cnt]&0xFF);
    printf("\r\n");
    for (; cnt<(4+16+16+32); cnt++)
        printf("%02x ", buf[cnt]&0xFF);
    printf("\r\n");
    for (; cnt<(4+16+16+32+14); cnt++)
        printf("%02x ", buf[cnt]&0xFF);
    printf("\r\n");
    for (; cnt<(4+16+16+32+14+15); cnt++)
        printf("%02x ", buf[cnt]&0xFF);
    printf("\r\n");
}

static void *Udp_Thread(void *argv)
{
    NotifyHost(host, LocalIP);
    return NULL;
}

extern int AgingPlayTest();
int gDestory = 0;
static void *led_Thread(void *argv)
{
    while(!gDestory)
    {
        AgingPlayTest();
        EnLEDAuto(1);
        sleep(5);
        AgingPlayTest();
        EnLEDAuto(0);
        sleep(5);
    }
    return NULL;
}
void AgainTest()
{
    int i=0;
    int cnt=0;
    int ir=0;
    pthread_t tid_led;

    int val = 0;
    int fd;
    const char* cName="/sys/class/gpio/gpio60/value";
    system("echo 60 > /sys/class/gpio/export");
    system("echo in > /sys/class/gpio/gpio60/direction");

    //run play sound thread

    //run switch led
    //led bgr
    system("echo 72 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio72/direction");
    system("echo 50 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio50/direction");
    system("echo 49 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio49/direction");
    system("echo 0 > /sys/class/gpio/gpio49/value");
    pthread_create(&tid_led, NULL, led_Thread, NULL);

    //ircut
    system("echo 81 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio81/direction");
    system("echo 82 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio82/direction");
    //ir
    system("echo 61 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio61/direction");

    gDestory = 0;
    while(1)
    {
        //switch ir & ircut
        EnIRForAuto(ir);
        EnIRCutForAuto(ir);
        ir = !ir;

        //check destory key
        cnt = 0;
        for (i=0; i<20; i++)
        {
            sleep(1);
            fd = open(cName, O_RDONLY, 0644);
            if (fd)
            {
                if (1 != read(fd, &val, 1))
                {
                    ///printf("TestKey read %s failed!\n", cName);
                }
                close(fd);
            }
            printf("KEY:0x%x\r\n", val);
            if (val=='0')
                cnt++;

            if (cnt>5)
            {
                //销毁程序
                printf("Test over, destory!!!\r\n");
                system("mv /etc/init.d/rcS.dh /etc/init.d/rcS");
                system("rm -f /etc/apkft/stage");
                system("rm -f /etc/apkft/apkft.sh");
                system("rm -rf /etc/apkft");
                system("sync");
                printf("\r\n server:remove test app & exit.\n\r\n");

                system("echo 0 > /sys/class/gpio/gpio50/value");
                system("echo 0 > /sys/class/gpio/gpio72/value");

                gDestory = 1;
                while(1)
                {
                    //蓝灯闪烁
                    system("echo 1 > /sys/class/gpio/gpio49/value");
                    usleep(300000); //300ms
                    system("echo 0 > /sys/class/gpio/gpio49/value");
                    usleep(300000); //300ms
                }
            }
        }
    }
}

int TEST_SPK_MIC(int argc, FACTORYTEST_DATA* plFtD);
int TEST_SPK_ONLY(int onoff);
#define MAXDATASIZE 100 /*每次最大数据传输量 */
int  main(int argc, char *argv[])
{
    int sockfd,new_fd; /* 监听socket: sock_fd,数据传输socket: new_fd */
    struct sockaddr_in my_addr; /* 本机地址信息 */
    struct sockaddr_in their_addr; /* 客户地址信息 */
    unsigned int sin_size, myport, lisnum;
    int i=0;

//  FACTORYTEST_DATA lFtDTmp;
    int iRecvBytes;
    char buf[MAXDATASIZE];
    int cnt=0;
    pthread_t tid_udp;

#if 0
    if (argv[1])  myport = atoi(argv[1]);
    else myport = 7838;
    if (argv[2])  lisnum = atoi(argv[2]);
    else lisnum = 2;
#else
    myport = 7838;
    lisnum = 5;
    if (argv[1])
    {
        if (strcmp(argv[1], "aging") == 0)
        {
            AgainTest();
        }

        strcpy(host, argv[1]);
        ///printf("host:%s\r\n", host);

        if (0 == getLocalIP(LocalIP))
        {
            pthread_create(&tid_udp, NULL, Udp_Thread, NULL);
        }
    }
#endif

    memset(&lFtD, 0, sizeof(lFtD));
    /*
    lFtD.MAGIC = SIG_MAGIC;
    memset(&lFtD.SN, 0, sizeof(lFtD.SN));
    memcpy(&lFtD.SN, "623456789098769", 15);
    memset(&lFtD.MAC, 0, sizeof(lFtD.SN));
    memcpy(&lFtD.MAC, "502143678909", 12);
    //WriteAndCheckSN(&lFtD);
    */

    //SOCK_NONBLOCK
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

#if 0
    {
        int bLinger = 1;
        struct timeval t;
        setsockopt(new_fd, SOL_SOCKET,SO_LINGER,(const char*)&bLinger,sizeof(int));

        ioctl(new_fd, FIONBIO, &bLinger);

        t.tv_sec = 2;   //设置2秒
        t.tv_usec = 0;
        setsockopt(new_fd, SOL_SOCKET,SO_SNDTIMEO, (char *)&t,sizeof(t));
        setsockopt(new_fd, SOL_SOCKET,SO_RCVTIMEO, (char *)&t,sizeof(t));
    }
#endif

    my_addr.sin_family=PF_INET;
    my_addr.sin_port=htons(myport);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(my_addr.sin_zero), 0);
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
    while(1) {
        RE_CONNECT:
        sin_size = sizeof(struct sockaddr_in);
        printf("server:accepting...\n");
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
            perror("accept");
            continue;
        }

        {
            int bLinger = 1;
            struct timeval t;
            setsockopt(new_fd, SOL_SOCKET,SO_LINGER,(const char*)&bLinger,sizeof(int));

            t.tv_sec = 2;   //设置2秒
            t.tv_usec = 0;
            setsockopt(new_fd, SOL_SOCKET,SO_SNDTIMEO, (char *)&t,sizeof(t));
            setsockopt(new_fd, SOL_SOCKET,SO_RCVTIMEO, (char *)&t,sizeof(t));
        }

        printf("\r\n>>[%d] server: got connection from %s\n",cnt++, inet_ntoa(their_addr.sin_addr));
        //if (!fork()) { /* 子进程代码段 */
        if (1)
        {
            ///printf("fork ok!\r\n");
            WAIT_DATA:
            printf("recving...%d\r\n", sizeof(lFtD));
            if ( (iRecvBytes=recv(new_fd, &lFtD, sizeof(lFtD), 0)) == -1)
            {
                printf("\r\nserver:recv error %x\n\r\n", errno);
                if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    printf("\r\n recv %x [%x %x %x]! \r\n", errno, EINTR, EWOULDBLOCK, EAGAIN);
                    goto WAIT_DATA;
                }
            }
            if (iRecvBytes == 0)
            {
                printf("\r\n server: recv connect losed! \r\n");
                continue;
            }

            printld(lFtD, "recv");

            memset(buf, 0, sizeof(buf));
            memcpy(buf, &lFtD.MAGIC, 4);

#if 0
            /////printf("\r\nMAGIC----------[%x, %x]\r\n", &lFtD, &lFtD.MAGIC);
            printf("\r\nserver:recv: MAGIC:%s (0x%08x)\r\n", buf, lFtD.MAGIC);
            for (cnt=0; cnt<16; cnt++)
                printf("%02x ", buf[cnt]);
            /////printf("\r\n\r\nSN----------[%x %x]\r\n", &lFtD, &lFtD.SN);
            printf("\r\n\r\nserver:recv: SN:%s\r\n", lFtD.SN);
            for (cnt=0; cnt<16; cnt++)
                printf("%02x ", lFtD.SN[cnt]);
            /////printf("\r\nMAC----------[%x %x]\r\n", &lFtD, &lFtD.MAC);
            printf("\r\n\r\nserver:recv: MAC:%s\r\n", lFtD.MAC);
            for (cnt=0; cnt<16; cnt++)
                printf("%02x ", lFtD.MAC[cnt]);
#endif

            //iperf
            if (lFtD.testWifi)
            {
                lFtD.rtWifi = 1;
            }

            //SN & MAC
            if (lFtD.testSN=='1')
            {
                CheckSNAndMAC(&lFtD);
                WriteAndCheckSN(&lFtD);
            }
            if (lFtD.testSN=='2')
                CheckSNAndMAC(&lFtD);

            //SPK & MAC
            if (lFtD.testMic)
            {
                switch (lFtD.testMic) {
                case '1': // auto test spk & mic
                    TEST_SPK_MIC(2, &lFtD);
                    if (lFtD.rtMic) {
                        // wifi 吞吐量测试通过，下一步功能测试
                        printf("change stage to all\r\n");
                        system("echo all > /etc/apkft/stage");
                    }
                    break;
                case '2':
                    TEST_SPK_ONLY(1);
                    break;
                case '3':
                    TEST_SPK_ONLY(0);
                    break;
                }
            }

            //LightSensor
            if (lFtD.testLightSensor)
                TestLightSensor(&lFtD);

            //IRCut
            if (lFtD.testIRCut)
            {
                TestIRCut(&lFtD);
            }

            //KEY
            //if (lFtD.testKey)
            {
                TestKey(&lFtD);
            }

            //LED
            if (lFtD.testLED)
            {
                g_LedTestAuto = 0;
                EnLED(&lFtD);
            }

            //IR LED
            if (lFtD.testIR)
            {
                g_IrTestAuto = 0;
                printf("\r\nserver:testIR:%d dis auto IR test\n\r\n", lFtD.testIR);
                EnIR(&lFtD);
            }

            //Version
            if (lFtD.testVersion)
                ChechVersion(&lFtD);

            printf("\r\nserver:sending...mic:%d , key=%d, exitTest=%d\n\r\n", lFtD.rtMic, lFtD.rtKey, lFtD.exitTest);
            for (i=0; i<3; i++)
            {
                printld(lFtD, "befor send");
                int isend = send(new_fd, &lFtD, sizeof(lFtD), 0);
                if (isend == -1) {
                    printf("\r\nserver:send error :%x.\n\r\n", errno);
                    if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                    {
                        printf("\r\n send %x [%x %x %x]! \r\n", errno, EINTR, EWOULDBLOCK, EAGAIN);
                        continue;
                    }
                    else
                        goto RE_CONNECT;
                }
                else if (isend==0)
                {
                    printf("\r\nserver:send connect losed.\n\r\n");
                    goto RE_CONNECT;
                }
                else
                    break;
            }
            if (lFtD.exitTest==0)
                goto WAIT_DATA;
            else if (lFtD.exitTest == 'c')
            {
                //camera 测试通过，下一步测试wifi吞吐量
                system("echo iperf > /etc/apkft/stage");
                system("sync");
            }
            else if (lFtD.exitTest == 'i')
            {
                //wifi吞吐量测试通过，下一步功能测试
                system("echo all > /etc/apkft/stage");
                system("sync");
            }
            else if (lFtD.exitTest == 'a')
            {
                //功能测试通过，进入老化测试
                system("echo aging > /etc/apkft/stage");
                system("sync");
            }
            else if (lFtD.exitTest == 'b')
            {
                //功能测试通过，销毁测试程序
                printf("Test over, destory!!!\r\n");
                system("mv /etc/init.d/rcS.dh /etc/init.d/rcS");
                system("rm -f /etc/apkft/stage");
                system("rm -f /etc/apkft/apkft.sh");
                system("rm -rf /etc/apkft");
                system("sync");
                printf("\r\n server:remove test app & exit.\n\r\n");

                //system("reboot");
                while(1) sleep(1);
                return 0;
            }
        }

        close(new_fd); /*父进程不再需要该socket*/

        printf("\r\nserver:wait exit.\n\r\n");
        waitpid(-1,NULL,0x01L);/*等待子进程结束，清除子进程所占用资源*/
    }

    printf("\r\nserver:EXIT.\n\r\n");
}
