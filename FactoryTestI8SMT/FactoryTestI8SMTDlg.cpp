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

#define TIMER_ID_OPEN_PLAYER  3

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

static int load_config_from_file(char *ver, char *log, char *uvc, char *uac)
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
            parse_params(buf, "version", ver);
            parse_params(buf, "logfile", log);
            parse_params(buf, "uvcdev" , uvc);
            parse_params(buf, "uacdev" , uac);
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
    ON_WM_LBUTTONDBLCLK()
    ON_MESSAGE(WM_TNP_UPDATE_UI   , &CFactoryTestI8SMTDlg::OnTnpUpdateUI   )
    ON_MESSAGE(WM_TNP_DEVICE_FOUND, &CFactoryTestI8SMTDlg::OnTnpDeviceFound)
    ON_MESSAGE(WM_TNP_DEVICE_LOST , &CFactoryTestI8SMTDlg::OnTnpDeviceLost )
    ON_BN_CLICKED(IDC_BTN_LED_RESULT, &CFactoryTestI8SMTDlg::OnBnClickedBtnLedResult)
    ON_BN_CLICKED(IDC_BTN_CAMERA_RESULT, &CFactoryTestI8SMTDlg::OnBnClickedBtnCameraResult)
    ON_BN_CLICKED(IDC_BTN_IR_RESULT, &CFactoryTestI8SMTDlg::OnBnClickedBtnIrResult)
    ON_BN_CLICKED(IDC_BTN_KEY_RESULT, &CFactoryTestI8SMTDlg::OnBnClickedBtnKeyResult)
END_MESSAGE_MAP()


// CFactoryTestI8SMTDlg 消息处理程序

BOOL CFactoryTestI8SMTDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE);         // 设置大图标
    SetIcon(m_hIcon, FALSE);        // 设置小图标

    // 在此添加额外的初始化代码
    strcpy(m_strTnpVer    , "version");
    strcpy(m_strLogFile   , "DEBUGER");
    strcpy(m_strUVCDev    , ""       );
    strcpy(m_strUACDev    , ""       );
    strcpy(m_strDeviceIP  , ""       );
    int ret = load_config_from_file(m_strTnpVer, m_strLogFile, m_strUVCDev, m_strUACDev);
    if (ret != 0) {
        AfxMessageBox(TEXT("无法打开测试配置文件！"), MB_OK);
    }
    log_init(m_strLogFile);
    log_printf("version  = %s\n", m_strTnpVer  );
    log_printf("logfile  = %s\n", m_strLogFile );
    log_printf("uvcdev   = %s\n", m_strUVCDev  );
    log_printf("uacdev   = %s\n", m_strUACDev  );

    m_strConnectState   = "等待设备连接...";
    m_bPlayerOpenOK     = FALSE;
    m_nLedTestResult    = -1;
    m_nCameraTestResult = -1;
    m_nIRTestResult     = -1;
    m_nKeyTestResult    = -1;
    m_nLSensorTestResult= -1;
    m_nSpkMicTestResult = -1;
    m_nVersionTestResult= -1;
    UpdateData(FALSE);

    m_pTnpContext = tnp_init(GetSafeHwnd());
    if (strcmp(m_strUVCDev, "") != 0) {
        MoveWindow(0, 0, 900, 600, FALSE);
        SetTimer(TIMER_ID_OPEN_PLAYER, 5000, NULL);
    }

    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CFactoryTestI8SMTDlg::OnDestroy()
{
    CDialog::OnDestroy();

    player_close(m_pFanPlayer);
    tnp_test_cancel(m_pTnpContext, TRUE);
    tnp_disconnect (m_pTnpContext);
    tnp_free(m_pTnpContext);
    log_done();
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
        int x = (rect.Width() - cxIcon + 1) / 2;
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
    case IDC_BTN_KEY_RESULT:    result = m_nKeyTestResult;    break;
    case IDC_BTN_LSENSOR_RESULT:result = m_nLSensorTestResult;break;
    case IDC_BTN_SPKMIC_RESULT: result = m_nSpkMicTestResult; break;
    case IDC_BTN_VERSION_RESULT:result = m_nVersionTestResult;break;
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
    case IDC_BTN_KEY_RESULT:
    case IDC_BTN_LSENSOR_RESULT:
    case IDC_BTN_SPKMIC_RESULT:
    case IDC_BTN_VERSION_RESULT:
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
    // set timeout to 10s
    tnp_set_timeout(m_pTnpContext, 10000);

    char strVer[32];
    strcpy(strVer, m_strTnpVer);
    if (tnp_test_smt(m_pTnpContext, strVer, &m_nLSensorTestResult, &m_nSpkMicTestResult, &m_nVersionTestResult) == 0) {
        GetDlgItem(IDC_BTN_LSENSOR_RESULT)->SetWindowText(m_nLSensorTestResult ? "PASS" : "NG");
        GetDlgItem(IDC_BTN_SPKMIC_RESULT )->SetWindowText(m_nSpkMicTestResult  ? "PASS" : "NG");
        GetDlgItem(IDC_BTN_VERSION_RESULT)->SetWindowText(m_nVersionTestResult ? "PASS" : "NG");
        m_strCurVer = strVer;
        PostMessage(WM_TNP_UPDATE_UI);
    }

    // set timeout to 5s
    tnp_set_timeout(m_pTnpContext, 5000);

    CloseHandle(m_hTestThread);
    m_hTestThread = NULL;
}

