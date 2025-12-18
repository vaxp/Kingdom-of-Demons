/*
 * display_backlight.c - Venom Display Backlight Control
 * التحكم بإضاءة شاشة اللابتوب
 */

#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

// ═══════════════════════════════════════════════════════════════════════════
// ثوابت
// ═══════════════════════════════════════════════════════════════════════════

#define BACKLIGHT_PATH "/sys/class/backlight"

// ═══════════════════════════════════════════════════════════════════════════
// دوال مساعدة
// ═══════════════════════════════════════════════════════════════════════════

static gchar* find_backlight_device(void) {
    DIR *dir = opendir(BACKLIGHT_PATH);
    if (!dir) return NULL;
    
    struct dirent *entry;
    gchar *result = NULL;
    
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] != '.') {
            result = g_strdup_printf("%s/%s", BACKLIGHT_PATH, entry->d_name);
            break;
        }
    }
    closedir(dir);
    return result;
}

static int read_sysfs_int(const gchar *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    
    int value;
    if (fscanf(f, "%d", &value) != 1) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return value;
}

static gboolean write_sysfs_int(const gchar *path, int value) {
    FILE *f = fopen(path, "w");
    if (!f) return FALSE;
    
    fprintf(f, "%d", value);
    fclose(f);
    return TRUE;
}

// ═══════════════════════════════════════════════════════════════════════════
// Backlight API
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_backlight_available(void) {
    gchar *device = find_backlight_device();
    gboolean result = (device != NULL);
    g_free(device);
    return result;
}

int display_backlight_get_max(void) {
    gchar *device = find_backlight_device();
    if (!device) return -1;
    
    gchar *path = g_strdup_printf("%s/max_brightness", device);
    int result = read_sysfs_int(path);
    
    g_free(path);
    g_free(device);
    return result;
}

int display_backlight_get(void) {
    gchar *device = find_backlight_device();
    if (!device) return -1;
    
    gchar *brightness_path = g_strdup_printf("%s/brightness", device);
    gchar *max_path = g_strdup_printf("%s/max_brightness", device);
    
    int current = read_sysfs_int(brightness_path);
    int max = read_sysfs_int(max_path);
    
    g_free(brightness_path);
    g_free(max_path);
    g_free(device);
    
    if (current < 0 || max <= 0) return -1;
    
    return (current * 100) / max;
}

gboolean display_backlight_set(int level) {
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    
    gchar *device = find_backlight_device();
    if (!device) return FALSE;
    
    gchar *brightness_path = g_strdup_printf("%s/brightness", device);
    gchar *max_path = g_strdup_printf("%s/max_brightness", device);
    
    int max = read_sysfs_int(max_path);
    g_free(max_path);
    
    if (max <= 0) {
        g_free(brightness_path);
        g_free(device);
        return FALSE;
    }
    
    int value = (level * max) / 100;
    if (value < 1 && level > 0) value = 1;
    
    gboolean result = write_sysfs_int(brightness_path, value);
    
    g_free(brightness_path);
    g_free(device);
    return result;
}

gchar* display_backlight_get_device(void) {
    return find_backlight_device();
}
