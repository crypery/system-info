/*
 * Git: https://github.com/crypery
 * Author: https://crypery.com
 * License: GNU AGPL v3 (Affero GPL)
 */
#include "info.h"
#include <stdio.h>

int main(void) {
    static char buf[512];
    static char lines[512][512];
    static char names[256][256];
    static info_display_t displays[16];
    static info_disk_t disks[32];
    int n;

    info_os_t os;
    info_os(&os);
    printf("\n  OS: \n");
    printf("\tName:\t\t%s\n", os.name);
    printf("\tFull name:\t%s\n", os.full_name);
    printf("\tVersion:\t%u.%u.%u build %u\n", os.major, os.minor, os.build, os.ubr);

    printf("\n  CPU: \n");
    info_cpu_model(buf, sizeof(buf));
    printf("\tModel name:\t%s\n", buf);
    info_cpu_vendor(buf, sizeof(buf));
    printf("\tVendor ID:\t%s\n", buf);
    info_cpu_architecture(buf, sizeof(buf));
    printf("\tArchitecture:\t%s\n", buf);
    info_cpu_frequency(buf, sizeof(buf));
    printf("\tFrequency:\t%s\n", buf);

    info_cpu_quantities_t q;
    info_cpu_quantities(&q);
    printf("\n  Quantities: \n");
    printf("\tLogical CPUs:\t%u\n", q.logical);
    printf("\tPhysical CPUs:\t%u\n", q.physical);
    printf("\tCPU packages:\t%u\n", q.packages);

    info_motherboard_t mb;
    info_motherboard(&mb);
    printf("\n  Motherboard: \n");
    printf("\tManufacturer:\t%s\n", mb.manufacturer);
    printf("\tProduct:\t\t%s\n", mb.product);
    if (mb.manufacturer_version[0]) printf("\tManufacturer version:\t%s\n", mb.manufacturer_version);
    if (mb.product_version[0]) printf("\tProduct version:\t%s\n", mb.product_version);
    if (mb.serial[0]) printf("\tSerial:\t\t%s\n", mb.serial);
    printf("\tSystem:\t\t%s %s\n", mb.system_manufacturer, mb.system_product);
    printf("\tBIOS:\t\t%s %s\n", mb.bios_manufacturer, mb.bios_version);

    info_memory_t mem;
    info_memory(&mem);
    printf("\n  Memory: \n");
    printf("\tAvailable:\t%llu Mb\n", (unsigned long long)mem.available_mb);
    printf("\tTotal:\t\t%llu Mb\n", (unsigned long long)mem.total_mb);

    int ndisk = info_disks(disks, 32);
    printf("\n  Disk: \n");
    for (int i = 0; i < ndisk; i++)
        printf("\t%s\t%s\t%s\t%s\t%lu\n",
            disks[i].root, disks[i].type, disks[i].fs, disks[i].label,
            (unsigned long)disks[i].serial);

    int ng = info_gpus(names, 16);
    printf("\n  GPU: \n");
    for (int i = 0; i < ng; i++) printf("\t%s\n", names[i]);

    info_hid_t hid;
    info_hid(&hid);
    printf("\n  Connected HIDs: \n");
    printf("\tMouse:\t\t%u\n", hid.mice);
    printf("\tKeyboards:\t%u\n", hid.keyboards);
    printf("\tOther:\t\t%u\n", hid.other);

    n = info_network(lines, 512);
    printf("\n  Network: \n");
    for (int i = 0; i < n; i++) printf("\t%s\n", lines[i]);

    int nd = info_displays(displays, 16);
    printf("\n  Displays: \n");
    if (nd == 0)
        printf("\tNone connected\n");
    for (int i = 0; i < nd; i++) {
        printf("\t#%d:\n", i + 1);
        printf("\t Resolution:\t%ux%u\n", displays[i].width, displays[i].height);
        printf("\t DPI:\t\t%u\n", displays[i].dpi);
        printf("\t Color depth:\t%u b\n", displays[i].bpp);
        printf("\t Refresh rate:\t%.2f Hz\n", displays[i].refresh);
    }

    info_time(buf, sizeof(buf));
    printf("\n  Time Date: \n");
    printf("\t%s\n", buf);

    int nl = info_languages(names, 16);
    printf("\n  Language: \n");
    for (int i = 0; i < nl; i++) printf("\t%s\n", names[i]);

    info_screen(buf, sizeof(buf));
    printf("\n  Screen: \n");
    printf("\t%s\n", buf);

    n = info_processes(lines, 512);
    printf("\n  Process: \n");
    for (int i = 0; i < n; i++) printf("\t%s\n", lines[i]);

    n = info_modules(lines, 512);
    printf("\n  Modules: \n");
    for (int i = 0; i < n; i++) printf("\t%s\n", lines[i]);

    n = info_drivers(lines, 512);
    printf("\n  Drivers: \n");
    for (int i = 0; i < n; i++) printf("\t%s\n", lines[i]);

    int ns = info_software(names, 256);
    printf("\n  Softwares: \n");
    for (int i = 0; i < ns; i++) printf("\t%s\n", names[i]);

    n = info_services(lines, 512);
    printf("\n  Services: \n");
    for (int i = 0; i < n; i++) printf("\t%s\n", lines[i]);

    return 0;
}
