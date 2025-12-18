/*
 * display_night.c - Venom Display Night Light
 * Night Light والـ gamma
 */

#include "display.h"
#include "venom_display.h"
#include <stdlib.h>
#include <math.h>

// ═══════════════════════════════════════════════════════════════════════════
// الحالة
// ═══════════════════════════════════════════════════════════════════════════

static NightLightSettings night_light = {FALSE, 6500};

// ═══════════════════════════════════════════════════════════════════════════
// تطبيق Gamma
// ═══════════════════════════════════════════════════════════════════════════

static void apply_gamma(const gchar *output, double r, double g, double b) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "xrandr --output %s --gamma %.2f:%.2f:%.2f 2>/dev/null",
             output, r, g, b);
    if (system(cmd)) { /* ignore */ }
}

// ═══════════════════════════════════════════════════════════════════════════
// Night Light
// ═══════════════════════════════════════════════════════════════════════════

NightLightSettings display_get_night_light(void) {
    return night_light;
}

gboolean display_set_night_light(gboolean enabled, int temperature) {
    if (temperature < 1000) temperature = 1000;
    if (temperature > 10000) temperature = 10000;
    
    night_light.enabled = enabled;
    night_light.temperature = temperature;
    
    double temp = temperature / 100.0;
    double r = 1.0, g = 1.0, b = 1.0;
    
    if (enabled && temperature < 6500) {
        if (temp <= 66) {
            r = 1.0;
            g = 0.39 * log(temp) - 0.63;
            if (g < 0) { g = 0; }
            if (g > 1) { g = 1; }
        }
        if (temp <= 65) {
            b = 0.0;
            if (temp > 19) {
                b = 0.54 * log(temp - 10) - 1.0;
                if (b < 0) { b = 0; }
                if (b > 1) { b = 1; }
            }
        }
    }
    
    GList *displays = display_get_all();
    for (GList *l = displays; l; l = l->next) {
        DisplayInfo *d = l->data;
        if (d->is_connected && d->crtc_id) {
            apply_gamma(d->name, r, g, b);
        }
        display_info_free(d);
    }
    g_list_free(displays);
    
    return TRUE;
}
