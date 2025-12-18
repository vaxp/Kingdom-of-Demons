#include "dbus_service.h"
#include "display.h"
#include "venom_display.h"
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// 📄 D-Bus Introspection XML
// ═══════════════════════════════════════════════════════════════════════════

const gchar* dbus_get_introspection_xml(void) {
    return
    "<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' "
    "'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>"
    "<node>"
    "  <interface name='org.venom.Display'>"
    // ═══════════ معلومات الشاشات ═══════════
    "    <method name='GetDisplays'>"
    "      <arg type='a(siidbbiix)' name='displays' direction='out'/>"
    "    </method>"
    "    <method name='GetPrimaryDisplay'>"
    "      <arg type='(siidbbiix)' name='display' direction='out'/>"
    "    </method>"
    "    <method name='GetDisplayInfo'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='(siidbbiix)' name='display' direction='out'/>"
    "    </method>"
    "    <method name='GetModes'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='a(iid)' name='modes' direction='out'/>"
    "    </method>"
    // ═══════════ تغيير الإعدادات ═══════════
    "    <method name='SetResolution'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='width' direction='in'/>"
    "      <arg type='i' name='height' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='SetRefreshRate'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='d' name='rate' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='SetMode'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='width' direction='in'/>"
    "      <arg type='i' name='height' direction='in'/>"
    "      <arg type='d' name='rate' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='SetPosition'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='x' direction='in'/>"
    "      <arg type='i' name='y' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='SetPrimary'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // ═══════════ التدوير ═══════════
    "    <method name='GetRotation'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='rotation' direction='out'/>"
    "    </method>"
    "    <method name='SetRotation'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='rotation' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // ═══════════ تشغيل/إيقاف ═══════════
    "    <method name='EnableOutput'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='DisableOutput'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // ═══════════ المطابقة ═══════════
    "    <method name='SetMirror'>"
    "      <arg type='s' name='source' direction='in'/>"
    "      <arg type='s' name='target' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='DisableMirror'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // ═══════════ التكبير ═══════════
    "    <method name='GetScale'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='d' name='scale' direction='out'/>"
    "    </method>"
    "    <method name='SetScale'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='d' name='scale' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // ═══════════ Night Light ═══════════
    "    <method name='GetNightLight'>"
    "      <arg type='(bi)' name='settings' direction='out'/>"
    "    </method>"
    "    <method name='SetNightLight'>"
    "      <arg type='b' name='enabled' direction='in'/>"
    "      <arg type='i' name='temperature' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // ═══════════ البروفايلات ═══════════
    "    <method name='SaveProfile'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='LoadProfile'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='GetProfiles'>"
    "      <arg type='as' name='profiles' direction='out'/>"
    "    </method>"
    "    <method name='DeleteProfile'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // ═══════════ DPMS ═══════════
    "    <method name='GetDPMSMode'>"
    "      <arg type='i' name='mode' direction='out'/>"
    "    </method>"
    "    <method name='SetDPMSMode'>"
    "      <arg type='i' name='mode' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='GetDPMSTimeouts'>"
    "      <arg type='(iii)' name='timeouts' direction='out'/>"
    "    </method>"
    "    <method name='SetDPMSTimeouts'>"
    "      <arg type='i' name='standby' direction='in'/>"
    "      <arg type='i' name='suspend' direction='in'/>"
    "      <arg type='i' name='off' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // ═══════════ Backlight ═══════════
    "    <method name='GetBacklight'>"
    "      <arg type='i' name='level' direction='out'/>"
    "    </method>"
    "    <method name='SetBacklight'>"
    "      <arg type='i' name='level' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='GetMaxBacklight'>"
    "      <arg type='i' name='max' direction='out'/>"
    "    </method>"
    // ═══════════ EDID ═══════════
    "    <method name='GetEDID'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='(sssuuiibb)' name='edid' direction='out'/>"
    "    </method>"
    "    <method name='GetEDIDHash'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='s' name='hash' direction='out'/>"
    "    </method>"
    // ═══════════ DDC/CI ═══════════
    "    <method name='GetHWBrightness'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='level' direction='out'/>"
    "    </method>"
    "    <method name='SetHWBrightness'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='level' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='GetHWContrast'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='level' direction='out'/>"
    "    </method>"
    "    <method name='SetHWContrast'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='level' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='PowerOffMonitor'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='PowerOnMonitor'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // ═══════════ VRR ═══════════
    "    <method name='IsVRRCapable'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='capable' direction='out'/>"
    "    </method>"
    "    <method name='IsVRREnabled'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='enabled' direction='out'/>"
    "    </method>"
    "    <method name='SetVRR'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='enable' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='GetVRRInfo'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='(bbii)' name='info' direction='out'/>"
    "    </method>"
    // ═══════════ Auto Profile ═══════════
    "    <method name='SetAutoProfile'>"
    "      <arg type='s' name='edid_match' direction='in'/>"
    "      <arg type='s' name='profile' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='RemoveAutoProfile'>"
    "      <arg type='s' name='edid_match' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='GetAutoProfiles'>"
    "      <arg type='a(ss)' name='rules' direction='out'/>"
    "    </method>"
    "    <method name='ApplyAutoProfiles'>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // ═══════════ Arrangement ═══════════
    "    <method name='Arrange'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='position' direction='in'/>"
    "      <arg type='s' name='relative_to' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='AutoArrange'>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='ExtendRight'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='ExtendLeft'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // ═══════════ الإشارات ═══════════
    "    <signal name='DisplayChanged'>"
    "      <arg type='s' name='name'/>"
    "    </signal>"
    "    <signal name='NightLightChanged'>"
    "      <arg type='b' name='enabled'/>"
    "      <arg type='i' name='temperature'/>"
    "    </signal>"
    "  </interface>"
    "</node>";
}

