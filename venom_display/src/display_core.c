/*
 * display_core.c - Venom Display Core Functions
 * التهيئة والوظائف الأساسية
 */

#include "display.h"
#include "venom_display.h"
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// الحالة العامة
// ═══════════════════════════════════════════════════════════════════════════

XRRScreenResources *screen_res = NULL;

// ═══════════════════════════════════════════════════════════════════════════
// التهيئة والتنظيف
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_init(void) {
    display_state.x_display = XOpenDisplay(NULL);
    if (!display_state.x_display) {
        return FALSE;
    }
    
    display_state.screen = DefaultScreen(display_state.x_display);
    display_state.root_window = RootWindow(display_state.x_display, display_state.screen);
    
    screen_res = XRRGetScreenResources(display_state.x_display, display_state.root_window);
    if (!screen_res) {
        return FALSE;
    }
    
    return TRUE;
}

void display_cleanup(void) {
    if (screen_res) {
        XRRFreeScreenResources(screen_res);
        screen_res = NULL;
    }
    if (display_state.x_display) {
        XCloseDisplay(display_state.x_display);
        display_state.x_display = NULL;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// دوال مساعدة
// ═══════════════════════════════════════════════════════════════════════════

void display_refresh_resources(void) {
    if (screen_res) {
        XRRFreeScreenResources(screen_res);
    }
    screen_res = XRRGetScreenResources(display_state.x_display, display_state.root_window);
}

XRRModeInfo* display_find_mode_by_id(RRMode mode_id) {
    for (int i = 0; i < screen_res->nmode; i++) {
        if (screen_res->modes[i].id == mode_id) {
            return &screen_res->modes[i];
        }
    }
    return NULL;
}

double display_get_mode_refresh_rate(XRRModeInfo *mode) {
    if (mode->hTotal && mode->vTotal) {
        return (double)mode->dotClock / ((double)mode->hTotal * (double)mode->vTotal);
    }
    return 0.0;
}

// ═══════════════════════════════════════════════════════════════════════════
// تنظيف الذاكرة
// ═══════════════════════════════════════════════════════════════════════════

void display_info_free(DisplayInfo *info) {
    if (info) {
        g_free(info->name);
        g_free(info);
    }
}

void display_mode_free(DisplayMode *mode) {
    if (mode) {
        g_free(mode->rates);
        g_free(mode);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// جلب معلومات الشاشات
// ═══════════════════════════════════════════════════════════════════════════

GList* display_get_all(void) {
    GList *displays = NULL;
    
    display_refresh_resources();
    
    RROutput primary = XRRGetOutputPrimary(display_state.x_display, display_state.root_window);
    
    for (int i = 0; i < screen_res->noutput; i++) {
        XRROutputInfo *output = XRRGetOutputInfo(display_state.x_display, screen_res, 
                                                  screen_res->outputs[i]);
        if (!output) continue;
        
        DisplayInfo *info = g_new0(DisplayInfo, 1);
        info->name = g_strdup(output->name);
        info->output_id = screen_res->outputs[i];
        info->is_connected = (output->connection == RR_Connected);
        info->is_primary = (screen_res->outputs[i] == primary);
        info->crtc_id = output->crtc;
        
        if (output->crtc) {
            XRRCrtcInfo *crtc = XRRGetCrtcInfo(display_state.x_display, screen_res, output->crtc);
            if (crtc) {
                info->width = crtc->width;
                info->height = crtc->height;
                info->x = crtc->x;
                info->y = crtc->y;
                
                XRRModeInfo *mode = display_find_mode_by_id(crtc->mode);
                if (mode) {
                    info->refresh_rate = display_get_mode_refresh_rate(mode);
                }
                XRRFreeCrtcInfo(crtc);
            }
        }
        
        displays = g_list_append(displays, info);
        XRRFreeOutputInfo(output);
    }
    
    return displays;
}

DisplayInfo* display_get_by_name(const gchar *name) {
    GList *displays = display_get_all();
    DisplayInfo *result = NULL;
    
    for (GList *l = displays; l; l = l->next) {
        DisplayInfo *info = l->data;
        if (g_strcmp0(info->name, name) == 0) {
            result = info;
        } else {
            display_info_free(info);
        }
    }
    g_list_free(displays);
    
    return result;
}

DisplayInfo* display_get_primary(void) {
    GList *displays = display_get_all();
    DisplayInfo *result = NULL;
    
    for (GList *l = displays; l; l = l->next) {
        DisplayInfo *info = l->data;
        if (info->is_primary && info->is_connected) {
            result = info;
        } else {
            display_info_free(info);
        }
    }
    g_list_free(displays);
    
    return result;
}
