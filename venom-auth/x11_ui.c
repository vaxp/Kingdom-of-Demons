#include "x11_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

// UI Constants
#define DIALOG_WIDTH  400
#define DIALOG_HEIGHT 240
#define BORDER_RADIUS 16

// Colors (RGBA)
// Background: #1E1E2E (Deep dark blue/grey)
// Accent: #89B4FA (Soft blue)
// Error: #F38BA8 (Soft red)
// Text: #CDD6F4 (White-ish)
// Input BG: #313244 (Lighter dark)

// Global X11 variables
static Display *display = NULL;
static int screen;
static Window root;

// Initialize X11
int x11_ui_init(void) {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open X11 display\n");
        return -1;
    }
    
    screen = DefaultScreen(display);
    root = RootWindow(display, screen);
    
    return 0;
}

// Cleanup X11
void x11_ui_cleanup(void) {
    if (display) {
        XCloseDisplay(display);
        display = NULL;
    }
}

// Helper: Draw rounded rectangle
static void draw_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r, r, -M_PI / 2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI / 2);
    cairo_arc(cr, x + r, y + h - r, r, M_PI / 2, M_PI);
    cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI / 2);
    cairo_close_path(cr);
}

// Helper: Draw lock icon
static void draw_lock_icon(cairo_t *cr, double x, double y, double scale) {
    cairo_save(cr);
    cairo_translate(cr, x, y);
    cairo_scale(cr, scale, scale);
    
    // Lock body
    draw_rounded_rect(cr, -10, 0, 20, 18, 2);
    cairo_fill(cr);
    
    // Lock shackle
    cairo_set_line_width(cr, 3);
    cairo_arc(cr, 0, 0, 7, M_PI, 2 * M_PI); // Top arc
    cairo_line_to(cr, 7, 0);
    cairo_line_to(cr, -7, 0);
    cairo_stroke(cr);
    
    cairo_restore(cr);
}

