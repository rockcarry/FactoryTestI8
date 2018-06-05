// FactoryTestI8FullDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "FactoryTestI8Full.h"
#include "FactoryTestI8FullDlg.h"
#include "TestNetProtocol.h"
#include "BenQGuruDll.h"
#include "fanplayer.h"
#include "log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ENABLE_MES_SYSTEM     TRUE
#define TIMER_ID_NEXT_TEST    1
#define TIMER_ID_SET_FOCUS    2
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

static int load_config_from_file(char *user, char *passwd, char *res, char *ver, char *login, char *route, char *log, char *uvc, char *uac)
{
    char  file[MAX_PATH];
    FILE *fp = NULL;
    char *buf= NULL;
    int   len= 0;

    // open params file
    get_app_dir(file, MAX_PATH);
    strcat(file, "\\factorytesti8full.ini");
    fp = fopen(file, "rb");

    if (fp) {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        buf = (char*)malloc(len);
        if (buf) {
            fseek(fp, 0, SEEK_SET);
            fread(buf, len, 1, fp);
            parse_params(buf, "username"  , user  );
            parse_params(buf, "password"  , passwd);
            parse_params(buf, "resource"  , res   );
            parse_params(buf, "version"   , ver   );
            parse_params(buf, "loginmode" , login );
            parse_params(buf, "routecheck", route );
            parse_params(buf, "logfile"   , log   );
            parse_params(buf, "uvcdev"    , uvc   );
            parse_params(buf, "uacdev"    , uac   );
            free(buf);
        }
        fclose(fp);
        return 0;
    }

    return -1;
}

// CFactoryTestI8FullDlg 对话框

CFactoryTestI8FullDlg::CFactoryTestI8FullDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CFactoryTestI8FullDlg::IDD, pParent)
    , m_strMesLoginState(_T(""))
    , m_strMesResource(_T(""))
    , m_strConnectState(_T(""))
    , m_strScanSN(_T(""))
    , m_strCurSN(_T(""))
    , m_strCurMac(_T(""))
    , m_strTestInfo(_T(""))
    , m_strSnMacVer(_T("设备实际 SN ： \r\n设备实际 MAC：\r\n设备实际 VER："))
    , m_pTnpContext(NULL)
    , m_pFanPlayer(NULL)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFactoryTestI8FullDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_TXT_MES_LOGIN, m_strMesLoginState);
    DDX_Text(pDX, IDC_TXT_MES_RESOURCE, m_strMesResource);
    DDX_Text(pDX, IDC_TXT_CONNECT_STATE, m_strConnectState);
    DDX_Text(pDX, IDC_TXT_TEST_INFO, m_strTestInfo);
    DDX_Text(pDX, IDC_TXT_SN_MAC_VER, m_strSnMacVer);
    DDX_Text(pDX, IDC_EDT_SCAN_SN, m_strScanSN);
    DDX_Text(pDX, IDC_EDT_CUR_SN, m_strCurSN);
    DDX_Text(pDX, IDC_EDT_CUR_MAC, m_strCurMac);
}

