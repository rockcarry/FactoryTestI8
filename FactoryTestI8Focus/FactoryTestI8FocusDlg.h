// FactoryTestI8FocusDlg.h : 头文件
//

#pragma once


// CFactoryTestI8FocusDlg 对话框
class CFactoryTestI8FocusDlg : public CDialog
{
// 构造
public:
    CFactoryTestI8FocusDlg(CWnd* pParent = NULL); // 标准构造函数

// 对话框数据
    enum { IDD = IDD_FACTORYTESTI8FOCUS_DIALOG };

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
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg HBRUSH  OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnEnChangeEdtScanSn();
    afx_msg void OnBnClickedBtnTestResult1();
    afx_msg void OnBnClickedBtnTestResult2();
    afx_msg void OnBnClickedBtnTestResult3();
    afx_msg void OnBnClickedBtnUpload();
    DECLARE_MESSAGE_MAP()

private:
    CString m_strMesLoginState;
    CString m_strMesResource;
    CString m_strMesGongDan;
    CString m_strTestInfo;
    CString m_strScanSN;
    CString m_strCurSN;

    void   *m_pFanPlayer;
    char    m_strUserName  [32];
    char    m_strPassWord  [32];
    char    m_strResource  [32];
    char    m_strGongDan   [32];
    char    m_strLoginMode [32];
    char    m_strRouteCheck[32];
    char    m_strLogFile   [32];
    char    m_strUVCDev    [MAX_PATH];
    char    m_strUACDev    [MAX_PATH];
    char    m_strCamType   [32];
    BOOL    m_bMesLoginOK;
    BOOL    m_bSnScaned;
    int     m_nPlayerOpenOK;
    int     m_nFocusTestResult1;
    int     m_nFocusTestResult2;
    int     m_nFocusTestResult3;
    CFont   m_fntTestResult;

private:
    int GetBackColorByCtrlId(int id);
};
