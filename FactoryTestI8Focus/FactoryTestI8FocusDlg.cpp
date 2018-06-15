// FactoryTestI8FocusDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "FactoryTestI8Focus.h"
#include "FactoryTestI8FocusDlg.h"
#include "BenQGuruDll.h"
#include "fanplayer.h"
#include "log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ENABLE_MES_SYSTEM     TRUE
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

static int load_config_from_file(char *user, char *passwd, char *res, char *gongdan, char *login, char *route, char *log, char *uvc, char *uac)
{
    char  file[MAX_PATH];
    FILE *fp = NULL;
    char *buf= NULL;
    int   len= 0;

    // open params file
    get_app_dir(file, MAX_PATH);
    strcat(file, "\\factorytesti8focus.ini");
    fp = fopen(file, "rb");

    if (fp) {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        buf = (char*)malloc(len);
        if (buf) {
            fseek(fp, 0, SEEK_SET);
            fread(buf, len, 1, fp);
            parse_params(buf, "username"  , user   );
            parse_params(buf, "password"  , passwd );
            parse_params(buf, "resource"  , res    );
            parse_params(buf, "gongdan"   , gongdan);
            parse_params(buf, "loginmode" , login  );
            parse_params(buf, "routecheck", route  );
            parse_params(buf, "logfile"   , log    );
            parse_params(buf, "uvcdev"    , uvc   );
            parse_params(buf, "uacdev"    , uac   );
            free(buf);
        }
        fclose(fp);
        return 0;
    }

    return -1;
}

// CFactoryTestI8FocusDlg 对话框

CFactoryTestI8FocusDlg::CFactoryTestI8FocusDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CFactoryTestI8FocusDlg::IDD, pParent)
    , m_strMesLoginState(_T(""))
    , m_strMesResource(_T(""))
    , m_strMesGongDan(_T(""))
    , m_strScanSN(_T(""))
    , m_strCurSN(_T(""))
    , m_strTestInfo(_T(""))
    , m_pFanPlayer(NULL)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFactoryTestI8FocusDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_TXT_MES_LOGIN, m_strMesLoginState);
    DDX_Text(pDX, IDC_TXT_MES_RESOURCE, m_strMesResource);
    DDX_Text(pDX, IDC_TXT_MES_GONGDAN, m_strMesGongDan);
    DDX_Text(pDX, IDC_EDT_SCAN_SN, m_strScanSN);
    DDX_Text(pDX, IDC_EDT_CUR_SN, m_strCurSN);
    DDX_Text(pDX, IDC_TXT_TEST_INFO, m_strTestInfo);
}

