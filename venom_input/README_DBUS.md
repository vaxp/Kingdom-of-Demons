# واجهة Venom Input D-Bus
## Venom Input Management Daemon - D-Bus Interface

---

## 📋 المعلومات الأساسية

| الخاصية | القيمة |
|--------|--------|
| **Bus Name** | `org.venom.Input` |
| **Object Path** | `/org/venom/Input` |
| **Interface** | `org.venom.Input` |
| **Bus Type** | SESSION (User Session) |
| **Config File** | `/etc/venom/input.json` |
| **Log File** | `/var/log/venom_input.log` |

---

## ⌨️ واجهات لوحة المفاتيح

### `SetKeyboardLayouts(s layouts) → b success`
تعيين اللغات المتعددة

**المعاملات:**
- `layouts` (string): اللغات مفصولة بفواصل، مثل: `"us,ara,fr"`

**العودة:**
- `success` (boolean): `true` إذا نجحت، `false` خلاف ذلك

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.SetKeyboardLayouts string:"us,ara"
```

---

### `SetKeyboardModel(s model) → b success`
تعيين نموذج لوحة المفاتيح

**المعاملات:**
- `model` (string): نموذج المفاتيح (مثل: `pc105`, `pc104`)

**العودة:**
- `success` (boolean): `true` إذا نجحت

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.SetKeyboardModel string:"pc105"
```

---

### `SetKeyboardOptions(s options) → b success`
تعيين خيارات X11 للمفاتيح

**المعاملات:**
- `options` (string): خيارات مفصولة بفواصل (مثل: `"grp:shift_alt_toggle,grp:win_space_toggle"`)

**العودة:**
- `success` (boolean): `true` إذا نجحت

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.SetKeyboardOptions string:"grp:alt_shift_toggle"
```

---

### `GetKeyboardSettings() → (s layouts, s model, s options)`
الحصول على إعدادات لوحة المفاتيح الحالية

**العودة:**
- `layouts` (string): اللغات الحالية
- `model` (string): نموذج المفاتيح
- `options` (string): خيارات X11

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.GetKeyboardSettings
```

**النتيجة:**
```
method return time=1773347480.589530
   string "us,ar"
   string "pc105"
   string "grp:shift_alt_toggle,grp:win_space_toggle"
```

---

### `AddKeyboardLayout(s layout) → b success`
إضافة لغة جديدة للقائمة الحالية

**المعاملات:**
- `layout` (string): رمز اللغة (مثل: `ara`, `fr`, `de`)

**العودة:**
- `success` (boolean): `true` إذا أضيفت، `false` إذا كانت موجودة مسبقاً

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.AddKeyboardLayout string:"fr"
```

---

### `RemoveKeyboardLayout(s layout) → b success`
حذف لغة من القائمة

⚠️ **تحذير:** لا يمكن حذف آخر لغة متبقية

**المعاملات:**
- `layout` (string): رمز اللغة

**العودة:**
- `success` (boolean): `true` إذا حُذفت، `false` إذا كانت الوحيدة أو غير موجودة

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.RemoveKeyboardLayout string:"fr"
```

---

### `ListKeyboardLayouts() → as layouts`
الحصول على قائمة اللغات كمصفوفة

**العودة:**
- `layouts` (array of strings): قائمة اللغات المفعّلة

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.ListKeyboardLayouts
```

**النتيجة:**
```
array [
   string "us"
   string "ar"
   string "fr"
]
```

---

## 🖱️ واجهات الماوس

### `SetMouseAccel(d accel) → b success`
تعيين تسريع الماوس

**المعاملات:**
- `accel` (double): التسريع من -1.0 إلى 1.0

**العودة:**
- `success` (boolean): `true` إذا نجحت

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.SetMouseAccel double:0.5
```

---

### `SetMouseSpeed(d speed) → b success`
تعيين سرعة الماوس

**المعاملات:**
- `speed` (double): السرعة من 0.0 إلى 2.0

**العودة:**
- `success` (boolean): `true` إذا نجحت

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.SetMouseSpeed double:1.5
```

---

### `SetMouseNaturalScroll(b enabled) → b success`
تفعيل/تعطيل التمرير الطبيعي (Natural Scrolling)

**المعاملات:**
- `enabled` (boolean): `true` للتفعيل، `false` للتعطيل

**العودة:**
- `success` (boolean): `true` إذا نجحت

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.SetMouseNaturalScroll boolean:true
```

---

### `SetMouseLeftHanded(b enabled) → b success`
تعيين وضع اليد اليسرى

**المعاملات:**
- `enabled` (boolean): `true` لليسار، `false` لليمين

