# info - Windows System Information Module

> [Russian version](README_RU.md)

**info** is a lightweight C module that collects hardware and software information on Windows using **only the Windows API and the Windows Registry**. The module does not use WMI, WMIC, OLE, or COM in any form.

The repository also contains **test_info** - a small console demo application that links the module and prints the collected data. The demo is secondary: the module is the deliverable and can be used in any C/C++ project.

## Overview

The module exposes a flat, data-oriented API: each function fills a caller-provided buffer or struct and returns a count or status. There are no globals, no internal allocations that outlive the call, no threads, and no initialization/shutdown sequence - a function can be called at any time and is safe to call repeatedly.

The design goal was to gather as much system information as possible without relying on the Windows Management Instrumentation stack. Everything is read through direct Win32 API calls and registry queries, which makes the module fast, dependency-free, and easy to audit. All wide strings are transcoded to UTF-8 internally, so consumers work with plain `char` buffers.

The sources are compiler-agnostic within the MinGW-w64 toolchain: the same files build as C with `gcc` and as C++ with `g++` without any changes (the public header is wrapped in `extern "C"`).

## Features

The module can collect:

- **CPU** - model name, vendor ID, architecture (x86/x64/ARM/ARM64/Itanium), base frequency, logical/physical core counts, package count
- **Motherboard** - board manufacturer, board product, system manufacturer/product, BIOS vendor and version
- **Memory** - total and available physical RAM
- **OS** - kernel name, full product name, major/minor version, build number, UBR
- **Disk** - drive letters, drive type, file system, volume label, volume serial number
- **GPU** - active display adapters with child device names
- **HID** - counts of connected mice, keyboards, and other HID devices
- **Network** - list of installed network adapters
- **Displays** - per-monitor resolution, DPI, color depth, refresh rate
- **Time** - local date/time and timezone bias
- **Language** - active keyboard layout languages
- **Screen** - virtual screen size
- **Process** - running processes located under `C:\Windows` (PID + image path)
- **Modules** - modules of a process located under `C:\Windows`
- **Drivers** - loaded kernel drivers located under `system32`
- **Softwares** - installed programs (Microsoft Corporation entries are skipped)
- **Services** - active services whose binary path is under `C:\Windows`

## Technologies

- **Language:**; the sources are also valid C++ and build unchanged with `g++`
- **Compiler:** MinGW-w64 `gcc` / `g++` (x86_64)
- **Platform:** Windows 10 / 11, x64
- **APIs:** Win32 API, Windows Registry, SetupAPI, PSAPI, Toolhelp32
- **Dependencies:** none - only system DLLs, no third-party libraries
- **Output encoding:** UTF-8 (all wide strings are transcoded with `WideCharToMultiByte`)

## Methodology

All information is obtained from exactly two sources: the **Windows Registry** and **direct Win32 API calls**.