// Draw the UI using Cairo
static void draw_ui(cairo_t *cr, int width, int height, const char *message, int password_len, int is_error) {
    // 1. Background
    // Clear background (transparent)
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    // Main window shape
    draw_rounded_rect(cr, 0, 0, width, height, BORDER_RADIUS);
    
    // Fill background
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.6); // #1E1E2E
    cairo_fill_preserve(cr);
    
    // Border
    if (is_error) {
        cairo_set_source_rgb(cr, 0.953, 0.545, 0.659); // #F38BA8 (Error)
    } else {
        cairo_set_source_rgb(cr, 0.537, 0.706, 0.980); // #89B4FA (Accent)
    }
    cairo_set_line_width(cr, 2);
    cairo_stroke(cr);
    
    // 2. Icon
    if (is_error) {
        cairo_set_source_rgb(cr, 0.953, 0.545, 0.659); // Error
    } else {
        cairo_set_source_rgb(cr, 0.537, 0.706, 0.980); // Accent
    }
    draw_lock_icon(cr, width / 2, 40, 1.5);
    
    // 3. Title
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 16);
    cairo_set_source_rgb(cr, 0.804, 0.839, 0.957); // #CDD6F4
    
    cairo_text_extents_t extents;
    const char *title = "Authentication Required";
    cairo_text_extents(cr, title, &extents);
    cairo_move_to(cr, (width - extents.width) / 2, 80);
    cairo_show_text(cr, title);
    
    // 4. Message
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12);
    cairo_set_source_rgb(cr, 0.659, 0.694, 0.827); // #A6ADC8
    
    if (message) {
        cairo_text_extents(cr, message, &extents);
        cairo_move_to(cr, (width - extents.width) / 2, 105);
        cairo_show_text(cr, message);
    }
    
    // 5. Input Box
    double input_w = 280;
    double input_h = 40;
    double input_x = (width - input_w) / 2;
    double input_y = 130;
    
    draw_rounded_rect(cr, input_x, input_y, input_w, input_h, 8);
    cairo_set_source_rgb(cr, 0.192, 0.196, 0.267); // #313244
    cairo_fill_preserve(cr);
    
    cairo_set_source_rgb(cr, 0.278, 0.286, 0.376); // #45475A
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);
    
    // 6. Password Dots
    cairo_set_source_rgb(cr, 0.804, 0.839, 0.957); // Text color
    double dot_spacing = 15;
    double dot_start_x = input_x + 15;
    double dot_y = input_y + (input_h / 2);
    
    for (int i = 0; i < password_len && i < 20; i++) {
        cairo_arc(cr, dot_start_x + (i * dot_spacing), dot_y, 3, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    
    // 7. Hint Text
    cairo_set_font_size(cr, 11);
    const char *hint = is_error ? "Incorrect password" : "Enter password (ESC to cancel)";
    
    if (is_error) {
        cairo_set_source_rgb(cr, 0.953, 0.545, 0.659); // Error
    } else {
        cairo_set_source_rgb(cr, 0.451, 0.475, 0.588); // #737994
    }
    
    cairo_text_extents(cr, hint, &extents);
    cairo_move_to(cr, (width - extents.width) / 2, 195);
    cairo_show_text(cr, hint);
}

// Get password from user
int x11_ui_get_password(const char *message, 
                        const char *icon_name,
                        char *password_out,
                        int max_length) {
    (void)icon_name;
    if (!display) {
        fprintf(stderr, "X11 not initialized\n");
        return UI_RESULT_ERROR;
    }
    
    // Get screen dimensions
    int screen_width = DisplayWidth(display, screen);
    int screen_height = DisplayHeight(display, screen);
    
    int win_x = (screen_width - DIALOG_WIDTH) / 2;
    int win_y = (screen_height - DIALOG_HEIGHT) / 2;
    
    // Create window attributes with proper ARGB visual for transparency
    XVisualInfo vinfo;
    Colormap cmap;
    int depth;
    Visual *visual;
    
    if (XMatchVisualInfo(display, screen, 32, TrueColor, &vinfo)) {
        // Use 32-bit ARGB visual
        cmap = XCreateColormap(display, root, vinfo.visual, AllocNone);
        depth = vinfo.depth;
        visual = vinfo.visual;
    } else {
        // Fallback to default visual
        fprintf(stderr, "No 32-bit TrueColor visual found, using default\n");
        cmap = DefaultColormap(display, screen);
        depth = DefaultDepth(display, screen);
        visual = DefaultVisual(display, screen);
    }
    
    // Setup attributes with ARGB visual (or default)
    XSetWindowAttributes attrs;
    attrs.colormap = cmap;
    attrs.background_pixel = 0;  // Transparent background
    attrs.border_pixel = 0;
    attrs.override_redirect = True;
    attrs.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;
    
    // Create window with proper visual
    Window win = XCreateWindow(display, root,
                               win_x, win_y,
                               DIALOG_WIDTH, DIALOG_HEIGHT,
                               0,
                               depth, InputOutput, visual,
                               CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect | CWEventMask,
                               &attrs);
    
    // Center window
    XMoveWindow(display, win, win_x, win_y);
    
    // Set window properties
    XStoreName(display, win, "Authentication");
    
    // Always on top
    Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom state_above = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    XChangeProperty(display, win, wm_state, XA_ATOM, 32,
                   PropModeReplace, (unsigned char *)&state_above, 1);
    
    // Map window
    XMapRaised(display, win);
    XFlush(display);
    
    // Grab keyboard
    int grab_attempts = 0;
    while (grab_attempts < 100) {
        if (XGrabKeyboard(display, win, True, GrabModeAsync, GrabModeAsync, CurrentTime) == GrabSuccess) {
            break;
        }
        usleep(10000);
        grab_attempts++;
    }
    
    // Create Cairo surface with proper visual
    cairo_surface_t *surface = cairo_xlib_surface_create(display, win, visual, DIALOG_WIDTH, DIALOG_HEIGHT);
    cairo_t *cr = cairo_create(surface);
    
    // Password buffer
    char password[256] = {0};
    int password_len = 0;
    int is_error = 0;
    int result = UI_RESULT_CANCEL;
    int done = 0;
    
    // Initial draw
    draw_ui(cr, DIALOG_WIDTH, DIALOG_HEIGHT, message, password_len, is_error);
    
    // Event loop
    XEvent event;
    while (!done) {
        XNextEvent(display, &event);
        
        switch (event.type) {
            case Expose:
                if (event.xexpose.count == 0) {
                    draw_ui(cr, DIALOG_WIDTH, DIALOG_HEIGHT, message, password_len, is_error);
                }
                break;
                
            case KeyPress: {
                KeySym keysym = XLookupKeysym(&event.xkey, 0);
                
                if (keysym == XK_Return || keysym == XK_KP_Enter) {
                    if (password_len > 0) {
                        strncpy(password_out, password, max_length - 1);
                        password_out[max_length - 1] = '\0';
                        result = UI_RESULT_SUCCESS;
                        done = 1;
                    }
                }
                else if (keysym == XK_Escape) {
                    result = UI_RESULT_CANCEL;
                    done = 1;
                }
                else if (keysym == XK_BackSpace) {
                    if (password_len > 0) {
                        password_len--;
                        password[password_len] = '\0';
                        is_error = 0;
                        draw_ui(cr, DIALOG_WIDTH, DIALOG_HEIGHT, message, password_len, is_error);
                    }
                }
                else {
                    char buf[10];
                    int len = XLookupString(&event.xkey, buf, sizeof(buf), NULL, NULL);
                    
                    if (len > 0 && password_len < max_length - 1) {
                        for (int i = 0; i < len; i++) {
                            if (buf[i] >= 32 && buf[i] <= 126) {
                                password[password_len++] = buf[i];
                                password[password_len] = '\0';
                            }
                        }
                        is_error = 0;
                        draw_ui(cr, DIALOG_WIDTH, DIALOG_HEIGHT, message, password_len, is_error);
                    }
                }
                break;
            }
        }
    }
    
    // Cleanup
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    XUngrabKeyboard(display, CurrentTime);
    XDestroyWindow(display, win);
    XFlush(display);
    
    memset(password, 0, sizeof(password));
    
    return result;
}
