/*
 * Git: https://github.com/crypery
 * Author: https://crypery.com
 * License: GNU AGPL v3 (Affero GPL)
 */
#define _CRT_SECURE_NO_WARNINGS
#include "info.h"

#include <tlhelp32.h>
#include <psapi.h>
#include <winternl.h>
#include <setupapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void w2a(const wchar_t* w, char* buf, size_t buf_size) {
    if (!buf || buf_size == 0) return;
    buf[0] = '\0';
    if (!w) return;
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    if (n <= 1) return;
    if ((size_t)(n - 1) >= buf_size) n = (int)buf_size;
    WideCharToMultiByte(CP_UTF8, 0, w, -1, buf, n, NULL, NULL);
    buf[n - 1] = '\0';
}

static int reg_query_sz(HKEY root, const char* subkey, const char* value, char* buf, size_t buf_size) {
    if (!buf || buf_size == 0) return 0;
    buf[0] = '\0';
    HKEY h;
    if (RegOpenKeyExA(root, subkey, 0, KEY_READ, &h) != ERROR_SUCCESS) return 0;
    DWORD size = (DWORD)buf_size;
    LONG rc = RegQueryValueExA(h, value, NULL, NULL, (LPBYTE)buf, &size);
    RegCloseKey(h);
    if (rc != ERROR_SUCCESS) { buf[0] = '\0'; return 0; }
    buf[buf_size - 1] = '\0';
    return 1;
}

static uint32_t popcount_mask(ULONG_PTR mask) {
    uint32_t c = 0;
    while (mask) { mask &= mask - 1; c++; }
    return c;
}

/* ---------------- CPU ---------------- */

void info_cpu_model(char* buf, size_t buf_size) {
    reg_query_sz(HKEY_LOCAL_MACHINE,
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        "ProcessorNameString", buf, buf_size);
}

void info_cpu_vendor(char* buf, size_t buf_size) {
    reg_query_sz(HKEY_LOCAL_MACHINE,
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        "VendorIdentifier", buf, buf_size);
}

void info_cpu_frequency(char* buf, size_t buf_size) {
    if (!buf || buf_size == 0) return;
    HKEY h;
    DWORD val = 0, size = sizeof(val);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &h) == ERROR_SUCCESS) {
        if (RegQueryValueExA(h, "~MHz", NULL, NULL, (LPBYTE)&val, &size) == ERROR_SUCCESS)
            snprintf(buf, buf_size, "%lu MHz", (unsigned long)val);
        RegCloseKey(h);
    } else {
        LARGE_INTEGER f;
        if (QueryPerformanceFrequency(&f) && f.QuadPart > 0)
            snprintf(buf, buf_size, "%llu MHz", (unsigned long long)(f.QuadPart / 1000000));
        else
            buf[0] = '\0';
    }
}

void info_cpu_architecture(char* buf, size_t buf_size) {
    if (!buf || buf_size == 0) return;
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    const char* name = "Unknown";
    switch (si.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64: name = "x64"; break;
    case PROCESSOR_ARCHITECTURE_ARM:   name = "ARM"; break;
    case PROCESSOR_ARCHITECTURE_ARM64: name = "ARM64"; break;
    case PROCESSOR_ARCHITECTURE_IA64:  name = "Itanium"; break;
    case PROCESSOR_ARCHITECTURE_INTEL: name = "x86"; break;
    default: break;
    }
    snprintf(buf, buf_size, "%s", name);
}

void info_cpu_quantities(info_cpu_quantities_t* out) {
    if (!out) return;
    out->logical = 0;
    out->physical = 0;
    out->packages = 0;

    DWORD size = 0;
    GetLogicalProcessorInformation(NULL, &size);
    if (size == 0) return;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)malloc(size);
    if (!info) return;
    if (GetLogicalProcessorInformation(info, &size)) {
        int count = (int)(size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        for (int i = 0; i < count; i++) {
            if (info[i].Relationship == RelationProcessorCore) {
                out->physical++;
                out->logical += popcount_mask(info[i].ProcessorMask);
            } else if (info[i].Relationship == RelationProcessorPackage) {
                out->packages++;
            }
        }
    }
    free(info);
}

/* ---------------- Motherboard ---------------- */

