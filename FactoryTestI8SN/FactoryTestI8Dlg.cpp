// FactoryTestI8Dlg.cpp : 实现文件
//

#include "stdafx.h"
#include "FactoryTestI8.h"
#include "FactoryTestI8Dlg.h"
#include "TestNetProtocol.h"
#include "BenQGuruDll.h"
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

    for (i=0; i<12; i++) {
        if (*p == ',' || *p == ';' || *p == '\r' || *p == '\n' || *p == '\0') {
            val[i] = '\0';
            break;
        } else {
            val[i] = *p++;
        }
    }
}

static int load_config_from_file(char *user, char *passwd, char *res, char *ver, char *log)
{
    char  file[MAX_PATH];
    FILE *fp = NULL;
    char *buf= NULL;
    int   len= 0;

    // open params file
    get_app_dir(file, MAX_PATH);
    strcat(file, "\\factorytest.ini");
    fp = fopen(file, "rb");

    if (fp) {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        buf = (char*)malloc(len);
        if (buf) {
            fseek(fp, 0, SEEK_SET);
            fread(buf, len, 1, fp);
            parse_params(buf, "username", user  );
            parse_params(buf, "password", passwd);
            parse_params(buf, "resource", res   );
            parse_params(buf, "version" , ver   );
            parse_params(buf, "logfile" , log   );
            free(buf);
        }
        fclose(fp);
        return 0;
    }

    return -1;
}

static float parse_iperf_log(CString path)
{
    CStdioFile file;
    CString    str;
    file.Open(path, CFile::modeRead);
    while (file.ReadString(str)) {
        if (str.Find(TEXT("sender"), 0) > 0) {
            int s = str.Find(TEXT("MBytes"), 0) + 6;
            int e = str.Find(TEXT("Mbits" ), 0);
            str = str.Mid(s, e - s).Trim();
            return (float)_tstof(str);
        }
    }
    return 0;
}

static DWORD WINAPI DeviceTestThreadProc(LPVOID pParam)
{
    CFactoryTestI8Dlg *dlg = (CFactoryTestI8Dlg*)pParam;
    dlg->DoDeviceTest();
    return 0;
}

