/*
 * venom_input.c
 * 
 * Venom OS — Input Management Daemon
 * المسؤول عن: لوحة المفاتيح، الماوس، التاتش باد
 * البروتوكول: X11 (setxkbmap / xinput / synclient)
 * الواجهة:   D-Bus (org.venom.Input) على SESSION bus
 *
 * التصميم: يعمل كـ daemon في الخلفية، يستقبل أوامر عبر D-Bus
 * ويطبّقها فوراً على X11 ويحفظها في /etc/venom/input.json
 *
 * المتطلبات:
 *   - glib-2.0, gio-2.0
 *   - setxkbmap, xinput (موجودون في معظم توزيعات Linux)
 *
 * التصريف:
 *   gcc venom_input.c -o venom_input \
 *       $(pkg-config --cflags --libs gio-2.0 glib-2.0) \
 *       -Wall -Wextra -O2
 */

#include <gio/gio.h>
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

/* =========================================================
 *  ثوابت المسارات والحدود
 * ========================================================= */

#define CONFIG_DIR         "/etc/venom"
#define CONFIG_FILE        "/etc/venom/input.json"
#define LOG_FILE           "/var/log/venom_input.log"

#define MAX_LAYOUT_LEN     128
#define MAX_OPTION_LEN     256
#define MAX_CMD_LEN        1024
#define MAX_BUFFER_LEN     4096

/* قيم افتراضية */
#define DEFAULT_LAYOUT     "us"
#define DEFAULT_MODEL      "pc105"
#define DEFAULT_OPTIONS    "grp:win_space_toggle"
#define DEFAULT_MOUSE_ACCEL "0.0"
#define DEFAULT_MOUSE_SPEED "1.0"
#define DEFAULT_TP_ENABLED  1
#define DEFAULT_TP_TAP      1
#define DEFAULT_TP_NATURAL  1
#define DEFAULT_TP_SCROLL   "two-finger"

/* =========================================================
 *  هياكل البيانات — حالة النظام الكاملة
 * ========================================================= */

typedef struct {
    /* لوحة المفاتيح */
    char kb_layouts[MAX_LAYOUT_LEN];   /* مثال: "us,ar" */
    char kb_model[64];                  /* مثال: "pc105" */
    char kb_options[MAX_OPTION_LEN];   /* مثال: "grp:shift_alt_toggle" */

    /* الماوس */
    double mouse_accel;                 /* التسريع: -1.0 إلى 1.0 */
    double mouse_speed;                 /* السرعة: 0.0 إلى 2.0 */
    int    mouse_natural_scroll;        /* 0 = طبيعي، 1 = معكوس */
    int    mouse_left_handed;           /* 0 = يمين، 1 = يسار */

    /* التاتش باد */
    int    tp_enabled;                  /* 0 = معطّل، 1 = مفعّل */
    int    tp_tap_to_click;             /* النقر بالضغط الخفيف */
    int    tp_natural_scroll;           /* التمرير الطبيعي */
    int    tp_two_finger_scroll;        /* التمرير بإصبعين */
    char   tp_scroll_method[32];        /* "two-finger" أو "edge" */
    double tp_speed;                    /* سرعة التاتش باد */
    int    tp_disable_while_typing;     /* تعطيل أثناء الكتابة */
} VenomInputState;

/* الحالة العالمية للنظام */
static VenomInputState g_state;

/* =========================================================
 *  دوال التسجيل (Logging)
 * ========================================================= */

static void venom_log(const char *level, const char *fmt, ...) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) log = stderr;

    /* الوقت */
    time_t now = time(NULL);
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(log, "[%s] [%s] ", timebuf, level);

    va_list args;
    va_start(args, fmt);
    vfprintf(log, fmt, args);
    va_end(args);

    fprintf(log, "\n");
    fflush(log);

    if (log != stderr) fclose(log);
}

#define LOG_INFO(...)  venom_log("INFO ", __VA_ARGS__)
#define LOG_WARN(...)  venom_log("WARN ", __VA_ARGS__)
#define LOG_ERROR(...) venom_log("ERROR", __VA_ARGS__)

/* =========================================================
 *  التحقق من المدخلات (Input Validation) — أمان ضد Command Injection
 * ========================================================= */

/*
 * يتحقق أن النص لا يحتوي على أحرف خطيرة
 * يسمح فقط بـ: حروف، أرقام، فواصل، شرطات، نقاط، مسافات، شرطة_سفلى
 */
