#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <glib.h>
#include <gio/gio.h>

// Globals
static Display *dpy;
static Window root;
static int screen;
static GMainLoop *loop;

// Configuration
static char config_save_path[1024] = "/home/x/Videos/";
static char config_img_format[16] = "png";
static int config_img_quality = 100;
static char config_vid_format[16] = "mp4";
static int config_vid_fps = 60;

// Selection state
static Window sel_win = None;
static Colormap sel_colormap = None;
static cairo_surface_t *sel_surface = NULL;
static double start_x, start_y, current_x, current_y;
static gboolean selecting = FALSE;
static gboolean recording_mode = FALSE;

// Recording state
static pid_t recording_pid = 0;

// Function prototypes
void take_screenshot(int x, int y, int w, int h);
void start_recording(int x, int y, int w, int h);
void stop_recording();
void start_selection_mode(gboolean record);
void stop_selection_mode();

// Helper to get timestamp filename
void get_timestamp_filename(char *buffer, size_t size, const char *ext) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "screenshot_%Y-%m-%d_%H-%M-%S", t);
    snprintf(buffer, size, "%s/%s.%s", config_save_path, time_str, ext);
}

void get_video_filename(char *buffer, size_t size, const char *ext) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "video_%Y-%m-%d_%H-%M-%S", t);
    snprintf(buffer, size, "%s/%s.%s", config_save_path, time_str, ext);
}

// --- X11 / Cairo Helpers ---

// Find a visual that supports 32-bit ARGB
Visual *find_argb_visual(Display *d, int scr) {
    XVisualInfo vinfo_template;
    vinfo_template.screen = scr;
    vinfo_template.depth = 32;
    vinfo_template.class = TrueColor;
    
    int nvisuals;
    XVisualInfo *vinfo = XGetVisualInfo(d, VisualScreenMask | VisualDepthMask | VisualClassMask, &vinfo_template, &nvisuals);
    
    if (vinfo && nvisuals > 0) {
        Visual *visual = vinfo->visual;
        XFree(vinfo);
        return visual;
    }
    return NULL;
}

// --- D-Bus Logic ---

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.venom.Shot'>"
  "    <method name='FullScreen' />"
  "    <method name='SelectRegion' />"
  "    <method name='StartRecord' />"
  "    <method name='SelectRecord' />"
  "    <method name='StopRecord' />"
  "    <method name='SetImageFormat'>"
  "      <arg type='s' name='format' direction='in'/>"
  "    </method>"
  "    <method name='SetImageQuality'>"
  "      <arg type='i' name='quality' direction='in'/>"
  "    </method>"
  "    <method name='SetVideoFormat'>"
  "      <arg type='s' name='format' direction='in'/>"
  "    </method>"
  "    <method name='SetVideoFPS'>"
  "      <arg type='i' name='fps' direction='in'/>"
  "    </method>"
  "    <method name='SetSavePath'>"
  "      <arg type='s' name='path' direction='in'/>"
  "    </method>"
  "  </interface>"
  "</node>";

static void handle_method_call(GDBusConnection *connection,
                               const gchar *sender,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *method_name,
                               GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data) {
    (void)connection; (void)sender; (void)object_path; (void)interface_name; (void)user_data;

    if (g_strcmp0(method_name, "FullScreen") == 0) {
        take_screenshot(0, 0, DisplayWidth(dpy, screen), DisplayHeight(dpy, screen));
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "SelectRegion") == 0) {
        start_selection_mode(FALSE);
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "StartRecord") == 0) {
        start_recording(0, 0, DisplayWidth(dpy, screen), DisplayHeight(dpy, screen));
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "SelectRecord") == 0) {
        start_selection_mode(TRUE);
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "StopRecord") == 0) {
        stop_recording();
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "SetImageFormat") == 0) {
        const gchar *fmt;
        g_variant_get(parameters, "(&s)", &fmt);
        strncpy(config_img_format, fmt, sizeof(config_img_format) - 1);
        g_print("Image format set to: %s\n", config_img_format);
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "SetImageQuality") == 0) {
        int q;
        g_variant_get(parameters, "(i)", &q);
        config_img_quality = q;
        g_print("Image quality set to: %d\n", config_img_quality);
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "SetVideoFormat") == 0) {
        const gchar *fmt;
        g_variant_get(parameters, "(&s)", &fmt);
        strncpy(config_vid_format, fmt, sizeof(config_vid_format) - 1);
        g_print("Video format set to: %s\n", config_vid_format);
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "SetVideoFPS") == 0) {
        int fps;
        g_variant_get(parameters, "(i)", &fps);
        config_vid_fps = fps;
        g_print("Video FPS set to: %d\n", config_vid_fps);
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "SetSavePath") == 0) {
        const gchar *path;
        g_variant_get(parameters, "(&s)", &path);
        strncpy(config_save_path, path, sizeof(config_save_path) - 1);
        g_print("Save path set to: %s\n", config_save_path);
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD, "Method not found");
    }
}

static const GDBusInterfaceVTable interface_vtable = {
  handle_method_call,
  NULL,
  NULL,
  { 0 }
};

