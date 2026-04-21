#include "restore_point.h"
#include "logger.h"

#include <windows.h>
#include <accctrl.h>
#include <aclapi.h>
#include <objbase.h>
#include <strsafe.h>
#include <srrestoreptapi.h>
#include <comdef.h>
#include <wbemidl.h>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "oleaut32.lib")

// Function pointer type for SRSetRestorePointW
typedef BOOL (WINAPI *PFN_SETRESTOREPTW)(PRESTOREPOINTINFOW, PSTATEMGRSTATUS);
// Function pointer type for SRRemoveRestorePoint
typedef DWORD (WINAPI *PFN_SRREMOVERESTOREPOINT)(DWORD);

// ---------------------------------------------------------------------------
// COM Security initialization (required by Microsoft for SRSetRestorePoint)
// ---------------------------------------------------------------------------
bool InitializeCOMSecurity() {
    SECURITY_DESCRIPTOR securityDesc = {0};
    EXPLICIT_ACCESS ea[5] = {0};
    ACL *pAcl = NULL;

    ULONGLONG rgSidBA[(SECURITY_MAX_SID_SIZE + sizeof(ULONGLONG) - 1) / sizeof(ULONGLONG)] = {0};
    ULONGLONG rgSidLS[(SECURITY_MAX_SID_SIZE + sizeof(ULONGLONG) - 1) / sizeof(ULONGLONG)] = {0};
    ULONGLONG rgSidNS[(SECURITY_MAX_SID_SIZE + sizeof(ULONGLONG) - 1) / sizeof(ULONGLONG)] = {0};
    ULONGLONG rgSidPS[(SECURITY_MAX_SID_SIZE + sizeof(ULONGLONG) - 1) / sizeof(ULONGLONG)] = {0};
    ULONGLONG rgSidSY[(SECURITY_MAX_SID_SIZE + sizeof(ULONGLONG) - 1) / sizeof(ULONGLONG)] = {0};

    DWORD cbSid = 0;
    BOOL fRet = FALSE;

    fRet = InitializeSecurityDescriptor(&securityDesc, SECURITY_DESCRIPTOR_REVISION);
    if (!fRet) return false;

    // Create well-known SIDs
    struct { WELL_KNOWN_SID_TYPE type; ULONGLONG* buf; } sids[] = {
        { WinBuiltinAdministratorsSid, rgSidBA },
        { WinLocalServiceSid,          rgSidLS },
        { WinNetworkServiceSid,        rgSidNS },
        { WinSelfSid,                  rgSidPS },
        { WinLocalSystemSid,           rgSidSY },
    };

    for (int i = 0; i < 5; i++) {
        cbSid = SECURITY_MAX_SID_SIZE;
        if (!CreateWellKnownSid(sids[i].type, NULL, sids[i].buf, &cbSid))
            return false;

        ea[i].grfAccessPermissions = COM_RIGHTS_EXECUTE | COM_RIGHTS_EXECUTE_LOCAL;
        ea[i].grfAccessMode = SET_ACCESS;
        ea[i].grfInheritance = NO_INHERITANCE;
        ea[i].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[i].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        ea[i].Trustee.ptstrName = (LPTSTR)sids[i].buf;
    }

    if (SetEntriesInAcl(5, ea, NULL, &pAcl) != ERROR_SUCCESS || !pAcl)
        return false;

    SetSecurityDescriptorOwner(&securityDesc, rgSidBA, FALSE);
    SetSecurityDescriptorGroup(&securityDesc, rgSidBA, FALSE);
    SetSecurityDescriptorDacl(&securityDesc, TRUE, pAcl, FALSE);

    HRESULT hr = CoInitializeSecurity(
        &securityDesc, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
        RPC_C_IMP_LEVEL_IDENTIFY,
        NULL,
        EOAC_DISABLE_AAA | EOAC_NO_CUSTOM_MARSHAL,
        NULL);

    LocalFree(pAcl);
    return SUCCEEDED(hr);
}

// ---------------------------------------------------------------------------
// Create a system restore point
// ---------------------------------------------------------------------------
INT64 CreateRestorePoint(const std::wstring& description) {
    HMODULE hSrClient = LoadLibraryW(L"srclient.dll");
    if (!hSrClient) {
        LOG(L"ERROR: srclient.dll not found. System Restore not available.");
        return 0;
    }

    auto fnSet = (PFN_SETRESTOREPTW)GetProcAddress(hSrClient, "SRSetRestorePointW");
    if (!fnSet) {
        LOG(L"ERROR: SRSetRestorePointW not found in srclient.dll");
        FreeLibrary(hSrClient);
        return 0;
    }

    RESTOREPOINTINFOW rpInfo = {0};
    STATEMGRSTATUS status = {0};

    rpInfo.dwEventType = BEGIN_SYSTEM_CHANGE;
    rpInfo.dwRestorePtType = MODIFY_SETTINGS;   // Generic type for our backup
    rpInfo.llSequenceNumber = 0;
    StringCbCopyW(rpInfo.szDescription, sizeof(rpInfo.szDescription), description.c_str());

    if (!fnSet(&rpInfo, &status)) {
        LOG(L"ERROR: Failed to BEGIN restore point. Status=" + std::to_wstring(status.nStatus));
        FreeLibrary(hSrClient);
        return 0;
    }

    INT64 seqNum = status.llSequenceNumber;
    LOG(L"Restore point BEGIN ok, seq=" + std::to_wstring(seqNum));

    // Immediately end/finalize the restore point
    rpInfo.dwEventType = END_SYSTEM_CHANGE;
    rpInfo.llSequenceNumber = seqNum;

    if (!fnSet(&rpInfo, &status)) {
        LOG(L"WARNING: Failed to END restore point. Status=" + std::to_wstring(status.nStatus));
    } else {
        LOG(L"Restore point finalized successfully.");
    }

    FreeLibrary(hSrClient);
    return seqNum;
}

