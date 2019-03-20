// FactoryTestI8SNDlg.h : 头文件
//

#pragma once


// CFactoryTestI8SNDlg 对话框
class CFactoryTestI8SNDlg : public CDialog
{
// 构造
public:
    CFactoryTestI8SNDlg(CWnd* pParent = NULL);    // 标准构造函数

// 对话框数据
    enum { IDD = IDD_FACTORYTESTI8SN_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void OnCancel();
    virtual void OnOK();

// 实现
protected:
    HICON m_hIcon;

    // 生成的消息映射函数
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnClose();
    afx_msg void OnPaint();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg HBRUSH  OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg LRESULT OnTnpUpdateUI   (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnTnpDeviceFound(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnTnpDeviceLost (WPARAM wParam, LPARAM lParam);
    afx_msg void    OnEnChangeEdtScanSn();
    DECLARE_MESSAGE_MAP()

private:
    CString m_strMesLoginState;
    CString m_strMesResource;
    CString m_strConnectState;
    CString m_strScanSN;
    CString m_strCurSN;
    CString m_strCurMac;
    CString m_strWiFiThroughPut;
    CString m_strTestResult;
    CString m_strTestInfo;
    CFont   m_fntResult;

    void   *m_pTnpContext;
    char    m_strUserName  [32];
    char    m_strPassWord  [32];
    char    m_strResource  [32];
    char    m_strTnpVer    [32];
    char    m_strLoginMode [32];
    char    m_strRouteCheck[32];
    char    m_strThroughPut[32];
    char    m_strTestSpkMic[32];
    char    m_strLogFile   [32];
    char    m_strDeviceIP  [32];
    BOOL    m_bMesLoginOK;
    BOOL    m_bConnectState;
    BOOL    m_bSnScaned;
    BOOL    m_bResultBurnSN;
    BOOL    m_bResultTestSpkMic;
    BOOL    m_bResultTestNet;
    BOOL    m_bResultDone;
    BOOL    m_bTestCancel;
    HANDLE  m_hTestThread;

public:
    void DoDeviceTest();
    void StartDeviceTest();
    void StopDeviceTest();
};