void CFactoryTestI8Dlg::DoDeviceTest()
{
    if (!m_bTestCancel) {
        m_strTestInfo   = "正在写号 ...\r\n";
        m_strTestResult = "正在写号";

        CString strErrMsg;
        CString strMAC;
        CString strBT ;
        CString strCode1;
        CString strCode2;
        CString strCode3;
        BOOL ret = MesDLL::GetInstance().GetAddressRangeByMO(m_strCurSN, strMAC, strBT, strCode1, strCode2, strCode3, strErrMsg);
        if (ret) {
            m_strCurMac = strMAC;
            SendMessage(WM_TNP_UPDATE_UI);
        } else {
            m_strTestInfo  += "无法从 MES 系统获取 MAC\r\n";
            return;
        }

        if (tnp_burn_snmac(m_tnpContext, m_strCurSN.GetBuffer(), m_strCurMac.GetBuffer())) {
            m_bResultBurnSNMac = TRUE;
        } else {
            m_bResultBurnSNMac = FALSE;
        }
        m_strCurSN .ReleaseBuffer();
        m_strCurMac.ReleaseBuffer();
        SendMessage(WM_TNP_UPDATE_UI);
    }

    if (!m_bTestCancel) {
        m_strTestInfo  += "正在测试喇叭咪头 ...\r\n";
        m_strTestResult = "正在测试";
        SendMessage(WM_TNP_UPDATE_UI);
        if (tnp_test_spkmic(m_tnpContext) == 0) {
            m_bResultTestSpkMic = TRUE;
        } else {
            m_bResultTestSpkMic = FALSE;
        }
        SendMessage(WM_TNP_UPDATE_UI);
    }

    if (!m_bTestCancel) {
        m_strTestInfo += "正在测试吞吐量 ...\r\n\r\n";
        SendMessage(WM_TNP_UPDATE_UI);

        CString cmd;
        cmd.Format(TEXT("cmd /c iperf3 -c %s > iperf.log"), CString(m_strDeviceIP));
        STARTUPINFO         si = {0};
        PROCESS_INFORMATION pi = {0};
        si.dwFlags     = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        CreateProcess(NULL, cmd.GetBuffer(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        cmd.ReleaseBuffer();
        CloseHandle(pi.hThread);
        WaitForSingleObject(pi.hProcess, -1);
        CloseHandle(pi.hProcess);

        float wifi = parse_iperf_log(TEXT("iperf.log"));
        m_strWiFiThroughPut.Format(TEXT("%.1f MBytes/sec"), wifi);
        SendMessage(WM_TNP_UPDATE_UI);
        if (wifi > 25.0) {
            m_bResultTestNet = TRUE;
        } else {
            m_bResultTestNet = FALSE;
        }
        SendMessage(WM_TNP_UPDATE_UI);

        if (m_bResultBurnSNMac && m_bResultTestSpkMic && m_bResultTestNet) {
            m_strTestResult = "OK";
        } else {
            m_strTestResult = "NG";
        }

        m_strTestInfo += m_bResultBurnSNMac ? "写号     -  OK\r\n" : "写号     -  NG\r\n";
        m_strTestInfo += m_bResultTestSpkMic? "喇叭咪头 -  OK\r\n" : "喇叭咪头 -  NG\r\n";
        m_strTestInfo += m_bResultTestNet   ? "吞吐量   -  OK\r\n" : "吞吐量   -  NG\r\n";

        m_bResultDone = TRUE;
        SendMessage(WM_TNP_UPDATE_UI);

        CString strErrCode;
        CString strErrMsg;
        if (!m_bResultBurnSNMac) {
            strErrMsg += "L001,L002,";
        }
        if (!m_bResultTestSpkMic) {
            strErrMsg += "L004,L005,";
        }
        if (!m_bResultTestNet) {
            strErrMsg += "L007";
        }
        MesDLL::GetInstance().SetMobileData(m_strCurSN, CString(m_strResource), CString(m_strUserName), m_strTestResult, strErrCode, strErrMsg);	
    }

    CloseHandle(m_hTestThread);
    m_hTestThread = NULL;
}

void CFactoryTestI8Dlg::StartDeviceTest()
{
    if (m_hTestThread) {
        log_printf("device test is running, please wait test done !\n");
        return;
    }

    m_bTestCancel = FALSE;
    tnp_test_cancel(m_tnpContext, FALSE);
    m_hTestThread = CreateThread(NULL, 0, DeviceTestThreadProc, this, 0, NULL);
}

void CFactoryTestI8Dlg::StopDeviceTest()
{
    m_bTestCancel = TRUE;
    tnp_test_cancel(m_tnpContext, TRUE);
    if (m_hTestThread) {
        WaitForSingleObject(m_hTestThread, -1);
    }
}


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// 对话框数据
    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CFactoryTestI8Dlg 对话框

CFactoryTestI8Dlg::CFactoryTestI8Dlg(CWnd* pParent /*=NULL*/)
    : CDialog(CFactoryTestI8Dlg::IDD, pParent)
    , m_strScanSN(_T(""))
    , m_strCurSN(_T(""))
    , m_strMesResource(_T(""))
    , m_strConnectState(_T(""))
    , m_strCurMac(_T(""))
    , m_strTestInfo(_T(""))
    , m_strWiFiThroughPut(_T(""))
    , m_strTestResult(_T(""))
    , m_hTestThread(NULL)

{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFactoryTestI8Dlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDT_SCAN_SN, m_strScanSN);
    DDX_Text(pDX, IDC_EDT_CUR_SN, m_strCurSN);
    DDX_Text(pDX, IDC_TXT_MES_RESOURCE, m_strMesResource);
    DDX_Text(pDX, IDC_TXT_CONNECT_STATE, m_strConnectState);
    DDX_Text(pDX, IDC_EDT_CUR_MAC, m_strCurMac);
    DDX_Text(pDX, IDC_TXT_TEST_INFO, m_strTestInfo);
    DDX_Text(pDX, IDC_EDT_IPREF_RESULT, m_strWiFiThroughPut);
    DDX_Text(pDX, IDC_TXT_TEST_RESULT, m_strTestResult);
}

BEGIN_MESSAGE_MAP(CFactoryTestI8Dlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_DESTROY()
    ON_EN_CHANGE(IDC_EDT_SCAN_SN, &CFactoryTestI8Dlg::OnEnChangeEdtScanSn)
    ON_MESSAGE(WM_TNP_UPDATE_UI   , &CFactoryTestI8Dlg::OnTnpUpdateUI   )
    ON_MESSAGE(WM_TNP_DEVICE_FOUND, &CFactoryTestI8Dlg::OnTnpDeviceFound)
    ON_MESSAGE(WM_TNP_DEVICE_LOST , &CFactoryTestI8Dlg::OnTnpDeviceLost )
    ON_WM_TIMER()
    ON_WM_KEYDOWN()
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CFactoryTestI8Dlg 消息处理程序

BOOL CFactoryTestI8Dlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // 将“关于...”菜单项添加到系统菜单中。

    // IDM_ABOUTBOX 必须在系统命令范围内。
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL) {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty()) {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE );        // 设置大图标
    SetIcon(m_hIcon, FALSE);        // 设置小图标

    m_fntResult.CreatePointFont(350, TEXT("黑体"), NULL);
    GetDlgItem(IDC_TXT_TEST_RESULT)->SetFont(&m_fntResult);
    m_strTestResult = "设备未连接";

    // TODO: 在此添加额外的初始化代码
    strcpy(m_strUserName, "username");
    strcpy(m_strPassWord, "password");
    strcpy(m_strResource, "resource");
    strcpy(m_strTnpVer  , "version" );
    strcpy(m_strLogFile , "DEBUGER" );
    int ret = load_config_from_file(m_strUserName, m_strPassWord, m_strResource, m_strTnpVer, m_strLogFile);
    if (ret != 0) {
        AfxMessageBox(TEXT("无法打开测试配置文件！"), MB_OK);
    }
    log_init(m_strLogFile);
    log_printf("username = %s\n", m_strUserName);
    log_printf("password = %s\n", m_strPassWord);
    log_printf("resource = %s\n", m_strResource);
    log_printf("version  = %s\n", m_strTnpVer  );
    log_printf("logfile  = %s\n", m_strLogFile );

    CString strJigCode;
    CString strErrMsg;
    ret = MesDLL::GetInstance().CheckUserAndResourcePassed (CString(m_strUserName), CString(m_strResource), CString(m_strPassWord), strJigCode, strErrMsg);
    if (!ret) {
        AfxMessageBox(TEXT("检查 MES 系统失败！"), MB_OK);
    }

    m_strMesResource   = CString(m_strResource);
    m_strConnectState  = "等待设备连接...";
    m_strTestInfo      = "请打开设备，进入测试模式。\r\n";
    m_bConnectState    = FALSE;
    m_bResultBurnSNMac = FALSE;
    m_bResultTestSpkMic= FALSE;
    m_bResultTestNet   = FALSE;
    m_bResultDone      = FALSE;
    UpdateData(FALSE);

    m_tnpContext = tnp_init(m_strTnpVer, GetSafeHwnd());
    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CFactoryTestI8Dlg::OnDestroy()
{
    CDialog::OnDestroy();

    // TODO: Add your message handler code here
    tnp_disconnect(m_tnpContext);
    tnp_free(m_tnpContext);
    log_done();

    CString strErrMsg;
    BOOL ret = MesDLL::GetInstance().ATELogOut(CString(m_strResource), strErrMsg);
    if (!ret) {
        log_printf("MesDLL ATELogOut failed !\n");
    }
}

void CFactoryTestI8Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    } else {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CFactoryTestI8Dlg::OnPaint()
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

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
//
HCURSOR CFactoryTestI8Dlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CFactoryTestI8Dlg::OnEnChangeEdtScanSn()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialog::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
    UpdateData(TRUE);
    if (m_strScanSN.GetLength() >= 15) {
        m_strCurSN  = m_strScanSN;
        m_strScanSN = "";
        UpdateData(FALSE);

        CString strErrMsg;
        BOOL ret = MesDLL::GetInstance().CheckRoutePassed(m_strCurSN, CString(m_strResource), strErrMsg);
        if (!ret) {
            AfxMessageBox(TEXT("该序列号没有按照途程生产！"), MB_OK);
            return;
        }

        if (m_bConnectState) {
            // start test
            StartDeviceTest();
        }
    }
}

