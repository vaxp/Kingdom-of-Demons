import 'package:dbus/dbus.dart';

// ═══════════════════════════════════════════════════════════════════════════
// 📶 WiFi Models
// ═══════════════════════════════════════════════════════════════════════════

class WiFiNetwork {
  final String ssid;
  final String bssid;
  final int strength;
  final int frequency;
  final bool secured;
  final bool connected;

  WiFiNetwork({
    required this.ssid,
    required this.bssid,
    required this.strength,
    required this.frequency,
    required this.secured,
    required this.connected,
  });

  factory WiFiNetwork.fromDBus(DBusStruct s) {
    final v = s.children.toList();
    return WiFiNetwork(
      ssid: (v[0] as DBusString).value,
      bssid: (v[1] as DBusString).value,
      strength: (v[2] as DBusInt32).value,
      frequency: (v[3] as DBusInt32).value,
      secured: (v[4] as DBusBoolean).value,
      connected: (v[5] as DBusBoolean).value,
    );
  }

  String get band => frequency > 5000 ? '5GHz' : '2.4GHz';
}

class WiFiStatus {
  final bool connected;
  final String ssid;
  final String ipAddress;
  final String gateway;
  final String subnet;
  final String dns;
  final int strength;
  final int speed;

  WiFiStatus({
    required this.connected,
    required this.ssid,
    required this.ipAddress,
    required this.gateway,
    required this.subnet,
    required this.dns,
    required this.strength,
    required this.speed,
  });

  factory WiFiStatus.fromDBus(DBusStruct s) {
    final v = s.children.toList();
    return WiFiStatus(
      connected: (v[0] as DBusBoolean).value,
      ssid: (v[1] as DBusString).value,
      ipAddress: (v[2] as DBusString).value,
      gateway: (v[3] as DBusString).value,
      subnet: (v[4] as DBusString).value,
      dns: (v[5] as DBusString).value,
      strength: (v[6] as DBusInt32).value,
      speed: (v[7] as DBusInt32).value,
    );
  }
}

class ConnectionDetails {
  final String ssid;
  final String ipAddress;
  final String gateway;
  final String subnet;
  final String dns;
  final bool autoConnect;
  final bool isDhcp;

  ConnectionDetails({
    required this.ssid,
    required this.ipAddress,
    required this.gateway,
    required this.subnet,
    required this.dns,
    required this.autoConnect,
    required this.isDhcp,
  });