BEGIN_MESSAGE_MAP(CFactoryTestI8FullDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_DRAWITEM()
    ON_WM_CTLCOLOR()
    ON_WM_QUERYDRAGICON()
    ON_WM_DESTROY()
    ON_WM_CLOSE()
    ON_WM_TIMER()
    ON_WM_SIZE()
    ON_EN_CHANGE(IDC_EDT_SCAN_SN, &CFactoryTestI8FullDlg::OnEnChangeEdtScanSn)
    ON_MESSAGE(WM_TNP_UPDATE_UI   , &CFactoryTestI8FullDlg::OnTnpUpdateUI   )
    ON_MESSAGE(WM_TNP_DEVICE_FOUND, &CFactoryTestI8FullDlg::OnTnpDeviceFound)
    ON_MESSAGE(WM_TNP_DEVICE_LOST , &CFactoryTestI8FullDlg::OnTnpDeviceLost )
    ON_BN_CLICKED(IDC_BTN_LED_RESULT, &CFactoryTestI8FullDlg::OnBnClickedBtnLedResult)
    ON_BN_CLICKED(IDC_BTN_SPK_RESULT, &CFactoryTestI8FullDlg::OnBnClickedBtnSpkResult)
    ON_BN_CLICKED(IDC_BTN_MIC_RESULT, &CFactoryTestI8FullDlg::OnBnClickedBtnMicResult)
    ON_BN_CLICKED(IDC_BTN_CAMERA_RESULT, &CFactoryTestI8FullDlg::OnBnClickedBtnCameraResult)
    ON_BN_CLICKED(IDC_BTN_IR_RESULT, &CFactoryTestI8FullDlg::OnBnClickedBtnIrResult)
    ON_BN_CLICKED(IDC_BTN_UPLOAD_REPORT, &CFactoryTestI8FullDlg::OnBnClickedBtnUploadReport)
END_MESSAGE_MAP()


// CFactoryTestI8FullDlg 消息处理程序

BOOL CFactoryTestI8FullDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // init COM
    CoInitialize(NULL);

    // 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
    // 执行此操作
    SetIcon(m_hIcon, TRUE );        // 设置大图标
    SetIcon(m_hIcon, FALSE);        // 设置小图标

    m_fntTestInfo.CreatePointFont(160, TEXT("黑体"), NULL);
    GetDlgItem(IDC_TXT_TEST_INFO)->SetFont(&m_fntTestInfo);

    // 在此添加额外的初始化代码
    strcpy(m_strUserName  , "username"      );
    strcpy(m_strPassWord  , "password"      );
    strcpy(m_strResource  , "resource"      );
    strcpy(m_strTnpVer    , "version"       );
    strcpy(m_strLoginMode , "alert_and_exit");
    strcpy(m_strRouteCheck, "yes"           );
    strcpy(m_strLogFile   , "DEBUGER"       );
    strcpy(m_strUVCDev    , ""              );
    strcpy(m_strUACDev    , ""              );
    strcpy(m_strDeviceIP  , ""              );
    int ret = load_config_from_file(m_strUserName, m_strPassWord, m_strResource, m_strTnpVer, m_strLoginMode, m_strRouteCheck, m_strLogFile, m_strUVCDev, m_strUACDev);
    if (ret != 0) {
        AfxMessageBox(TEXT("无法打开测试配置文件！"), MB_OK);
    }
    log_init(m_strLogFile);
    log_printf("username = %s\n", m_strUserName);
    log_printf("password = %s\n", m_strPassWord);
    log_printf("resource = %s\n", m_strResource);
    log_printf("version  = %s\n", m_strTnpVer  );
    log_printf("logfile  = %s\n", m_strLogFile );
    log_printf("uvcdev   = %s\n", m_strUVCDev  );
    log_printf("uacdev   = %s\n", m_strUACDev  );

#if ENABLE_MES_SYSTEM
    CString strJigCode;
    CString strErrMsg;
    m_bMesLoginOK = MesDLL::GetInstance().CheckUserAndResourcePassed (CString(m_strUserName), CString(m_strResource), CString(m_strPassWord), strJigCode, strErrMsg);
    if (strcmp(m_strLoginMode, "alert_and_exit") == 0) {
        if (!m_bMesLoginOK) {
            AfxMessageBox(TEXT("\n登录 MES 系统失败！\n\n请检查网络配置和 MES 系统，然后重试。\n\n谢谢！"), MB_OK);
            EndDialog(IDCANCEL); return FALSE;
        }
    }
    m_strMesLoginState = m_bMesLoginOK ? "登录 MES 成功" : "登录 MES 失败";
#endif

    m_strMesResource    = CString(m_strResource);
    m_strConnectState   = "等待设备连接...";
    m_strTestInfo       = "请打开设备...\r\n";
    m_bSnScaned         = FALSE;
    m_nPlayerOpenOK     =  0;
    m_nLedTestResult    = -1;
    m_nCameraTestResult = -1;
    m_nIRTestResult     = -1;
    m_nSpkTestResult    = -1;
    m_nMicTestResult    = -1;
    m_nKeyTestResult    = -1;
    m_nLSensorTestResult= -1;
    m_nSnTestResult     = -1;
    m_nMacTestResult    = -1;
    m_nVersionTestResult= -1;
    UpdateData(FALSE);

    m_pTnpContext = tnp_init(GetSafeHwnd());

    SetTimer(TIMER_ID_SET_FOCUS  , 1000, NULL);
    if (strcmp(m_strUVCDev, "") != 0) {
        MoveWindow(0, 0, 1200, 780, FALSE);
        SetTimer(TIMER_ID_OPEN_PLAYER, 100, NULL);
    }
    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CFactoryTestI8FullDlg::OnDestroy()
{
    CDialog::OnDestroy();

    player_close(m_pFanPlayer);
    tnp_test_cancel(m_pTnpContext, TRUE);
    tnp_disconnect (m_pTnpContext);
    tnp_free(m_pTnpContext);
    log_done();

#if ENABLE_MES_SYSTEM
    CString strErrMsg;
    BOOL ret = MesDLL::GetInstance().ATELogOut(CString(m_strResource), strErrMsg);
    if (!ret) {
        log_printf("MesDLL ATELogOut failed !\n");
    }
#endif

    // uninit COM
    CoUninitialize();
}

// 当用户拖动最小化窗口时系统调用此函数取得光标显示。
//
HCURSOR CFactoryTestI8FullDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

// 如果向对话框添加最小化按钮，则需要下面的代码
// 来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
// 这将由框架自动完成。

void CFactoryTestI8FullDlg::OnPaint()
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

HBRUSH CFactoryTestI8FullDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    switch (pWnd->GetDlgCtrlID()) {
    case IDC_TXT_MES_LOGIN:
        pDC->SetTextColor(m_bMesLoginOK ? RGB(0, 180, 0) : RGB(255, 0, 0));
        break;
    }
    return hbr;
}

