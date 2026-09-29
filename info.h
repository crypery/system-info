/*
 * Git: https://github.com/crypery
 * Author: https://crypery.com
 * License: GNU AGPL v3 (Affero GPL)
 */
#ifndef INFO_H
#define INFO_H

#include <windows.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CPU */
typedef struct {
    uint32_t logical;
    uint32_t physical;
    uint32_t packages;
} info_cpu_quantities_t;

void info_cpu_model(char* buf, size_t buf_size);
void info_cpu_vendor(char* buf, size_t buf_size);
void info_cpu_architecture(char* buf, size_t buf_size);
void info_cpu_frequency(char* buf, size_t buf_size);
void info_cpu_quantities(info_cpu_quantities_t* out);

/* Motherboard (registry HARDWARE\DESCRIPTION\System\BIOS) */
typedef struct {
    char manufacturer[128];
    char product[128];
    char manufacturer_version[64];
    char product_version[64];
    char serial[64];
    char system_manufacturer[128];
    char system_product[128];
    char bios_manufacturer[128];
    char bios_version[64];
} info_motherboard_t;

void info_motherboard(info_motherboard_t* out);

/* Memory */
typedef struct {
    uint64_t available_mb;
    uint64_t total_mb;
} info_memory_t;

void info_memory(info_memory_t* out);

/* OS */
typedef struct {
    char name[64];
    char full_name[128];
    uint32_t major;
    uint32_t minor;
    uint32_t build;
    uint32_t ubr;
} info_os_t;

void info_os(info_os_t* out);

/* HID devices */
typedef struct {
    uint32_t mice;
    uint32_t keyboards;
    uint32_t other;
} info_hid_t;

void info_hid(info_hid_t* out);

/* Installed network devices (SetupAPI, network class) */
int info_network(char (*lines)[512], int max_count);

/* Displays */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t dpi;
    uint32_t bpp;
    double refresh;
} info_display_t;

int info_displays(info_display_t* out, int max_count);

/* GPU adapters */
int info_gpus(char (*names)[256], int max_count);

/* Time / timezone */
void info_time(char* buf, size_t buf_size);

/* Keyboard languages */
int info_languages(char (*names)[256], int max_count);

/* Virtual screen */
void info_screen(char* buf, size_t buf_size);

/* Processes (system32 only), lines: "pid\texe path" */
int info_processes(char (*lines)[512], int max_count);

/* Modules of current process (system32 only) */
int info_modules(char (*lines)[512], int max_count);

/* Loaded kernel drivers (system32 only) */
int info_drivers(char (*lines)[512], int max_count);

/* Disks */
typedef struct {
    char root[8];
    char type[32];
    char fs[32];
    char label[260];
    uint32_t serial;
} info_disk_t;

int info_disks(info_disk_t* out, int max_count);

/* Installed software (registry Uninstall keys, Microsoft skipped) */
int info_software(char (*names)[256], int max_count);

/* Active services (system32 only), lines: "name\tbinary path" */
int info_services(char (*lines)[512], int max_count);

#ifdef __cplusplus
}
#endif

#endif /* INFO_H */
