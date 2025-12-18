/*
 * display_layout.c - Venom Display Layout Management
 * الموقع والتدوير والمطابقة
 */

#include "display.h"
#include "venom_display.h"
#include <stdlib.h>

// ═══════════════════════════════════════════════════════════════════════════
// دوال خارجية
// ═══════════════════════════════════════════════════════════════════════════

extern XRRScreenResources *screen_res;

// ═══════════════════════════════════════════════════════════════════════════
// تحويل التدوير
// ═══════════════════════════════════════════════════════════════════════════

static Rotation rotation_type_to_xrandr(RotationType type) {
    switch (type) {
        case ROTATION_NORMAL: return RR_Rotate_0;
        case ROTATION_LEFT: return RR_Rotate_90;
        case ROTATION_INVERTED: return RR_Rotate_180;
        case ROTATION_RIGHT: return RR_Rotate_270;
        default: return RR_Rotate_0;
    }
}

static RotationType xrandr_to_rotation_type(Rotation rot) {
    if (rot & RR_Rotate_90) return ROTATION_LEFT;
    if (rot & RR_Rotate_180) return ROTATION_INVERTED;
    if (rot & RR_Rotate_270) return ROTATION_RIGHT;
    return ROTATION_NORMAL;
}

// ═══════════════════════════════════════════════════════════════════════════
// الموقع
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_set_position(const gchar *name, int x, int y) {
    DisplayInfo *info = display_get_by_name(name);
    if (!info || !info->crtc_id) {
        display_info_free(info);
        return FALSE;
    }
    
    XRRCrtcInfo *crtc = XRRGetCrtcInfo(display_state.x_display, screen_res, info->crtc_id);
    if (!crtc) {
        display_info_free(info);
        return FALSE;
    }
    
    Status status = XRRSetCrtcConfig(display_state.x_display, screen_res, info->crtc_id,
                                      CurrentTime, x, y, crtc->mode,
                                      crtc->rotation, crtc->outputs, crtc->noutput);
    
    XRRFreeCrtcInfo(crtc);
    display_info_free(info);
    
    return (status == RRSetConfigSuccess);
}

gboolean display_set_primary(const gchar *name) {
    for (int i = 0; i < screen_res->noutput; i++) {
        XRROutputInfo *output = XRRGetOutputInfo(display_state.x_display, screen_res, 
                                                  screen_res->outputs[i]);
        if (!output) continue;
        
        if (g_strcmp0(output->name, name) == 0) {
            XRRSetOutputPrimary(display_state.x_display, display_state.root_window, 
                               screen_res->outputs[i]);
            XRRFreeOutputInfo(output);
            return TRUE;
        }
        XRRFreeOutputInfo(output);
    }
    return FALSE;
}

// ═══════════════════════════════════════════════════════════════════════════
// التدوير
// ═══════════════════════════════════════════════════════════════════════════

RotationType display_get_rotation(const gchar *name) {
    DisplayInfo *info = display_get_by_name(name);
    if (!info || !info->crtc_id) {
        display_info_free(info);
        return ROTATION_NORMAL;
    }
    
    XRRCrtcInfo *crtc = XRRGetCrtcInfo(display_state.x_display, screen_res, info->crtc_id);
    if (!crtc) {
        display_info_free(info);
        return ROTATION_NORMAL;
    }
    
    RotationType result = xrandr_to_rotation_type(crtc->rotation);
    XRRFreeCrtcInfo(crtc);
    display_info_free(info);
    return result;
}

gboolean display_set_rotation(const gchar *name, RotationType rotation) {
    DisplayInfo *info = display_get_by_name(name);
    if (!info || !info->crtc_id) {
        display_info_free(info);
        return FALSE;
    }
    
    XRRCrtcInfo *crtc = XRRGetCrtcInfo(display_state.x_display, screen_res, info->crtc_id);
    if (!crtc) {
        display_info_free(info);
        return FALSE;
    }
    
    Rotation xrot = rotation_type_to_xrandr(rotation);
    Status status = XRRSetCrtcConfig(display_state.x_display, screen_res, info->crtc_id,
                                      CurrentTime, crtc->x, crtc->y, crtc->mode,
                                      xrot, crtc->outputs, crtc->noutput);
    
    XRRFreeCrtcInfo(crtc);
    display_info_free(info);
    
    return (status == RRSetConfigSuccess);
}