LRESULT CFactoryTestI8Dlg::OnTnpUpdateUI(WPARAM wParam, LPARAM lParam)
{
    UpdateData(FALSE);
    return 0;
}

LRESULT CFactoryTestI8Dlg::OnTnpDeviceFound(WPARAM wParam, LPARAM lParam)
{
    struct in_addr addr;
    addr.S_un.S_addr = (u_long)lParam;

    m_strDeviceIP = inet_ntoa(addr);
    m_strConnectState.Format(TEXT("发现设备 %s ！"), CString(m_strDeviceIP));
    UpdateData(FALSE);

    int ret = tnp_connect(m_tnpContext, addr);
    if (ret == 0) {
        m_strConnectState.Format(TEXT("设备连接成功！（%s）"), CString(m_strDeviceIP));
        m_strTestInfo   = "设备已连接，请扫描条码。\r\n";
        m_strTestResult = "请扫描条码";
        m_strScanSN     = "";
        m_bConnectState = TRUE;
    } else {
        m_strConnectState.Format(TEXT("设备连接失败！（%s）"), CString(m_strDeviceIP));
        m_strTestInfo   = "设备连接失败，请重启设备。\r\n";
        m_bConnectState = FALSE;
    }
    UpdateData(FALSE);
    return 0;
}