void info_motherboard(info_motherboard_t* out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    const char* key = "HARDWARE\\DESCRIPTION\\System\\BIOS";
    reg_query_sz(HKEY_LOCAL_MACHINE, key, "BaseBoardManufacturer", out->manufacturer, sizeof(out->manufacturer));
    reg_query_sz(HKEY_LOCAL_MACHINE, key, "BaseBoardProduct", out->product, sizeof(out->product));
    reg_query_sz(HKEY_LOCAL_MACHINE, key, "BaseBoardManufacturerVersion", out->manufacturer_version, sizeof(out->manufacturer_version));
    reg_query_sz(HKEY_LOCAL_MACHINE, key, "BaseBoardProductVersion", out->product_version, sizeof(out->product_version));
    reg_query_sz(HKEY_LOCAL_MACHINE, key, "BaseBoardSerialNumber", out->serial, sizeof(out->serial));
    reg_query_sz(HKEY_LOCAL_MACHINE, key, "SystemManufacturer", out->system_manufacturer, sizeof(out->system_manufacturer));
    reg_query_sz(HKEY_LOCAL_MACHINE, key, "SystemProductName", out->system_product, sizeof(out->system_product));
    reg_query_sz(HKEY_LOCAL_MACHINE, key, "BIOSManufacturer", out->bios_manufacturer, sizeof(out->bios_manufacturer));
    reg_query_sz(HKEY_LOCAL_MACHINE, key, "BIOSVersion", out->bios_version, sizeof(out->bios_version));
}

/* ---------------- Memory ---------------- */

void info_memory(info_memory_t* out) {
    if (!out) return;
    out->available_mb = 0;
    out->total_mb = 0;
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        out->available_mb = ms.ullAvailPhys / (1024 * 1024);
        out->total_mb = ms.ullTotalPhys / (1024 * 1024);
    }
}

/* ---------------- OS ---------------- */

typedef LONG (NTAPI *RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

void info_os(info_os_t* out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    strncpy(out->name, "Windows NT", sizeof(out->name) - 1);

    HMODULE ntdll = LoadLibraryA("ntdll.dll");
    if (ntdll) {
        RtlGetVersionPtr fn = (RtlGetVersionPtr)(void*)GetProcAddress(ntdll, "RtlGetVersion");
        if (fn) {
            RTL_OSVERSIONINFOW v;
            v.dwOSVersionInfoSize = sizeof(v);
            if (fn(&v) == 0) {
                out->major = v.dwMajorVersion;
                out->minor = v.dwMinorVersion;
                out->build = v.dwBuildNumber;
            }
        }
        FreeLibrary(ntdll);
    }

    reg_query_sz(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        "ProductName", out->full_name, sizeof(out->full_name));

    HKEY h;
    DWORD ubr = 0, size = sizeof(ubr);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &h) == ERROR_SUCCESS) {
        if (RegQueryValueExA(h, "UBR", NULL, NULL, (LPBYTE)&ubr, &size) == ERROR_SUCCESS)
            out->ubr = ubr;
        RegCloseKey(h);
    }
}

/* ---------------- HID ---------------- */

void info_hid(info_hid_t* out) {
    if (!out) return;
    out->mice = 0;
    out->keyboards = 0;
    out->other = 0;
    RAWINPUTDEVICELIST list[64];
    UINT n = 64;
    if (GetRawInputDeviceList(list, &n, sizeof(RAWINPUTDEVICELIST)) >= 0) {
        for (UINT i = 0; i < n; i++) {
            if (list[i].dwType == RIM_TYPEMOUSE) out->mice++;
            else if (list[i].dwType == RIM_TYPEKEYBOARD) out->keyboards++;
            else if (list[i].dwType == RIM_TYPEHID) out->other++;
        }
    }
}

/* ---------------- Network ---------------- */

static const GUID NET_CLASS_GUID =
    {0x4d36e972, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}};