// ═══════════════════════════════════════════════════════════════════════════
// تشغيل/إيقاف
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_enable(const gchar *name) {
    for (int i = 0; i < screen_res->noutput; i++) {
        XRROutputInfo *output = XRRGetOutputInfo(display_state.x_display, screen_res, 
                                                  screen_res->outputs[i]);
        if (!output) continue;
        
        if (g_strcmp0(output->name, name) == 0 && output->nmode > 0) {
            XRRModeInfo *mode = display_find_mode_by_id(output->modes[0]);
            if (mode) {
                for (int j = 0; j < output->ncrtc; j++) {
                    XRRCrtcInfo *crtc = XRRGetCrtcInfo(display_state.x_display, screen_res, 
                                                        output->crtcs[j]);
                    if (crtc && crtc->noutput == 0) {
                        Status status = XRRSetCrtcConfig(display_state.x_display, screen_res,
                            output->crtcs[j], CurrentTime, 0, 0, mode->id, RR_Rotate_0,
                            &screen_res->outputs[i], 1);
                        XRRFreeCrtcInfo(crtc);
                        XRRFreeOutputInfo(output);
                        return (status == RRSetConfigSuccess);
                    }
                    if (crtc) XRRFreeCrtcInfo(crtc);
                }
            }
        }
        XRRFreeOutputInfo(output);
    }
    return FALSE;
}

gboolean display_disable(const gchar *name) {
    DisplayInfo *info = display_get_by_name(name);
    if (!info || !info->crtc_id) {
        display_info_free(info);
        return FALSE;
    }
    
    Status status = XRRSetCrtcConfig(display_state.x_display, screen_res, info->crtc_id,
                                      CurrentTime, 0, 0, None, RR_Rotate_0, NULL, 0);
    display_info_free(info);
    
    return (status == RRSetConfigSuccess);
}

// ═══════════════════════════════════════════════════════════════════════════
// المطابقة (Mirror)
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_set_mirror(const gchar *source, const gchar *target) {
    DisplayInfo *src = display_get_by_name(source);
    DisplayInfo *tgt = display_get_by_name(target);
    
    if (!src || !tgt) {
        display_info_free(src);
        display_info_free(tgt);
        return FALSE;
    }
    
    gboolean result = display_set_mode(target, src->width, src->height, -1);
    if (result) {
        result = display_set_position(target, src->x, src->y);
    }
    
    display_info_free(src);
    display_info_free(tgt);
    
    return result;
}

gboolean display_disable_mirror(const gchar *name) {
    DisplayInfo *info = display_get_by_name(name);
    if (!info) return FALSE;
    
    GList *displays = display_get_all();
    int new_x = 0;
    for (GList *l = displays; l; l = l->next) {
        DisplayInfo *other = l->data;
        if (g_strcmp0(other->name, name) != 0 && other->is_connected) {
            new_x = other->x + other->width;
        }
        display_info_free(other);
    }
    g_list_free(displays);
    
    gboolean result = display_set_position(name, new_x, 0);
    display_info_free(info);
    
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// التكبير (Scale)
// ═══════════════════════════════════════════════════════════════════════════

double display_get_scale(const gchar *name) {
    (void)name;
    return 1.0;
}

gboolean display_set_scale(const gchar *name, double scale) {
    if (scale <= 0 || scale > 4.0) return FALSE;
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "xrandr --output %s --scale %.2fx%.2f 2>/dev/null", 
             name, 1.0/scale, 1.0/scale);
    int result = system(cmd);
    
    return (result == 0);
}
