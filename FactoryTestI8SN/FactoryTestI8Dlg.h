// FactoryTestI8Dlg.h : 头文件
//

#pragma once


// CFactoryTestI8Dlg 对话框
class CFactoryTestI8Dlg : public CDialog
{
// 构造
public:
    CFactoryTestI8Dlg(CWnd* pParent = NULL);    // 标准构造函数

// 对话框数据
    enum { IDD = IDD_FACTORYTESTI8_DIALOG };

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    virtual BOOL PreTranslateMessage(MSG* pMsg);

// 实现
protected:
    HICON m_hIcon;

    // 生成的消息映射函数
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg HBRUSH  OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg LRESULT OnTnpUpdateUI   (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnTnpDeviceFound(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnTnpDeviceLost (WPARAM wParam, LPARAM lParam);
    afx_msg HCURSOR OnQueryDragIcon();
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

    void   *m_tnpContext;
    char    m_strUserName [32];
    char    m_strPassWord [32];
    char    m_strResource [32];
    char    m_strTnpVer   [32];
    char    m_strLoginMode[32];
    char    m_strLogFile  [32];
    char   *m_strDeviceIP;
    BOOL    m_bMesLoginOK;
    BOOL    m_bConnectState;
    BOOL    m_bResultBurnSNMac;
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