int info_network(char (*lines)[512], int max_count) {
    if (!lines || max_count <= 0) return 0;
    int count = 0;
    HDEVINFO hdevinfo = SetupDiGetClassDevs(&NET_CLASS_GUID, NULL, NULL, DIGCF_PRESENT);
    if (hdevinfo == INVALID_HANDLE_VALUE) return 0;
    for (DWORD i = 0; count < max_count; i++) {
        SP_DEVINFO_DATA devinfo;
        devinfo.cbSize = sizeof(devinfo);
        if (!SetupDiEnumDeviceInfo(hdevinfo, i, &devinfo)) break;
        char desc[512];
        desc[0] = '\0';
        DWORD desc_size = sizeof(desc);
        if (SetupDiGetDeviceRegistryPropertyA(hdevinfo, &devinfo, SPDRP_DEVICEDESC,
                NULL, (LPBYTE)desc, desc_size, NULL)) {
            snprintf(lines[count], 512, "%s", desc);
            count++;
        }
    }
    SetupDiDestroyDeviceInfoList(hdevinfo);
    return count;
}

/* ---------------- Displays ---------------- */

typedef struct {
    info_display_t* out;
    int max;
    int count;
    HDC desktop;
} display_bundle;

static BOOL CALLBACK enum_display(HMONITOR, HDC hdc, LPRECT rect, LPARAM lp) {
    display_bundle* b = (display_bundle*)lp;
    if (b->count >= b->max) return TRUE;
    unsigned dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    unsigned bpp = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
    double refresh = GetDeviceCaps(hdc, VREFRESH);
    if (!dpi) dpi = GetDeviceCaps(b->desktop, LOGPIXELSX);
    if (!bpp) bpp = GetDeviceCaps(b->desktop, BITSPIXEL) * GetDeviceCaps(b->desktop, PLANES);
    if (!refresh) refresh = GetDeviceCaps(b->desktop, VREFRESH);
    b->out[b->count].width = (uint32_t)(rect->right - rect->left);
    b->out[b->count].height = (uint32_t)(rect->bottom - rect->top);
    b->out[b->count].dpi = dpi;
    b->out[b->count].bpp = bpp;
    b->out[b->count].refresh = refresh;
    b->count++;
    return TRUE;
}

int info_displays(info_display_t* out, int max_count) {
    if (!out || max_count <= 0) return 0;
    display_bundle b;
    b.out = out;
    b.max = max_count;
    b.count = 0;
    b.desktop = GetDC(NULL);
    EnumDisplayMonitors(b.desktop, NULL, enum_display, (LPARAM)&b);
    ReleaseDC(NULL, b.desktop);
    return b.count;
}

/* ---------------- GPU ---------------- */

int info_gpus(char (*names)[256], int max_count) {
    if (!names || max_count <= 0) return 0;
    int count = 0;
    for (DWORD i = 0; count < max_count; i++) {
        DISPLAY_DEVICEA adapter;
        adapter.cb = sizeof(adapter);
        if (!EnumDisplayDevicesA(NULL, i, &adapter, 0)) break;
        if (!(adapter.StateFlags & DISPLAY_DEVICE_ACTIVE)) continue;

        char line[512];
        strncpy(line, adapter.DeviceString, sizeof(line) - 1);
        line[sizeof(line) - 1] = '\0';

        for (DWORD j = 0; ; j++) {
            DISPLAY_DEVICEA child;
            child.cb = sizeof(child);
            if (!EnumDisplayDevicesA(adapter.DeviceName, j, &child, 0)) break;
            char tail[280];
            snprintf(tail, sizeof(tail), ", %s", child.DeviceString);
            strncat(line, tail, sizeof(line) - strlen(line) - 1);
        }
        snprintf(names[count], 256, "%s", line);
        count++;
    }
    return count;
}

/* ---------------- Time ---------------- */

void info_time(char* buf, size_t buf_size) {
    if (!buf || buf_size == 0) return;
    SYSTEMTIME st;
    GetLocalTime(&st);
    TIME_ZONE_INFORMATION tz;
    GetTimeZoneInformation(&tz);
    snprintf(buf, buf_size, "%d.%d.%d %d:%d:%d bias %d min",
        st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, (int)(tz.Bias / 60));
}

/* ---------------- Languages ---------------- */