static int is_safe_string(const char *str, size_t max_len) {
    if (!str || strlen(str) == 0 || strlen(str) > max_len) return 0;
    for (const char *p = str; *p; p++) {
        if (!isalnum((unsigned char)*p) &&
            *p != ',' && *p != '-' && *p != '_' &&
            *p != '.' && *p != ' ' && *p != '+' && *p != ':') {
            return 0;
        }
    }
    return 1;
}

/* التحقق من قيمة رقمية ضمن نطاق */
static int is_in_range(double val, double min, double max) {
    return (val >= min && val <= max);
}

/* =========================================================
 *  تهيئة الحالة الافتراضية
 * ========================================================= */

static void state_init_defaults(void) {
    /* لوحة المفاتيح */
    strncpy(g_state.kb_layouts, DEFAULT_LAYOUT,  sizeof(g_state.kb_layouts)  - 1);
    strncpy(g_state.kb_model,   DEFAULT_MODEL,   sizeof(g_state.kb_model)    - 1);
    strncpy(g_state.kb_options, DEFAULT_OPTIONS, sizeof(g_state.kb_options)  - 1);
    g_state.kb_layouts[sizeof(g_state.kb_layouts)   - 1] = '\0';
    g_state.kb_model[sizeof(g_state.kb_model)       - 1] = '\0';
    g_state.kb_options[sizeof(g_state.kb_options)   - 1] = '\0';

    /* الماوس */
    g_state.mouse_accel          = 0.0;
    g_state.mouse_speed          = 1.0;
    g_state.mouse_natural_scroll = 0;
    g_state.mouse_left_handed    = 0;

    /* التاتش باد */
    g_state.tp_enabled              = DEFAULT_TP_ENABLED;
    g_state.tp_tap_to_click         = DEFAULT_TP_TAP;
    g_state.tp_natural_scroll       = DEFAULT_TP_NATURAL;
    g_state.tp_two_finger_scroll    = 1;
    g_state.tp_disable_while_typing = 1;
    g_state.tp_speed                = 0.5;
    strncpy(g_state.tp_scroll_method, DEFAULT_TP_SCROLL, sizeof(g_state.tp_scroll_method) - 1);
    g_state.tp_scroll_method[sizeof(g_state.tp_scroll_method) - 1] = '\0';
}

/* =========================================================
 *  تطبيق الإعدادات على X11
 * ========================================================= */

/* --- لوحة المفاتيح --- */
static void apply_keyboard(void) {
    /* التحقق مرة أخيرة قبل التطبيق */
    if (!is_safe_string(g_state.kb_layouts, MAX_LAYOUT_LEN) ||
        !is_safe_string(g_state.kb_model,   64)             ||
        !is_safe_string(g_state.kb_options, MAX_OPTION_LEN)) {
        LOG_ERROR("apply_keyboard: unsafe string detected, aborting");
        return;
    }

    char cmd[MAX_CMD_LEN];
    snprintf(cmd, sizeof(cmd),
        "setxkbmap -rules evdev -model '%s' -layout '%s' -option '%s' 2>/dev/null",
        g_state.kb_model,
        g_state.kb_layouts,
        g_state.kb_options);

    int ret = system(cmd);
    if (ret != 0)
        LOG_WARN("apply_keyboard: setxkbmap returned %d", ret);
    else
        LOG_INFO("apply_keyboard: layout='%s' model='%s'", g_state.kb_layouts, g_state.kb_model);
}

/* --- دوال إدارة اللغات --- */