**العودة:**
- `success` (boolean): `true` إذا نجحت

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.SetMouseLeftHanded boolean:true
```

---

### `GetMouseSettings() → (d accel, d speed, b natural_scroll, b left_handed)`
الحصول على جميع إعدادات الماوس

**العودة:**
- `accel` (double): التسريع الحالي
- `speed` (double): السرعة الحالية
- `natural_scroll` (boolean): حالة التمرير الطبيعي
- `left_handed` (boolean): حالة وضع اليد اليسرى

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.GetMouseSettings
```

**النتيجة:**
```
method return time=1773347480.589530
   double 0.5
   double 1.5
   boolean false
   boolean false
```

---

## 🖱️ واجهات التاتش باد (Touchpad)

### `SetTouchpadEnabled(b enabled) → b success`
تفعيل/تعطيل التاتش باد

**المعاملات:**
- `enabled` (boolean): `true` للتفعيل، `false` للتعطيل

**العودة:**
- `success` (boolean): `true` إذا نجحت

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.SetTouchpadEnabled boolean:true
```

---

### `SetTouchpadTapToClick(b enabled) → b success`
تفعيل/تعطيل الضغط الخفيف للنقر

**المعاملات:**
- `enabled` (boolean): `true` للتفعيل

**العودة:**
- `success` (boolean): `true` إذا نجحت

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.SetTouchpadTapToClick boolean:true
```

---

### `SetTouchpadNaturalScroll(b enabled) → b success`
تفعيل/تعطيل التمرير الطبيعي للتاتش باد

**المعاملات:**
- `enabled` (boolean): `true` للتفعيل

**العودة:**
- `success` (boolean): `true` إذا نجحت

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.SetTouchpadNaturalScroll boolean:true
```

---

### `SetTouchpadScrollMethod(s method) → b success`
تعيين طريقة التمرير

**المعاملات:**
- `method` (string): أحد القيم:
  - `"two-finger"` - التمرير بإصبعين
  - `"edge"` - التمرير من الحافة
  - `"none"` - بدون تمرير

**العودة:**
- `success` (boolean): `true` إذا نجحت

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.SetTouchpadScrollMethod string:"two-finger"
```

---

### `SetTouchpadSpeed(d speed) → b success`
تعيين سرعة التاتش باد

**المعاملات:**
- `speed` (double): السرعة من 0.0 إلى 1.0

**العودة:**
- `success` (boolean): `true` إذا نجحت

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.SetTouchpadSpeed double:0.7
```

---

### `SetTouchpadDisableWhileTyping(b enabled) → b success`
تفعيل/تعطيل خاصية تعطيل التاتش باد أثناء الكتابة

**المعاملات:**
- `enabled` (boolean): `true` للتفعيل

**العودة:**
- `success` (boolean): `true` إذا نجحت

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.SetTouchpadDisableWhileTyping boolean:true
```

---

### `GetTouchpadSettings() → (b enabled, b tap_to_click, b natural_scroll, s scroll_method, d speed, b disable_while_typing)`
الحصول على جميع إعدادات التاتش باد

**العودة:**
- `enabled` (boolean): التاتش باد فعّال
- `tap_to_click` (boolean): الضغط الخفيف فعّال
- `natural_scroll` (boolean): التمرير الطبيعي فعّال
- `scroll_method` (string): طريقة التمرير
- `speed` (double): السرعة
- `disable_while_typing` (boolean): التعطيل أثناء الكتابة فعّال

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.GetTouchpadSettings
```

**النتيجة:**
```
method return time=1773347480.589530
   boolean true
   boolean true
   boolean true
   string "two-finger"
   double 0.5
   boolean true
```

---

## 🔧 واجهات عامة

### `ReloadConfig() → b success`
إعادة تحميل الإعدادات من ملف JSON

**العودة:**
- `success` (boolean): `true` إذا نجحت

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.ReloadConfig
```

---

### `GetAllSettings() → s json`
الحصول على جميع الإعدادات كـ JSON واحد

**العودة:**
- `json` (string): كائن JSON يحتوي على جميع الإعدادات

**مثال:**
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.GetAllSettings
```

**النتيجة:**
```json
{
  "keyboard": {
    "layouts": "us,ar",
    "model": "pc105",
    "options": "grp:shift_alt_toggle"
  },
  "mouse": {
    "accel": 0.5,
    "speed": 1.5,
    "natural_scroll": 0,
    "left_handed": 0
  },
  "touchpad": {
    "enabled": 1,
    "tap_to_click": 1,
    "natural_scroll": 1,
    "scroll_method": "two-finger",
    "speed": 0.5,
    "disable_while_typing": 1
  }
}
```

---

## 📝 نموذج ملف الإعدادات `/etc/venom/input.json`

```json
{
  "keyboard": {
    "layouts": "us,ar",
    "model": "pc105",
    "options": "grp:shift_alt_toggle,grp:win_space_toggle"
  },
  "mouse": {
    "accel": 0.0,
    "speed": 1.0,
    "natural_scroll": 0,
    "left_handed": 0
  },
  "touchpad": {
    "enabled": 1,
    "tap_to_click": 1,
    "natural_scroll": 1,
    "two_finger_scroll": 1,
    "scroll_method": "two-finger",
    "speed": 0.5,
    "disable_while_typing": 1
  }
}
```

---

## 💻 أمثلة من لغات البرمجة المختلفة

### Python (Using dbus-python)

```python
import dbus

