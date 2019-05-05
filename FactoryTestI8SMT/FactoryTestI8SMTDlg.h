// FactoryTestI8SMTDlg.h : ͷ�ļ�
//

#pragma once

// CFactoryTestI8SMTDlg �Ի���
class CFactoryTestI8SMTDlg : public CDialog
{
// ����
public:
    CFactoryTestI8SMTDlg(CWnd* pParent = NULL); // ��׼���캯��

// �Ի�������
    enum { IDD = IDD_FACTORYTESTI8SMT_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void OnCancel();
    virtual void OnOK();

// ʵ��
protected:
    HICON m_hIcon;

    // ���ɵ���Ϣӳ�亯��
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnClose();
    afx_msg void OnPaint();
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg LRESULT OnTnpUpdateUI   (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnTnpDeviceFound(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnTnpDeviceLost (WPARAM wParam, LPARAM lParam);
    afx_msg void OnBnClickedBtnLedResult();
    afx_msg void OnBnClickedBtnCameraResult();
    afx_msg void OnBnClickedBtnIrResult();
    afx_msg void OnBnClickedBtnKeyResult();
    afx_msg void OnBnClickedBtnLsensorResult();
    afx_msg void OnBnClickedBtnSpkResult();
    afx_msg void OnBnClickedBtnMicResult();
    afx_msg void OnBnClickedBtnWifiResult();
    afx_msg void OnBnClickedBtnNextDevice();
    DECLARE_MESSAGE_MAP()

private:
    CString m_strConnectState;
    CString m_strCurVer;
    void   *m_pTnpContext;
    void   *m_pFanPlayer;
    char    m_strFwVer   [32];
    char    m_strAppVer  [32];
    char    m_strLogFile [32];
    char    m_strFtUid   [32];
    char    m_strUVCDev  [MAX_PATH];
    char    m_strUACDev  [MAX_PATH];
    char    m_strCamType [32];
    char    m_strDeviceIP[32];
    int     m_nLedTestResult;
    int     m_nCameraTestResult;
    int     m_nIRTestResult;
    int     m_nWiFiTestResult;
    int     m_nKeyTestResult;
    int     m_nLSensorTestResult;
    int     m_nSpkTestResult;
    int     m_nMicTestResult;
    int     m_nVersionTestResult;
    BOOL    m_bTestCancel;
    HANDLE  m_hTestThread;

private:
    int GetBackColorByCtrlId(int id);

public:
    void DoDeviceTest();
    void StartDeviceTest();
    void StopDeviceTest();
};