  factory ConnectionDetails.fromDBus(DBusStruct s) {
    final v = s.children.toList();
    return ConnectionDetails(
      ssid: (v[0] as DBusString).value,
      ipAddress: (v[1] as DBusString).value,
      gateway: (v[2] as DBusString).value,
      subnet: (v[3] as DBusString).value,
      dns: (v[4] as DBusString).value,
      autoConnect: (v[5] as DBusBoolean).value,
      isDhcp: (v[6] as DBusBoolean).value,
    );
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// 📱 Bluetooth Models (Enhanced)
// ═══════════════════════════════════════════════════════════════════════════

class BluetoothDevice {
  final String address;
  final String name;
  final String icon;
  final bool paired;
  final bool connected;
  final bool trusted;

  BluetoothDevice({
    required this.address,
    required this.name,
    required this.icon,
    required this.paired,
    required this.connected,
    required this.trusted,
  });

  factory BluetoothDevice.fromDBus(DBusStruct s) {
    final v = s.children.toList();
    return BluetoothDevice(
      address: (v[0] as DBusString).value,
      name: (v[1] as DBusString).value,
      icon: (v[2] as DBusString).value,
      paired: (v[3] as DBusBoolean).value,
      connected: (v[4] as DBusBoolean).value,
      trusted: (v[5] as DBusBoolean).value,
    );
  }
}

class BluetoothStatus {
  final bool powered;
  final bool discovering;
  final bool pairable;
  final String name;
  final String address;

  BluetoothStatus({
    required this.powered,
    required this.discovering,
    required this.pairable,
    required this.name,
    required this.address,
  });

  factory BluetoothStatus.fromDBus(DBusStruct s) {
    final v = s.children.toList();
    return BluetoothStatus(
      powered: (v[0] as DBusBoolean).value,
      discovering: (v[1] as DBusBoolean).value,
      pairable: (v[2] as DBusBoolean).value,
      name: (v[3] as DBusString).value,
      address: (v[4] as DBusString).value,
    );
  }
}

/// بطارية جهاز Bluetooth
class BluetoothBattery {
  final int percentage; // 0-100, -1 إذا غير متاح
  final bool available;

  BluetoothBattery({required this.percentage, required this.available});

  factory BluetoothBattery.fromDBus(DBusStruct s) {
    final v = s.children.toList();
    return BluetoothBattery(
      percentage: (v[0] as DBusInt32).value,
      available: (v[1] as DBusBoolean).value,
    );
  }
}

/// بروفايلات Bluetooth المدعومة
class BluetoothProfiles {
  final bool a2dpSink; // سماعات/مكبرات صوت
  final bool a2dpSource; // مصدر صوت
  final bool hfpHf; // Handsfree
  final bool hfpAg; // Audio Gateway
  final bool hid; // لوحة مفاتيح/ماوس

  BluetoothProfiles({
    required this.a2dpSink,
    required this.a2dpSource,
    required this.hfpHf,
    required this.hfpAg,
    required this.hid,
  });

  factory BluetoothProfiles.fromDBus(DBusStruct s) {
    final v = s.children.toList();
    return BluetoothProfiles(
      a2dpSink: (v[0] as DBusBoolean).value,
      a2dpSource: (v[1] as DBusBoolean).value,
      hfpHf: (v[2] as DBusBoolean).value,
      hfpAg: (v[3] as DBusBoolean).value,
      hid: (v[4] as DBusBoolean).value,
    );
  }

  bool get hasAudio => a2dpSink || a2dpSource || hfpHf || hfpAg;
  bool get isInputDevice => hid;

  String get description {
    final parts = <String>[];
    if (a2dpSink) parts.add('🎧 Audio');
    if (hfpHf || hfpAg) parts.add('📞 Handsfree');
    if (hid) parts.add('⌨️ Input');
    return parts.isEmpty ? 'Unknown' : parts.join(' • ');
  }
}

/// نتيجة عملية Bluetooth مع رسالة خطأ
class BluetoothResult {
  final bool success;
  final String message;

  BluetoothResult({required this.success, required this.message});

  factory BluetoothResult.fromDBus(DBusStruct s) {
    final v = s.children.toList();
    return BluetoothResult(
      success: (v[0] as DBusBoolean).value,
      message: (v[1] as DBusString).value,
    );
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// 🔌 Ethernet Models
// ═══════════════════════════════════════════════════════════════════════════

class EthernetInterface {
  final String name;
  final String macAddress;
  final String ipAddress;
  final String gateway;
  final int speed;
  final bool connected;
  final bool enabled;

  EthernetInterface({
    required this.name,
    required this.macAddress,
    required this.ipAddress,
    required this.gateway,
    required this.speed,
    required this.connected,
    required this.enabled,
  });

  factory EthernetInterface.fromDBus(DBusStruct s) {
    final v = s.children.toList();
    return EthernetInterface(
      name: (v[0] as DBusString).value,
      macAddress: (v[1] as DBusString).value,
      ipAddress: (v[2] as DBusString).value,
      gateway: (v[3] as DBusString).value,
      speed: (v[4] as DBusInt32).value,
      connected: (v[5] as DBusBoolean).value,
      enabled: (v[6] as DBusBoolean).value,
    );
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// 🌐 Network Service
// ═══════════════════════════════════════════════════════════════════════════

class NetworkService {
  static const serviceName = 'org.venom.Network';

  late DBusClient _client;
  late DBusRemoteObject _wifi;
  late DBusRemoteObject _bluetooth;
  late DBusRemoteObject _ethernet;
  bool _connected = false;

  bool get isConnected => _connected;

  Future<bool> connect() async {
    try {
      _client = DBusClient.session();
      _wifi = DBusRemoteObject(
        _client,
        name: serviceName,
        path: DBusObjectPath('/org/venom/Network/WiFi'),
      );
      _bluetooth = DBusRemoteObject(
        _client,
        name: serviceName,
        path: DBusObjectPath('/org/venom/Network/Bluetooth'),
      );
      _ethernet = DBusRemoteObject(
        _client,
        name: serviceName,
        path: DBusObjectPath('/org/venom/Network/Ethernet'),
      );
      _connected = true;
      return true;
    } catch (e) {
      return false;
    }
  }

  Future<void> disconnect() async {
    if (_connected) {
      await _client.close();
      _connected = false;
    }
  }

  // ════════════════════════════════════════════════════════════════════════
  // 📶 WiFi
  // ════════════════════════════════════════════════════════════════════════

  Future<WiFiStatus> getWifiStatus() async {
    try {
      final r = await _wifi.callMethod(
        'org.venom.Network.WiFi',
        'GetStatus',
        [],
      );
      return WiFiStatus.fromDBus(r.values.first as DBusStruct);
    } catch (e) {
      return WiFiStatus(
        connected: false,
        ssid: '',
        ipAddress: '',
        gateway: '',
        subnet: '',
        dns: '',
        strength: 0,
        speed: 0,
      );
    }
  }

  Future<List<WiFiNetwork>> getWifiNetworks() async {
    try {
      final r = await _wifi.callMethod(
        'org.venom.Network.WiFi',
        'GetNetworks',
        [],
      );
      return (r.values.first as DBusArray).children
          .map((v) => WiFiNetwork.fromDBus(v as DBusStruct))
          .toList();
    } catch (e) {
      return [];
    }
  }

  Future<List<String>> getSavedNetworks() async {
    try {
      final r = await _wifi.callMethod(
        'org.venom.Network.WiFi',
        'GetSavedNetworks',
        [],
      );
      return (r.values.first as DBusArray).children
          .map((v) => (v as DBusString).value)
          .toList();
    } catch (e) {
      return [];
    }
  }

  Future<bool> isWifiEnabled() async {
    try {
      final r = await _wifi.callMethod(
        'org.venom.Network.WiFi',
        'IsEnabled',
        [],
      );
      return (r.values.first as DBusBoolean).value;
    } catch (e) {
      return false;
    }
  }

  Future<bool> setWifiEnabled(bool enabled) => _callBool(
    _wifi,
    'org.venom.Network.WiFi',
    'SetEnabled',
    [DBusBoolean(enabled)],
  );
  Future<bool> wifiConnect(String ssid, String password) => _callBool(
    _wifi,
    'org.venom.Network.WiFi',
    'Connect',
    [DBusString(ssid), DBusString(password)],
  );
  Future<bool> wifiDisconnect() =>
      _callBool(_wifi, 'org.venom.Network.WiFi', 'Disconnect', []);
  Future<bool> forgetNetwork(String ssid) => _callBool(
    _wifi,
    'org.venom.Network.WiFi',
    'ForgetNetwork',
    [DBusString(ssid)],
  );

  // ════════════════════════════════════════════════════════════════════════
  // 📶 WiFi - ميزات متقدمة
  // ════════════════════════════════════════════════════════════════════════

  Future<ConnectionDetails> getConnectionDetails(String ssid) async {
    try {
      final r = await _wifi.callMethod(
        'org.venom.Network.WiFi',
        'GetConnectionDetails',
        [DBusString(ssid)],
      );
      return ConnectionDetails.fromDBus(r.values.first as DBusStruct);
    } catch (e) {
      return ConnectionDetails(
        ssid: ssid,
        ipAddress: '',
        gateway: '',
        subnet: '',
        dns: '',
        autoConnect: true,
        isDhcp: true,
      );
    }
  }

  Future<bool> setStaticIP(
    String ssid,
    String ip,
    String gateway,
    String subnet,
    String dns,
  ) => _callBool(_wifi, 'org.venom.Network.WiFi', 'SetStaticIP', [
    DBusString(ssid),
    DBusString(ip),
    DBusString(gateway),
    DBusString(subnet),
    DBusString(dns),
  ]);

  Future<bool> setDHCP(String ssid) =>
      _callBool(_wifi, 'org.venom.Network.WiFi', 'SetDHCP', [DBusString(ssid)]);

  Future<bool> setDNS(String ssid, String dns1, String dns2) => _callBool(
    _wifi,
    'org.venom.Network.WiFi',
    'SetDNS',
    [DBusString(ssid), DBusString(dns1), DBusString(dns2)],
  );

  Future<bool> setAutoConnect(String ssid, bool autoConnect) => _callBool(
    _wifi,
    'org.venom.Network.WiFi',
    'SetAutoConnect',
    [DBusString(ssid), DBusBoolean(autoConnect)],
  );

  // ════════════════════════════════════════════════════════════════════════
  // 📱 Bluetooth - Basic
  // ════════════════════════════════════════════════════════════════════════

  /// التحقق من توفر BlueZ
  Future<bool> isBluetoothAvailable() async {
    try {
      final r = await _bluetooth.callMethod(
        'org.venom.Network.Bluetooth',
        'IsAvailable',
        [],
      );
      return (r.values.first as DBusBoolean).value;
    } catch (e) {
      return false;
    }
  }

  /// التحقق من وجود adapter
  Future<bool> hasBluetoothAdapter() async {
    try {
      final r = await _bluetooth.callMethod(
        'org.venom.Network.Bluetooth',
        'HasAdapter',
        [],
      );
      return (r.values.first as DBusBoolean).value;
    } catch (e) {
      return false;
    }
  }

  Future<BluetoothStatus> getBluetoothStatus() async {
    try {
      final r = await _bluetooth.callMethod(
        'org.venom.Network.Bluetooth',
        'GetStatus',
        [],
      );
      return BluetoothStatus.fromDBus(r.values.first as DBusStruct);
    } catch (e) {
      return BluetoothStatus(
        powered: false,
        discovering: false,
        pairable: false,
        name: '',
        address: '',
      );
    }
  }

  Future<List<BluetoothDevice>> getBluetoothDevices() async {
    try {
      final r = await _bluetooth.callMethod(
        'org.venom.Network.Bluetooth',
        'GetDevices',
        [],
      );
      return (r.values.first as DBusArray).children
          .map((v) => BluetoothDevice.fromDBus(v as DBusStruct))
          .toList();
    } catch (e) {
      return [];
    }
  }

  Future<bool> setBluetoothPowered(bool p) => _callBool(
    _bluetooth,
    'org.venom.Network.Bluetooth',
    'SetPowered',
    [DBusBoolean(p)],
  );

  Future<bool> setBluetoothDiscoverable(bool d) => _callBool(
    _bluetooth,
    'org.venom.Network.Bluetooth',
    'SetDiscoverable',
    [DBusBoolean(d)],
  );

  Future<bool> setBluetoothPairable(bool p) => _callBool(
    _bluetooth,
    'org.venom.Network.Bluetooth',
    'SetPairable',
    [DBusBoolean(p)],
  );

  Future<bool> startBluetoothScan() =>
      _callBool(_bluetooth, 'org.venom.Network.Bluetooth', 'StartScan', []);
  Future<bool> stopBluetoothScan() =>
      _callBool(_bluetooth, 'org.venom.Network.Bluetooth', 'StopScan', []);

  // ════════════════════════════════════════════════════════════════════════
  // 📱 Bluetooth - Device Operations (with error messages)
  // ════════════════════════════════════════════════════════════════════════

  /// إقران جهاز مع رسالة خطأ
  Future<BluetoothResult> pairDevice(String addr) async {
    try {
      final r = await _bluetooth.callMethod(
        'org.venom.Network.Bluetooth',
        'Pair',
        [DBusString(addr)],
      );
      return BluetoothResult.fromDBus(r.values.first as DBusStruct);
    } catch (e) {
      return BluetoothResult(success: false, message: e.toString());
    }
  }

  /// الاتصال بجهاز مع رسالة خطأ
  Future<BluetoothResult> connectDevice(String addr) async {
    try {
      final r = await _bluetooth.callMethod(
        'org.venom.Network.Bluetooth',
        'Connect',
        [DBusString(addr)],
      );
      return BluetoothResult.fromDBus(r.values.first as DBusStruct);
    } catch (e) {
      return BluetoothResult(success: false, message: e.toString());
    }
  }

  /// قطع الاتصال بجهاز مع رسالة خطأ
  Future<BluetoothResult> disconnectDevice(String addr) async {
    try {
      final r = await _bluetooth.callMethod(
        'org.venom.Network.Bluetooth',
        'Disconnect',
        [DBusString(addr)],
      );
      return BluetoothResult.fromDBus(r.values.first as DBusStruct);
    } catch (e) {
      return BluetoothResult(success: false, message: e.toString());
    }
  }

  Future<bool> removeDevice(String addr) => _callBool(
    _bluetooth,
    'org.venom.Network.Bluetooth',
    'Remove',
    [DBusString(addr)],
  );

  Future<bool> trustDevice(String addr, bool trusted) => _callBool(
    _bluetooth,
    'org.venom.Network.Bluetooth',
    'Trust',
    [DBusString(addr), DBusBoolean(trusted)],
  );

  // ════════════════════════════════════════════════════════════════════════
  // 📱 Bluetooth - Battery & Profiles
  // ════════════════════════════════════════════════════════════════════════

  /// الحصول على مستوى البطارية
  Future<BluetoothBattery> getBluetoothBattery(String addr) async {
    try {
      final r = await _bluetooth.callMethod(
        'org.venom.Network.Bluetooth',
        'GetBattery',
        [DBusString(addr)],
      );
      return BluetoothBattery.fromDBus(r.values.first as DBusStruct);
    } catch (e) {
      return BluetoothBattery(percentage: -1, available: false);
    }
  }

  /// الحصول على البروفايلات المدعومة
  Future<BluetoothProfiles> getBluetoothProfiles(String addr) async {
    try {
      final r = await _bluetooth.callMethod(
        'org.venom.Network.Bluetooth',
        'GetProfiles',
        [DBusString(addr)],
      );
      return BluetoothProfiles.fromDBus(r.values.first as DBusStruct);
    } catch (e) {
      return BluetoothProfiles(
        a2dpSink: false,
        a2dpSource: false,
        hfpHf: false,
        hfpAg: false,
        hid: false,
      );
    }
  }

  // ════════════════════════════════════════════════════════════════════════
  // 🔌 Ethernet
  // ════════════════════════════════════════════════════════════════════════

  Future<List<EthernetInterface>> getEthernetInterfaces() async {
    try {
      final r = await _ethernet.callMethod(
        'org.venom.Network.Ethernet',
        'GetInterfaces',
        [],
      );
      return (r.values.first as DBusArray).children
          .map((v) => EthernetInterface.fromDBus(v as DBusStruct))
          .toList();
    } catch (e) {
      return [];
    }
  }

  Future<bool> enableEthernet(String name) => _callBool(
    _ethernet,
    'org.venom.Network.Ethernet',
    'Enable',
    [DBusString(name)],
  );
  Future<bool> disableEthernet(String name) => _callBool(
    _ethernet,
    'org.venom.Network.Ethernet',
    'Disable',
    [DBusString(name)],
  );

  // Helper
  Future<bool> _callBool(
    DBusRemoteObject obj,
    String iface,
    String method,
    List<DBusValue> args,
  ) async {
    try {
      final r = await obj.callMethod(iface, method, args);
      return (r.values.first as DBusBoolean).value;
    } catch (e) {
      return false;
    }
  }
}