# الاتصال بـ Session Bus
bus = dbus.SessionBus()

# الحصول على الكائن
obj = bus.get_object('org.venom.Input', '/org/venom/Input')
interface = dbus.Interface(obj, 'org.venom.Input')

# الحصول على إعدادات اللغات
layouts, model, options = interface.GetKeyboardSettings()
print(f"Layouts: {layouts}")

# إضافة لغة
success = interface.AddKeyboardLayout("fr")
print(f"Added French: {success}")

# الحصول على جميع اللغات
languages = interface.ListKeyboardLayouts()
print(f"Available languages: {languages}")

# تعيين التاتش باد
interface.SetTouchpadSpeed(0.7)
```

### C (Using GDBus)

```c
#include <gio/gio.h>

void get_keyboard_settings() {
    GError *error = NULL;
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    
    GVariant *result = g_dbus_connection_call_sync(
        conn,
        "org.venom.Input",
        "/org/venom/Input",
        "org.venom.Input",
        "GetKeyboardSettings",
        NULL,
        G_VARIANT_TYPE("(sss)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1, NULL, &error);
    
    if (result) {
        gchar *layouts, *model, *options;
        g_variant_get(result, "(sss)", &layouts, &model, &options);
        g_print("Layouts: %s\n", layouts);
        g_variant_unref(result);
    }
}
```

### JavaScript (Node.js with dbus)

```javascript
const dbus = require('dbus');

const systemBus = dbus.sessionBus();

const service = systemBus.getService('org.venom.Input');
const object = service.getObject('/org/venom/Input');

// الاستدعاء المتزامن
object.org.venom.Input.GetKeyboardSettings(function(error, result) {
    if (error) return console.error(error);
    console.log('Layouts:', result[0]);
    console.log('Model:', result[1]);
    console.log('Options:', result[2]);
});

// إضافة لغة
object.org.venom.Input.AddKeyboardLayout('fr', function(error, result) {
    console.log('Added French:', result);
});
```

### Shell Script (Command Line)

```bash
#!/bin/bash

# الحصول على إعدادات اللغة
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.GetKeyboardSettings | grep "string" | sed 's/.*string "\([^"]*\)".*/\1/'

# إضافة لغة فرنسية
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.AddKeyboardLayout string:"fr"

# عرض جميع اللغات المتاحة
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.ListKeyboardLayouts | grep "string"
```

---

## ⚙️ متطلبات التشغيل

```bash
# تثبيت المتطلبات
sudo apt install libglib2.0-0 libgio-2.0-0 dbus xinput setxkbmap

# ترجمة البرنامج
gcc venom_input.c -o venom_input $(pkg-config --cflags --libs gio-2.0 glib-2.0) -Wall -Wextra -O2

# تشغيل الـ daemon
./venom_input &
```

---

## 🔐 الأمان

✅ **حماية ضد Command Injection:**
- جميع المدخلات يتم التحقق منها قبل التطبيق
- فقط الأحرف الآمنة مسموحة: حروف، أرقام، فواصل، شرطات، نقاط

✅ **معالجة الأخطاء:**
- الدوال ترجع `false` في حالة الفشل
- السجلات مفصلة في `/var/log/venom_input.log`

✅ **الحماية من التفريغ الكامل:**
- لا يمكن حذف آخر لغة متبقية
- النظام يحافظ على لغة واحدة على الأقل

---

## 📊 رموز الخروج والأخطاء

| القيمة | المعنى |
|--------|--------|
| `true` (1) | العملية نجحت |
| `false` (0) | العملية فشلت |
| D-Bus Error | اتصال غير صحيح أو service غير متاح |

---

## 🐛 استكشاف الأخطاء

### التحقق من تشغيل الـ daemon:
```bash
ps aux | grep venom_input
```

### عرض السجلات:
```bash
tail -f /var/log/venom_input.log
```

### قائمة D-Bus services:
```bash
dbus-send --session --print-reply \
  --dest=org.freedesktop.DBus /org/freedesktop/DBus \
  org.freedesktop.DBus.ListActivatableNames | grep venom
```

### اختبار الاتصال:
```bash
dbus-send --session --print-reply \
  --dest=org.venom.Input /org/venom/Input \
  org.venom.Input.ListKeyboardLayouts
```

---

## 📄 الترخيص والمساهمة

لـ Venom OS - Input Management System

تم التطوير بـ: GLib, GIO, D-Bus
