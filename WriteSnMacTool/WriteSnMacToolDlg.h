// WriteSnMacToolDlg.h : header file
//

#pragma once


// CWriteSnMacToolDlg dialog
class CWriteSnMacToolDlg : public CDialog
{
// Construction
public:
    CWriteSnMacToolDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    enum { IDD = IDD_WRITESNMACTOOL_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg LRESULT OnTnpDeviceFound(WPARAM wParam, LPARAM lParam);
    afx_msg void OnBnClickedBtnWriteSnMac();
    DECLARE_MESSAGE_MAP()

private:
    void   *m_pTnpContext;
    CString m_strDevStatus;
    CString m_strSN;
    CString m_strMAC;
    CString m_strDevInfo;
    char    m_strCheckIP [32];
    char    m_strDeviceIP[32];
    char    m_strLocalHostIP[32];
    char    m_curSN   [33];
    char    m_curMAC  [20];
    char    m_curFWVer[64];

private:
    void UpdateDevInfo();
};