int info_languages(char (*names)[256], int max_count) {
    if (!names || max_count <= 0) return 0;
    int count = GetKeyboardLayoutList(0, NULL);
    if (count <= 0) return 0;
    HKL* list = (HKL*)malloc((size_t)count * sizeof(HKL));
    if (!list) return 0;
    GetKeyboardLayoutList(count, list);
    int n = 0;
    for (int i = 0; i < count && n < max_count; i++) {
        char lang[256];
        if (VerLanguageNameA(LOWORD((ULONG_PTR)list[i]), lang, sizeof(lang))) {
            snprintf(names[n], 256, "%s", lang);
            n++;
        }
    }
    free(list);
    return n;
}

/* ---------------- Screen ---------------- */

void info_screen(char* buf, size_t buf_size) {
    if (!buf || buf_size == 0) return;
    RECT r;
    if (GetWindowRect(GetDesktopWindow(), &r))
        snprintf(buf, buf_size, "%ldx%ld", (long)r.right, (long)r.bottom);
    else
        buf[0] = '\0';
}

/* ---------------- Processes ---------------- */

int info_processes(char (*lines)[512], int max_count) {
    if (!lines || max_count <= 0) return 0;
    int count = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
            if (h) {
                wchar_t path[MAX_PATH];
                DWORD size = MAX_PATH;
                if (QueryFullProcessImageNameW(h, 0, path, &size)) {
                    if (wcsstr(path, L":\\Windows\\")) {
                        char narrow[512];
                        w2a(path, narrow, sizeof(narrow));
                        snprintf(lines[count], 512, "%lu\t%s", (unsigned long)pe.th32ProcessID, narrow);
                        count++;
                        if (count >= max_count) break;
                    }
                }
                CloseHandle(h);
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return count;
}

/* ---------------- Modules ---------------- */

int info_modules(char (*lines)[512], int max_count) {
    if (!lines || max_count <= 0) return 0;
    int count = 0;
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
        FALSE, GetCurrentProcessId());
    if (!h) return 0;
    HMODULE mods[1024];
    DWORD needed = 0;
    if (EnumProcessModules(h, mods, sizeof(mods), &needed)) {
        int n = (int)(needed / sizeof(HMODULE));
        for (int i = 0; i < n && count < max_count; i++) {
            wchar_t path[MAX_PATH];
            if (GetModuleFileNameExW(h, mods[i], path, MAX_PATH)) {
                if (wcsstr(path, L":\\Windows\\")) {
                    char narrow[512];
                    w2a(path, narrow, sizeof(narrow));
                    snprintf(lines[count], 512, "%s", narrow);
                    count++;
                }
            }
        }
    }
    CloseHandle(h);
    return count;
}

/* ---------------- Drivers ---------------- */

int info_drivers(char (*lines)[512], int max_count) {
    if (!lines || max_count <= 0) return 0;
    int count = 0;
    LPVOID drivers[1024];
    DWORD needed = 0;
    if (EnumDeviceDrivers(drivers, sizeof(drivers), &needed) && needed < sizeof(drivers)) {
        int n = (int)(needed / sizeof(LPVOID));
        for (int i = 0; i < n && count < max_count; i++) {
            wchar_t name[512];
            if (GetDeviceDriverFileNameW(drivers[i], name, sizeof(name) / sizeof(name[0]))) {
                if (wcsstr(name, L"ystem32\\")) {
                    char narrow[512];
                    w2a(name, narrow, sizeof(narrow));
                    snprintf(lines[count], 512, "%s", narrow);
                    count++;
                }
            }
        }
    }
    return count;
}

/* ---------------- Disks ---------------- */

int info_disks(info_disk_t* out, int max_count) {
    if (!out || max_count <= 0) return 0;
    int count = 0;
    wchar_t drives[1024];
    DWORD n = GetLogicalDriveStringsW(1024, drives);
    if (!n) return 0;
    for (DWORD i = 0; i < n && count < max_count; i++) {
        wchar_t* root = &drives[i];
        info_disk_t* d = &out[count];
        w2a(root, d->root, sizeof(d->root));
        switch (GetDriveTypeW(root)) {
        case DRIVE_FIXED:     strcpy(d->type, "DRIVE_FIXED"); break;
        case DRIVE_REMOVABLE: strcpy(d->type, "DRIVE_REMOVABLE"); break;
        case DRIVE_REMOTE:    strcpy(d->type, "DRIVE_REMOTE"); break;
        case DRIVE_CDROM:     strcpy(d->type, "DRIVE_CDROM"); break;
        case DRIVE_RAMDISK:   strcpy(d->type, "DRIVE_RAMDISK"); break;
        default:              strcpy(d->type, "DRIVE_UNKNOWN"); break;
        }
        wchar_t vol[MAX_PATH], fs[MAX_PATH];
        DWORD serial = 0, maxcomp = 0, flags = 0;
        if (GetVolumeInformationW(root, vol, MAX_PATH, &serial, &maxcomp, &flags, fs, MAX_PATH)) {
            w2a(vol, d->label, sizeof(d->label));
            w2a(fs, d->fs, sizeof(d->fs));
            d->serial = serial;
        }
        count++;
        i += wcslen(root);
    }
    return count;
}