/* تحويل "us,ar,fr" إلى array */
static int layouts_to_array(const char *layouts, char arr[][32], int max) {
    int count = 0;
    char tmp[MAX_LAYOUT_LEN];
    strncpy(tmp, layouts, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    char *tok = strtok(tmp, ",");
    while (tok && count < max) {
        strncpy(arr[count++], tok, 31);
        tok = strtok(NULL, ",");
    }
    return count;
}

/* إضافة لغة إذا لم تكن موجودة */
static int layout_add(const char *lang) {
    if (!is_safe_string(lang, 16)) return 0;
    char arr[16][32];
    int n = layouts_to_array(g_state.kb_layouts, arr, 16);
    for (int i = 0; i < n; i++)
        if (strcmp(arr[i], lang) == 0) return 0; /* موجودة مسبقاً */
    if (n >= 16) return 0; /* حد أقصى */
    char new_layouts[MAX_LAYOUT_LEN];
    snprintf(new_layouts, sizeof(new_layouts), "%s,%s", g_state.kb_layouts, lang);
    strncpy(g_state.kb_layouts, new_layouts, sizeof(g_state.kb_layouts) - 1);
    return 1;
}

/* حذف لغة */
static int layout_remove(const char *lang) {
    if (!is_safe_string(lang, 16)) return 0;
    char arr[16][32];
    int n = layouts_to_array(g_state.kb_layouts, arr, 16);
    if (n <= 1) return 0; /* لا يمكن حذف آخر لغة */
    char result[MAX_LAYOUT_LEN] = "";
    int found = 0;
    for (int i = 0; i < n; i++) {
        if (strcmp(arr[i], lang) == 0) { found = 1; continue; }
        if (result[0]) strncat(result, ",", sizeof(result) - strlen(result) - 1);
        strncat(result, arr[i], sizeof(result) - strlen(result) - 1);
    }
    if (!found) return 0;
    strncpy(g_state.kb_layouts, result, sizeof(g_state.kb_layouts) - 1);
    return 1;
}

/* --- الماوس --- */

/*
 * يحصل على device ID للماوس أو التاتش باد عبر xinput
 * type: "mouse" أو "touchpad"
 * يعيد رقم الـ ID أو -1 إذا لم يجد
 */
static int xinput_find_device(const char *type) {
    char cmd[256];
    /* البحث في قائمة xinput عن الجهاز */
    if (strcmp(type, "mouse") == 0) {
        snprintf(cmd, sizeof(cmd),
            "xinput list --id-only 'pointer:' 2>/dev/null | head -1");
    } else {
        snprintf(cmd, sizeof(cmd),
            "xinput list 2>/dev/null | grep -i 'touchpad\\|synaptics' | "
            "grep -o 'id=[0-9]*' | head -1 | cut -d= -f2");
    }
    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;
    int id = -1;
    fscanf(fp, "%d", &id);
    pclose(fp);
    return id;
}

static void apply_mouse(void) {
    char cmd[MAX_CMD_LEN];

    /* تطبيق التسريع عبر xinput على كل الأجهزة المؤشِّرة */
    snprintf(cmd, sizeof(cmd),
        "xinput list 2>/dev/null | grep -i 'pointer' | grep -o 'id=[0-9]*' | "
        "cut -d= -f2 | while read id; do "
        "  xinput set-prop $id 'libinput Accel Speed' %.2f 2>/dev/null; "
        "  xinput set-prop $id 'libinput Natural Scrolling Enabled' %d 2>/dev/null; "
        "  xinput set-prop $id 'libinput Left Handed Enabled' %d 2>/dev/null; "
        "done",
        g_state.mouse_accel,
        g_state.mouse_natural_scroll,
        g_state.mouse_left_handed);

    int ret = system(cmd);
    if (ret != 0)
        LOG_WARN("apply_mouse: xinput returned %d", ret);
    else
        LOG_INFO("apply_mouse: accel=%.2f natural=%d left_handed=%d",
                 g_state.mouse_accel,
                 g_state.mouse_natural_scroll,
                 g_state.mouse_left_handed);
}

/* --- التاتش باد --- */
static void apply_touchpad(void) {
    char cmd[MAX_CMD_LEN];

    /* تمكين/تعطيل التاتش باد */
    snprintf(cmd, sizeof(cmd),
        "xinput list 2>/dev/null | grep -i 'touchpad\\|synaptics' | "
        "grep -o 'id=[0-9]*' | cut -d= -f2 | while read id; do "
        "  xinput %s $id 2>/dev/null; "
        "  xinput set-prop $id 'libinput Tapping Enabled' %d 2>/dev/null; "
        "  xinput set-prop $id 'libinput Natural Scrolling Enabled' %d 2>/dev/null; "
        "  xinput set-prop $id 'libinput Accel Speed' %.2f 2>/dev/null; "
        "  xinput set-prop $id 'libinput Disable While Typing Enabled' %d 2>/dev/null; "
        "  xinput set-prop $id 'libinput Scroll Method Enabled' %d %d 0 2>/dev/null; "
        "done",
        g_state.tp_enabled ? "enable" : "disable",
        g_state.tp_tap_to_click,
        g_state.tp_natural_scroll,
        g_state.tp_speed,
        g_state.tp_disable_while_typing,
        /* Two-Finger=1,0,0  Edge=0,1,0  None=0,0,0 */
        (strcmp(g_state.tp_scroll_method, "two-finger") == 0) ? 1 : 0,
        (strcmp(g_state.tp_scroll_method, "edge")        == 0) ? 1 : 0);

    int ret = system(cmd);
    if (ret != 0)
        LOG_WARN("apply_touchpad: xinput returned %d", ret);
    else
        LOG_INFO("apply_touchpad: enabled=%d tap=%d natural=%d speed=%.2f scroll='%s'",
                 g_state.tp_enabled,
                 g_state.tp_tap_to_click,
                 g_state.tp_natural_scroll,
                 g_state.tp_speed,
                 g_state.tp_scroll_method);
}

/* تطبيق كل الإعدادات دفعة واحدة */
static void apply_all(void) {
    apply_keyboard();
    apply_mouse();
    apply_touchpad();
}

/* =========================================================
 *  حفظ وتحميل الإعدادات (JSON يدوي)
 * ========================================================= */

static void save_config(void) {
    /* إنشاء المجلد إن لم يكن موجوداً */
    struct stat st = {0};
    if (stat(CONFIG_DIR, &st) == -1) {
        if (mkdir(CONFIG_DIR, 0755) != 0 && errno != EEXIST) {
            LOG_ERROR("save_config: cannot create %s: %s", CONFIG_DIR, strerror(errno));
            return;
        }
    }

    FILE *f = fopen(CONFIG_FILE, "w");
    if (!f) {
        LOG_ERROR("save_config: cannot open %s: %s", CONFIG_FILE, strerror(errno));
        return;
    }

    fprintf(f,
        "{\n"
        "  \"keyboard\": {\n"
        "    \"layouts\": \"%s\",\n"
        "    \"model\": \"%s\",\n"
        "    \"options\": \"%s\"\n"
        "  },\n"
        "  \"mouse\": {\n"
        "    \"accel\": %.2f,\n"
        "    \"speed\": %.2f,\n"
        "    \"natural_scroll\": %d,\n"
        "    \"left_handed\": %d\n"
        "  },\n"
        "  \"touchpad\": {\n"
        "    \"enabled\": %d,\n"
        "    \"tap_to_click\": %d,\n"
        "    \"natural_scroll\": %d,\n"
        "    \"two_finger_scroll\": %d,\n"
        "    \"scroll_method\": \"%s\",\n"
        "    \"speed\": %.2f,\n"
        "    \"disable_while_typing\": %d\n"
        "  }\n"
        "}\n",
        g_state.kb_layouts,
        g_state.kb_model,
        g_state.kb_options,
        g_state.mouse_accel,
        g_state.mouse_speed,
        g_state.mouse_natural_scroll,
        g_state.mouse_left_handed,
        g_state.tp_enabled,
        g_state.tp_tap_to_click,
        g_state.tp_natural_scroll,
        g_state.tp_two_finger_scroll,
        g_state.tp_scroll_method,
        g_state.tp_speed,
        g_state.tp_disable_while_typing
    );

    fclose(f);
    LOG_INFO("save_config: saved to %s", CONFIG_FILE);
}

/* مساعد: استخراج قيمة string من JSON بسيط */
static int json_get_string(const char *json, const char *key, char *out, size_t out_size) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char *p = strstr(json, search);
    if (!p) return 0;
    p += strlen(search);
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return 0;
    p++; /* تخطي " */
    const char *end = strchr(p, '"');
    if (!end) return 0;
    size_t len = (size_t)(end - p);
    if (len >= out_size) len = out_size - 1;
    memcpy(out, p, len);
    out[len] = '\0';
    return 1;
}