BEGIN_MESSAGE_MAP(CFactoryTestI8FocusDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_DRAWITEM()
    ON_WM_CTLCOLOR()
    ON_WM_QUERYDRAGICON()
    ON_WM_DESTROY()
    ON_WM_CLOSE()
    ON_WM_TIMER()
    ON_WM_SIZE()
    ON_EN_CHANGE(IDC_EDT_SCAN_SN, &CFactoryTestI8FocusDlg::OnEnChangeEdtScanSn)
    ON_BN_CLICKED(IDC_BTN_TEST_RESULT1, &CFactoryTestI8FocusDlg::OnBnClickedBtnTestResult1)
    ON_BN_CLICKED(IDC_BTN_TEST_RESULT2, &CFactoryTestI8FocusDlg::OnBnClickedBtnTestResult2)
    ON_BN_CLICKED(IDC_BTN_TEST_RESULT3, &CFactoryTestI8FocusDlg::OnBnClickedBtnTestResult3)
    ON_BN_CLICKED(IDC_BTN_UPLOAD, &CFactoryTestI8FocusDlg::OnBnClickedBtnUpload)
END_MESSAGE_MAP()


// CFactoryTestI8FocusDlg 消息处理程序

BOOL CFactoryTestI8FocusDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // init COM
    CoInitialize(NULL);

    // 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
    // 执行此操作
    SetIcon(m_hIcon, TRUE);         // 设置大图标
    SetIcon(m_hIcon, FALSE);        // 设置小图标
//  m_fntTestResult.CreatePointFont(160, TEXT("黑体"), NULL);

    // 在此添加额外的初始化代码
    strcpy(m_strUserName  , "username"      );
    strcpy(m_strPassWord  , "password"      );
    strcpy(m_strResource  , "resource"      );
    strcpy(m_strGongDan   , "gongdan"       );
    strcpy(m_strLoginMode , "alert_and_exit");
    strcpy(m_strRouteCheck, "yes"           );
    strcpy(m_strLogFile   , "DEBUGER"       );
    strcpy(m_strUVCDev    , ""              );
    strcpy(m_strUACDev    , ""              );
    int ret = load_config_from_file(m_strUserName, m_strPassWord, m_strResource, m_strGongDan, m_strLoginMode, m_strRouteCheck, m_strLogFile, m_strUVCDev, m_strUACDev);
    if (ret != 0) {
        AfxMessageBox(TEXT("无法打开测试配置文件！"), MB_OK);
    }
    log_init(m_strLogFile);
    log_printf("username = %s\n", m_strUserName);
    log_printf("password = %s\n", m_strPassWord);
    log_printf("resource = %s\n", m_strResource);
    log_printf("gongdan  = %s\n", m_strGongDan );
    log_printf("logfile  = %s\n", m_strLogFile );
    log_printf("uvcdev   = %s\n", m_strUVCDev  );
    log_printf("uacdev   = %s\n", m_strUACDev  );

#if ENABLE_MES_SYSTEM
    CString strJigCode;
    CString strErrMsg;
    m_bMesLoginOK = MesDLL::GetInstance().CheckUserAndResourcePassed(CString(m_strUserName), CString(m_strResource), CString(m_strPassWord), strJigCode, strErrMsg);
    if (strcmp(m_strLoginMode, "alert_and_exit") == 0) {
        if (!m_bMesLoginOK) {
            AfxMessageBox(CString("\n登录 MES 系统失败！\n\n请检查网络配置和 MES 系统，然后重试。\n\n") + strErrMsg, MB_OK);
            EndDialog(IDCANCEL); return FALSE;
        }
    }
    m_strMesLoginState = m_bMesLoginOK ? "登录 MES 成功" : "登录 MES 失败";
#endif

    m_strMesResource    = CString(m_strResource);
    m_strMesGongDan     = CString(m_strGongDan );
    m_strTestInfo       = TEXT("请扫描条码..." );
    m_bSnScaned         = FALSE;
    m_nPlayerOpenOK     =  0;
    m_nFocusTestResult1 = -1;
    m_nFocusTestResult2 = -1;
    m_nFocusTestResult3 = -1;
    UpdateData(FALSE);

    SetTimer(TIMER_ID_SET_FOCUS, 1000, NULL);
    if (strcmp(m_strUVCDev, "") != 0) {
        MoveWindow(0, 0, 1200, 780, FALSE);
        SetTimer(TIMER_ID_OPEN_PLAYER, 100, NULL);
    }
    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CFactoryTestI8FocusDlg::OnDestroy()
{
    CDialog::OnDestroy();

    player_close(m_pFanPlayer);
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
HCURSOR CFactoryTestI8FocusDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

// 如果向对话框添加最小化按钮，则需要下面的代码
// 来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
// 这将由框架自动完成。

void CFactoryTestI8FocusDlg::OnPaint()
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

HBRUSH CFactoryTestI8FocusDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    switch (pWnd->GetDlgCtrlID()) {
    case IDC_TXT_MES_LOGIN:
        pDC->SetTextColor(m_bMesLoginOK ? RGB(0, 180, 0) : RGB(255, 0, 0));
        break;
    case IDC_TXT_TEST_INFO:
        if (m_strTestInfo.Find("成功") != -1) {
            pDC->SetTextColor(RGB(0, 180, 0));
        } else if (m_strTestInfo.Find("失败") != -1) {
            pDC->SetTextColor(RGB(255, 0, 0));
        } else {
            pDC->SetTextColor(RGB(0, 120, 255));
        }
        break;
    }
    return hbr;
}

int CFactoryTestI8FocusDlg::GetBackColorByCtrlId(int id)
{
    int result = -1;
    switch (id) {
    case IDC_BTN_TEST_RESULT1: result = m_nFocusTestResult1; break;
    case IDC_BTN_TEST_RESULT2: result = m_nFocusTestResult2; break;
    case IDC_BTN_TEST_RESULT3: result = m_nFocusTestResult3; break;
    }

    switch (result) {
    case 0:  return RGB(255, 0, 0);
    case 1:  return RGB(0, 255, 0);
    default: return RGB(236, 233, 216);
    }
}

void CFactoryTestI8FocusDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    switch (nIDCtl) {
    case IDC_BTN_TEST_RESULT1:
    case IDC_BTN_TEST_RESULT2:
    case IDC_BTN_TEST_RESULT3:
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
//          dc.SelectObject(m_fntTestResult);
            dc.DrawText(buffer, &rect, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            dc.Detach();
        }
        break;
    }
    CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CFactoryTestI8FocusDlg::OnEnChangeEdtScanSn()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialog::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
    UpdateData(TRUE);
    if (m_strScanSN.GetLength() >= 16) {
        m_strCurSN  = m_strScanSN.Trim();
        m_strScanSN = "";
        m_bSnScaned = TRUE;

#if ENABLE_MES_SYSTEM
        if (m_bMesLoginOK && stricmp(m_strRouteCheck, "yes") == 0) {
            CString strErrMsg;
            BOOL ret = MesDLL::GetInstance().CheckRoutePassed(m_strCurSN, CString(m_strResource), strErrMsg);
            if (!ret && strErrMsg.Compare("SN_Not_Exist") != 0) {
                AfxMessageBox(CString("该工位没有按照途程生产！\r\n") + strErrMsg, MB_OK);
                UpdateData(FALSE);
                return;
            }
        }
#endif

        m_strTestInfo = TEXT("请进行验焦测试...");
        UpdateData(FALSE);
    }
}

