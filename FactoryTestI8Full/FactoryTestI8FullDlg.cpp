// FactoryTestI8FullDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "FactoryTestI8Full.h"
#include "FactoryTestI8FullDlg.h"
#include "TestNetProtocol.h"
#include "BenQGuruDll.h"
#include "log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ENABLE_MES_SYSTEM  TRUE

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

    for (i=0; i<32; i++) {
        if (*p == ',' || *p == ';' || *p == '\r' || *p == '\n' || *p == '\0') {
            val[i] = '\0';
            break;
        } else {
            val[i] = *p++;
        }
    }
}

static int load_config_from_file(char *user, char *passwd, char *res, char *ver, char *login, char *log)
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
            parse_params(buf, "username" , user  );
            parse_params(buf, "password" , passwd);
            parse_params(buf, "resource" , res   );
            parse_params(buf, "version"  , ver   );
            parse_params(buf, "loginmode", login );
            parse_params(buf, "logfile"  , log   );
            free(buf);
        }
        fclose(fp);
        return 0;
    }

    return -1;
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
    , m_tnpContext(NULL)
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
    DDX_Text(pDX, IDC_EDT_SCAN_SN, m_strScanSN);
    DDX_Text(pDX, IDC_EDT_CUR_SN, m_strCurSN);
    DDX_Text(pDX, IDC_EDT_CUR_MAC, m_strCurMac);
   
}

BEGIN_MESSAGE_MAP(CFactoryTestI8FullDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_CTLCOLOR()
    ON_WM_QUERYDRAGICON()
    //}}AFX_MSG_MAP
    ON_WM_DESTROY()
    ON_WM_CLOSE()
    ON_EN_CHANGE(IDC_EDT_SCAN_SN, &CFactoryTestI8FullDlg::OnEnChangeEdtScanSn)
    ON_MESSAGE(WM_TNP_UPDATE_UI   , &CFactoryTestI8FullDlg::OnTnpUpdateUI   )
    ON_MESSAGE(WM_TNP_DEVICE_FOUND, &CFactoryTestI8FullDlg::OnTnpDeviceFound)
    ON_MESSAGE(WM_TNP_DEVICE_LOST , &CFactoryTestI8FullDlg::OnTnpDeviceLost )
END_MESSAGE_MAP()


// CFactoryTestI8FullDlg 消息处理程序

BOOL CFactoryTestI8FullDlg::OnInitDialog()
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
    SetIcon(m_hIcon, TRUE);         // 设置大图标
    SetIcon(m_hIcon, FALSE);        // 设置小图标

    // 在此添加额外的初始化代码
    strcpy(m_strUserName , "username"      );
    strcpy(m_strPassWord , "password"      );
    strcpy(m_strResource , "resource"      );
    strcpy(m_strTnpVer   , "version"       );
    strcpy(m_strLoginMode, "alert_and_exit");
    strcpy(m_strLogFile  , "DEBUGER"       );
    int ret = load_config_from_file(m_strUserName, m_strPassWord, m_strResource, m_strTnpVer, m_strLoginMode, m_strLogFile);
    if (ret != 0) {
        AfxMessageBox(TEXT("无法打开测试配置文件！"), MB_OK);
    }
    log_init(m_strLogFile);
    log_printf("username = %s\n", m_strUserName);
    log_printf("password = %s\n", m_strPassWord);
    log_printf("resource = %s\n", m_strResource);
    log_printf("version  = %s\n", m_strTnpVer  );
    log_printf("logfile  = %s\n", m_strLogFile );

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

    m_strMesResource   = CString(m_strResource);
    m_strConnectState  = "等待设备连接...";
    m_strTestInfo      = "请打开设备，进入测试模式。\r\n";
    m_bConnectState    = FALSE;
    UpdateData(FALSE);

    m_tnpContext = tnp_init(m_strTnpVer, GetSafeHwnd());
    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CFactoryTestI8FullDlg::OnDestroy()
{
    CDialog::OnDestroy();

    // TODO: Add your message handler code here
}

void CFactoryTestI8FullDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    } else {
        CDialog::OnSysCommand(nID, lParam);
    }
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
        int x = (rect.Width() - cxIcon + 1) / 2;
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
    case IDC_STATIC_VIDEO:
        return (HBRUSH)GetStockObject(BLACK_BRUSH);
    }
    return hbr;
}

void CFactoryTestI8FullDlg::OnEnChangeEdtScanSn()
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

#if ENABLE_MES_SYSTEM
        if (m_bMesLoginOK) {
            CString strErrMsg;
            BOOL ret = MesDLL::GetInstance().CheckRoutePassed(m_strCurSN, CString(m_strResource), strErrMsg);
            if (!ret) {
                AfxMessageBox(TEXT("该工位没有按照途程生产！"), MB_OK);
                return;
            }
        }
#endif

        if (m_bConnectState) {
            // todo...
        }
    }
}

LRESULT CFactoryTestI8FullDlg::OnTnpUpdateUI(WPARAM wParam, LPARAM lParam)
{
    UpdateData(FALSE);
    return 0;
}

LRESULT CFactoryTestI8FullDlg::OnTnpDeviceFound(WPARAM wParam, LPARAM lParam)
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

LRESULT CFactoryTestI8FullDlg::OnTnpDeviceLost (WPARAM wParam, LPARAM lParam)
{
    m_strDeviceIP = NULL;
    m_strConnectState = "等待设备连接...";
    tnp_disconnect(m_tnpContext);
    m_strTestInfo = "请打开设备，进入测试模式。\r\n";
    UpdateData(FALSE);
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
    if (pMsg->message == WM_KEYDOWN) {
        GetDlgItem(IDC_EDT_SCAN_SN)->SetFocus();
    }
    return CDialog::PreTranslateMessage(pMsg);
}