/* مساعد: استخراج قيمة رقمية من JSON بسيط */
static int json_get_double(const char *json, const char *key, double *out) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char *p = strstr(json, search);
    if (!p) return 0;
    p += strlen(search);
    while (*p == ' ' || *p == '\t') p++;
    char *endptr;
    double val = strtod(p, &endptr);
    if (endptr == p) return 0;
    *out = val;
    return 1;
}

/* مساعد: استخراج قيمة int من JSON بسيط */
static int json_get_int(const char *json, const char *key, int *out) {
    double val;
    if (!json_get_double(json, key, &val)) return 0;
    *out = (int)val;
    return 1;
}

static void load_config(void) {
    state_init_defaults();

    FILE *f = fopen(CONFIG_FILE, "r");
    if (!f) {
        LOG_INFO("load_config: no config file found, using defaults");
        apply_all();
        return;
    }

    char buf[MAX_BUFFER_LEN];
    size_t len = fread(buf, 1, sizeof(buf) - 1, f);
    buf[len] = '\0';
    fclose(f);

    /* --- لوحة المفاتيح --- */
    char tmp[MAX_LAYOUT_LEN];
    if (json_get_string(buf, "layouts", tmp, sizeof(tmp)) && is_safe_string(tmp, MAX_LAYOUT_LEN))
        strncpy(g_state.kb_layouts, tmp, sizeof(g_state.kb_layouts) - 1);

    if (json_get_string(buf, "model", tmp, sizeof(tmp)) && is_safe_string(tmp, 64))
        strncpy(g_state.kb_model, tmp, sizeof(g_state.kb_model) - 1);

    char opt_tmp[MAX_OPTION_LEN];
    if (json_get_string(buf, "options", opt_tmp, sizeof(opt_tmp)) && is_safe_string(opt_tmp, MAX_OPTION_LEN))
        strncpy(g_state.kb_options, opt_tmp, sizeof(g_state.kb_options) - 1);

    /* --- الماوس --- */
    double dval;
    int    ival;
    if (json_get_double(buf, "accel", &dval) && is_in_range(dval, -1.0, 1.0))
        g_state.mouse_accel = dval;
    if (json_get_double(buf, "speed", &dval) && is_in_range(dval, 0.0, 2.0))
        g_state.mouse_speed = dval;
    if (json_get_int(buf, "natural_scroll", &ival)) g_state.mouse_natural_scroll = ival ? 1 : 0;
    if (json_get_int(buf, "left_handed",    &ival)) g_state.mouse_left_handed    = ival ? 1 : 0;

    /* --- التاتش باد --- */
    if (json_get_int(buf, "enabled",              &ival)) g_state.tp_enabled              = ival ? 1 : 0;
    if (json_get_int(buf, "tap_to_click",         &ival)) g_state.tp_tap_to_click         = ival ? 1 : 0;
    if (json_get_int(buf, "two_finger_scroll",    &ival)) g_state.tp_two_finger_scroll    = ival ? 1 : 0;
    if (json_get_int(buf, "disable_while_typing", &ival)) g_state.tp_disable_while_typing = ival ? 1 : 0;

    /* natural_scroll للتاتش باد — يمكن أن يختلف عن الماوس */
    const char *tp_section = strstr(buf, "\"touchpad\"");
    if (tp_section) {
        if (json_get_int(tp_section, "natural_scroll", &ival)) g_state.tp_natural_scroll = ival ? 1 : 0;
        if (json_get_double(tp_section, "speed", &dval) && is_in_range(dval, 0.0, 1.0))
            g_state.tp_speed = dval;
        char scroll_tmp[32];
        if (json_get_string(tp_section, "scroll_method", scroll_tmp, sizeof(scroll_tmp)) &&
            (strcmp(scroll_tmp, "two-finger") == 0 || strcmp(scroll_tmp, "edge") == 0 || strcmp(scroll_tmp, "none") == 0))
            strncpy(g_state.tp_scroll_method, scroll_tmp, sizeof(g_state.tp_scroll_method) - 1);
    }

    LOG_INFO("load_config: loaded from %s", CONFIG_FILE);
    apply_all();
}

