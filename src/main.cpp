// ShutdownRestoreService - main.cpp
// Entry point: supports running as a Windows Service or with CLI commands
// for install/uninstall/test.

#include "service.h"
#include "restore_point.h"
#include "logger.h"
#include <windows.h>
#include <objbase.h>
#include <cstdio>
#include <string>

// ---------------------------------------------------------------------------
// Install the service into the SCM database
// ---------------------------------------------------------------------------
static bool InstallService() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);

    SC_HANDLE hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCM) {
        wprintf(L"ERROR: Cannot open SCM. Run as Administrator!\n");
        return false;
    }

    SC_HANDLE hSvc = CreateServiceW(
        hSCM, SERVICE_NAME, SERVICE_DISPLAY,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,        // Start automatically with Windows
        SERVICE_ERROR_NORMAL,
        path,
        NULL, NULL, NULL,
        L"LocalSystem",            // Must run as SYSTEM for restore points
        NULL);

    if (!hSvc) {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_EXISTS) {
            wprintf(L"Service already exists.\n");
        } else {
            wprintf(L"CreateService failed: %lu\n", err);
        }
        CloseServiceHandle(hSCM);
        return false;
    }

    // Set the preshutdown timeout to 3 minutes
    SERVICE_PRESHUTDOWN_INFO psInfo = {0};
    psInfo.dwPreshutdownTimeout = 180000; // 180 seconds
    ChangeServiceConfig2W(hSvc, SERVICE_CONFIG_PRESHUTDOWN_INFO, &psInfo);

    // Set service description
    SERVICE_DESCRIPTIONW desc = {0};
    desc.lpDescription = (LPWSTR)SERVICE_DESC;
    ChangeServiceConfig2W(hSvc, SERVICE_CONFIG_DESCRIPTION, &desc);

    wprintf(L"Service installed successfully!\n");
    wprintf(L"Starting service...\n");

    if (StartServiceW(hSvc, 0, NULL)) {
        wprintf(L"Service started.\n");
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_ALREADY_RUNNING)
            wprintf(L"Service is already running.\n");
        else
            wprintf(L"Failed to start service: %lu\n", err);
    }

    CloseServiceHandle(hSvc);
    CloseServiceHandle(hSCM);
    return true;
}

// ---------------------------------------------------------------------------
// Uninstall the service
// ---------------------------------------------------------------------------
static bool UninstallService() {
    SC_HANDLE hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCM) {
        wprintf(L"ERROR: Cannot open SCM. Run as Administrator!\n");
        return false;
    }

    SC_HANDLE hSvc = OpenServiceW(hSCM, SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (!hSvc) {
        wprintf(L"Service not found.\n");
        CloseServiceHandle(hSCM);
        return false;
    }

    // Stop if running
    SERVICE_STATUS ss;
    ControlService(hSvc, SERVICE_CONTROL_STOP, &ss);
    Sleep(1000);

    if (DeleteService(hSvc)) {
        wprintf(L"Service uninstalled successfully.\n");
    } else {
        wprintf(L"Failed to delete service: %lu\n", GetLastError());
    }

    CloseServiceHandle(hSvc);
    CloseServiceHandle(hSCM);
    return true;
}

// ---------------------------------------------------------------------------
// Test mode: run the rotation logic directly (not as a service)
// ---------------------------------------------------------------------------
static void TestMode() {
    wprintf(L"=== TEST MODE ===\n");
    wprintf(L"Running restore point rotation directly...\n\n");

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    InitializeCOMSecurity();

    PerformRestorePointRotation();

    CoUninitialize();
    wprintf(L"\nDone. Check ShutdownRestore.log for details.\n");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int wmain(int argc, wchar_t* argv[]) {
    // If launched with arguments, handle CLI commands
    if (argc >= 2) {
        std::wstring cmd = argv[1];

        if (cmd == L"--install" || cmd == L"-i") {
            InstallService();
            return 0;
        }
        if (cmd == L"--uninstall" || cmd == L"-u") {
            UninstallService();
            return 0;
        }
        if (cmd == L"--test" || cmd == L"-t") {
            TestMode();
            return 0;
        }

        wprintf(L"ShutdownRestoreService - Auto restore point on shutdown\n\n");
        wprintf(L"Usage:\n");
        wprintf(L"  --install   (-i)   Install and start the service\n");
        wprintf(L"  --uninstall (-u)   Stop and remove the service\n");
        wprintf(L"  --test      (-t)   Run restore point rotation now (test)\n");
        return 0;
    }

    // No arguments = launched by the SCM as a service
    SERVICE_TABLE_ENTRYW dispatchTable[] = {
        { (LPWSTR)SERVICE_NAME, ServiceMain },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcherW(dispatchTable)) {
        DWORD err = GetLastError();
        if (err == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            // Not running as a service — show help
            wprintf(L"This program runs as a Windows Service.\n");
            wprintf(L"Use --install to install it, or --test to test.\n");
        }
    }

    return 0;
}