int CFactoryTestI8FullDlg::GetBackColorByCtrlId(int id)
{
    int result = -1;
    switch (id) {
    case IDC_BTN_LED_RESULT:    result = m_nLedTestResult;    break;
    case IDC_BTN_IR_RESULT:     result = m_nIRTestResult;     break;
    case IDC_BTN_CAMERA_RESULT: result = m_nCameraTestResult; break;
    case IDC_BTN_SPK_RESULT:    result = m_nSpkTestResult;    break;
    case IDC_BTN_MIC_RESULT:    result = m_nMicTestResult;    break;
    case IDC_BTN_KEY_RESULT:    result = m_nKeyTestResult;    break;
    case IDC_BTN_LSENSOR_RESULT:result = m_nLSensorTestResult;break;
    case IDC_BTN_SN_RESULT:     result = m_nSnTestResult;     break;
    case IDC_BTN_MAC_RESULT:    result = m_nMacTestResult;    break;
    case IDC_BTN_VERSION_RESULT:result = m_nVersionTestResult;break;
    }

    switch (result) {
    case 0:  return RGB(255, 0, 0);
    case 1:  return RGB(0, 255, 0);
    default: return RGB(236, 233, 216);
    }
}

void CFactoryTestI8FullDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    switch (nIDCtl) {
    case IDC_BTN_LED_RESULT:
    case IDC_BTN_IR_RESULT:
    case IDC_BTN_CAMERA_RESULT:
    case IDC_BTN_SPK_RESULT:
    case IDC_BTN_MIC_RESULT:
    case IDC_BTN_KEY_RESULT:
    case IDC_BTN_LSENSOR_RESULT:
    case IDC_BTN_SN_RESULT:
    case IDC_BTN_MAC_RESULT:
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

void CFactoryTestI8FullDlg::OnEnChangeEdtScanSn()
{
    // TODO: If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialog::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO: Add your control notification handler code here
    UpdateData(TRUE);
    if (m_strScanSN.GetLength() >= 15) {
        m_strCurSN  = m_strScanSN.Trim();
        m_strScanSN = "";
        m_bSnScaned = TRUE;

#if ENABLE_MES_SYSTEM
        if (m_bMesLoginOK && stricmp(m_strRouteCheck, "yes") == 0) {
            CString strErrMsg;
            BOOL ret = MesDLL::GetInstance().CheckRoutePassed(m_strCurSN, CString(m_strResource), strErrMsg);
            if (!ret) {
                AfxMessageBox(CString("该工位没有按照途程生产！\r\n") + strErrMsg, MB_OK);
                UpdateData(FALSE);
                return;
            }
        }

        if (m_bMesLoginOK) {
            CString strErrMsg;
            CString strMAC;
            CString strBT ;
            CString strCode1;
            CString strCode2;
            CString strCode3;
            BOOL ret = MesDLL::GetInstance().GetAddressRangeByMO(m_strCurSN, strMAC, strBT, strCode1, strCode2, strCode3, strErrMsg);
            if (ret) {
                m_strCurMac = strMAC;
            } else {
                AfxMessageBox(TEXT("无法从 MES 系统获取 MAC 地址！"), MB_OK);
                UpdateData(FALSE);
                return;
            }
        } else
#endif
        {
            m_strCurMac = "4637E68F43E5";
        }

        char strsn [32];
        char strmac[32];
        char strver[32];
        strcpy(strsn , m_strCurSN );
        strcpy(strmac, m_strCurMac);
        strcpy(strver, m_strTnpVer);
        struct in_addr addr = {0};
        int ret = tnp_connect_by_sn(m_pTnpContext, strsn, &addr);
        if (ret == 0) {
            strcpy(m_strDeviceIP, inet_ntoa(addr));
            m_strConnectState.Format(TEXT("已连接 %s"), CString(m_strDeviceIP));

            if (tnp_test_sensor_snmac_version(m_pTnpContext, strsn, strmac, strver, &m_nKeyTestResult,
                    &m_nLSensorTestResult, &m_nSnTestResult, &m_nMacTestResult, &m_nVersionTestResult) == 0)
            {
                m_strSnMacVer.Format("设备实际 SN ：%s\r\n设备实际 MAC：%s\r\n设备实际 VER：%s", CString(strsn).Trim(), CString(strmac).Trim(), CString(strver).Trim());
                GetDlgItem(IDC_BTN_KEY_RESULT    )->SetWindowText(m_nKeyTestResult     ? "PASS" : "NG");
                GetDlgItem(IDC_BTN_LSENSOR_RESULT)->SetWindowText(m_nLSensorTestResult ? "PASS" : "NG");
                GetDlgItem(IDC_BTN_SN_RESULT     )->SetWindowText(m_nSnTestResult      ? "PASS" : "NG");
                GetDlgItem(IDC_BTN_MAC_RESULT    )->SetWindowText(m_nMacTestResult     ? "PASS" : "NG");
                GetDlgItem(IDC_BTN_VERSION_RESULT)->SetWindowText(m_nVersionTestResult ? "PASS" : "NG");
                m_strTestInfo = "测试完成，请上传...";
            } else {
                m_strTestInfo = "请重新扫码...";
            }

            // set timeout to 5s
            tnp_set_timeout(m_pTnpContext, 5000);
        } else {
            m_strTestInfo = "请打开设备...\r\n";
        }
        UpdateData(FALSE);
    }
}

LRESULT CFactoryTestI8FullDlg::OnTnpUpdateUI(WPARAM wParam, LPARAM lParam)
{
    UpdateData(FALSE);
    return 0;
}

LRESULT CFactoryTestI8FullDlg::OnTnpDeviceFound(WPARAM wParam, LPARAM lParam)
{
    if (!m_bSnScaned) return 0;

    if (strcmp(m_strDeviceIP, "") != 0) {
        log_printf("already have a device connected !\n");
        return 0;
    }

    char strsn [32];
    char strmac[32];
    char strver[32];
    strcpy(strsn , m_strCurSN );
    strcpy(strmac, m_strCurMac);
    strcpy(strver, m_strTnpVer);

    struct in_addr addr;
    int ret = tnp_connect_by_sn(m_pTnpContext, strsn, &addr);
    if (ret == 0) {
        strcpy(m_strDeviceIP, inet_ntoa(addr));
        m_strConnectState.Format(TEXT("已连接 %s"), CString(m_strDeviceIP));

        if (tnp_test_sensor_snmac_version(m_pTnpContext, strsn, strmac, strver, &m_nKeyTestResult,
                &m_nLSensorTestResult, &m_nSnTestResult, &m_nMacTestResult, &m_nVersionTestResult) == 0)
        {
            m_strSnMacVer.Format("设备实际 SN ：%s\r\n设备实际 MAC：%s\r\n设备实际 VER：%s", strsn, strmac, strver);
            GetDlgItem(IDC_BTN_KEY_RESULT    )->SetWindowText(m_nKeyTestResult     ? "PASS" : "NG");
            GetDlgItem(IDC_BTN_LSENSOR_RESULT)->SetWindowText(m_nLSensorTestResult ? "PASS" : "NG");
            GetDlgItem(IDC_BTN_SN_RESULT     )->SetWindowText(m_nSnTestResult      ? "PASS" : "NG");
            GetDlgItem(IDC_BTN_MAC_RESULT    )->SetWindowText(m_nMacTestResult     ? "PASS" : "NG");
            GetDlgItem(IDC_BTN_VERSION_RESULT)->SetWindowText(m_nVersionTestResult ? "PASS" : "NG");
        }

        m_strTestInfo = "测试完成请上传结果！";
//      m_bSnScaned   = FALSE;

        // set timeout to 5s
        tnp_set_timeout(m_pTnpContext, 5000);
        UpdateData(FALSE);
    }

    return 0;
}

LRESULT CFactoryTestI8FullDlg::OnTnpDeviceLost(WPARAM wParam, LPARAM lParam)
{
    char strsn[32];
    strcpy(strsn, m_strCurSN);
    if (tnp_disconnect_by_sn(m_pTnpContext, strsn) != 0) {
        log_printf("tnp_disconnect_by_sn failed !\n");
        return 0;
    }

    struct in_addr addr;
    addr.S_un.S_addr = (u_long)lParam;
    if (strcmp(m_strDeviceIP, inet_ntoa(addr)) != 0) {
        log_printf("this is not current connected device lost !\n");
        return 0;
    }
    SetTimer(TIMER_ID_NEXT_TEST, 100, NULL);
    return 0;
}

void CFactoryTestI8FullDlg::OnCancel() {}
void CFactoryTestI8FullDlg::OnOK()     {}

void CFactoryTestI8FullDlg::OnClose()
{
    CDialog::OnClose();
    EndDialog(IDCANCEL);
}

BOOL CFactoryTestI8FullDlg::PreTranslateMessage(MSG *pMsg)
{
    if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP) {
        switch (pMsg->wParam) {
        case 'Z'     : if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnLedResult  (); return TRUE;
        case 'X'     : if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnSpkResult  (); return TRUE;
        case 'C'     : if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnMicResult  (); return TRUE;
        case 'V'     : if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnCameraResult(); return TRUE;
        case 'B'     : if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnIrResult   (); return TRUE;
        case VK_SPACE: if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnUploadReport(); return TRUE;
        }
    } else if (pMsg->message == MSG_FFPLAYER) {
        if (pMsg->wParam == MSG_OPEN_DONE) {
            log_printf("MSG_OPEN_DONE\n");
            m_nPlayerOpenOK = 1;
            player_play(m_pFanPlayer);
            RECT rect = {0};
            int  mode = VIDEO_MODE_STRETCHED;
            GetClientRect(&rect);
            player_setrect (m_pFanPlayer, 0, 218, 0, rect.right - 218, rect.bottom);
            player_setparam(m_pFanPlayer, PARAM_VIDEO_MODE, &mode);
        } else if (pMsg->wParam == MSG_OPEN_FAILED) {
            log_printf("MSG_OPEN_FAILED\n");
            m_nPlayerOpenOK = 0;
        }
    }
    return CDialog::PreTranslateMessage(pMsg);
}

