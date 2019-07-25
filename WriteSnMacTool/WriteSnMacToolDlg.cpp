// WriteSnMacToolDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WriteSnMacTool.h"
#include "WriteSnMacToolDlg.h"
#include "TestNetProtocol.h"
#include "log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CWriteSnMacToolDlg dialog

CWriteSnMacToolDlg::CWriteSnMacToolDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CWriteSnMacToolDlg::IDD, pParent)
    , m_strSN(_T(""))
    , m_strMAC(_T(""))
    , m_strDevStatus(_T("device status: not connect"))
    , m_strDevInfo(_T("device info:\n\n - SN:\n - MAC:\n - FWVer:"))
    , m_pTnpContext(NULL)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWriteSnMacToolDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_TXT_DEV_STATUS, m_strDevStatus);
    DDX_Text(pDX, IDC_EDT_SN  , m_strSN );
    DDX_Text(pDX, IDC_EDT_MAC , m_strMAC);
    DDX_Text(pDX, IDC_DEV_INFO, m_strDevInfo);
}

BEGIN_MESSAGE_MAP(CWriteSnMacToolDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_BTN_WRITE_SN_MAC, &CWriteSnMacToolDlg::OnBnClickedBtnWriteSnMac)
    ON_MESSAGE(WM_TNP_DEVICE_FOUND, &CWriteSnMacToolDlg::OnTnpDeviceFound)
    ON_WM_DESTROY()
END_MESSAGE_MAP()


// CWriteSnMacToolDlg message handlers

BOOL CWriteSnMacToolDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    strcpy(m_strDeviceIP   , "");
    strcpy(m_strLocalHostIP, "");
    strcpy(m_curSN         , "");
    strcpy(m_curMAC        , "");
    strcpy(m_curFWVer      , "");

    m_pTnpContext = tnp_init(GetSafeHwnd(), TRUE);
    tnp_get_localhost_ip(m_strLocalHostIP, sizeof(m_strLocalHostIP));
    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CWriteSnMacToolDlg::OnDestroy()
{
    CDialog::OnDestroy();

    tnp_disconnect(m_pTnpContext);
    tnp_free(m_pTnpContext);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWriteSnMacToolDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    } else {
        CDialog::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWriteSnMacToolDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CWriteSnMacToolDlg::OnTnpDeviceFound(WPARAM wParam, LPARAM lParam)
{
    if (strcmp(m_strDeviceIP, "") != 0) {
        log_printf("already have a device connected !\n");
        return 0;
    }

    struct in_addr addr;
    int ret = tnp_connect(m_pTnpContext, m_strLocalHostIP, &addr);
    if (ret == 0) {
        strcpy(m_strDeviceIP, inet_ntoa(addr));
        m_strDevStatus.Format(TEXT("connected - %s"), CString(m_strDeviceIP));
        UpdateDevInfo();
    }
    UpdateData(FALSE);
    return 0;
}

void CWriteSnMacToolDlg::OnBnClickedBtnWriteSnMac()
{
    char sn[33] = "";
    UpdateData(TRUE);
    tnp_set_sn(m_pTnpContext, m_strSN.GetBuffer());
    tnp_get_sn(m_pTnpContext, sn, sizeof(sn));
    if (strcmp(sn, m_strSN) == 0) {
//      AfxMessageBox("write sn ok !\n");
    } else {
        AfxMessageBox("write sn failed !\n");
    }
    UpdateDevInfo();
}

void CWriteSnMacToolDlg::UpdateDevInfo()
{
    tnp_get_sn   (m_pTnpContext, m_curSN   , sizeof(m_curSN   ));
    tnp_get_mac  (m_pTnpContext, m_curMAC  , sizeof(m_curMAC  ));
    tnp_get_fwver(m_pTnpContext, m_curFWVer, sizeof(m_curFWVer));
    m_strDevInfo.Format(TEXT("device info:\n\n - SN:    %s\n - MAC:   %s\n - FWVer: %s"), m_curSN, m_curMAC, m_curFWVer);
    UpdateData(FALSE);
}