/* =========================================================
 *  D-Bus Interface XML
 * ========================================================= */

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.venom.Input'>"
  /* ========== لوحة المفاتيح ========== */
  "    <method name='SetKeyboardLayouts'>"
  "      <arg type='s' name='layouts' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='SetKeyboardModel'>"
  "      <arg type='s' name='model' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='SetKeyboardOptions'>"
  "      <arg type='s' name='options' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='GetKeyboardSettings'>"
  "      <arg type='s' name='layouts' direction='out'/>"
  "      <arg type='s' name='model'   direction='out'/>"
  "      <arg type='s' name='options' direction='out'/>"
  "    </method>"
  "    <method name='AddKeyboardLayout'>"
  "      <arg type='s' name='layout' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='RemoveKeyboardLayout'>"
  "      <arg type='s' name='layout' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='ListKeyboardLayouts'>"
  "      <arg type='as' name='layouts' direction='out'/>"
  "    </method>"
  /* ========== الماوس ========== */
  "    <method name='SetMouseAccel'>"
  "      <arg type='d' name='accel' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='SetMouseSpeed'>"
  "      <arg type='d' name='speed' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='SetMouseNaturalScroll'>"
  "      <arg type='b' name='enabled' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='SetMouseLeftHanded'>"
  "      <arg type='b' name='enabled' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='GetMouseSettings'>"
  "      <arg type='d' name='accel'          direction='out'/>"
  "      <arg type='d' name='speed'          direction='out'/>"
  "      <arg type='b' name='natural_scroll' direction='out'/>"
  "      <arg type='b' name='left_handed'    direction='out'/>"
  "    </method>"
  /* ========== التاتش باد ========== */
  "    <method name='SetTouchpadEnabled'>"
  "      <arg type='b' name='enabled' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='SetTouchpadTapToClick'>"
  "      <arg type='b' name='enabled' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='SetTouchpadNaturalScroll'>"
  "      <arg type='b' name='enabled' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='SetTouchpadScrollMethod'>"
  "      <arg type='s' name='method' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='SetTouchpadSpeed'>"
  "      <arg type='d' name='speed' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='SetTouchpadDisableWhileTyping'>"
  "      <arg type='b' name='enabled' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='GetTouchpadSettings'>"
  "      <arg type='b' name='enabled'               direction='out'/>"
  "      <arg type='b' name='tap_to_click'          direction='out'/>"
  "      <arg type='b' name='natural_scroll'        direction='out'/>"
  "      <arg type='s' name='scroll_method'         direction='out'/>"
  "      <arg type='d' name='speed'                 direction='out'/>"
  "      <arg type='b' name='disable_while_typing'  direction='out'/>"
  "    </method>"
  /* ========== عام ========== */
  "    <method name='ReloadConfig'>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='GetAllSettings'>"
  "      <arg type='s' name='json' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