// ═══════════════════════════════════════════════════════════════════════════
// 🔌 معالج طرق D-Bus
// ═══════════════════════════════════════════════════════════════════════════

static GVariant* display_info_to_variant(DisplayInfo *info) {
    return g_variant_new("(siidbbiix)",
        info->name,
        info->width,
        info->height,
        info->refresh_rate,
        info->is_connected,
        info->is_primary,
        info->x,
        info->y,
        (gint64)info->output_id);
}

void dbus_handle_method_call(GDBusConnection *connection,
                             const gchar *sender,
                             const gchar *object_path,
                             const gchar *interface_name,
                             const gchar *method_name,
                             GVariant *parameters,
                             GDBusMethodInvocation *invocation,
                             gpointer user_data) {
    (void)connection; (void)sender; (void)object_path; (void)interface_name; (void)user_data;
    
    // ═══════════ GetDisplays ═══════════
    if (g_strcmp0(method_name, "GetDisplays") == 0) {
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a(siidbbiix)"));
        GList *displays = display_get_all();
        for (GList *l = displays; l; l = l->next) {
            DisplayInfo *info = l->data;
            g_variant_builder_add_value(&builder, display_info_to_variant(info));
            display_info_free(info);
        }
        g_list_free(displays);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(a(siidbbiix))", &builder));
    }
    // ═══════════ GetPrimaryDisplay ═══════════
    else if (g_strcmp0(method_name, "GetPrimaryDisplay") == 0) {
        DisplayInfo *info = display_get_primary();
        if (info) {
            g_dbus_method_invocation_return_value(invocation, 
                g_variant_new("((siidbbiix))", info->name, info->width, info->height, 
                    info->refresh_rate, info->is_connected, info->is_primary, 
                    info->x, info->y, (gint64)info->output_id));
            display_info_free(info);
        } else {
            g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "No primary display");
        }
    }
    // ═══════════ GetDisplayInfo ═══════════
    else if (g_strcmp0(method_name, "GetDisplayInfo") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        DisplayInfo *info = display_get_by_name(name);
        if (info) {
            g_dbus_method_invocation_return_value(invocation, 
                g_variant_new("((siidbbiix))", info->name, info->width, info->height, 
                    info->refresh_rate, info->is_connected, info->is_primary, 
                    info->x, info->y, (gint64)info->output_id));
            display_info_free(info);
        } else {
            g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Display not found");
        }
    }
    // ═══════════ GetModes ═══════════
    else if (g_strcmp0(method_name, "GetModes") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a(iid)"));
        GList *modes = display_get_modes(name);
        for (GList *l = modes; l; l = l->next) {
            DisplayMode *mode = l->data;
            g_variant_builder_add(&builder, "(iid)", mode->width, mode->height, mode->rates[0]);
            display_mode_free(mode);
        }
        g_list_free(modes);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(a(iid))", &builder));
    }
    // ═══════════ SetResolution ═══════════
    else if (g_strcmp0(method_name, "SetResolution") == 0) {
        const gchar *name; gint32 w, h;
        g_variant_get(parameters, "(&sii)", &name, &w, &h);
        gboolean success = display_set_resolution(name, w, h);
        if (success) display_persist_save();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", success));
    }
    // ═══════════ SetRefreshRate ═══════════
    else if (g_strcmp0(method_name, "SetRefreshRate") == 0) {
        const gchar *name; gdouble rate;
        g_variant_get(parameters, "(&sd)", &name, &rate);
        gboolean success = display_set_refresh_rate(name, rate);
        if (success) display_persist_save();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", success));
    }
    // ═══════════ SetMode ═══════════
    else if (g_strcmp0(method_name, "SetMode") == 0) {
        const gchar *name; gint32 w, h; gdouble rate;
        g_variant_get(parameters, "(&siid)", &name, &w, &h, &rate);
        gboolean success = display_set_mode(name, w, h, rate);
        if (success) display_persist_save();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", success));
    }
    // ═══════════ SetPosition ═══════════
    else if (g_strcmp0(method_name, "SetPosition") == 0) {
        const gchar *name; gint32 x, y;
        g_variant_get(parameters, "(&sii)", &name, &x, &y);
        gboolean success = display_set_position(name, x, y);
        if (success) display_persist_save();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", success));
    }
    // ═══════════ SetPrimary ═══════════
    else if (g_strcmp0(method_name, "SetPrimary") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        gboolean success = display_set_primary(name);
        if (success) display_persist_save();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", success));
    }
    // ═══════════ GetRotation ═══════════
    else if (g_strcmp0(method_name, "GetRotation") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", (gint32)display_get_rotation(name)));
    }
    // ═══════════ SetRotation ═══════════
    else if (g_strcmp0(method_name, "SetRotation") == 0) {
        const gchar *name; gint32 rotation;
        g_variant_get(parameters, "(&si)", &name, &rotation);
        gboolean success = display_set_rotation(name, (RotationType)rotation);
        if (success) display_persist_save();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", success));
    }
    // ═══════════ EnableOutput ═══════════
    else if (g_strcmp0(method_name, "EnableOutput") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        gboolean success = display_enable(name);
        if (success) display_persist_save();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", success));
    }
    // ═══════════ DisableOutput ═══════════
    else if (g_strcmp0(method_name, "DisableOutput") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        gboolean success = display_disable(name);
        if (success) display_persist_save();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", success));
    }
    // ═══════════ SetMirror ═══════════
    else if (g_strcmp0(method_name, "SetMirror") == 0) {
        const gchar *source, *target;
        g_variant_get(parameters, "(&s&s)", &source, &target);
        gboolean success = display_set_mirror(source, target);
        if (success) display_persist_save();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", success));
    }
    // ═══════════ DisableMirror ═══════════
    else if (g_strcmp0(method_name, "DisableMirror") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        gboolean success = display_disable_mirror(name);
        if (success) display_persist_save();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", success));
    }
    // ═══════════ GetScale ═══════════
    else if (g_strcmp0(method_name, "GetScale") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(d)", display_get_scale(name)));
    }
    // ═══════════ SetScale ═══════════
    else if (g_strcmp0(method_name, "SetScale") == 0) {
        const gchar *name; gdouble scale;
        g_variant_get(parameters, "(&sd)", &name, &scale);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_set_scale(name, scale)));
    }
    // ═══════════ GetNightLight ═══════════
    else if (g_strcmp0(method_name, "GetNightLight") == 0) {
        NightLightSettings nl = display_get_night_light();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("((bi))", nl.enabled, nl.temperature));
    }
    // ═══════════ SetNightLight ═══════════
    else if (g_strcmp0(method_name, "SetNightLight") == 0) {
        gboolean enabled; gint32 temp;
        g_variant_get(parameters, "(bi)", &enabled, &temp);
        gboolean success = display_set_night_light(enabled, temp);
        if (success) display_persist_save_night_light();
        dbus_emit_signal("NightLightChanged", g_variant_new("(bi)", enabled, temp));
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", success));
    }
    // ═══════════ SaveProfile ═══════════
    else if (g_strcmp0(method_name, "SaveProfile") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_save_profile(name)));
    }
    // ═══════════ LoadProfile ═══════════
    else if (g_strcmp0(method_name, "LoadProfile") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_load_profile(name)));
    }
    // ═══════════ GetProfiles ═══════════
    else if (g_strcmp0(method_name, "GetProfiles") == 0) {
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
        GList *profiles = display_get_profiles();
        for (GList *l = profiles; l; l = l->next) {
            g_variant_builder_add(&builder, "s", (gchar*)l->data);
            g_free(l->data);
        }
        g_list_free(profiles);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(as)", &builder));
    }
    // ═══════════ DeleteProfile ═══════════
    else if (g_strcmp0(method_name, "DeleteProfile") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_delete_profile(name)));
    }
    // ═══════════ DPMS Methods ═══════════
    else if (g_strcmp0(method_name, "GetDPMSMode") == 0) {
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", (gint32)display_dpms_get_mode()));
    }
    else if (g_strcmp0(method_name, "SetDPMSMode") == 0) {
        gint32 mode;
        g_variant_get(parameters, "(i)", &mode);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_dpms_set_mode((DPMSMode)mode)));
    }
    else if (g_strcmp0(method_name, "GetDPMSTimeouts") == 0) {
        int s, sp, o;
        display_dpms_get_timeouts(&s, &sp, &o);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("((iii))", s, sp, o));
    }
    else if (g_strcmp0(method_name, "SetDPMSTimeouts") == 0) {
        gint32 s, sp, o;
        g_variant_get(parameters, "(iii)", &s, &sp, &o);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_dpms_set_timeouts(s, sp, o)));
    }
    // ═══════════ Backlight Methods ═══════════
    else if (g_strcmp0(method_name, "GetBacklight") == 0) {
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", display_backlight_get()));
    }
    else if (g_strcmp0(method_name, "SetBacklight") == 0) {
        gint32 level;
        g_variant_get(parameters, "(i)", &level);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_backlight_set(level)));
    }
    else if (g_strcmp0(method_name, "GetMaxBacklight") == 0) {
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", display_backlight_get_max()));
    }
    // ═══════════ EDID Methods ═══════════
    else if (g_strcmp0(method_name, "GetEDID") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        EDIDInfo *edid = display_get_edid(name);
        if (edid) {
            g_dbus_method_invocation_return_value(invocation, 
                g_variant_new("((sssuuiibb))", edid->manufacturer, edid->model, edid->serial,
                    edid->product_code, edid->serial_number, edid->max_width, edid->max_height,
                    edid->hdr_supported, edid->vrr_supported));
            display_edid_free(edid);
        } else {
            g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "EDID not found");
        }
    }
    else if (g_strcmp0(method_name, "GetEDIDHash") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        gchar *hash = display_get_edid_hash(name);
        if (hash) {
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", hash));
            g_free(hash);
        } else {
            g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "EDID not found");
        }
    }
    // ═══════════ DDC/CI Methods ═══════════
    else if (g_strcmp0(method_name, "GetHWBrightness") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", display_ddc_get_brightness(name)));
    }
    else if (g_strcmp0(method_name, "SetHWBrightness") == 0) {
        const gchar *name; gint32 level;
        g_variant_get(parameters, "(&si)", &name, &level);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_ddc_set_brightness(name, level)));
    }
    else if (g_strcmp0(method_name, "GetHWContrast") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", display_ddc_get_contrast(name)));
    }
    else if (g_strcmp0(method_name, "SetHWContrast") == 0) {
        const gchar *name; gint32 level;
        g_variant_get(parameters, "(&si)", &name, &level);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_ddc_set_contrast(name, level)));
    }
    else if (g_strcmp0(method_name, "PowerOffMonitor") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_ddc_power_off(name)));
    }
    else if (g_strcmp0(method_name, "PowerOnMonitor") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_ddc_power_on(name)));
    }
    // ═══════════ VRR Methods ═══════════
    else if (g_strcmp0(method_name, "IsVRRCapable") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_vrr_capable(name)));
    }
    else if (g_strcmp0(method_name, "IsVRREnabled") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_vrr_enabled(name)));
    }
    else if (g_strcmp0(method_name, "SetVRR") == 0) {
        const gchar *name; gboolean enable;
        g_variant_get(parameters, "(&sb)", &name, &enable);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_vrr_set(name, enable)));
    }
    else if (g_strcmp0(method_name, "GetVRRInfo") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        VRRInfo info = display_vrr_get_info(name);
        g_dbus_method_invocation_return_value(invocation, 
            g_variant_new("((bbii))", info.capable, info.enabled, info.min_rate, info.max_rate));
    }
    // ═══════════ Auto Profile Methods ═══════════
    else if (g_strcmp0(method_name, "SetAutoProfile") == 0) {
        const gchar *edid_match, *profile;
        g_variant_get(parameters, "(&s&s)", &edid_match, &profile);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_auto_set_rule(edid_match, profile)));
    }
    else if (g_strcmp0(method_name, "RemoveAutoProfile") == 0) {
        const gchar *edid_match;
        g_variant_get(parameters, "(&s)", &edid_match);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_auto_remove_rule(edid_match)));
    }
    else if (g_strcmp0(method_name, "GetAutoProfiles") == 0) {
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a(ss)"));
        GList *rules = display_auto_get_rules();
        for (GList *l = rules; l; l = l->next) {
            AutoProfileRule *rule = l->data;
            g_variant_builder_add(&builder, "(ss)", rule->edid_match, rule->profile_name);
            display_auto_rule_free(rule);
        }
        g_list_free(rules);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(a(ss))", &builder));
    }
    else if (g_strcmp0(method_name, "ApplyAutoProfiles") == 0) {
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_auto_apply()));
    }
    // ═══════════ Arrangement Methods ═══════════
    else if (g_strcmp0(method_name, "Arrange") == 0) {
        const gchar *name, *relative_to; gint32 pos;
        g_variant_get(parameters, "(&si&s)", &name, &pos, &relative_to);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_arrange(name, (ArrangePosition)pos, relative_to)));
    }
    else if (g_strcmp0(method_name, "AutoArrange") == 0) {
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_auto_arrange()));
    }
    else if (g_strcmp0(method_name, "ExtendRight") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_extend_right(name)));
    }
    else if (g_strcmp0(method_name, "ExtendLeft") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_extend_left(name)));
    }
    // ═══════════ Unknown ═══════════
    else {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD, "Unknown: %s", method_name);
    }
}

void dbus_emit_signal(const gchar *signal_name, GVariant *parameters) {
    if (!display_state.session_conn) return;
    GError *error = NULL;
    g_dbus_connection_emit_signal(display_state.session_conn, NULL, DBUS_OBJECT_PATH, 
        DBUS_INTERFACE_NAME, signal_name, parameters, &error);
    if (error) { g_error_free(error); }
}

const GDBusInterfaceVTable dbus_interface_vtable = {
    .method_call = dbus_handle_method_call,
    .get_property = NULL,
    .set_property = NULL
};