void CFactoryTestI8FullDlg::OnBnClickedBtnLedResult()
{
    if (m_nLedTestResult != 1) {
        m_nLedTestResult = 1;
        GetDlgItem(IDC_BTN_LED_RESULT)->SetWindowText("PASS");
    } else {
        m_nLedTestResult = 0;
        GetDlgItem(IDC_BTN_LED_RESULT)->SetWindowText("NG");
    }
}

void CFactoryTestI8FullDlg::OnBnClickedBtnSpkResult()
{
    if (m_nSpkTestResult != 1) {
        m_nSpkTestResult = 1;
        GetDlgItem(IDC_BTN_SPK_RESULT)->SetWindowText("PASS");
    } else {
        m_nSpkTestResult = 0;
        GetDlgItem(IDC_BTN_SPK_RESULT)->SetWindowText("NG");
    }
}

void CFactoryTestI8FullDlg::OnBnClickedBtnMicResult()
{
    if (m_nMicTestResult != 1) {
        m_nMicTestResult = 1;
        GetDlgItem(IDC_BTN_MIC_RESULT)->SetWindowText("PASS");
    } else {
        m_nMicTestResult = 0;
        GetDlgItem(IDC_BTN_MIC_RESULT)->SetWindowText("NG");
    }
}

void CFactoryTestI8FullDlg::OnBnClickedBtnCameraResult()
{
    if (m_nCameraTestResult != 1) {
        m_nCameraTestResult = 1;
        GetDlgItem(IDC_BTN_CAMERA_RESULT)->SetWindowText("PASS");
    } else {
        m_nCameraTestResult = 0;
        GetDlgItem(IDC_BTN_CAMERA_RESULT)->SetWindowText("NG");
    }
}

