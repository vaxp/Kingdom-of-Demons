/*
 * display_mode.c - Venom Display Mode Management
 * الدقة ومعدل التحديث
 */

#include "display.h"
#include "venom_display.h"
#include <math.h>

// ═══════════════════════════════════════════════════════════════════════════
// دوال خارجية
// ═══════════════════════════════════════════════════════════════════════════

extern XRRScreenResources *screen_res;

// ═══════════════════════════════════════════════════════════════════════════
// البحث عن الوضع
// ═══════════════════════════════════════════════════════════════════════════

static RRMode find_mode(const gchar *output_name, int width, int height, double rate) {
    for (int i = 0; i < screen_res->noutput; i++) {
        XRROutputInfo *output = XRRGetOutputInfo(display_state.x_display, screen_res, 
                                                  screen_res->outputs[i]);
        if (!output) continue;
        
        if (g_strcmp0(output->name, output_name) == 0) {
            for (int j = 0; j < output->nmode; j++) {
                XRRModeInfo *mode = display_find_mode_by_id(output->modes[j]);
                if (mode && mode->width == (unsigned)width && mode->height == (unsigned)height) {
                    double mode_rate = display_get_mode_refresh_rate(mode);
                    if (rate <= 0 || fabs(mode_rate - rate) < 1.0) {
                        RRMode result = mode->id;
                        XRRFreeOutputInfo(output);
                        return result;
                    }
                }
            }
        }
        XRRFreeOutputInfo(output);
    }
    return None;
}

// ═══════════════════════════════════════════════════════════════════════════
// الأوضاع المتاحة
// ═══════════════════════════════════════════════════════════════════════════

GList* display_get_modes(const gchar *name) {
    GList *modes = NULL;
    
    for (int i = 0; i < screen_res->noutput; i++) {
        XRROutputInfo *output = XRRGetOutputInfo(display_state.x_display, screen_res, 
                                                  screen_res->outputs[i]);
        if (!output) continue;
        
        if (g_strcmp0(output->name, name) == 0) {
            for (int j = 0; j < output->nmode; j++) {
                XRRModeInfo *mode_info = display_find_mode_by_id(output->modes[j]);
                if (mode_info) {
                    DisplayMode *mode = g_new0(DisplayMode, 1);
                    mode->width = mode_info->width;
                    mode->height = mode_info->height;
                    mode->rates = g_new(double, 1);
                    mode->rates[0] = display_get_mode_refresh_rate(mode_info);
                    mode->rate_count = 1;
                    modes = g_list_append(modes, mode);
                }
            }
        }
        XRRFreeOutputInfo(output);
    }
    
    return modes;
}

// ═══════════════════════════════════════════════════════════════════════════
// تغيير الإعدادات
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_set_mode(const gchar *name, int width, int height, double rate) {
    DisplayInfo *info = display_get_by_name(name);
    if (!info || !info->crtc_id) {
        display_info_free(info);
        return FALSE;
    }
    
    RRMode mode = find_mode(name, width, height, rate);
    if (mode == None) {
        display_info_free(info);
        return FALSE;
    }
    
    Status status = XRRSetCrtcConfig(display_state.x_display, screen_res, info->crtc_id,
                                      CurrentTime, info->x, info->y, mode,
                                      RR_Rotate_0, &info->output_id, 1);
    
    display_info_free(info);
    
    return (status == RRSetConfigSuccess);
}

gboolean display_set_resolution(const gchar *name, int width, int height) {
    return display_set_mode(name, width, height, -1);
}

gboolean display_set_refresh_rate(const gchar *name, double rate) {
    DisplayInfo *info = display_get_by_name(name);
    if (!info) return FALSE;
    
    gboolean result = display_set_mode(name, info->width, info->height, rate);
    display_info_free(info);
    return result;
}
