#pragma once
#include <windows.h>
#include <vector>
#include <string>

// Info about one restore point (from WMI)
struct RestorePointInfo {
    DWORD sequenceNumber;
    std::wstring description;
    std::wstring creationTime;
};

// Initialize COM security so SRSetRestorePoint works
bool InitializeCOMSecurity();

// Create a new restore point. Returns the sequence number (0 on failure).
INT64 CreateRestorePoint(const std::wstring& description);

// Remove a restore point by its sequence number.
bool RemoveRestorePoint(DWORD sequenceNumber);

// Enumerate all existing restore points via WMI.
std::vector<RestorePointInfo> EnumerateRestorePoints();

// Bypass the 24-hour creation frequency limit (sets registry key to 0).
bool SetRestorePointFrequencyBypass();
