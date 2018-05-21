#pragma once

#include <afxmt.h>
#include "stdafx.h"

#define FRAME_METHOD_EXPORTS
#ifdef FRAME_METHOD_EXPORTS
# define DLLExport _declspec(dllexport)
#else
# define DLLExport _declspec(dllimport)
#endif

class DLLExport MesDLL
{
private:
    MesDLL();
    MesDLL(const MesDLL &);
    MesDLL & operator = (const MesDLL &);

public:
    static MesDLL& GetInstance();
    ~MesDLL();

    // 根据 SN 得到生产信息
    BOOL GetRcardMOInfo(CString iSN, CString &oMoCode, CString &oErrMessage);

    // 检查登录界面对应的人员是否正确，资源是否正确，人员是否拥有资源权限
    BOOL CheckUserAndResourcePassed(CString iUserCode, CString iResCode, CString iPassWord, CString iJigCode, CString &oErrMessage);

    // 检查 SN 是否按照途程生产
    BOOL CheckRoutePassed(CString iSN, CString iResCode , CString &oErrMessage);

    // 分配 MAC 或 BT 地址
    BOOL GetAddressRangeByMO(CString iSN, CString &oWIFI, CString &oBT, CString &oCode1, CString &oCode2, CString &oCode3, CString &oErrMessage);

    // 测试工具回传数据，MES 记录 Mac 或 BT 地址使用信息
    BOOL SetAddressInfo(CString iSN, CString iWIFI, CString iBT, CString iCode1, CString iCode2, CString iCode3, CString &oErrMessage);

    // 记录测试步骤信息
    BOOL SetTestDetail(CString iSN, CString iClass, CString iSubClass1, CString iSubClass2, CString iSubClass3, CString iMaxValue, CString iMinValue, CString iActualValue, CString iValue1, CString iValue2, CString iValue3, CString iTestResult, CString iResCode, CString &oErrMessage);

    // 根据 IMEI 或 NetCode，获取 IMEI、NETCODE、PSN 信息（Check)
    BOOL GetMEIOrNetCodeRange(CString iSN, CString iIMEI, CString iNetCode, CString &oIMEI1, CString &oIMEI2, CString &oMEID, CString &oNetCode, CString &oPSN, CString &oID1, CString &oID2, CString &oID3, CString &oID4, CString &oID5, CString &oErrMessage);

    // 设备回传数据 IMEI、NETCODE、PSN 信息，MES 系统记录使用信息
    BOOL SetIMEIInfo(CString iSN, CString iIMEI, CString &oErrMessage);

    // 工具通过读取 SN，获取 MES 所有记录信息（WIFI、BT、IMEI1、IMEI2、NETCODE、PSN）
    BOOL GetMobileAllInfo(CString iSN, CString &oWIFI, CString &oBT, CString &oCode1, CString &oCode2, CString &oCode3, CString &oIMEI1, CString &oIMEI2, CString &oMEID, CString &oNetCode, CString &oPSN, CString &oID1, CString &oID2, CString &oID3, CString &oID4, CString &oID5, CString &oErrMessage);

    // 记录打印记录
    BOOL SetPrintRecord(CString iSN, CString iOperator, CString &oErrMessage);

    // MES 接收生产管理资料（记录生产信息）
    BOOL SetMobileData(CString iSN, CString iResCode, CString iOperator, CString iResult, CString iErrCode, CString &oErrMessage);

    // MES 良品不良品采集，带归属工单功能。
    BOOL SetMobileDataWithMO(CString iSN, CString iResCode, CString iOperator, CString iResult, CString iErrCode, CString iMO, CString &oErrMessage);

    // 导入 IMEI 以及 MACBT
    BOOL ImportImeiMacBt(CString strImputXml, CString iType, CString &oErrMessage);

    // 彩标打印 DLL 封装方法
    BOOL CBLabelPrint(CString iRcard, CString iPrinter, CString iOperator, CString iResCode, CString &oErrMessage);

    BOOL ATELogOut(CString iResCode, CString &oErrMessage);
};