static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    (void)name; (void)user_data;
    GDBusNodeInfo *introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
    g_dbus_connection_register_object(connection,
                                      "/org/venom/Shot",
                                      introspection_data->interfaces[0],
                                      &interface_vtable,
                                      NULL,
                                      NULL,
                                      NULL);
}

static void on_name_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    (void)connection; (void)name; (void)user_data;
    g_print("Acquired the name %s\n", name);
}

static void on_name_lost(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    (void)connection; (void)name; (void)user_data;
    g_print("Lost the name %s. Exiting.\n", name);
    g_main_loop_quit(loop);
}

// --- Screenshot Logic ---

void take_screenshot(int x, int y, int w, int h) {
    // Use XGetImage directly to avoid Cairo Xlib surface overhead/caching issues
    XImage *image = XGetImage(dpy, root, x, y, w, h, AllPlanes, ZPixmap);
    if (!image) {
        g_print("Failed to capture image\n");
        return;
    }

    // Determine Cairo format based on XImage depth
    cairo_format_t format = CAIRO_FORMAT_ARGB32;
    if (image->depth == 24) {
        format = CAIRO_FORMAT_RGB24;
    } else if (image->depth != 32) {
        g_print("Unsupported depth: %d\n", image->depth);
        XDestroyImage(image);
        return;
    }

    // Create Cairo surface from XImage data
    // Note: stride is bytes_per_line
    cairo_surface_t *surf = cairo_image_surface_create_for_data(
        (unsigned char *)image->data,
        format,
        w,
        h,
        image->bytes_per_line
    );

    if (cairo_surface_status(surf) != CAIRO_STATUS_SUCCESS) {
        g_print("Failed to create cairo surface\n");
        XDestroyImage(image);
        return;
    }

    char filename[256];
    
    if (strcmp(config_img_format, "png") == 0) {
        get_timestamp_filename(filename, sizeof(filename), "png");
        cairo_surface_write_to_png(surf, filename);
        g_print("Saved to %s\n", filename);
    } else {
        // Save as temp png then convert
        char temp_filename[256];
        snprintf(temp_filename, sizeof(temp_filename), "/tmp/venom_temp_%d.png", rand());
        cairo_surface_write_to_png(surf, temp_filename);
        
        get_timestamp_filename(filename, sizeof(filename), config_img_format);
        
        // Convert using ffmpeg
        char cmd[1024];
        // Use -q:v for quality (1-31 for jpg where 1 is best? Or 2-31? ffmpeg jpg quality is weird. 
        // Actually -q:v 1 is best, 31 is worst.
        // Let's map 0-100 quality to ffmpeg scale roughly.
        // 100 -> 1, 0 -> 31.
        int q = 31 - (config_img_quality * 30 / 100);
        if (q < 1) q = 1;
        if (q > 31) q = 31;
        
        sprintf(cmd, "ffmpeg -y -v quiet -i %s -q:v %d %s", temp_filename, q, filename);
        system(cmd);
        remove(temp_filename);
        g_print("Saved to %s (converted from png)\n", filename);
    }

    cairo_surface_destroy(surf);
    XDestroyImage(image); // This frees the image structure AND the data allocated by XGetImage
}

// --- Recording Logic ---

void start_recording(int x, int y, int w, int h) {
    if (recording_pid > 0) {
        g_print("Recording already in progress\n");
        return;
    }

    char filename[256];
    get_video_filename(filename, sizeof(filename), config_vid_format);
    
    // Ensure even dimensions
    w -= (w % 2);
    h -= (h % 2);
    
    g_print("Starting recording to: %s\n", filename);
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        // Redirect stdout/stderr to /dev/null
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        
        char video_size[32];
        sprintf(video_size, "%dx%d", w, h);
        
        char input_pos[32];
        sprintf(input_pos, ":0.0+%d,%d", x, y);
        
        char fps_str[8];
        sprintf(fps_str, "%d", config_vid_fps);
        
        execlp("ffmpeg", "ffmpeg", 
               "-y", 
               "-f", "x11grab", 
               "-video_size", video_size, 
               "-i", input_pos, 
               "-framerate", fps_str, 
               "-c:v", "libx264", 
               "-preset", "ultrafast", 
               filename, 
               NULL);
        
        // If execlp fails
        exit(1);
    } else if (pid > 0) {
        recording_pid = pid;
        g_print("Recording started (PID: %d)\n", recording_pid);
    } else {
        g_print("Failed to fork for recording\n");
    }
}

void stop_recording() {
    if (recording_pid > 0) {
        g_print("Stopping recording (PID: %d)...\n", recording_pid);
        kill(recording_pid, SIGINT); // Send SIGINT to let ffmpeg finish gracefully
        waitpid(recording_pid, NULL, 0);
        recording_pid = 0;
        g_print("Recording stopped.\n");
    } else {
        g_print("No recording in progress.\n");
    }
}



// --- Selection Logic ---

