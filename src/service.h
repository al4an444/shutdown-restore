#pragma once
#include <windows.h>

// Service name used for installation and SCM registration
#define SERVICE_NAME L"ShutdownRestoreService"
#define SERVICE_DISPLAY L"Shutdown Restore Point Service"
#define SERVICE_DESC L"Creates a system restore point and removes the oldest one before every shutdown."

// Entry point called by SCM
void WINAPI ServiceMain(DWORD argc, LPWSTR* argv);

// Handler for SCM control codes (stop, preshutdown)
DWORD WINAPI ServiceCtrlHandlerEx(DWORD control, DWORD eventType,
                                   LPVOID eventData, LPVOID context);

// The actual work: create new restore point, delete oldest
void PerformRestorePointRotation();