LRESULT CFactoryTestI8Dlg::OnTnpDeviceLost (WPARAM wParam, LPARAM lParam)
{
    m_strDeviceIP = NULL;
    m_bResultDone = FALSE;
    StopDeviceTest(); // stop test
    m_strConnectState = "等待设备连接...";
    tnp_disconnect(m_tnpContext);
    m_strTestInfo = "请打开设备，进入测试模式。\r\n";
    UpdateData(FALSE);
    return 0;
}

void CFactoryTestI8Dlg::OnTimer(UINT_PTR nIDEvent)
{
    // TODO: Add your message handler code here and/or call default

    CDialog::OnTimer(nIDEvent);
}

BOOL CFactoryTestI8Dlg::PreTranslateMessage(MSG *pMsg) 
{
    if (pMsg->message == WM_KEYDOWN) {
        GetDlgItem(IDC_EDT_SCAN_SN)->SetFocus();
    }
    return CDialog::PreTranslateMessage(pMsg);
}

HBRUSH CFactoryTestI8Dlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

    // TODO:  Change any attributes of the DC here
    switch (pWnd->GetDlgCtrlID()) {
    case IDC_TXT_TEST_RESULT:
        if (!m_bConnectState || !m_bResultDone) {
            pDC->SetTextColor(RGB(188, 100, 0));
        } else if (m_bResultBurnSNMac && m_bResultTestSpkMic && m_bResultTestNet) {
            pDC->SetTextColor(RGB(0, 255, 0));
        } else {
            pDC->SetTextColor(RGB(255, 0, 0));
        }
        break;
    }
    // TODO:  Return a different brush if the default is not desired
    return hbr;
}
