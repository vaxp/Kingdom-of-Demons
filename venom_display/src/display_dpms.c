/*
 * display_dpms.c - Venom Display Power Management
 * إدارة طاقة الشاشة DPMS
 */

#include "display.h"
#include "venom_display.h"
#include <X11/extensions/dpms.h>

// ═══════════════════════════════════════════════════════════════════════════
// DPMS - Display Power Management Signaling
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_dpms_is_supported(void) {
    int event_base, error_base;
    return DPMSQueryExtension(display_state.x_display, &event_base, &error_base);
}

DPMSMode display_dpms_get_mode(void) {
    if (!display_dpms_is_supported()) return DPMS_MODE_ON;
    
    CARD16 power_level;
    BOOL state;
    
    if (DPMSInfo(display_state.x_display, &power_level, &state)) {
        if (!state) return DPMS_MODE_ON;
        
        switch (power_level) {
            case DPMSModeOn:      return DPMS_MODE_ON;
            case DPMSModeStandby: return DPMS_MODE_STANDBY;
            case DPMSModeSuspend: return DPMS_MODE_SUSPEND;
            case DPMSModeOff:     return DPMS_MODE_OFF;
            default:              return DPMS_MODE_ON;
        }
    }
    return DPMS_MODE_ON;
}

gboolean display_dpms_set_mode(DPMSMode mode) {
    if (!display_dpms_is_supported()) return FALSE;
    
    CARD16 power_level;
    switch (mode) {
        case DPMS_MODE_ON:      power_level = DPMSModeOn; break;
        case DPMS_MODE_STANDBY: power_level = DPMSModeStandby; break;
        case DPMS_MODE_SUSPEND: power_level = DPMSModeSuspend; break;
        case DPMS_MODE_OFF:     power_level = DPMSModeOff; break;
        default:                power_level = DPMSModeOn; break;
    }
    
    if (mode == DPMS_MODE_ON) {
        DPMSEnable(display_state.x_display);
    }
    
    DPMSForceLevel(display_state.x_display, power_level);
    XFlush(display_state.x_display);
    return TRUE;
}

gboolean display_dpms_set_timeouts(int standby, int suspend, int off) {
    if (!display_dpms_is_supported()) return FALSE;
    DPMSSetTimeouts(display_state.x_display, standby, suspend, off);
    XFlush(display_state.x_display);
    return TRUE;
}

void display_dpms_get_timeouts(int *standby, int *suspend, int *off) {
    *standby = *suspend = *off = 0;
    if (!display_dpms_is_supported()) return;
    
    CARD16 s, sp, o;
    DPMSGetTimeouts(display_state.x_display, &s, &sp, &o);
    *standby = s;
    *suspend = sp;
    *off = o;
}

gboolean display_dpms_enable(void) {
    if (!display_dpms_is_supported()) return FALSE;
    DPMSEnable(display_state.x_display);
    XFlush(display_state.x_display);
    return TRUE;
}

gboolean display_dpms_disable(void) {
    if (!display_dpms_is_supported()) return FALSE;
    DPMSDisable(display_state.x_display);
    XFlush(display_state.x_display);
    return TRUE;
}
