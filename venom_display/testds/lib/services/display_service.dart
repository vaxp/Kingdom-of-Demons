import 'package:dbus/dbus.dart';

/// Venom Display D-Bus Service
class DisplayService {
  static const _serviceName = 'org.venom.Display';
  static const _objectPath = '/org/venom/Display';
  static const _interface = 'org.venom.Display';

  final DBusClient _client = DBusClient.session();
  late final DBusRemoteObject _display;

  DisplayService() {
    _display = DBusRemoteObject(
      _client,
      name: _serviceName,
      path: DBusObjectPath(_objectPath),
    );
  }

  void dispose() => _client.close();

  // ═══════════════════════════════════════════════════════════════════════════
  // معلومات الشاشات
  // ═══════════════════════════════════════════════════════════════════════════

  Future<List<DisplayInfo>> getDisplays() async {
    final result = await _call('GetDisplays', [], 'a(siidbbiix)');
    return (result[0] as DBusArray).children.map((s) {
      final struct = s as DBusStruct;
      return DisplayInfo(
        name: _str(struct, 0),
        width: _int(struct, 1),
        height: _int(struct, 2),
        rate: _double(struct, 3),
        connected: _bool(struct, 4),
        primary: _bool(struct, 5),
        x: _int(struct, 6),
        y: _int(struct, 7),
        outputId: _int64(struct, 8),
      );
    }).toList();
  }

  Future<DisplayInfo?> getPrimaryDisplay() async {
    try {
      final result = await _call('GetPrimaryDisplay', [], '(siidbbiix)');
      final struct = result[0] as DBusStruct;
      return DisplayInfo(
        name: _str(struct, 0),
        width: _int(struct, 1),
        height: _int(struct, 2),
        rate: _double(struct, 3),
        connected: _bool(struct, 4),
        primary: _bool(struct, 5),
        x: _int(struct, 6),
        y: _int(struct, 7),
        outputId: _int64(struct, 8),
      );
    } catch (_) {
      return null;
    }
  }

  Future<DisplayInfo?> getDisplayInfo(String name) async {
    try {
      final result = await _call('GetDisplayInfo', [
        DBusString(name),
      ], '(siidbbiix)');
      final struct = result[0] as DBusStruct;
      return DisplayInfo(
        name: _str(struct, 0),
        width: _int(struct, 1),
        height: _int(struct, 2),
        rate: _double(struct, 3),
        connected: _bool(struct, 4),
        primary: _bool(struct, 5),
        x: _int(struct, 6),
        y: _int(struct, 7),
        outputId: _int64(struct, 8),
      );
    } catch (_) {
      return null;
    }
  }