void draw_selection() {
    if (!sel_surface) return;
    
    cairo_t *cr = cairo_create(sel_surface);
    
    // Clear everything
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0); // Transparent
    cairo_paint(cr);
    
    // Draw dim overlay
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.1); // 10% black
    cairo_paint(cr);
    
    if (selecting) {
        double x = MIN(start_x, current_x);
        double y = MIN(start_y, current_y);
        double w = ABS(current_x - start_x);
        double h = ABS(current_y - start_y);
        
        // Clear selection area
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_rectangle(cr, x, y, w, h);
        cairo_fill(cr);
        
        // Draw border
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_set_line_width(cr, 1.0);
        cairo_rectangle(cr, x, y, w, h);
        cairo_stroke(cr);
    }
    
    cairo_destroy(cr);
    XFlush(dpy);
}

gboolean x11_event_handler(GIOChannel *source, GIOCondition condition, gpointer data) {
    (void)source; (void)condition; (void)data;
    
    while (XPending(dpy) > 0) {
        XEvent ev;
        XNextEvent(dpy, &ev);
        
        if (ev.type == ButtonPress && ev.xbutton.button == 1) {
            start_x = ev.xbutton.x_root;
            start_y = ev.xbutton.y_root;
            current_x = start_x;
            current_y = start_y;
            selecting = TRUE;
            draw_selection();
        } else if (ev.type == MotionNotify) {
            if (selecting) {
                current_x = ev.xmotion.x_root;
                current_y = ev.xmotion.y_root;
                draw_selection();
            }
        } else if (ev.type == ButtonRelease && ev.xbutton.button == 1) {
            if (selecting) {
                selecting = FALSE;
                current_x = ev.xbutton.x_root;
                current_y = ev.xbutton.y_root;
                
                int x = (int)MIN(start_x, current_x);
                int y = (int)MIN(start_y, current_y);
                int w = (int)ABS(current_x - start_x);
                int h = (int)ABS(current_y - start_y);
                
                stop_selection_mode();
                
                if (w > 0 && h > 0) {
                    if (recording_mode) {
                        start_recording(x, y, w, h);
                    } else {
                        take_screenshot(x, y, w, h);
                    }
                }
            }
        } else if (ev.type == KeyPress) {
            // Check for Escape
            KeySym keysym = XLookupKeysym(&ev.xkey, 0);
            if (keysym == XK_Escape) {
                stop_selection_mode();
            }
        } else if (ev.type == Expose) {
             if (ev.xexpose.window == sel_win) {
                 draw_selection();
             }
        }
    }
    return TRUE;
}

void start_selection_mode(gboolean record) {
    if (sel_win != None) return; // Already selecting
    
    recording_mode = record;
    
    int w = DisplayWidth(dpy, screen);
    int h = DisplayHeight(dpy, screen);
    
    Visual *visual = find_argb_visual(dpy, screen);
    if (!visual) {
        g_print("No ARGB visual found. Transparency will not work.\n");
        visual = DefaultVisual(dpy, screen);
    }
    
    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;
    
    // Create and store colormap to free it later
    sel_colormap = XCreateColormap(dpy, root, visual, AllocNone);
    attrs.colormap = sel_colormap;
    
    attrs.event_mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | ExposureMask;
    
    sel_win = XCreateWindow(dpy, root, 0, 0, w, h, 0, 32, InputOutput, visual, 
                            CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWColormap | CWEventMask, &attrs);
    
    // Create Cairo surface for the window
    sel_surface = cairo_xlib_surface_create(dpy, sel_win, visual, w, h);
    
    XMapWindow(dpy, sel_win);
    
    // Grab pointer and keyboard
    XSync(dpy, False);
    
    int ret = XGrabPointer(dpy, sel_win, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, 
                           GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    if (ret != GrabSuccess) g_print("Failed to grab pointer\n");
    
    XGrabKeyboard(dpy, sel_win, False, GrabModeAsync, GrabModeAsync, CurrentTime);
    
    draw_selection();
}

void stop_selection_mode() {
    if (sel_win != None) {
        XUngrabPointer(dpy, CurrentTime);
        XUngrabKeyboard(dpy, CurrentTime);
        if (sel_surface) {
            cairo_surface_destroy(sel_surface);
            sel_surface = NULL;
        }
        XDestroyWindow(dpy, sel_win);
        sel_win = None;
        
        if (sel_colormap != None) {
            XFreeColormap(dpy, sel_colormap);
            sel_colormap = None;
        }
        
        XFlush(dpy);
    }
}


int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    
    loop = g_main_loop_new(NULL, FALSE);
    
    // Register D-Bus
    g_bus_own_name(G_BUS_TYPE_SESSION,
                   "org.venom.Shot",
                   G_BUS_NAME_OWNER_FLAGS_NONE,
                   on_bus_acquired,
                   on_name_acquired,
                   on_name_lost,
                   NULL,
                   NULL);
                   
    // Integrate X11 events into GLib loop
    GIOChannel *channel = g_io_channel_unix_new(ConnectionNumber(dpy));
    g_io_add_watch(channel, G_IO_IN, x11_event_handler, NULL);
    g_io_channel_unref(channel);
    
    g_print("Venom Shot Daemon started...\n");
    g_main_loop_run(loop);
    
    XCloseDisplay(dpy);
    return 0;
}