void CFactoryTestI8FocusDlg::OnCancel() {}
void CFactoryTestI8FocusDlg::OnOK()     {}

void CFactoryTestI8FocusDlg::OnClose()
{
    CDialog::OnClose();
    EndDialog(IDCANCEL);
}

BOOL CFactoryTestI8FocusDlg::PreTranslateMessage(MSG *pMsg)
{
    if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP) {
        switch (pMsg->wParam) {
        case 'Z'     : if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnTestResult1(); return TRUE;
        case 'X'     : if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnTestResult2(); return TRUE;
        case 'C'     : if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnTestResult3(); return TRUE;
        case VK_SPACE: if (pMsg->message == WM_KEYDOWN) OnBnClickedBtnUpload     (); return TRUE;
        }
    } else if (pMsg->message == MSG_FFPLAYER) {
        if (pMsg->wParam == MSG_OPEN_DONE) {
            log_printf("MSG_OPEN_DONE\n");
            m_nPlayerOpenOK = 1;
            player_play(m_pFanPlayer);
            RECT rect = {0};
            int  mode = VIDEO_MODE_STRETCHED;
            int  diff =-100;
            GetClientRect(&rect);
            player_setrect (m_pFanPlayer, 0, 218, 0, rect.right - 218, rect.bottom);
            player_setparam(m_pFanPlayer, PARAM_VIDEO_MODE, &mode);
            player_setparam(m_pFanPlayer, PARAM_AVSYNC_TIME_DIFF, &diff);
        } else if (pMsg->wParam == MSG_OPEN_FAILED) {
            log_printf("MSG_OPEN_FAILED\n");
            m_nPlayerOpenOK = 0;
        }
    }
    return CDialog::PreTranslateMessage(pMsg);
}

void CFactoryTestI8FocusDlg::OnBnClickedBtnTestResult1()
{
    GetDlgItem(IDC_EDT_SCAN_SN)->SetFocus();
    if (m_nFocusTestResult1 != 1) {
        m_nFocusTestResult1 = 1;
        GetDlgItem(IDC_BTN_TEST_RESULT1)->SetWindowText("PASS");
    } else {
        m_nFocusTestResult1 = 0;
        GetDlgItem(IDC_BTN_TEST_RESULT1)->SetWindowText("NG");
    }
    UpdateData(FALSE);
}