  Future<List<DisplayMode>> getModes(String name) async {
    final result = await _call('GetModes', [DBusString(name)], 'a(iid)');
    return (result[0] as DBusArray).children.map((s) {
      final struct = s as DBusStruct;
      return DisplayMode(
        width: _int(struct, 0),
        height: _int(struct, 1),
        rate: _double(struct, 2),
      );
    }).toList();
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // تغيير الإعدادات
  // ═══════════════════════════════════════════════════════════════════════════

  Future<bool> setResolution(String name, int width, int height) async {
    return _callBool('SetResolution', [
      DBusString(name),
      DBusInt32(width),
      DBusInt32(height),
    ]);
  }

  Future<bool> setRefreshRate(String name, double rate) async {
    return _callBool('SetRefreshRate', [DBusString(name), DBusDouble(rate)]);
  }

  Future<bool> setMode(String name, int width, int height, double rate) async {
    return _callBool('SetMode', [
      DBusString(name),
      DBusInt32(width),
      DBusInt32(height),
      DBusDouble(rate),
    ]);
  }

  Future<bool> setPosition(String name, int x, int y) async {
    return _callBool('SetPosition', [
      DBusString(name),
      DBusInt32(x),
      DBusInt32(y),
    ]);
  }

  Future<bool> setPrimary(String name) async {
    return _callBool('SetPrimary', [DBusString(name)]);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // التدوير
  // ═══════════════════════════════════════════════════════════════════════════

  Future<int> getRotation(String name) async {
    final result = await _call('GetRotation', [DBusString(name)], 'i');
    return (result[0] as DBusInt32).value;
  }

  Future<bool> setRotation(String name, int rotation) async {
    return _callBool('SetRotation', [DBusString(name), DBusInt32(rotation)]);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // تشغيل/إيقاف
  // ═══════════════════════════════════════════════════════════════════════════

  Future<bool> enableOutput(String name) async {
    return _callBool('EnableOutput', [DBusString(name)]);
  }

  Future<bool> disableOutput(String name) async {
    return _callBool('DisableOutput', [DBusString(name)]);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // المطابقة
  // ═══════════════════════════════════════════════════════════════════════════

  Future<bool> setMirror(String source, String target) async {
    return _callBool('SetMirror', [DBusString(source), DBusString(target)]);
  }

  Future<bool> disableMirror(String name) async {
    return _callBool('DisableMirror', [DBusString(name)]);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // التكبير
  // ═══════════════════════════════════════════════════════════════════════════

  Future<double> getScale(String name) async {
    final result = await _call('GetScale', [DBusString(name)], 'd');
    return (result[0] as DBusDouble).value;
  }

  Future<bool> setScale(String name, double scale) async {
    return _callBool('SetScale', [DBusString(name), DBusDouble(scale)]);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // Night Light
  // ═══════════════════════════════════════════════════════════════════════════

  Future<NightLightSettings> getNightLight() async {
    final result = await _call('GetNightLight', [], '(bi)');
    final struct = result[0] as DBusStruct;
    return NightLightSettings(
      enabled: _bool(struct, 0),
      temperature: _int(struct, 1),
    );
  }

  Future<bool> setNightLight(bool enabled, int temperature) async {
    return _callBool('SetNightLight', [
      DBusBoolean(enabled),
      DBusInt32(temperature),
    ]);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // البروفايلات
  // ═══════════════════════════════════════════════════════════════════════════

  Future<bool> saveProfile(String name) async {
    return _callBool('SaveProfile', [DBusString(name)]);
  }

  Future<bool> loadProfile(String name) async {
    return _callBool('LoadProfile', [DBusString(name)]);
  }

  Future<List<String>> getProfiles() async {
    final result = await _call('GetProfiles', [], 'as');
    return (result[0] as DBusArray).children
        .map((s) => (s as DBusString).value)
        .toList();
  }

  Future<bool> deleteProfile(String name) async {
    return _callBool('DeleteProfile', [DBusString(name)]);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // DPMS
  // ═══════════════════════════════════════════════════════════════════════════

  Future<int> getDPMSMode() async {
    final result = await _call('GetDPMSMode', [], 'i');
    return (result[0] as DBusInt32).value;
  }

  Future<bool> setDPMSMode(int mode) async {
    return _callBool('SetDPMSMode', [DBusInt32(mode)]);
  }

  Future<DPMSTimeouts> getDPMSTimeouts() async {
    final result = await _call('GetDPMSTimeouts', [], '(iii)');
    final struct = result[0] as DBusStruct;
    return DPMSTimeouts(
      standby: _int(struct, 0),
      suspend: _int(struct, 1),
      off: _int(struct, 2),
    );
  }

  Future<bool> setDPMSTimeouts(int standby, int suspend, int off) async {
    return _callBool('SetDPMSTimeouts', [
      DBusInt32(standby),
      DBusInt32(suspend),
      DBusInt32(off),
    ]);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // Backlight
  // ═══════════════════════════════════════════════════════════════════════════

  Future<int> getBacklight() async {
    final result = await _call('GetBacklight', [], 'i');
    return (result[0] as DBusInt32).value;
  }

  Future<bool> setBacklight(int level) async {
    return _callBool('SetBacklight', [DBusInt32(level)]);
  }

  Future<int> getMaxBacklight() async {
    final result = await _call('GetMaxBacklight', [], 'i');
    return (result[0] as DBusInt32).value;
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // EDID
  // ═══════════════════════════════════════════════════════════════════════════

  Future<EDIDInfo?> getEDID(String name) async {
    try {
      final result = await _call('GetEDID', [DBusString(name)], '(sssuuiibb)');
      final struct = result[0] as DBusStruct;
      return EDIDInfo(
        manufacturer: _str(struct, 0),
        model: _str(struct, 1),
        serial: _str(struct, 2),
        productCode: _uint(struct, 3),
        serialNumber: _uint(struct, 4),
        maxWidth: _int(struct, 5),
        maxHeight: _int(struct, 6),
        hdrSupported: _bool(struct, 7),
        vrrSupported: _bool(struct, 8),
      );
    } catch (_) {
      return null;
    }
  }

  Future<String?> getEDIDHash(String name) async {
    try {
      final result = await _call('GetEDIDHash', [DBusString(name)], 's');
      return (result[0] as DBusString).value;
    } catch (_) {
      return null;
    }
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // DDC/CI
  // ═══════════════════════════════════════════════════════════════════════════

  Future<int> getHWBrightness(String name) async {
    final result = await _call('GetHWBrightness', [DBusString(name)], 'i');
    return (result[0] as DBusInt32).value;
  }

  Future<bool> setHWBrightness(String name, int level) async {
    return _callBool('SetHWBrightness', [DBusString(name), DBusInt32(level)]);
  }

  Future<int> getHWContrast(String name) async {
    final result = await _call('GetHWContrast', [DBusString(name)], 'i');
    return (result[0] as DBusInt32).value;
  }

  Future<bool> setHWContrast(String name, int level) async {
    return _callBool('SetHWContrast', [DBusString(name), DBusInt32(level)]);
  }

  Future<bool> powerOffMonitor(String name) async {
    return _callBool('PowerOffMonitor', [DBusString(name)]);
  }

  Future<bool> powerOnMonitor(String name) async {
    return _callBool('PowerOnMonitor', [DBusString(name)]);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // VRR
  // ═══════════════════════════════════════════════════════════════════════════

  Future<bool> isVRRCapable(String name) async {
    final result = await _call('IsVRRCapable', [DBusString(name)], 'b');
    return (result[0] as DBusBoolean).value;
  }

  Future<bool> isVRREnabled(String name) async {
    final result = await _call('IsVRREnabled', [DBusString(name)], 'b');
    return (result[0] as DBusBoolean).value;
  }

  Future<bool> setVRR(String name, bool enable) async {
    return _callBool('SetVRR', [DBusString(name), DBusBoolean(enable)]);
  }

  Future<VRRInfo> getVRRInfo(String name) async {
    final result = await _call('GetVRRInfo', [DBusString(name)], '(bbii)');
    final struct = result[0] as DBusStruct;
    return VRRInfo(
      capable: _bool(struct, 0),
      enabled: _bool(struct, 1),
      minRate: _int(struct, 2),
      maxRate: _int(struct, 3),
    );
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // Auto Profile
  // ═══════════════════════════════════════════════════════════════════════════

  Future<bool> setAutoProfile(String edidMatch, String profile) async {
    return _callBool('SetAutoProfile', [
      DBusString(edidMatch),
      DBusString(profile),
    ]);
  }

  Future<bool> removeAutoProfile(String edidMatch) async {
    return _callBool('RemoveAutoProfile', [DBusString(edidMatch)]);
  }

  Future<List<AutoProfileRule>> getAutoProfiles() async {
    final result = await _call('GetAutoProfiles', [], 'a(ss)');
    return (result[0] as DBusArray).children.map((s) {
      final struct = s as DBusStruct;
      return AutoProfileRule(
        edidMatch: _str(struct, 0),
        profileName: _str(struct, 1),
      );
    }).toList();
  }

  Future<bool> applyAutoProfiles() async {
    return _callBool('ApplyAutoProfiles', []);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // Arrangement
  // ═══════════════════════════════════════════════════════════════════════════

  Future<bool> arrange(String name, int position, String relativeTo) async {
    return _callBool('Arrange', [
      DBusString(name),
      DBusInt32(position),
      DBusString(relativeTo),
    ]);
  }

  Future<bool> autoArrange() async {
    return _callBool('AutoArrange', []);
  }

  Future<bool> extendRight(String name) async {
    return _callBool('ExtendRight', [DBusString(name)]);
  }

  Future<bool> extendLeft(String name) async {
    return _callBool('ExtendLeft', [DBusString(name)]);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // Helpers
  // ═══════════════════════════════════════════════════════════════════════════

  Future<List<DBusValue>> _call(
    String method,
    List<DBusValue> args,
    String sig,
  ) async {
    final result = await _display.callMethod(
      _interface,
      method,
      args,
      replySignature: DBusSignature(sig),
    );
    return result.values;
  }

  Future<bool> _callBool(String method, List<DBusValue> args) async {
    final result = await _call(method, args, 'b');
    return (result[0] as DBusBoolean).value;
  }

  String _str(DBusStruct s, int i) => (s.children[i] as DBusString).value;
  int _int(DBusStruct s, int i) => (s.children[i] as DBusInt32).value;
  int _uint(DBusStruct s, int i) => (s.children[i] as DBusUint32).value;
  int _int64(DBusStruct s, int i) => (s.children[i] as DBusInt64).value;
  double _double(DBusStruct s, int i) => (s.children[i] as DBusDouble).value;
  bool _bool(DBusStruct s, int i) => (s.children[i] as DBusBoolean).value;
}

// ═══════════════════════════════════════════════════════════════════════════
// Models
// ═══════════════════════════════════════════════════════════════════════════

class DisplayInfo {
  final String name;
  final int width, height, x, y;
  final double rate;
  final bool connected, primary;
  final int outputId;

  DisplayInfo({
    required this.name,
    required this.width,
    required this.height,
    required this.rate,
    required this.connected,
    required this.primary,
    required this.x,
    required this.y,
    required this.outputId,
  });
}

class DisplayMode {
  final int width, height;
  final double rate;
  DisplayMode({required this.width, required this.height, required this.rate});
}

class NightLightSettings {
  final bool enabled;
  final int temperature;
  NightLightSettings({required this.enabled, required this.temperature});
}

class DPMSTimeouts {
  final int standby, suspend, off;
  DPMSTimeouts({
    required this.standby,
    required this.suspend,
    required this.off,
  });
}

class EDIDInfo {
  final String manufacturer, model, serial;
  final int productCode, serialNumber, maxWidth, maxHeight;
  final bool hdrSupported, vrrSupported;

  EDIDInfo({
    required this.manufacturer,
    required this.model,
    required this.serial,
    required this.productCode,
    required this.serialNumber,
    required this.maxWidth,
    required this.maxHeight,
    required this.hdrSupported,
    required this.vrrSupported,
  });
}

class VRRInfo {
  final bool capable, enabled;
  final int minRate, maxRate;
  VRRInfo({
    required this.capable,
    required this.enabled,
    required this.minRate,
    required this.maxRate,
  });
}

class AutoProfileRule {
  final String edidMatch, profileName;
  AutoProfileRule({required this.edidMatch, required this.profileName});
}