/* ---------------- Software ---------------- */

int info_software(char (*names)[256], int max_count) {
    if (!names || max_count <= 0) return 0;
    int count = 0;
    const char* roots[2] = {
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
    };
    for (int r = 0; r < 2 && count < max_count; r++) {
        HKEY h;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, roots[r], 0, KEY_READ, &h) != ERROR_SUCCESS) continue;
        for (DWORD i = 0; count < max_count; i++) {
            char subkey[256];
            DWORD subkey_size = sizeof(subkey);
            if (RegEnumKeyExA(h, i, subkey, &subkey_size, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) break;
            char full[512];
            snprintf(full, sizeof(full), "%s\\%s", roots[r], subkey);
            HKEY app;
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, full, 0, KEY_READ, &app) != ERROR_SUCCESS) continue;

            char publisher[256];
            publisher[0] = '\0';
            DWORD psize = sizeof(publisher);
            RegQueryValueExA(app, "Publisher", NULL, NULL, (LPBYTE)publisher, &psize);
            publisher[sizeof(publisher) - 1] = '\0';
            if (strcmp(publisher, "Microsoft Corporation") == 0) {
                RegCloseKey(app);
                continue;
            }

            char display[256];
            DWORD dsize = sizeof(display);
            if (RegQueryValueExA(app, "DisplayName", NULL, NULL, (LPBYTE)display, &dsize) == ERROR_SUCCESS) {
                display[sizeof(display) - 1] = '\0';
                snprintf(names[count], 256, "%s", display);
                count++;
            }
            RegCloseKey(app);
        }
        RegCloseKey(h);
    }
    return count;
}

/* ---------------- Services ---------------- */

int info_services(char (*lines)[512], int max_count) {
    if (!lines || max_count <= 0) return 0;
    int count = 0;
    SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (!scm) return 0;

    DWORD bytes = 0, num = 0, resume = 0;
    EnumServicesStatusA(scm, SERVICE_WIN32, SERVICE_ACTIVE, NULL, 0, &bytes, &num, &resume);
    if (bytes == 0) {
        CloseServiceHandle(scm);
        return 0;
    }
    ENUM_SERVICE_STATUSA* list = (ENUM_SERVICE_STATUSA*)malloc(bytes);
    if (!list) {
        CloseServiceHandle(scm);
        return 0;
    }
    if (EnumServicesStatusA(scm, SERVICE_WIN32, SERVICE_ACTIVE, list, bytes, &bytes, &num, &resume)) {
        for (DWORD i = 0; i < num && count < max_count; i++) {
            SC_HANDLE svc = OpenServiceA(scm, list[i].lpServiceName, SERVICE_QUERY_CONFIG);
            if (!svc) continue;
            char cfgbuf[8192];
            DWORD cfgsize = sizeof(cfgbuf);
            if (QueryServiceConfigA(svc, (LPQUERY_SERVICE_CONFIGA)cfgbuf, cfgsize, &cfgsize)) {
                QUERY_SERVICE_CONFIGA* cfg = (QUERY_SERVICE_CONFIGA*)cfgbuf;
                if (strstr(cfg->lpBinaryPathName, ":\\Windows\\")) {
                    snprintf(lines[count], 512, "%s\t%s", list[i].lpServiceName, cfg->lpBinaryPathName);
                    count++;
                }
            }
            CloseServiceHandle(svc);
        }
    }
    free(list);
    CloseServiceHandle(scm);
    return count;
}
