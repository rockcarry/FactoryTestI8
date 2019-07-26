// FactoryTestI8SMTDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "FactoryTestI8SMT.h"
#include "FactoryTestI8SMTDlg.h"
#include "TestNetProtocol.h"
#include "fanplayer.h"
#include "log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static void get_app_dir(char *path, int size)
{
    HMODULE handle = GetModuleHandle(NULL);
    GetModuleFileNameA(handle, path, size);
    char  *str = path + strlen(path);
    while (*--str != '\\');
    *str = '\0';
}

static void parse_params(const char *str, const char *key, char *val)
{
    char *p = (char*)strstr(str, key);
    int   i;

    if (!p) return;
    p += strlen(key);
    if (*p == '\0') return;

    while (1) {
        if (*p != ' ' && *p != '=' && *p != ':') break;
        else p++;
    }

    for (i=0; i<MAX_PATH; i++) {
        if (*p == ',' || *p == ';' || *p == '\r' || *p == '\n' || *p == '\0') {
            val[i] = '\0';
            break;
        } else {
            val[i] = *p++;
        }
    }
}

static int load_config_from_file(char *fwver, char *log, char *uvc, char *uac, char *cam, char *checkip)
{
    char  file[MAX_PATH];
    FILE *fp = NULL;
    char *buf= NULL;
    int   len= 0;

    // open params file
    get_app_dir(file, MAX_PATH);
    strcat(file, "\\factorytesti8smt.ini");
    fp = fopen(file, "rb");

    if (fp) {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        buf = (char*)malloc(len);
        if (buf) {
            fseek(fp, 0, SEEK_SET);
            fread(buf, len, 1, fp);
            parse_params(buf, "fw_ver"  , fwver);
            parse_params(buf, "logfile" , log);
            parse_params(buf, "uvcdev"  , uvc);
            parse_params(buf, "uacdev"  , uac);
            parse_params(buf, "camtype" , cam);
            parse_params(buf, "checkip" , checkip);
            free(buf);
        }
        fclose(fp);
        return 0;
    }

    return -1;
}
// CFactoryTestI8SMTDlg 对话框




CFactoryTestI8SMTDlg::CFactoryTestI8SMTDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CFactoryTestI8SMTDlg::IDD, pParent)
    , m_strConnectState(_T(""))
    , m_strCurVer(_T(""))
    , m_pTnpContext(NULL)
    , m_pFanPlayer(NULL)
    , m_hTestThread(NULL)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFactoryTestI8SMTDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_TXT_CONNECT_STATE, m_strConnectState);
    DDX_Text(pDX, IDC_TXT_CUR_VER, m_strCurVer);
}