void CFactoryTestI8SMTDlg::StartDeviceTest()
{
    if (m_hTestThread) {
        log_printf("device test is running, please wait test done !\n");
        return;
    }

    m_bTestCancel = FALSE;
    tnp_test_cancel(m_pTnpContext, FALSE);
    m_hTestThread = CreateThread(NULL, 0, DeviceTestThreadProc, this, 0, NULL);
}

void CFactoryTestI8SMTDlg::StopDeviceTest()
{
    m_bTestCancel = TRUE;
    tnp_test_cancel(m_pTnpContext, TRUE);
    if (m_hTestThread) {
        WaitForSingleObject(m_hTestThread, -1);
    }
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
    addr.S_un.S_addr = (u_long)lParam;

    int ret = tnp_connect(m_pTnpContext, addr);
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

    m_strConnectState    = "等待设备连接...";
    m_strCurVer          = "";
    m_strDeviceIP[0]     = '\0';
    m_nLedTestResult     = -1;
    m_nCameraTestResult  = -1;
    m_nIRTestResult      = -1;
    m_nKeyTestResult     = -1;
    m_nLSensorTestResult = -1;
    m_nSpkMicTestResult  = -1;
    m_nVersionTestResult = -1;
    tnp_disconnect(m_pTnpContext);
    UpdateData(FALSE);

    GetDlgItem(IDC_BTN_LED_RESULT    )->SetWindowText("NG");
    GetDlgItem(IDC_BTN_CAMERA_RESULT )->SetWindowText("NG");
    GetDlgItem(IDC_BTN_IR_RESULT     )->SetWindowText("NG");
    GetDlgItem(IDC_BTN_KEY_RESULT    )->SetWindowText("NG");
    GetDlgItem(IDC_BTN_LSENSOR_RESULT)->SetWindowText("NG");
    GetDlgItem(IDC_BTN_SPKMIC_RESULT )->SetWindowText("NG");
    GetDlgItem(IDC_BTN_VERSION_RESULT)->SetWindowText("NG");
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
        case 'Z': if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnLedResult   (); return TRUE;
        case 'X': if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnCameraResult(); return TRUE;
        case 'C': if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnIrResult    (); return TRUE;
        case 'V': if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnKeyResult   (); return TRUE;
        case VK_SPACE: return TRUE;
        }
    } else if (pMsg->message == MSG_FFPLAYER) {
        if (pMsg->wParam == MSG_OPEN_DONE) {
            log_printf("MSG_OPEN_DONE\n");
            m_bPlayerOpenOK = TRUE;
            player_play(m_pFanPlayer);
            RECT rect = {0};
            int  mode = VIDEO_MODE_STRETCHED;
            GetClientRect(&rect);
            player_setrect (m_pFanPlayer, 0, 218, 0, rect.right - 218, rect.bottom);
            player_setparam(m_pFanPlayer, PARAM_VIDEO_MODE, &mode);
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

void CFactoryTestI8SMTDlg::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent) {
    case TIMER_ID_OPEN_PLAYER:
        if (m_bPlayerOpenOK) {
            LONGLONG pos = 0;
            player_getparam(m_pFanPlayer, PARAM_MEDIA_POSITION, &pos);
            if (pos == -1) m_bPlayerOpenOK = FALSE;
            break;
        }
        //++ reopen fanplayer to play camera stream
        if (m_pFanPlayer) {
            player_close(m_pFanPlayer);
            m_pFanPlayer = NULL;
        }
        if (TRUE) {
            PLAYER_INIT_PARAMS params = {0};
            params.init_timeout = 5000;
            params.video_vwidth = 640;
            params.video_vheight= 480;
            char  url_gb2312 [MAX_PATH];
            WCHAR url_unicode[MAX_PATH];
            char  url_utf8   [MAX_PATH];
            sprintf(url_gb2312, "dshow://video=%s", m_strUVCDev);
            MultiByteToWideChar(CP_ACP , 0, url_gb2312 , -1, url_unicode, MAX_PATH);
            WideCharToMultiByte(CP_UTF8, 0, url_unicode, -1, url_utf8, MAX_PATH, NULL, NULL);
            m_pFanPlayer = player_open(url_utf8, GetSafeHwnd(), &params);
        }
        //-- reopen fanplayer to play camera stream
        break;
    }
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

void CFactoryTestI8SMTDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    if (strcmp(m_strUVCDev, "") != 0) {
        m_bPlayerOpenOK = FALSE;
        SetTimer(TIMER_ID_OPEN_PLAYER, 5000, NULL);
    }
    CDialog::OnLButtonDblClk(nFlags, point);
}

