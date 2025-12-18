/*
 * display_vrr.c - Venom Display VRR (Variable Refresh Rate)
 * معدل التحديث المتغير
 */

#include "display.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>

// ═══════════════════════════════════════════════════════════════════════════
// ثوابت
// ═══════════════════════════════════════════════════════════════════════════

#define DRM_PATH "/sys/class/drm"

// ═══════════════════════════════════════════════════════════════════════════
// دوال مساعدة
// ═══════════════════════════════════════════════════════════════════════════

static gchar* find_drm_connector(const gchar *display_name) {
    DIR *dir = opendir(DRM_PATH);
    if (!dir) return NULL;
    
    struct dirent *entry;
    gchar *result = NULL;
    
    while ((entry = readdir(dir))) {
        if (g_str_has_prefix(entry->d_name, "card") && 
            strstr(entry->d_name, display_name)) {
            result = g_strdup_printf("%s/%s", DRM_PATH, entry->d_name);
            break;
        }
    }
    closedir(dir);
    
    // If not found by name, try matching by connector type
    if (!result) {
        dir = opendir(DRM_PATH);
        if (dir) {
            while ((entry = readdir(dir))) {
                if (g_str_has_prefix(entry->d_name, "card0-")) {
                    // Check if this might match (HDMI, DP, eDP, etc.)
                    gchar *type = strchr(display_name, '-');
                    if (type && strstr(entry->d_name, display_name)) {
                        result = g_strdup_printf("%s/%s", DRM_PATH, entry->d_name);
                        break;
                    }
                }
            }
            closedir(dir);
        }
    }
    
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
// VRR API
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_vrr_capable(const gchar *name) {
    gchar *connector = find_drm_connector(name);
    if (!connector) return FALSE;
    
    gchar *path = g_strdup_printf("%s/vrr_capable", connector);
    int capable = read_sysfs_int(path);
    
    g_free(path);
    g_free(connector);
    
    return (capable == 1);
}

gboolean display_vrr_enabled(const gchar *name) {
    // Check kernel AMDGPU/i915 VRR status
    gchar *connector = find_drm_connector(name);
    if (!connector) return FALSE;
    
    // Try AMDGPU path
    gchar *path = g_strdup_printf("%s/../vrr_enabled", connector);
    int enabled = read_sysfs_int(path);
    g_free(path);
    
    if (enabled < 0) {
        // Try alternative path
        path = g_strdup_printf("/sys/module/amdgpu/parameters/freesync_video");
        enabled = read_sysfs_int(path);
        g_free(path);
    }
    
    g_free(connector);
    
    return (enabled == 1);
}

gboolean display_vrr_set(const gchar *name, gboolean enable) {
    // For AMDGPU: write to freesync parameter
    gchar *path = g_strdup_printf("/sys/module/amdgpu/parameters/freesync_video");
    gboolean result = write_sysfs_int(path, enable ? 1 : 0);
    g_free(path);
    
    if (!result) {
        // Try i915 path
        path = g_strdup_printf("/sys/module/i915/parameters/enable_fbc");
        result = write_sysfs_int(path, enable ? 1 : 0);
        g_free(path);
    }
    
    (void)name;  // Name used for future per-display VRR
    return result;
}

VRRInfo display_vrr_get_info(const gchar *name) {
    VRRInfo info = {0};
    
    info.capable = display_vrr_capable(name);
    info.enabled = display_vrr_enabled(name);
    
    // Try to read min/max refresh rates from EDID or sysfs
    EDIDInfo *edid = display_get_edid(name);
    if (edid) {
        info.min_rate = 48;   // Common VRR minimum
        info.max_rate = 144;  // Estimate, should parse from EDID VRR block
        display_edid_free(edid);
    }
    
    return info;
}
