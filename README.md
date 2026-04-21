# Shutdown Restore Service

A lightweight, native Windows Service written in C++ that automatically creates a System Restore Point every time you shut down your PC. It intercepts the shutdown signal (`PRESHUTDOWN`), bypasses the Windows 24-hour restriction, creates a new backup, and rotates old ones to save disk space.

---

## Technical Overview

This project is a Windows Service designed to run silently in the background. Its primary purpose is to ensure a recent system state backup is available before every shutdown, mitigating risks from failed updates or configuration changes in the next boot.

### Core Features

* **Background Execution**: Runs entirely in the background as a Windows Service with no GUI.
* **Native Integration**: Leverages native Windows APIs (`SRSetRestorePointW`) and WMI queries for restore point enumeration.
* **Frequency Limit Bypass**: Windows natively restricts automatic restore points to one per 24 hours. This service safely overrides this limitation by modifying the registry (`SystemRestorePointCreationFrequency = 0`).
* **Rolling Backup Strategy**: Enumerates existing system restore points and removes the oldest backup before creating a new one, maintaining a fixed rolling window and preventing disk exhaustion.

### System Requirements

* **OS**: Windows 10 or Windows 11 (64-bit).
* **System Protection**: Must be enabled on the system drive (C:).
* **Build Tools**: CMake and MSVC (Visual Studio Build Tools) required for compiling from source.

---

## Build and Installation

### 1. Compilation
To build the project from source, execute the provided `build.bat` script.
This script automatically generates a `build/` directory and compiles the `Release` version of the executable using CMake.

### 2. Service Installation
**Note**: All installation commands require Administrative privileges.

1. Open **Command Prompt** or **PowerShell** as Administrator.
2. Navigate to the generated release directory:
   ```cmd
   cd C:\Path\To\Your\Directory\build\Release
   ```
3. Install and start the service by executing:
   ```cmd
   ShutdownRestoreService.exe --install
   ```

Once installed, the service will register a `SERVICE_CONTROL_PRESHUTDOWN` handler. Upon computer shutdown, it will temporarily pause the shutdown sequence, create the restore point, remove the oldest one, and resume the shutdown.

---

## Additional Commands

You can verify the core logic without shutting down the system using the test mode (requires Administrator privileges):

```cmd
ShutdownRestoreService.exe --test
```

To uninstall and remove the service from the system:

```cmd
ShutdownRestoreService.exe --uninstall
```

## Logging
The service does not emit standard output while running. For debugging purposes, it generates a log file named `ShutdownRestore.log` in the same directory as the executable. This file contains the complete execution flow, sequence numbers, and any encountered errors.
