#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h> // المكتبة السحرية الجديدة
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    Display *display;
    Window window;
    int screen;

    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    screen = DefaultScreen(display);
    int screen_width = DisplayWidth(display, screen);
    int screen_height = DisplayHeight(display, screen);
    int bar_height = 60;  // ارتفاع الشريط
    int y_pos = screen_height - bar_height;  // في الأسفل

    // إنشاء النافذة
    XSetWindowAttributes attrs;
    attrs.override_redirect = True; // مهم جداً: يمنع مدير النوافذ من وضع إطار أو التحكم بها
    
    window = XCreateWindow(display, RootWindow(display, screen), 
                           0, y_pos, screen_width, bar_height, 0, 
                           CopyFromParent, InputOutput, CopyFromParent, 
                           CWOverrideRedirect, &attrs);

    // 1. ضبط اللون الأسود
    XSetWindowBackground(display, window, BlackPixel(display, screen));
    XClearWindow(display, window);

    // 2. ضبط الشفافية (0% = شفاف تماماً)
    double alpha = 0.0;
    unsigned long opacity = (unsigned long)(0xFFFFFFFF * alpha);
    Atom opacityAtom = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", False);
    XChangeProperty(display, window, opacityAtom, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&opacity, 1);

    // 3. جعل النافذة تخترق الضغطات (Click-Through) باستخدام XFixes
    // نقوم بإنشاء منطقة فارغة تماماً
    XserverRegion region = XFixesCreateRegion(display, NULL, 0);
    
    // نطبق هذه المنطقة الفارغة على "شكل الإدخال" للنافذة
    // هذا يعني حرفياً: مساحة النافذة التي تقبل الماوس هي صفر
    XFixesSetWindowShapeRegion(display, window, ShapeInput, 0, 0, region);
    
    // تنظيف الذاكرة
    XFixesDestroyRegion(display, region);

    // 4. إبقاء النافذة دائماً في الأعلى (Always on Top)
    Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom state_above = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    XChangeProperty(display, window, wm_state, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&state_above, 1);
    
    // 5. خصائص Dock
    Atom window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom type_dock = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(display, window, window_type, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&type_dock, 1);

    // إظهار النافذة
    XMapWindow(display, window);
    XFlush(display);

    printf("Ghost Overlay Active using XFixes. Clicks should pass through now.\n");

    while (1) {
        XEvent event;
        XNextEvent(display, &event);
    }

    XCloseDisplay(display);
    return 0;
}