/* =========================================================
 *  معالج D-Bus
 * ========================================================= */

static void handle_method_call(GDBusConnection       *connection,
                               const gchar           *sender,
                               const gchar           *object_path,
                               const gchar           *interface_name,
                               const gchar           *method_name,
                               GVariant              *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer               user_data)
{
    (void)connection; (void)sender; (void)object_path;
    (void)interface_name; (void)user_data;

    LOG_INFO("D-Bus call: %s", method_name);

    /* ================================================
     * لوحة المفاتيح
     * ================================================ */
    if (g_strcmp0(method_name, "SetKeyboardLayouts") == 0) {
        const gchar *val;
        g_variant_get(parameters, "(&s)", &val);
        if (!is_safe_string(val, MAX_LAYOUT_LEN)) {
            LOG_WARN("SetKeyboardLayouts: invalid input '%s'", val);
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", FALSE));
            return;
        }
        strncpy(g_state.kb_layouts, val, sizeof(g_state.kb_layouts) - 1);
        g_state.kb_layouts[sizeof(g_state.kb_layouts) - 1] = '\0';
        apply_keyboard();
        save_config();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    }

    else if (g_strcmp0(method_name, "SetKeyboardModel") == 0) {
        const gchar *val;
        g_variant_get(parameters, "(&s)", &val);
        if (!is_safe_string(val, 64)) {
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", FALSE));
            return;
        }
        strncpy(g_state.kb_model, val, sizeof(g_state.kb_model) - 1);
        g_state.kb_model[sizeof(g_state.kb_model) - 1] = '\0';
        apply_keyboard();
        save_config();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    }

    else if (g_strcmp0(method_name, "SetKeyboardOptions") == 0) {
        const gchar *val;
        g_variant_get(parameters, "(&s)", &val);
        if (!is_safe_string(val, MAX_OPTION_LEN)) {
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", FALSE));
            return;
        }
        strncpy(g_state.kb_options, val, sizeof(g_state.kb_options) - 1);
        g_state.kb_options[sizeof(g_state.kb_options) - 1] = '\0';
        apply_keyboard();
        save_config();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    }

    else if (g_strcmp0(method_name, "GetKeyboardSettings") == 0) {
        g_dbus_method_invocation_return_value(invocation,
            g_variant_new("(sss)",
                g_state.kb_layouts,
                g_state.kb_model,
                g_state.kb_options));
    }

    else if (g_strcmp0(method_name, "AddKeyboardLayout") == 0) {
        const gchar *val;
        g_variant_get(parameters, "(&s)", &val);
        gboolean ok = layout_add(val);
        if (ok) { apply_keyboard(); save_config(); }
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", ok));
    }

    else if (g_strcmp0(method_name, "RemoveKeyboardLayout") == 0) {
        const gchar *val;
        g_variant_get(parameters, "(&s)", &val);
        gboolean ok = layout_remove(val);
        if (ok) { apply_keyboard(); save_config(); }
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", ok));
    }

    else if (g_strcmp0(method_name, "ListKeyboardLayouts") == 0) {
        char arr[16][32];
        int n = layouts_to_array(g_state.kb_layouts, arr, 16);
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
        for (int i = 0; i < n; i++)
            g_variant_builder_add(&builder, "s", arr[i]);
        g_dbus_method_invocation_return_value(invocation,
            g_variant_new("(as)", &builder));
    }

    /* ================================================
     * الماوس
     * ================================================ */
    else if (g_strcmp0(method_name, "SetMouseAccel") == 0) {
        gdouble val;
        g_variant_get(parameters, "(d)", &val);
        if (!is_in_range(val, -1.0, 1.0)) {
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", FALSE));
            return;
        }
        g_state.mouse_accel = val;
        apply_mouse();
        save_config();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    }

    else if (g_strcmp0(method_name, "SetMouseSpeed") == 0) {
        gdouble val;
        g_variant_get(parameters, "(d)", &val);
        if (!is_in_range(val, 0.0, 2.0)) {
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", FALSE));
            return;
        }
        g_state.mouse_speed = val;
        apply_mouse();
        save_config();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    }

    else if (g_strcmp0(method_name, "SetMouseNaturalScroll") == 0) {
        gboolean val;
        g_variant_get(parameters, "(b)", &val);
        g_state.mouse_natural_scroll = val ? 1 : 0;
        apply_mouse();
        save_config();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    }

    else if (g_strcmp0(method_name, "SetMouseLeftHanded") == 0) {
        gboolean val;
        g_variant_get(parameters, "(b)", &val);
        g_state.mouse_left_handed = val ? 1 : 0;
        apply_mouse();
        save_config();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    }

    else if (g_strcmp0(method_name, "GetMouseSettings") == 0) {
        g_dbus_method_invocation_return_value(invocation,
            g_variant_new("(ddbb)",
                g_state.mouse_accel,
                g_state.mouse_speed,
                (gboolean)g_state.mouse_natural_scroll,
                (gboolean)g_state.mouse_left_handed));
    }

    /* ================================================
     * التاتش باد
     * ================================================ */
    else if (g_strcmp0(method_name, "SetTouchpadEnabled") == 0) {
        gboolean val;
        g_variant_get(parameters, "(b)", &val);
        g_state.tp_enabled = val ? 1 : 0;
        apply_touchpad();
        save_config();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    }

    else if (g_strcmp0(method_name, "SetTouchpadTapToClick") == 0) {
        gboolean val;
        g_variant_get(parameters, "(b)", &val);
        g_state.tp_tap_to_click = val ? 1 : 0;
        apply_touchpad();
        save_config();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    }

    else if (g_strcmp0(method_name, "SetTouchpadNaturalScroll") == 0) {
        gboolean val;
        g_variant_get(parameters, "(b)", &val);
        g_state.tp_natural_scroll = val ? 1 : 0;
        apply_touchpad();
        save_config();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    }

    else if (g_strcmp0(method_name, "SetTouchpadScrollMethod") == 0) {
        const gchar *val;
        g_variant_get(parameters, "(&s)", &val);
        if (strcmp(val, "two-finger") != 0 &&
            strcmp(val, "edge")        != 0 &&
            strcmp(val, "none")        != 0) {
            LOG_WARN("SetTouchpadScrollMethod: invalid method '%s'", val);
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", FALSE));
            return;
        }
        strncpy(g_state.tp_scroll_method, val, sizeof(g_state.tp_scroll_method) - 1);
        g_state.tp_scroll_method[sizeof(g_state.tp_scroll_method) - 1] = '\0';
        apply_touchpad();
        save_config();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    }

    else if (g_strcmp0(method_name, "SetTouchpadSpeed") == 0) {
        gdouble val;
        g_variant_get(parameters, "(d)", &val);
        if (!is_in_range(val, 0.0, 1.0)) {
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", FALSE));
            return;
        }
        g_state.tp_speed = val;
        apply_touchpad();
        save_config();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    }

    else if (g_strcmp0(method_name, "SetTouchpadDisableWhileTyping") == 0) {
        gboolean val;
        g_variant_get(parameters, "(b)", &val);
        g_state.tp_disable_while_typing = val ? 1 : 0;
        apply_touchpad();
        save_config();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    }

    else if (g_strcmp0(method_name, "GetTouchpadSettings") == 0) {
        g_dbus_method_invocation_return_value(invocation,
            g_variant_new("(bbbsdb)",
                (gboolean)g_state.tp_enabled,
                (gboolean)g_state.tp_tap_to_click,
                (gboolean)g_state.tp_natural_scroll,
                g_state.tp_scroll_method,
                g_state.tp_speed,
                (gboolean)g_state.tp_disable_while_typing));
    }

    /* ================================================
     * عام
     * ================================================ */
    else if (g_strcmp0(method_name, "ReloadConfig") == 0) {
        load_config();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    }

    else if (g_strcmp0(method_name, "GetAllSettings") == 0) {
        /* إعادة الحالة الكاملة كـ JSON string */
        char json[MAX_BUFFER_LEN];
        snprintf(json, sizeof(json),
            "{"
            "\"keyboard\":{"
              "\"layouts\":\"%s\","
              "\"model\":\"%s\","
              "\"options\":\"%s\""
            "},"
            "\"mouse\":{"
              "\"accel\":%.2f,"
              "\"speed\":%.2f,"
              "\"natural_scroll\":%d,"
              "\"left_handed\":%d"
            "},"
            "\"touchpad\":{"
              "\"enabled\":%d,"
              "\"tap_to_click\":%d,"
              "\"natural_scroll\":%d,"
              "\"scroll_method\":\"%s\","
              "\"speed\":%.2f,"
              "\"disable_while_typing\":%d"
            "}"
            "}",
            g_state.kb_layouts, g_state.kb_model, g_state.kb_options,
            g_state.mouse_accel, g_state.mouse_speed,
            g_state.mouse_natural_scroll, g_state.mouse_left_handed,
            g_state.tp_enabled, g_state.tp_tap_to_click,
            g_state.tp_natural_scroll, g_state.tp_scroll_method,
            g_state.tp_speed, g_state.tp_disable_while_typing);

        g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", json));
    }

    else {
        /* method غير معروف */
        g_dbus_method_invocation_return_dbus_error(invocation,
            "org.venom.Input.Error.UnknownMethod",
            "Unknown method");
        LOG_WARN("Unknown D-Bus method: %s", method_name);
    }
}

