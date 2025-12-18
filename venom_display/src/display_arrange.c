/*
 * display_arrange.c - Venom Display Arrangement
 * ترتيب الشاشات الذكي
 */

#include "display.h"
#include "venom_display.h"
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// دوال خارجية
// ═══════════════════════════════════════════════════════════════════════════

extern XRRScreenResources *screen_res;

// ═══════════════════════════════════════════════════════════════════════════
// Arrangement API
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_arrange(const gchar *name, ArrangePosition pos, const gchar *relative_to) {
    DisplayInfo *target = display_get_by_name(name);
    DisplayInfo *ref = display_get_by_name(relative_to);
    
    if (!target || !ref) {
        display_info_free(target);
        display_info_free(ref);
        return FALSE;
    }
    
    int new_x = 0, new_y = 0;
    
    switch (pos) {
        case ARRANGE_LEFT_OF:
            new_x = ref->x - target->width;
            new_y = ref->y;
            break;
            
        case ARRANGE_RIGHT_OF:
            new_x = ref->x + ref->width;
            new_y = ref->y;
            break;
            
        case ARRANGE_ABOVE:
            new_x = ref->x;
            new_y = ref->y - target->height;
            break;
            
        case ARRANGE_BELOW:
            new_x = ref->x;
            new_y = ref->y + ref->height;
            break;
            
        case ARRANGE_SAME_AS:
            new_x = ref->x;
            new_y = ref->y;
            break;
    }
    
    // Ensure no negative positions
    if (new_x < 0) {
        // Shift all displays to the right
        int shift = -new_x;
        GList *displays = display_get_all();
        for (GList *l = displays; l; l = l->next) {
            DisplayInfo *d = l->data;
            if (d->is_connected && d->crtc_id && g_strcmp0(d->name, name) != 0) {
                display_set_position(d->name, d->x + shift, d->y);
            }
            display_info_free(d);
        }
        g_list_free(displays);
        new_x = 0;
    }
    
    if (new_y < 0) new_y = 0;
    
    gboolean result = display_set_position(name, new_x, new_y);
    
    display_info_free(target);
    display_info_free(ref);
    
    return result;
}

static gint compare_displays_by_x(gconstpointer a, gconstpointer b) {
    const DisplayInfo *da = a;
    const DisplayInfo *db = b;
    return da->x - db->x;
}

gboolean display_auto_arrange(void) {
    GList *displays = display_get_all();
    GList *connected = NULL;
    
    // Filter connected and enabled displays
    for (GList *l = displays; l; l = l->next) {
        DisplayInfo *d = l->data;
        if (d->is_connected && d->crtc_id) {
            connected = g_list_append(connected, d);
        } else {
            display_info_free(d);
        }
    }
    g_list_free(displays);
    
    if (!connected) return FALSE;
    
    // Sort by current x position
    connected = g_list_sort(connected, compare_displays_by_x);
    
    // Arrange sequentially from left to right
    int current_x = 0;
    for (GList *l = connected; l; l = l->next) {
        DisplayInfo *d = l->data;
        
        if (d->x != current_x || d->y != 0) {
            display_set_position(d->name, current_x, 0);
        }
        
        current_x += d->width;
        display_info_free(d);
    }
    g_list_free(connected);
    
    return TRUE;
}

ArrangePosition display_get_arrangement(const gchar *name, const gchar *relative_to) {
    DisplayInfo *target = display_get_by_name(name);
    DisplayInfo *ref = display_get_by_name(relative_to);
    
    if (!target || !ref) {
        display_info_free(target);
        display_info_free(ref);
        return ARRANGE_RIGHT_OF;  // Default
    }
    
    ArrangePosition result;
    
    // Check position relationship
    if (target->x == ref->x && target->y == ref->y) {
        result = ARRANGE_SAME_AS;
    } else if (target->x + target->width <= ref->x) {
        result = ARRANGE_LEFT_OF;
    } else if (target->x >= ref->x + ref->width) {
        result = ARRANGE_RIGHT_OF;
    } else if (target->y + target->height <= ref->y) {
        result = ARRANGE_ABOVE;
    } else {
        result = ARRANGE_BELOW;
    }
    
    display_info_free(target);
    display_info_free(ref);
    
    return result;
}

gboolean display_extend_right(const gchar *name) {
    DisplayInfo *primary = display_get_primary();
    if (!primary) return FALSE;
    
    gboolean result = display_arrange(name, ARRANGE_RIGHT_OF, primary->name);
    display_info_free(primary);
    return result;
}

gboolean display_extend_left(const gchar *name) {
    DisplayInfo *primary = display_get_primary();
    if (!primary) return FALSE;
    
    gboolean result = display_arrange(name, ARRANGE_LEFT_OF, primary->name);
    display_info_free(primary);
    return result;
}