- **CPU identification** is read from `HKLM\HARDWARE\DESCRIPTION\System\CentralProcessor\0` (`ProcessorNameString`, `VendorIdentifier`, `~MHz`). Core and package topology is computed from `GetLogicalProcessorInformation` by counting `RelationProcessorCore` / `RelationProcessorPackage` entries and popcounting the processor masks.
- **Motherboard and BIOS** data comes from `HKLM\HARDWARE\DESCRIPTION\System\BIOS` (`BaseBoardManufacturer`, `BaseBoardProduct`, `SystemManufacturer`, `SystemProductName`, `BIOSManufacturer`, `BIOSVersion`).
- **OS version** is obtained from `RtlGetVersion`, which is resolved at runtime from `ntdll.dll` via `LoadLibrary`/`GetProcAddress` (the function is not linked at build time). This is used because `GetVersionEx` is deprecated and returns stubbed values on Windows 8+. The product name and UBR are read from `HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion`.
- **Memory** is reported by `GlobalMemoryStatusEx`.
- **Displays** are enumerated with `EnumDisplayMonitors`; per-monitor DPI, color depth, and refresh rate are read with `GetDeviceCaps`, falling back to desktop values when a monitor reports zero.
- **GPU adapters** are enumerated with `EnumDisplayDevices` (active adapters plus their child device strings).
- **Network adapters** are enumerated through SetupAPI: `SetupDiGetClassDevs` with the network class GUID (`{4d36e972-e325-11ce-bfc1-08002be10318}`) and `DIGCF_PRESENT`, with the device description read via `SetupDiGetDeviceRegistryProperty` (`SPDRP_DEVICEDESC`).
- **HID devices** are counted with `GetRawInputDeviceList` by device type (mouse / keyboard / HID).
- **Time and timezone** come from `GetLocalTime` and `GetTimeZoneInformation`.
- **Keyboard languages** are resolved from `GetKeyboardLayoutList` + `VerLanguageNameA`.
- **Processes** are captured with a Toolhelp32 snapshot; each process image path is resolved with `QueryFullProcessImageName` and filtered to `C:\Windows` paths.
- **Modules and drivers** are enumerated with PSAPI (`EnumProcessModules`, `GetModuleFileNameEx`, `EnumDeviceDrivers`, `GetDeviceDriverFileName`) and filtered to `system32` paths.
- **Installed software** is read from the `Uninstall` registry keys in both the native 64-bit view and the `WOW6432Node` view; entries published by Microsoft Corporation are skipped.
- **Services** are enumerated through the Service Control Manager (`OpenSCManager`, `EnumServicesStatus`, `QueryServiceConfig`); only active services with a `C:\Windows` binary path are reported.

Filtering to system paths keeps the output focused on the OS itself and reduces noise from user-space applications.

## Modules Used

| Module | Role in the project |
|---|---|
| `kernel32.dll` | Registry access, system information, memory status, time, drive enumeration, dynamic loading of `ntdll.dll` |
| `advapi32.dll` | Service Control Manager: service enumeration and configuration queries |
| `user32.dll` | Display monitor enumeration, keyboard layouts, virtual screen, raw input device list |
| `gdi32.dll` | Device capabilities: DPI, color depth, refresh rate |
| `psapi.dll` | Process module enumeration, kernel driver enumeration |
| `version.dll` | Keyboard layout language names |
| `setupapi.dll` | Network device class enumeration |
| `ntdll.dll` | `RtlGetVersion` for the real OS version (loaded dynamically at runtime, not linked) |

## Building the Module

Requirements: Windows 10/11 x64 and MinGW-w64 `gcc` or `g++` in `PATH`.

As C:

```bat
gcc -O2 -Wall -c info.c -o info.o
```

As C++:

```bat
g++ -O2 -Wall -c info.c -o info.o
```

Link your application with the module object and the system libraries:

```bat
-lpsapi -lversion -ladvapi32 -luser32 -lgdi32 -lsetupapi
```

(`kernel32` and `ntdll` are not linked explicitly: `kernel32` is linked by default, `ntdll` is loaded dynamically at runtime.)

## Demo Application: test_info

**test_info** is a console demo that links the module and prints every section it can collect. It exists to demonstrate the module's API and to serve as a reference for calling conventions; it is not part of the module itself.

### Build

```bat
build_info.bat:: build the demo as C with gcc
build_info.bat cpp:: build the demo as C++ with g++
```

Both variants produce `test_info.exe` in the project directory.

### Run

```bat
test_info.exe
```

The demo prints the following sections in order:

1. OS
2. CPU
3. Quantities
4. Motherboard
5. Memory
6. Disk
7. GPU
8. Connected HIDs
9. Network
10. Displays
11. Time Date
12. Language
13. Screen
14. Process
15. Modules
16. Drivers
17. Softwares
18. Services

### Notes

- No administrator rights are required; protected system processes may simply be skipped.
- The demo performs no network activity and collects no telemetry.
- The result is a single self-contained executable with no runtime dependencies beyond the Windows system DLLs.

## License

GNU AGPL v3 (Affero GPL)