// ---------------------------------------------------------------------------
// Remove a restore point by sequence number
// ---------------------------------------------------------------------------
bool RemoveRestorePoint(DWORD sequenceNumber) {
    HMODULE hSrClient = LoadLibraryW(L"srclient.dll");
    if (!hSrClient) return false;

    auto fnRemove = (PFN_SRREMOVERESTOREPOINT)GetProcAddress(hSrClient, "SRRemoveRestorePoint");
    if (!fnRemove) {
        FreeLibrary(hSrClient);
        return false;
    }

    DWORD result = fnRemove(sequenceNumber);
    FreeLibrary(hSrClient);

    if (result == ERROR_SUCCESS) {
        LOG(L"Removed restore point seq=" + std::to_wstring(sequenceNumber));
        return true;
    } else {
        LOG(L"Failed to remove restore point seq=" + std::to_wstring(sequenceNumber)
            + L" error=" + std::to_wstring(result));
        return false;
    }
}

// ---------------------------------------------------------------------------
// Enumerate restore points via WMI (root\default -> SystemRestore)
// ---------------------------------------------------------------------------
std::vector<RestorePointInfo> EnumerateRestorePoints() {
    std::vector<RestorePointInfo> points;

    IWbemLocator* pLocator = nullptr;
    IWbemServices* pServices = nullptr;
    IEnumWbemClassObject* pEnum = nullptr;

    HRESULT hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IWbemLocator, (void**)&pLocator);
    if (FAILED(hr)) {
        LOG(L"WMI: CoCreateInstance failed");
        return points;
    }

    // System Restore lives in root\default, NOT root\cimv2
    hr = pLocator->ConnectServer(_bstr_t(L"root\\default"), NULL, NULL, NULL,
                                  0, NULL, NULL, &pServices);
    if (FAILED(hr)) {
        LOG(L"WMI: ConnectServer to root\\default failed");
        pLocator->Release();
        return points;
    }

    hr = CoSetProxyBlanket(pServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                           RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                           NULL, EOAC_NONE);
    if (FAILED(hr)) {
        LOG(L"WMI: CoSetProxyBlanket failed");
        pServices->Release();
        pLocator->Release();
        return points;
    }

    hr = pServices->ExecQuery(_bstr_t(L"WQL"),
                               _bstr_t(L"SELECT * FROM SystemRestore"),
                               WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                               NULL, &pEnum);
    if (FAILED(hr)) {
        LOG(L"WMI: ExecQuery failed");
        pServices->Release();
        pLocator->Release();
        return points;
    }

    IWbemClassObject* pObj = nullptr;
    ULONG uReturn = 0;

    while (pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturn) == S_OK && uReturn > 0) {
        RestorePointInfo rp = {0};
        VARIANT var;

        if (SUCCEEDED(pObj->Get(L"SequenceNumber", 0, &var, NULL, NULL))) {
            rp.sequenceNumber = var.uintVal;
            VariantClear(&var);
        }
        if (SUCCEEDED(pObj->Get(L"Description", 0, &var, NULL, NULL))) {
            if (var.bstrVal) rp.description = var.bstrVal;
            VariantClear(&var);
        }
        if (SUCCEEDED(pObj->Get(L"CreationTime", 0, &var, NULL, NULL))) {
            if (var.bstrVal) rp.creationTime = var.bstrVal;
            VariantClear(&var);
        }

        points.push_back(rp);
        pObj->Release();
    }

    pEnum->Release();
    pServices->Release();
    pLocator->Release();

    // Sort by sequence number ascending (oldest first)
    std::sort(points.begin(), points.end(),
              [](const RestorePointInfo& a, const RestorePointInfo& b) {
                  return a.sequenceNumber < b.sequenceNumber;
              });

    LOG(L"Enumerated " + std::to_wstring(points.size()) + L" restore points.");
    return points;
}

// ---------------------------------------------------------------------------
// Set registry key to bypass the 24-hour restore-point creation limit
// ---------------------------------------------------------------------------
bool SetRestorePointFrequencyBypass() {
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore",
        0, KEY_SET_VALUE, &hKey);

    if (result != ERROR_SUCCESS) {
        LOG(L"Failed to open SystemRestore registry key, error=" + std::to_wstring(result));
        return false;
    }

    DWORD value = 0; // 0 = no frequency limit
    result = RegSetValueExW(hKey, L"SystemRestorePointCreationFrequency",
                            0, REG_DWORD, (BYTE*)&value, sizeof(value));
    RegCloseKey(hKey);

    if (result == ERROR_SUCCESS) {
        LOG(L"Frequency bypass set (SystemRestorePointCreationFrequency=0)");
        return true;
    }
    LOG(L"Failed to set frequency bypass, error=" + std::to_wstring(result));
    return false;
}