void CFactoryTestI8FullDlg::OnBnClickedBtnIrResult()
{
    if (m_nIRTestResult != 1) {
        m_nIRTestResult = 1;
        GetDlgItem(IDC_BTN_IR_RESULT)->SetWindowText("PASS");
    } else {
        m_nIRTestResult = 0;
        GetDlgItem(IDC_BTN_IR_RESULT)->SetWindowText("NG");
    }
    m_strTestInfo = "请扫描条码...";
    UpdateData(FALSE);
}

void CFactoryTestI8FullDlg::OnBnClickedBtnUploadReport()
{
    if (!m_bSnScaned) return;

#if ENABLE_MES_SYSTEM
    CString strTestResult;
    CString strErrCode;
    CString strErrMsg;
    if (m_nLedTestResult != 1) {
        strErrCode += "L001,";
    }
    if (m_nCameraTestResult != 1) {
        strErrCode += "L002,";
    }
    if (m_nIRTestResult != 1) {
        strErrCode += "L003,";
    }
    if (m_nSpkTestResult != 1) {
        strErrCode += "L004,";
    }
    if (m_nMicTestResult != 1) {
        strErrCode += "L005,";
    }
    if (m_nKeyTestResult != 1) {
        strErrCode += "L006,";
    }
    if (m_nLSensorTestResult != 1) {
        strErrCode += "L007,";
    }
    if (m_nSnTestResult != 1) {
        strErrCode += "L008,";
    }
    if (m_nMacTestResult != 1) {
        strErrCode += "L009,";
    }
    if (m_nVersionTestResult != 1) {
        strErrCode += "L010,";
    }
    if (  m_nLedTestResult == 1 && m_nCameraTestResult == 1 && m_nIRTestResult == 1 && m_nSpkTestResult == 1 && m_nMicTestResult == 1
       && m_nKeyTestResult == 1 && m_nLSensorTestResult == 1 && m_nSnTestResult == 1 && m_nMacTestResult == 1 && m_nVersionTestResult == 1) {
        strTestResult = "OK";
    } else {
        strTestResult = "NG";
    }
    if (m_bMesLoginOK) {
        BOOL ret = MesDLL::GetInstance().SetMobileData(m_strCurSN, CString(m_strResource), CString(m_strUserName), strTestResult, strErrCode, strErrMsg);
        if (!ret) {
            if (strErrMsg.Find("CS_RepeatCollect_OnOneOP") != -1) {
                m_strTestInfo = "重复采集！";
            } else {
                m_strTestInfo = "上传测试结果失败！";
            }
        } else {
            m_strTestInfo = "上传测试结果成功！";
        }
        if (m_strTestInfo.Find("失败") == -1 && strTestResult.Compare("OK") == 0) {
            if (tnp_test_done(m_pTnpContext) == 0) {
                AfxMessageBox(m_strTestInfo + "\r\n" + strErrMsg);
            } else {
                AfxMessageBox(m_strTestInfo + "\r\n进入老化模式失败！");
            }
        }
    }
    SetTimer(TIMER_ID_NEXT_TEST, 100, NULL);
    m_bSnScaned = FALSE;
#endif
}

