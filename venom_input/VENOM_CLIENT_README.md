# Venom Input Client Library

مكتبة C للاتصال بـ Venom Input D-Bus Daemon

## الملفات

- `venom_input_client.h` - تعريفات الدوال وأنواع البيانات
- `venom_input_client.c` - تطبيق الدوال وربط D-Bus
- `example_settings_app.c` - مثال على الاستخدام

## الاستخدام في تطبيقك

### 1. إنشاء عميل D-Bus
```c
VenomInputClient *client = venom_input_client_new();
```

### 2. التحقق من الاتصال
```c
if (!client->connection) {
    fprintf(stderr, "خطأ: %s\n", venom_input_client_get_error(client));
    venom_input_client_free(client);
    return 1;
}
```

### 3. استدعاء الدوال
```c
// إضافة لغة
if (venom_input_add_keyboard_layout(client, "fr")) {
    printf("تمت الإضافة\n");
}

// قراءة إعدادات الماوس
double accel, speed;
gboolean natural_scroll, left_handed;
venom_input_get_mouse_settings(client, &accel, &speed, 
                               &natural_scroll, &left_handed);

// تعيين إعدادات التاتش باد
venom_input_set_touchpad_speed(client, 0.7);
```

### 4. تحرير الموارد
```c
venom_input_client_free(client);
```

## الترجمة

```bash
gcc your_app.c venom_input_client.c -o your_app \
    $(pkg-config --cflags --libs gio-2.0 glib-2.0) \
    -Wall -Wextra -O2
```

## الدوال المتاحة

### لوحة المفاتيح
- `venom_input_set_keyboard_layouts()`
- `venom_input_set_keyboard_model()`
- `venom_input_set_keyboard_options()`
- `venom_input_get_keyboard_settings()`
- `venom_input_add_keyboard_layout()`
- `venom_input_remove_keyboard_layout()`
- `venom_input_list_keyboard_layouts()`

### الماوس
- `venom_input_set_mouse_accel()`
- `venom_input_set_mouse_speed()`
- `venom_input_set_mouse_natural_scroll()`
- `venom_input_set_mouse_left_handed()`
- `venom_input_get_mouse_settings()`

### التاتش باد
- `venom_input_set_touchpad_enabled()`
- `venom_input_set_touchpad_tap_to_click()`
- `venom_input_set_touchpad_natural_scroll()`
- `venom_input_set_touchpad_scroll_method()`
- `venom_input_set_touchpad_speed()`
- `venom_input_set_touchpad_disable_while_typing()`
- `venom_input_get_touchpad_settings()`

## معالجة الأخطاء

جميع الدوال ترجع `TRUE` عند النجاح و `FALSE` عند الفشل.

للحصول على رسالة الخطأ:
```c
const char* error = venom_input_client_get_error(client);
if (error) {
    fprintf(stderr, "خطأ: %s\n", error);
}
```
