/*
 * display_edid.c - Venom Display EDID Reader
 * قراءة معلومات الشاشة من EDID
 */

#include "display.h"
#include "venom_display.h"
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// دوال خارجية
// ═══════════════════════════════════════════════════════════════════════════

extern XRRScreenResources *screen_res;

// ═══════════════════════════════════════════════════════════════════════════
// EDID Parsing
// ═══════════════════════════════════════════════════════════════════════════

static void parse_manufacturer(unsigned char *edid, char *out) {
    unsigned int manufacturer = (edid[8] << 8) | edid[9];
    out[0] = ((manufacturer >> 10) & 0x1F) + 'A' - 1;
    out[1] = ((manufacturer >> 5) & 0x1F) + 'A' - 1;
    out[2] = (manufacturer & 0x1F) + 'A' - 1;
    out[3] = '\0';
}

static void parse_descriptor_string(unsigned char *desc, char *out, int max_len) {
    int j = 0;
    for (int i = 5; i < 18 && j < max_len - 1; i++) {
        if (desc[i] == 0x0A || desc[i] == 0x00) break;
        if (desc[i] >= 0x20 && desc[i] <= 0x7E) {
            out[j++] = desc[i];
        }
    }
    out[j] = '\0';
    
    // Trim trailing spaces
    while (j > 0 && out[j-1] == ' ') {
        out[--j] = '\0';
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// EDID API
// ═══════════════════════════════════════════════════════════════════════════

EDIDInfo* display_get_edid(const gchar *name) {
    if (!display_state.x_display || !screen_res) return NULL;
    
    Atom edid_atom = XInternAtom(display_state.x_display, "EDID", False);
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;
    
    // Find output
    RROutput output = None;
    for (int i = 0; i < screen_res->noutput; i++) {
        XRROutputInfo *info = XRRGetOutputInfo(display_state.x_display, screen_res, 
                                                screen_res->outputs[i]);
        if (info) {
            if (g_strcmp0(info->name, name) == 0) {
                output = screen_res->outputs[i];
            }
            XRRFreeOutputInfo(info);
            if (output != None) break;
        }
    }
    
    if (output == None) return NULL;
    
    // Get EDID property
    if (XRRGetOutputProperty(display_state.x_display, output, edid_atom,
                             0, 128, False, False, AnyPropertyType,
                             &actual_type, &actual_format, &nitems,
                             &bytes_after, &prop) != Success) {
        return NULL;
    }
    
    if (!prop || nitems < 128) {
        if (prop) XFree(prop);
        return NULL;
    }
    
    EDIDInfo *edid = g_new0(EDIDInfo, 1);
    
    // Validate EDID header
    if (prop[0] != 0x00 || prop[1] != 0xFF || prop[2] != 0xFF || prop[3] != 0xFF) {
        XFree(prop);
        g_free(edid);
        return NULL;
    }
    
    // Parse manufacturer
    parse_manufacturer(prop, edid->manufacturer);
    
    // Product code
    edid->product_code = prop[10] | (prop[11] << 8);
    
    // Serial number (raw)
    edid->serial_number = prop[12] | (prop[13] << 8) | (prop[14] << 16) | (prop[15] << 24);
    
    // Year
    edid->year = 1990 + prop[17];
    
    // Max resolution from detailed timing (first descriptor)
    edid->max_width = ((prop[58] & 0xF0) << 4) | prop[56];
    edid->max_height = ((prop[61] & 0xF0) << 4) | prop[59];
    
    // Parse descriptor blocks for model name and serial
    for (int i = 0; i < 4; i++) {
        unsigned char *desc = prop + 54 + (i * 18);
        
        if (desc[0] == 0 && desc[1] == 0 && desc[2] == 0) {
            if (desc[3] == 0xFC) {
                // Monitor name
                parse_descriptor_string(desc, edid->model, sizeof(edid->model));
            } else if (desc[3] == 0xFF) {
                // Serial string
                parse_descriptor_string(desc, edid->serial, sizeof(edid->serial));
            }
        }
    }
    
    // Check for HDR support in extension blocks (basic check)
    edid->hdr_supported = FALSE;
    if (bytes_after > 0) {
        unsigned char *ext_prop = NULL;
        unsigned long ext_nitems;
        if (XRRGetOutputProperty(display_state.x_display, output, edid_atom,
                                 128, 256, False, False, AnyPropertyType,
                                 &actual_type, &actual_format, &ext_nitems,
                                 &bytes_after, &ext_prop) == Success && ext_prop) {
            // CTA-861 extension check for HDR
            if (ext_nitems >= 128 && ext_prop[0] == 0x02) {
                edid->hdr_supported = TRUE;  // Simplified: assume HDR if CTA extension exists
            }
            XFree(ext_prop);
        }
    }
    
    // VRR detection from EDID
    edid->vrr_supported = FALSE;  // Needs deeper parsing or sysfs check
    
    XFree(prop);
    return edid;
}

void display_edid_free(EDIDInfo *info) {
    g_free(info);
}

gchar* display_get_edid_hash(const gchar *name) {
    EDIDInfo *edid = display_get_edid(name);
    if (!edid) return NULL;
    
    gchar *hash = g_strdup_printf("%s_%04X_%08X", 
                                   edid->manufacturer, 
                                   edid->product_code,
                                   edid->serial_number);
    display_edid_free(edid);
    return hash;
}