void CFactoryTestI8FullDlg::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent) {
    case TIMER_ID_NEXT_TEST:
        KillTimer(TIMER_ID_NEXT_TEST);
        m_strConnectState   = "等待设备连接...";
        m_strTestInfo       = "请打开设备...\r\n";
        m_strSnMacVer       = "";
        m_strScanSN         = "";
        m_strCurSN          = "";
        m_strCurMac         = "";
        m_strDeviceIP[0]    = '\0';
//      m_bSnScaned         = FALSE;
        m_nLedTestResult    = -1;
        m_nCameraTestResult = -1;
        m_nIRTestResult     = -1;
        m_nSpkTestResult    = -1;
        m_nMicTestResult    = -1;
        m_nKeyTestResult    = -1;
        m_nLSensorTestResult= -1;
        m_nSnTestResult     = -1;
        m_nMacTestResult    = -1;
        m_nVersionTestResult= -1;
        UpdateData(FALSE);

        GetDlgItem(IDC_BTN_LED_RESULT    )->SetWindowText("NG");
        GetDlgItem(IDC_BTN_SPK_RESULT    )->SetWindowText("NG");
        GetDlgItem(IDC_BTN_MIC_RESULT    )->SetWindowText("NG");
        GetDlgItem(IDC_BTN_CAMERA_RESULT )->SetWindowText("NG");
        GetDlgItem(IDC_BTN_IR_RESULT     )->SetWindowText("NG");
        GetDlgItem(IDC_BTN_KEY_RESULT    )->SetWindowText("NG");
        GetDlgItem(IDC_BTN_LSENSOR_RESULT)->SetWindowText("NG");
        GetDlgItem(IDC_BTN_SN_RESULT     )->SetWindowText("NG");
        GetDlgItem(IDC_BTN_MAC_RESULT    )->SetWindowText("NG");
        GetDlgItem(IDC_BTN_VERSION_RESULT)->SetWindowText("NG");
        break;
    case TIMER_ID_SET_FOCUS:
        GetDlgItem(IDC_EDT_SCAN_SN)->SetFocus();
        break;
    case TIMER_ID_OPEN_PLAYER:
        if (m_nPlayerOpenOK == 1) {
            LONGLONG pos = 0;
            player_getparam(m_pFanPlayer, PARAM_MEDIA_POSITION, &pos);
            if (pos == -1) m_nPlayerOpenOK = 0;
        } else if (m_nPlayerOpenOK == 0) { // reopen
            PLAYER_INIT_PARAMS params = {0};
            params.init_timeout = 5000;
            params.video_vwidth = 1280;
            params.video_vheight= 720;
            char  url_gb2312 [MAX_PATH];
            WCHAR url_unicode[MAX_PATH];
            char  url_utf8   [MAX_PATH];
            sprintf(url_gb2312, "dshow://video=%s", m_strUVCDev);
            MultiByteToWideChar(CP_ACP , 0, url_gb2312 , -1, url_unicode, MAX_PATH);
            WideCharToMultiByte(CP_UTF8, 0, url_unicode, -1, url_utf8, MAX_PATH, NULL, NULL);
            player_close(m_pFanPlayer);
            m_pFanPlayer = player_open(url_utf8, GetSafeHwnd(), &params);
            m_nPlayerOpenOK = -1;
        } else if (m_nPlayerOpenOK == -1) {
            // wait open result
            log_printf("m_nPlayerOpenOK = %d\n", m_nPlayerOpenOK);
            RECT rect = {0}; GetClientRect(&rect); rect.left = 218;
            InvalidateRect(&rect, TRUE);
        }
        break;
    }
    CDialog::OnTimer(nIDEvent);
}

void CFactoryTestI8FullDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);
    if (nType != SIZE_MINIMIZED) {
        RECT rect = {0};
        GetClientRect (&rect);
        player_setrect(m_pFanPlayer, 0, 218, 0, rect.right - 218, rect.bottom);
    }
}


