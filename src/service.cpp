#include "service.h"
#include "restore_point.h"
#include "logger.h"
#include <objbase.h>

// Globals for the service
static SERVICE_STATUS        g_ServiceStatus = {0};
static SERVICE_STATUS_HANDLE g_StatusHandle   = NULL;
static HANDLE                g_StopEvent      = NULL;

// ---------------------------------------------------------------------------
// The core logic: create a new restore point and remove the oldest one
// ---------------------------------------------------------------------------
void PerformRestorePointRotation() {
    LOG(L"=== Starting restore point rotation ===");

    // 1. Bypass the 24-hour frequency limit
    SetRestorePointFrequencyBypass();

    // 2. Enumerate existing restore points BEFORE creating
    auto pointsBefore = EnumerateRestorePoints();

    for (auto& rp : pointsBefore) {
        LOG(L"  Existing: seq=" + std::to_wstring(rp.sequenceNumber)
            + L" desc=\"" + rp.description + L"\""
            + L" time=" + rp.creationTime);
    }

    // 3. Create a new restore point with timestamp in description
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t desc[256];
    wsprintfW(desc, L"Auto-Shutdown Backup %04d-%02d-%02d %02d:%02d",
              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);

    INT64 newSeq = CreateRestorePoint(desc);
    if (newSeq == 0) {
        LOG(L"ERROR: Could not create restore point. Aborting rotation.");
        return;
    }
    LOG(L"New restore point created: seq=" + std::to_wstring(newSeq));

    // 4. Remove the oldest restore point (only if we had at least one before)
    if (!pointsBefore.empty()) {
        DWORD oldestSeq = pointsBefore.front().sequenceNumber;
        LOG(L"Removing oldest restore point: seq=" + std::to_wstring(oldestSeq)
            + L" desc=\"" + pointsBefore.front().description + L"\"");
        RemoveRestorePoint(oldestSeq);
    } else {
        LOG(L"No previous restore points to remove.");
    }

    LOG(L"=== Restore point rotation complete ===");
}

// ---------------------------------------------------------------------------
// SCM control handler — we care about PRESHUTDOWN and STOP
// ---------------------------------------------------------------------------
DWORD WINAPI ServiceCtrlHandlerEx(DWORD control, DWORD eventType,
                                   LPVOID eventData, LPVOID context)
{
    (void)eventType; (void)eventData; (void)context;

    switch (control) {
    case SERVICE_CONTROL_PRESHUTDOWN:
        LOG(L"Received SERVICE_CONTROL_PRESHUTDOWN");

        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwCheckPoint = 1;
        g_ServiceStatus.dwWaitHint = 180000; // 3 minutes max
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

        // Do the work
        PerformRestorePointRotation();

        // Signal stop
        SetEvent(g_StopEvent);
        return NO_ERROR;

    case SERVICE_CONTROL_STOP:
        LOG(L"Received SERVICE_CONTROL_STOP");

        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwCheckPoint = 1;
        g_ServiceStatus.dwWaitHint = 5000;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

        SetEvent(g_StopEvent);
        return NO_ERROR;

    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;

    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

// ---------------------------------------------------------------------------
// ServiceMain — entry point called by the SCM
// ---------------------------------------------------------------------------
void WINAPI ServiceMain(DWORD argc, LPWSTR* argv) {
    (void)argc; (void)argv;

    g_StatusHandle = RegisterServiceCtrlHandlerExW(SERVICE_NAME,
                                                    ServiceCtrlHandlerEx, NULL);
    if (!g_StatusHandle) {
        LOG(L"RegisterServiceCtrlHandlerExW failed");
        return;
    }

    g_StopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    // Initialize COM (needed for WMI + SRSetRestorePoint)
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    InitializeCOMSecurity();

    // Tell SCM we're running and accept PRESHUTDOWN + STOP
    g_ServiceStatus.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState     = SERVICE_RUNNING;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PRESHUTDOWN;
    g_ServiceStatus.dwWin32ExitCode    = NO_ERROR;
    g_ServiceStatus.dwCheckPoint       = 0;
    g_ServiceStatus.dwWaitHint         = 0;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    LOG(L"Service started and waiting for shutdown signal...");

    // Just wait — the service does nothing until shutdown/stop
    WaitForSingleObject(g_StopEvent, INFINITE);

    // Cleanup
    CoUninitialize();
    CloseHandle(g_StopEvent);

    g_ServiceStatus.dwCurrentState  = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    LOG(L"Service stopped.");
}