BEGIN_MESSAGE_MAP(CFactoryTestI8SMTDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_DRAWITEM()
    ON_WM_CTLCOLOR()
    ON_WM_QUERYDRAGICON()
    ON_WM_DESTROY()
    ON_WM_CLOSE()
    ON_WM_TIMER()
    ON_WM_SIZE()
    ON_MESSAGE(WM_TNP_UPDATE_UI   , &CFactoryTestI8SMTDlg::OnTnpUpdateUI   )
    ON_MESSAGE(WM_TNP_DEVICE_FOUND, &CFactoryTestI8SMTDlg::OnTnpDeviceFound)
    ON_MESSAGE(WM_TNP_DEVICE_LOST , &CFactoryTestI8SMTDlg::OnTnpDeviceLost )
    ON_BN_CLICKED(IDC_BTN_LED_RESULT, &CFactoryTestI8SMTDlg::OnBnClickedBtnLedResult)
    ON_BN_CLICKED(IDC_BTN_CAMERA_RESULT, &CFactoryTestI8SMTDlg::OnBnClickedBtnCameraResult)
    ON_BN_CLICKED(IDC_BTN_IR_RESULT, &CFactoryTestI8SMTDlg::OnBnClickedBtnIrResult)
    ON_BN_CLICKED(IDC_BTN_KEY_RESULT, &CFactoryTestI8SMTDlg::OnBnClickedBtnKeyResult)
    ON_BN_CLICKED(IDC_BTN_LSENSOR_RESULT, &CFactoryTestI8SMTDlg::OnBnClickedBtnLsensorResult)
    ON_BN_CLICKED(IDC_BTN_NEXT_DEVICE, &CFactoryTestI8SMTDlg::OnBnClickedBtnNextDevice)
    ON_BN_CLICKED(IDC_BTN_SPK_RESULT, &CFactoryTestI8SMTDlg::OnBnClickedBtnSpkResult)
    ON_BN_CLICKED(IDC_BTN_MIC_RESULT, &CFactoryTestI8SMTDlg::OnBnClickedBtnMicResult)
END_MESSAGE_MAP()


// CFactoryTestI8SMTDlg 消息处理程序

BOOL CFactoryTestI8SMTDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // init COM
    CoInitialize(NULL);

    // 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE);         // 设置大图标
    SetIcon(m_hIcon, FALSE);        // 设置小图标

    // 在此添加额外的初始化代码
    strcpy(m_strFwVer      , ""       );
    strcpy(m_strLogFile    , "DEBUGER");
    strcpy(m_strUVCDev     , ""       );
    strcpy(m_strUACDev     , ""       );
    strcpy(m_strCamType    , "uvc"    );
    strcpy(m_strCheckIP    , "true"   );
    strcpy(m_strDeviceIP   , ""       );
    strcpy(m_strLocalHostIP, ""       );

    int ret = load_config_from_file(m_strFwVer, m_strLogFile, m_strUVCDev, m_strUACDev, m_strCamType, m_strCheckIP);
    if (ret != 0) {
        AfxMessageBox(TEXT("无法打开测试配置文件！"), MB_OK);
    }
    log_init(m_strLogFile);
    log_printf("fwver    = %s\n", m_strFwVer   );
    log_printf("logfile  = %s\n", m_strLogFile );
    log_printf("uvcdev   = %s\n", m_strUVCDev  );
    log_printf("uacdev   = %s\n", m_strUACDev  );

    m_strConnectState   = "等待设备连接...";
    m_nLedTestResult    = -1;
    m_nCameraTestResult = -1;
    m_nIRTestResult     = -1;
    m_nWiFiTestResult   = -1;
    m_nKeyTestResult    = -1;
    m_nLSensorTestResult= -1;
    m_nSpkTestResult    = -1;
    m_nMicTestResult    = -1;
    m_nVersionTestResult= -1;
    m_nSDCardTestResult = -1;
    UpdateData(FALSE);

    m_pTnpContext = tnp_init(GetSafeHwnd(), TRUE);
    if (strcmp(m_strCheckIP, "true") == 0) {
        tnp_get_localhost_ip(m_strLocalHostIP, sizeof(m_strLocalHostIP));
    }

    if (strcmp(m_strUVCDev, "") != 0 || strcmp(m_strCamType, "rtsp") == 0) {
        MoveWindow(0, 0, 900, 600, FALSE);

        PLAYER_INIT_PARAMS params = {0};
        char  url_gb2312 [MAX_PATH];
        WCHAR url_unicode[MAX_PATH];
        char  url_utf8   [MAX_PATH];
        if (strcmp(m_strCamType, "uvc") == 0) {
            params.init_timeout     = 1000;
            params.auto_reconnect   = 1000;
            params.video_vwidth     = 1920;
            params.video_vheight    = 1080;
            params.video_frame_rate = 25;
            sprintf(url_gb2312, "dshow://video=%s", m_strUVCDev);
            MultiByteToWideChar(CP_ACP , 0, url_gb2312 , -1, url_unicode, MAX_PATH);
            WideCharToMultiByte(CP_UTF8, 0, url_unicode, -1, url_utf8, MAX_PATH, NULL, NULL);
            m_pFanPlayer = player_open(url_utf8, GetSafeHwnd(), &params);
        }
    }
    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CFactoryTestI8SMTDlg::OnDestroy()
{
    CDialog::OnDestroy();

    StopDeviceTest();
    tnp_disconnect(m_pTnpContext);
    tnp_free(m_pTnpContext);
    log_done();

    // uninit COM
    CoUninitialize();
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
//
HCURSOR CFactoryTestI8SMTDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

// 如果向对话框添加最小化按钮，则需要下面的代码
// 来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
// 这将由框架自动完成。

void CFactoryTestI8SMTDlg::OnPaint()
{
    if (IsIconic()) {
        CPaintDC dc(this); // 用于绘制的设备上下文

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 使图标在工作矩形中居中
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width () - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 绘制图标
        dc.DrawIcon(x, y, m_hIcon);
    } else {
        CDialog::OnPaint();
    }
}

int CFactoryTestI8SMTDlg::GetBackColorByCtrlId(int id)
{
    int result = -1;
    switch (id) {
    case IDC_BTN_LED_RESULT:    result = m_nLedTestResult;    break;
    case IDC_BTN_CAMERA_RESULT: result = m_nCameraTestResult; break;
    case IDC_BTN_IR_RESULT:     result = m_nIRTestResult;     break;
    case IDC_BTN_WIFI_RESULT:   result = m_nWiFiTestResult;   break;
    case IDC_BTN_KEY_RESULT:    result = m_nKeyTestResult;    break;
    case IDC_BTN_LSENSOR_RESULT:result = m_nLSensorTestResult;break;
    case IDC_BTN_SPK_RESULT:    result = m_nSpkTestResult;    break;
    case IDC_BTN_MIC_RESULT:    result = m_nMicTestResult;    break;
    case IDC_BTN_VERSION_RESULT:result = m_nVersionTestResult;break;
    case IDC_BTN_SDCARD_RESULT: result = m_nSDCardTestResult; break;
    }

    switch (result) {
    case 0:  return RGB(255, 0, 0);
    case 1:  return RGB(0, 255, 0);
    default: return RGB(236, 233, 216);
    }
}

void CFactoryTestI8SMTDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    switch (nIDCtl) {
    case IDC_BTN_LED_RESULT:
    case IDC_BTN_CAMERA_RESULT:
    case IDC_BTN_IR_RESULT:
    case IDC_BTN_WIFI_RESULT:
    case IDC_BTN_KEY_RESULT:
    case IDC_BTN_LSENSOR_RESULT:
    case IDC_BTN_SPK_RESULT:
    case IDC_BTN_MIC_RESULT:
    case IDC_BTN_VERSION_RESULT:
    case IDC_BTN_SDCARD_RESULT:
        {
            RECT rect;
            CDC  dc;
            rect = lpDrawItemStruct->rcItem;
            dc.Attach(lpDrawItemStruct->hDC);
            dc.FillSolidRect(&rect, GetBackColorByCtrlId(nIDCtl));
            dc.Draw3dRect(&rect, RGB(255, 255, 255), RGB(0, 0, 0));
            dc.SetBkMode(TRANSPARENT);
            dc.SetTextColor(RGB(0 ,0, 0));
            TCHAR buffer[MAX_PATH] = {0};
            ::GetWindowText(lpDrawItemStruct->hwndItem, buffer, MAX_PATH);
            dc.DrawText(buffer, &rect, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            dc.Detach();
        }
        break;
    }
    CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

static DWORD WINAPI DeviceTestThreadProc(LPVOID pParam)
{
    CFactoryTestI8SMTDlg *dlg = (CFactoryTestI8SMTDlg*)pParam;
    dlg->DoDeviceTest();
    return 0;
}

void CFactoryTestI8SMTDlg::DoDeviceTest()
{
    DWORD tick_next = 0;
    int   tick_sleep= 0;
    char strVersion[128];
    char strResult [256];
    tnp_get_fwver(m_pTnpContext, strVersion, sizeof(strVersion));
    m_nVersionTestResult = strcmp(strVersion, m_strFwVer) == 0 ? 1 : 0;

//  tnp_test_auto(m_pTnpContext, NULL, NULL, &m_nSpkMicTestResult, NULL);
    GetDlgItem(IDC_BTN_WIFI_RESULT   )->SetWindowText(m_nWiFiTestResult    ? "PASS" : "NG");
//  GetDlgItem(IDC_BTN_SPKMIC_RESULT )->SetWindowText(m_nSpkMicTestResult  ? "PASS" : "NG");
    GetDlgItem(IDC_BTN_VERSION_RESULT)->SetWindowText(m_nVersionTestResult ? "PASS" : "NG");
    m_strCurVer = CString(strVersion).Trim();
    PostMessage(WM_TNP_UPDATE_UI);

    if (strcmp(m_strCamType, "rtsp") == 0) {
        PLAYER_INIT_PARAMS params = {0};
        char  url[MAX_PATH];
        params.init_timeout     = 5000;
        params.auto_reconnect   = 5000;
        params.rtsp_transport   = 2;
        params.audio_buffer_num = 8;
        sprintf(url, "rtsp://%s/video0", m_strDeviceIP);
        if (m_pFanPlayer) player_close(m_pFanPlayer);
        m_pFanPlayer = player_open(url, GetSafeHwnd(), &params);
    }

    for (int i=0; i<30&&!m_bTestCancel; i++) Sleep(100);
    tnp_test_spkmic(m_pTnpContext);
    tick_next = GetTickCount();
    while (!m_bTestCancel && (m_nSpkTestResult == -1 || m_nMicTestResult == -1 || m_nKeyTestResult == -1 || m_nLSensorTestResult == -1)) {
        tick_next += 1000;
        tnp_get_result(m_pTnpContext, strResult, sizeof(strResult));
        if (strResult[0] == 'y' && (m_nSpkTestResult !=  1 || m_nMicTestResult !=  1)) {
            m_nSpkTestResult = 1; GetDlgItem(IDC_BTN_SPK_RESULT)->SetWindowText("PASS");
            m_nMicTestResult = 1; GetDlgItem(IDC_BTN_MIC_RESULT)->SetWindowText("PASS");
        }
        if (strResult[0] == 'n' && (m_nSpkTestResult == -1 || m_nMicTestResult == -1)) {
            m_nSpkTestResult = 0; GetDlgItem(IDC_BTN_SPK_RESULT)->SetWindowText("NG");
            m_nMicTestResult = 0; GetDlgItem(IDC_BTN_MIC_RESULT)->SetWindowText("NG");
        }
        if (strResult[1] == 'y' && m_nKeyTestResult == -1) {
            m_nKeyTestResult = 1; GetDlgItem(IDC_BTN_KEY_RESULT)->SetWindowText("PASS");
        }
        if (strResult[2] == 'y' && m_nLSensorTestResult == -1) {
            m_nLSensorTestResult = 1; GetDlgItem(IDC_BTN_LSENSOR_RESULT)->SetWindowText("PASS");
        }
        if (strResult[3] == 'y' && m_nSDCardTestResult == -1) {
            m_nSDCardTestResult = (strResult[3] == 'y'); GetDlgItem(IDC_BTN_SDCARD_RESULT)->SetWindowText(m_nSDCardTestResult ? "PASS" : "NG");
        }
        PostMessage(WM_TNP_UPDATE_UI);
        while (!m_bTestCancel && (LONGLONG)tick_next - (LONGLONG)GetTickCount() > 0) Sleep(20);
    }

    CloseHandle(m_hTestThread);
    m_hTestThread = NULL;
}

void CFactoryTestI8SMTDlg::StartDeviceTest()
{
    if (m_hTestThread) {
        log_printf("device test is running, please wait test done !\n");
        return;
    }

    m_nWiFiTestResult = 1;
    m_bTestCancel = FALSE;
    m_hTestThread = CreateThread(NULL, 0, DeviceTestThreadProc, this, 0, NULL);
}

void CFactoryTestI8SMTDlg::StopDeviceTest()
{
    m_bTestCancel = TRUE;
    if (m_hTestThread) {
        WaitForSingleObject(m_hTestThread, -1);
    }

    if (m_pFanPlayer) {
        player_close(m_pFanPlayer);
        m_pFanPlayer = NULL;
    }
    RECT rect;
    GetClientRect (&rect);
    rect.left = 218;
    InvalidateRect(&rect, TRUE);
}

LRESULT CFactoryTestI8SMTDlg::OnTnpUpdateUI(WPARAM wParam, LPARAM lParam)
{
    UpdateData(FALSE);
    return 0;
}

LRESULT CFactoryTestI8SMTDlg::OnTnpDeviceFound(WPARAM wParam, LPARAM lParam)
{
    if (strcmp(m_strDeviceIP, "") != 0) {
        log_printf("already have a device connected !\n");
        return 0;
    }

    struct in_addr addr;
    int ret = tnp_connect(m_pTnpContext, m_strLocalHostIP, &addr);
    if (ret == 0) {
        strcpy(m_strDeviceIP, inet_ntoa(addr));
        m_strConnectState.Format(TEXT("已连接 %s"), CString(m_strDeviceIP));
        StartDeviceTest();
    }
    UpdateData(FALSE);
    return 0;
}

LRESULT CFactoryTestI8SMTDlg::OnTnpDeviceLost(WPARAM wParam, LPARAM lParam)
{
    struct in_addr addr;
    addr.S_un.S_addr = (u_long)lParam;
    if (strcmp(m_strDeviceIP, inet_ntoa(addr)) != 0) {
        log_printf("this is not current connected device lost !\n");
        return 0;
    }

    StopDeviceTest();
    m_strConnectState    = "等待设备连接...";
    m_strCurVer          = "";
    m_strDeviceIP[0]     = '\0';
    m_nLedTestResult     = -1;
    m_nCameraTestResult  = -1;
    m_nIRTestResult      = -1;
    m_nWiFiTestResult    = -1;
    m_nKeyTestResult     = -1;
    m_nLSensorTestResult = -1;
    m_nSpkTestResult     = -1;
    m_nMicTestResult     = -1;
    m_nVersionTestResult = -1;
    m_nSDCardTestResult  = -1;
    tnp_disconnect(m_pTnpContext);
    UpdateData(FALSE);

    GetDlgItem(IDC_BTN_LED_RESULT    )->SetWindowText("NG");
    GetDlgItem(IDC_BTN_CAMERA_RESULT )->SetWindowText("NG");
    GetDlgItem(IDC_BTN_IR_RESULT     )->SetWindowText("NG");
    GetDlgItem(IDC_BTN_WIFI_RESULT   )->SetWindowText("NG");
    GetDlgItem(IDC_BTN_KEY_RESULT    )->SetWindowText("NG");
    GetDlgItem(IDC_BTN_LSENSOR_RESULT)->SetWindowText("NG");
    GetDlgItem(IDC_BTN_SPK_RESULT    )->SetWindowText("NG");
    GetDlgItem(IDC_BTN_MIC_RESULT    )->SetWindowText("NG");
    GetDlgItem(IDC_BTN_VERSION_RESULT)->SetWindowText("NG");
    GetDlgItem(IDC_BTN_SDCARD_RESULT )->SetWindowText("NG");
    return 0;
}

void CFactoryTestI8SMTDlg::OnCancel() {}
void CFactoryTestI8SMTDlg::OnOK()     {}

void CFactoryTestI8SMTDlg::OnClose()
{
    CDialog::OnClose();
    EndDialog(IDCANCEL);
}

BOOL CFactoryTestI8SMTDlg::PreTranslateMessage(MSG *pMsg)
{
    if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP) {
        switch (pMsg->wParam) {
        case 'Z': if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnLedResult    (); return TRUE;
        case 'X': if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnCameraResult (); return TRUE;
        case 'C': if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnIrResult     (); return TRUE;
        case ' ': if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnNextDevice   (); return TRUE;
        }
    } else if (pMsg->message == MSG_FANPLAYER) {
        if (pMsg->wParam == MSG_OPEN_DONE) {
            log_printf("MSG_OPEN_DONE\n");
            player_play(m_pFanPlayer);
            RECT rect = {0};
            int  mode = VIDEO_MODE_STRETCHED;
            GetClientRect(&rect);
            player_setrect (m_pFanPlayer, 0, 218, 0, rect.right - 218, rect.bottom);
            player_setparam(m_pFanPlayer, PARAM_VIDEO_MODE, &mode);
        }
        if (pMsg->wParam == MSG_STREAM_DISCONNECT) {
            RECT rect = {0};
            GetClientRect(&rect);
            InvalidateRect(&rect, 1);
        }
    }
    return CDialog::PreTranslateMessage(pMsg);
}

void CFactoryTestI8SMTDlg::OnBnClickedBtnLedResult()
{
    if (m_nLedTestResult != 1) {
        m_nLedTestResult = 1;
        GetDlgItem(IDC_BTN_LED_RESULT)->SetWindowText("PASS");
    } else {
        m_nLedTestResult = 0;
        GetDlgItem(IDC_BTN_LED_RESULT)->SetWindowText("NG");
    }
}

void CFactoryTestI8SMTDlg::OnBnClickedBtnCameraResult()
{
    if (m_nCameraTestResult != 1) {
        m_nCameraTestResult = 1;
        GetDlgItem(IDC_BTN_CAMERA_RESULT)->SetWindowText("PASS");
    } else {
        m_nCameraTestResult = 0;
        GetDlgItem(IDC_BTN_CAMERA_RESULT)->SetWindowText("NG");
    }
}

void CFactoryTestI8SMTDlg::OnBnClickedBtnIrResult()
{
    if (m_nIRTestResult != 1) {
        m_nIRTestResult = 1;
        GetDlgItem(IDC_BTN_IR_RESULT)->SetWindowText("PASS");
    } else {
        m_nIRTestResult = 0;
        GetDlgItem(IDC_BTN_IR_RESULT)->SetWindowText("NG");
    }
    UpdateData(FALSE);
}

void CFactoryTestI8SMTDlg::OnBnClickedBtnKeyResult()
{
    if (m_nKeyTestResult != 1) {
        m_nKeyTestResult = 1;
        GetDlgItem(IDC_BTN_KEY_RESULT)->SetWindowText("PASS");
    } else {
        m_nKeyTestResult = 0;
        GetDlgItem(IDC_BTN_KEY_RESULT)->SetWindowText("NG");
    }
    UpdateData(FALSE);
}

void CFactoryTestI8SMTDlg::OnBnClickedBtnLsensorResult()
{
    if (m_nLSensorTestResult != 1) {
        m_nLSensorTestResult = 1;
        GetDlgItem(IDC_BTN_LSENSOR_RESULT)->SetWindowText("PASS");
    } else {
        m_nLSensorTestResult = 0;
        GetDlgItem(IDC_BTN_LSENSOR_RESULT)->SetWindowText("NG");
    }
    UpdateData(FALSE);
}

void CFactoryTestI8SMTDlg::OnBnClickedBtnSpkResult()
{
    if (m_nSpkTestResult != 1) {
        m_nSpkTestResult = 1;
        GetDlgItem(IDC_BTN_SPK_RESULT)->SetWindowText("PASS");
    } else {
        m_nSpkTestResult = 0;
        GetDlgItem(IDC_BTN_SPK_RESULT)->SetWindowText("NG");
    }
    UpdateData(FALSE);
}

void CFactoryTestI8SMTDlg::OnBnClickedBtnMicResult()
{
    if (m_nMicTestResult != 1) {
        m_nMicTestResult = 1;
        GetDlgItem(IDC_BTN_MIC_RESULT)->SetWindowText("PASS");
    } else {
        m_nMicTestResult = 0;
        GetDlgItem(IDC_BTN_MIC_RESULT)->SetWindowText("NG");
    }
    UpdateData(FALSE);
}

void CFactoryTestI8SMTDlg::OnBnClickedBtnNextDevice()
{
    OnTnpDeviceLost(0, inet_addr(m_strDeviceIP));
}

void CFactoryTestI8SMTDlg::OnTimer(UINT_PTR nIDEvent)
{
    CDialog::OnTimer(nIDEvent);
}

void CFactoryTestI8SMTDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);
    if (nType != SIZE_MINIMIZED) {
        RECT rect = {0};
        GetClientRect (&rect);
        player_setrect(m_pFanPlayer, 0, 218, 0, rect.right - 218, rect.bottom);
    }
}