static const GDBusInterfaceVTable interface_vtable = {
    handle_method_call, NULL, NULL
};

/* =========================================================
 *  D-Bus — الاتصال بـ Bus
 * ========================================================= */

static void on_bus_acquired(GDBusConnection *connection,
                            const gchar     *name,
                            gpointer         user_data)
{
    (void)name; (void)user_data;
    GError *error = NULL;

    GDBusNodeInfo *node_info = g_dbus_node_info_new_for_xml(introspection_xml, &error);
    if (error) {
        LOG_ERROR("on_bus_acquired: XML parse error: %s", error->message);
        g_error_free(error);
        return;
    }

    guint reg_id = g_dbus_connection_register_object(
        connection,
        "/org/venom/Input",
        node_info->interfaces[0],
        &interface_vtable,
        NULL, NULL, &error);

    g_dbus_node_info_unref(node_info);

    if (error) {
        LOG_ERROR("on_bus_acquired: register error: %s", error->message);
        g_error_free(error);
        return;
    }

    LOG_INFO("D-Bus object registered (id=%u)", reg_id);

    /* تحميل الإعدادات المحفوظة وتطبيقها */
    load_config();
}

static void on_name_acquired(GDBusConnection *connection,
                             const gchar     *name,
                             gpointer         user_data)
{
    (void)connection; (void)user_data;
    LOG_INFO("D-Bus name acquired: %s", name);
}

static void on_name_lost(GDBusConnection *connection,
                         const gchar     *name,
                         gpointer         user_data)
{
    (void)connection; (void)user_data;
    LOG_ERROR("D-Bus name lost: %s — exiting", name);
    exit(EXIT_FAILURE);
}

/* =========================================================
 *  نقطة الدخول الرئيسية
 * ========================================================= */

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    LOG_INFO("venom_input daemon starting (PID=%d)", (int)getpid());

    /* تهيئة القيم الافتراضية */
    state_init_defaults();

    /* الاتصال بـ Session Bus وحجز الاسم */
    guint owner_id = g_bus_own_name(
        G_BUS_TYPE_SESSION,
        "org.venom.Input",
        G_BUS_NAME_OWNER_FLAGS_NONE,
        on_bus_acquired,
        on_name_acquired,
        on_name_lost,
        NULL, NULL);

    /* تشغيل حلقة الأحداث الرئيسية */
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    /* تنظيف */
    g_bus_unown_name(owner_id);
    g_main_loop_unref(loop);

    LOG_INFO("venom_input daemon stopped");
    return EXIT_SUCCESS;
}