void CFactoryTestI8FocusDlg::OnBnClickedBtnTestResult2()
{
    GetDlgItem(IDC_EDT_SCAN_SN)->SetFocus();
    if (m_nFocusTestResult2 != 1) {
        m_nFocusTestResult2 = 1;
        GetDlgItem(IDC_BTN_TEST_RESULT2)->SetWindowText("PASS");
    } else {
        m_nFocusTestResult2 = 0;
        GetDlgItem(IDC_BTN_TEST_RESULT2)->SetWindowText("NG");
    }
    UpdateData(FALSE);
}

void CFactoryTestI8FocusDlg::OnBnClickedBtnTestResult3()
{
    GetDlgItem(IDC_EDT_SCAN_SN)->SetFocus();
    if (m_nFocusTestResult3 != 1) {
        m_nFocusTestResult3 = 1;
        GetDlgItem(IDC_BTN_TEST_RESULT3)->SetWindowText("PASS");
    } else {
        m_nFocusTestResult3 = 0;
        GetDlgItem(IDC_BTN_TEST_RESULT3)->SetWindowText("NG");
    }
    UpdateData(FALSE);
}

void CFactoryTestI8FocusDlg::OnBnClickedBtnUpload()
{
    GetDlgItem(IDC_EDT_SCAN_SN)->SetFocus();
    if (!m_bSnScaned) return;

#if ENABLE_MES_SYSTEM
    CString strTestResult;
    CString strErrCode;
    CString strMO;
    CString strErrMsg;

    if (m_nFocusTestResult1 == 1 && m_nFocusTestResult2 == 1 && m_nFocusTestResult3 == 1) {
        strTestResult = "OK";
    } else {
        strTestResult = "NG";
    }

    if (m_nFocusTestResult1 != 1) {
        strErrCode    = "L017";
    }
    if (m_nFocusTestResult2 != 1) {
        strErrCode    = "L015";
    }
    if (m_nFocusTestResult3 != 1) {
        strErrCode    = "L016";
    }

    strMO = m_strGongDan;
    if (m_bMesLoginOK) {
        BOOL ret = MesDLL::GetInstance().SetMobileDataWithMO(m_strCurSN, CString(m_strResource), CString(m_strUserName), strTestResult, strErrCode, strMO, strErrMsg);
        if (!ret) {
            if (strErrMsg.Find("CS_Route_Failed_FirstOP") != -1) {
                m_strTestInfo = "资源码错误！";
            }
            else if (strErrMsg.Find("CS_ID_Has_Already_Belong_To_This_MO") != -1) {
                m_strTestInfo = "重复采集！";
            } else if (strErrMsg.Find("CS_MO_NOT_EXIST") != -1) {
                m_strTestInfo = "工单错误！";
            } else {
                m_strTestInfo = "上传测试结果失败！";
            }
        } else {
            m_strTestInfo = "上传测试结果成功！";
        }
    } else {
        m_strTestInfo = "上传失败，MES 系统未登录！";
    }
    AfxMessageBox(m_strTestInfo + "\r\n" + strErrMsg);
#endif

    m_strTestInfo = "请扫描条码...";
    m_bSnScaned   = FALSE;
    m_nFocusTestResult1 = -1;
    m_nFocusTestResult2 = -1;
    m_nFocusTestResult3 = -1;
    GetDlgItem(IDC_BTN_TEST_RESULT1)->SetWindowText("NG");
    GetDlgItem(IDC_BTN_TEST_RESULT2)->SetWindowText("NG");
    GetDlgItem(IDC_BTN_TEST_RESULT3)->SetWindowText("NG");
    UpdateData(FALSE);
}

void CFactoryTestI8FocusDlg::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent) {
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
            params.init_timeout     = 5000;
            params.video_vwidth     = 1280;
            params.video_vheight    = 720;
            params.video_frame_rate = 30;
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

void CFactoryTestI8FocusDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);
    if (nType != SIZE_MINIMIZED) {
        RECT rect = {0};
        GetClientRect (&rect);
        player_setrect(m_pFanPlayer, 0, 218, 0, rect.right - 218, rect.bottom);
    }
}

