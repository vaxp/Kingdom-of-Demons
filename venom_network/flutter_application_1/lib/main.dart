import 'package:flutter/material.dart';
import 'network_service.dart';

void main() => runApp(const NetworkApp());

class NetworkApp extends StatelessWidget {
  const NetworkApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Venom Network',
      debugShowCheckedModeBanner: false,
      theme: ThemeData.dark().copyWith(
        scaffoldBackgroundColor: const Color(0xFF0D1117),
        colorScheme: ColorScheme.dark(
          primary: Colors.teal,
          secondary: Colors.teal.shade300,
          surface: const Color(0xFF161B22),
        ),
        cardColor: const Color(0xFF161B22),
        appBarTheme: const AppBarTheme(
          backgroundColor: Color(0xFF161B22),
          elevation: 0,
        ),
      ),
      home: const NetworkPage(),
    );
  }
}

class NetworkPage extends StatefulWidget {
  const NetworkPage({super.key});
  @override
  State<NetworkPage> createState() => _NetworkPageState();
}

class _NetworkPageState extends State<NetworkPage>
    with SingleTickerProviderStateMixin {
  final NetworkService _service = NetworkService();
  late TabController _tabController;

  bool _loading = true;
  bool _connected = false;

  // WiFi
  WiFiStatus? _wifiStatus;
  List<WiFiNetwork> _wifiNetworks = [];
  bool _wifiEnabled = false;

  // Bluetooth
  BluetoothStatus? _btStatus;
  List<BluetoothDevice> _btDevices = [];
  bool _btScanning = false;
  bool _btAvailable = false;
  bool _btHasAdapter = false;

  // Ethernet
  List<EthernetInterface> _ethInterfaces = [];

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 3, vsync: this);
    _connect();
  }

  @override
  void dispose() {
    _tabController.dispose();
    _service.disconnect();
    super.dispose();
  }

  Future<void> _connect() async {
    _connected = await _service.connect();
    if (_connected) await _refresh();
    setState(() => _loading = false);
  }

  Future<void> _refresh() async {
    setState(() => _loading = true);
    _wifiStatus = await _service.getWifiStatus();
    _wifiNetworks = await _service.getWifiNetworks();
    _wifiEnabled = await _service.isWifiEnabled();
    _btAvailable = await _service.isBluetoothAvailable();
    _btHasAdapter = await _service.hasBluetoothAdapter();
    _btStatus = await _service.getBluetoothStatus();
    _btDevices = await _service.getBluetoothDevices();
    _ethInterfaces = await _service.getEthernetInterfaces();
    setState(() => _loading = false);
  }

  void _snack(String msg, {bool error = false}) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(msg),
        backgroundColor: error ? Colors.red.shade700 : Colors.teal.shade700,
        behavior: SnackBarBehavior.floating,
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Row(
          children: [
            Icon(Icons.wifi, color: Colors.teal),
            SizedBox(width: 8),
            Text('Venom Network'),
          ],
        ),
        actions: [
          IconButton(icon: const Icon(Icons.refresh), onPressed: _refresh),
        ],
        bottom: TabBar(
          controller: _tabController,
          tabs: const [
            Tab(icon: Icon(Icons.wifi), text: 'WiFi'),
            Tab(icon: Icon(Icons.bluetooth), text: 'Bluetooth'),
            Tab(icon: Icon(Icons.settings_ethernet), text: 'Ethernet'),
          ],
        ),
      ),
      body: _loading
          ? const Center(child: CircularProgressIndicator())
          : !_connected
          ? _buildError()
          : TabBarView(
              controller: _tabController,
              children: [_buildWifi(), _buildBluetooth(), _buildEthernet()],
            ),
    );
  }

  Widget _buildError() => Center(
    child: Column(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        Icon(Icons.error_outline, size: 64, color: Colors.red.shade400),
        const SizedBox(height: 16),
        const Text('غير متصل بـ venom_network'),
        const SizedBox(height: 16),
        ElevatedButton.icon(
          onPressed: _connect,
          icon: const Icon(Icons.refresh),
          label: const Text('إعادة المحاولة'),
        ),
      ],
    ),
  );

  // ═══════════════════════════════════════════════════════════════════════════
  // 📶 WiFi Tab
  // ═══════════════════════════════════════════════════════════════════════════

  Widget _buildWifi() => SingleChildScrollView(
    padding: const EdgeInsets.all(16),
    child: Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        _buildWifiStatus(),
        const SizedBox(height: 16),
        _buildWifiNetworks(),
      ],
    ),
  );

  Widget _buildWifiStatus() => Card(
    child: Padding(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              const Text(
                '📶 WiFi',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
              ),
              const Spacer(),
              Switch(
                value: _wifiEnabled,
                activeTrackColor: Colors.teal,
                onChanged: (v) async {
                  final ok = await _service.setWifiEnabled(v);
                  if (ok) {
                    setState(() => _wifiEnabled = v);
                    _refresh();
                  }
                },
              ),
            ],
          ),
          const Divider(),
          if (_wifiStatus != null && _wifiStatus!.connected) ...[
            _infoRow('الشبكة', _wifiStatus!.ssid),
            _infoRow('IP', _wifiStatus!.ipAddress),
            _infoRow('القوة', '${_wifiStatus!.strength}%'),
            const SizedBox(height: 8),
            ElevatedButton.icon(
              onPressed: () async {
                await _service.wifiDisconnect();
                _refresh();
                _snack('تم قطع الاتصال');
              },
              icon: const Icon(Icons.link_off),
              label: const Text('قطع الاتصال'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.red.shade700,
              ),
            ),
          ] else
            const Text('غير متصل', style: TextStyle(color: Colors.grey)),
        ],
      ),
    ),
  );

  Widget _buildWifiNetworks() => Card(
    child: Padding(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            '🔍 الشبكات المتاحة',
            style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
          ),
          const SizedBox(height: 12),
          if (_wifiNetworks.isEmpty)
            const Text(
              'لم يتم العثور على شبكات',
              style: TextStyle(color: Colors.grey),
            )
          else
            ListView.builder(
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              itemCount: _wifiNetworks.length,
              itemBuilder: (_, i) {
                final n = _wifiNetworks[i];
                return ListTile(
                  leading: Icon(
                    _getWifiIcon(n.strength),
                    color: n.connected ? Colors.teal : null,
                  ),
                  title: Text(n.ssid.isEmpty ? n.bssid : n.ssid),
                  subtitle: Text(
                    '${n.band} • ${n.strength}%${n.secured ? ' 🔒' : ''}',
                  ),
                  trailing: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      if (n.connected)
                        IconButton(
                          icon: const Icon(Icons.settings),
                          tooltip: 'إعدادات',
                          onPressed: () => _showNetworkSettings(n.ssid),
                        )
                      else
                        ElevatedButton(
                          onPressed: () => _showConnectDialog(n),
                          child: const Text('اتصال'),
                        ),
                    ],
                  ),
                  onTap: n.connected
                      ? () => _showNetworkSettings(n.ssid)
                      : null,
                );
              },
            ),
        ],
      ),
    ),
  );

  void _showNetworkSettings(String ssid) async {
    final details = await _service.getConnectionDetails(ssid);
    if (!mounted) return;

    showModalBottomSheet(
      context: context,
      isScrollControlled: true,
      backgroundColor: const Color(0xFF161B22),
      builder: (ctx) => Padding(
        padding: EdgeInsets.only(bottom: MediaQuery.of(ctx).viewInsets.bottom),
        child: _NetworkSettingsSheet(
          details: details,
          service: _service,
          onSaved: () {
            Navigator.pop(ctx);
            _refresh();
            _snack('✅ تم الحفظ');
          },
        ),
      ),
    );
  }

  void _showConnectDialog(WiFiNetwork n) {
    final passCtrl = TextEditingController();
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text('الاتصال بـ ${n.ssid}'),
        content: n.secured
            ? TextField(
                controller: passCtrl,
                obscureText: true,
                decoration: const InputDecoration(hintText: 'كلمة المرور'),
              )
            : const Text('شبكة مفتوحة'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('إلغاء'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(ctx);
              _snack('جاري الاتصال...');
              final ok = await _service.wifiConnect(n.ssid, passCtrl.text);
              if (ok) {
                _refresh();
                _snack('✅ تم الاتصال');
              } else {
                _snack('❌ فشل الاتصال', error: true);
              }
            },
            child: const Text('اتصال'),
          ),
        ],
      ),
    );
  }

  IconData _getWifiIcon(int strength) {
    if (strength >= 70) return Icons.wifi;
    if (strength >= 40) return Icons.wifi_2_bar;
    return Icons.wifi_1_bar;
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // 📱 Bluetooth Tab (Enhanced)
  // ═══════════════════════════════════════════════════════════════════════════

  Widget _buildBluetooth() => SingleChildScrollView(
    padding: const EdgeInsets.all(16),
    child: Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        _buildBluetoothStatus(),
        const SizedBox(height: 16),
        _buildBluetoothDevices(),
      ],
    ),
  );

  Widget _buildBluetoothStatus() => Card(
    child: Padding(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              const Text(
                '📱 Bluetooth',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
              ),
              const Spacer(),
              // Availability indicator
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                decoration: BoxDecoration(
                  color: _btAvailable
                      ? Colors.green.shade800
                      : Colors.red.shade800,
                  borderRadius: BorderRadius.circular(12),
                ),
                child: Text(
                  _btAvailable ? 'BlueZ ✓' : 'BlueZ ✗',
                  style: const TextStyle(fontSize: 12),
                ),
              ),
              const SizedBox(width: 8),
              Switch(
                value: _btStatus?.powered ?? false,
                activeTrackColor: Colors.blue,
                onChanged: _btAvailable
                    ? (v) async {
                        await _service.setBluetoothPowered(v);
                        _refresh();
                      }
                    : null,
              ),
            ],
          ),
          if (!_btAvailable)
            Container(
              margin: const EdgeInsets.only(top: 8),
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Colors.red.shade900.withValues(alpha: 0.3),
                borderRadius: BorderRadius.circular(8),
                border: Border.all(color: Colors.red.shade700),
              ),
              child: const Row(
                children: [
                  Icon(Icons.warning, color: Colors.orange),
                  SizedBox(width: 8),
                  Expanded(
                    child: Text(
                      'خدمة BlueZ غير متوفرة. تأكد من تشغيل bluetoothd',
                      style: TextStyle(color: Colors.orange),
                    ),
                  ),
                ],
              ),
            )
          else if (!_btHasAdapter)
            Container(
              margin: const EdgeInsets.only(top: 8),
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Colors.orange.shade900.withValues(alpha: 0.3),
                borderRadius: BorderRadius.circular(8),
                border: Border.all(color: Colors.orange.shade700),
              ),
              child: const Row(
                children: [
                  Icon(Icons.bluetooth_disabled, color: Colors.orange),
                  SizedBox(width: 8),
                  Expanded(
                    child: Text(
                      'لم يتم العثور على محول Bluetooth',
                      style: TextStyle(color: Colors.orange),
                    ),
                  ),
                ],
              ),
            )
          else if (_btStatus != null) ...[
            const Divider(),
            _infoRow('الاسم', _btStatus!.name),
            _infoRow('العنوان', _btStatus!.address),
            _infoRow('البحث', _btStatus!.discovering ? '🟢 نشط' : '⚪ متوقف'),
            _infoRow('قابل للإقران', _btStatus!.pairable ? '🟢 نعم' : '⚪ لا'),
          ],
          const SizedBox(height: 12),
          if (_btAvailable && _btHasAdapter)
            Wrap(
              spacing: 8,
              runSpacing: 8,
              children: [
                ElevatedButton.icon(
                  onPressed: _btScanning
                      ? null
                      : () async {
                          setState(() => _btScanning = true);
                          await _service.startBluetoothScan();
                          await Future.delayed(const Duration(seconds: 5));
                          await _service.stopBluetoothScan();
                          await _refresh();
                          setState(() => _btScanning = false);
                        },
                  icon: _btScanning
                      ? const SizedBox(
                          width: 16,
                          height: 16,
                          child: CircularProgressIndicator(strokeWidth: 2),
                        )
                      : const Icon(Icons.search),
                  label: Text(_btScanning ? 'جارٍ البحث...' : 'بحث'),
                ),
                OutlinedButton.icon(
                  onPressed: () async {
                    final current = _btStatus?.pairable ?? false;
                    await _service.setBluetoothPairable(!current);
                    _refresh();
                  },
                  icon: Icon(
                    _btStatus?.pairable == true
                        ? Icons.visibility_off
                        : Icons.visibility,
                  ),
                  label: Text(
                    _btStatus?.pairable == true ? 'إخفاء' : 'السماح بالإقران',
                  ),
                ),
              ],
            ),
        ],
      ),
    ),
  );

  Widget _buildBluetoothDevices() => Card(
    child: Padding(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            '📋 الأجهزة',
            style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
          ),
          const SizedBox(height: 12),
          if (_btDevices.isEmpty)
            const Text(
              'لم يتم العثور على أجهزة',
              style: TextStyle(color: Colors.grey),
            )
          else
            ListView.builder(
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              itemCount: _btDevices.length,
              itemBuilder: (_, i) {
                final d = _btDevices[i];
                return _BluetoothDeviceTile(
                  device: d,
                  service: _service,
                  onAction: (msg, {error = false}) => _snack(msg, error: error),
                  onRefresh: _refresh,
                );
              },
            ),
        ],
      ),
    ),
  );

  // ═══════════════════════════════════════════════════════════════════════════
  // 🔌 Ethernet Tab
  // ═══════════════════════════════════════════════════════════════════════════

  Widget _buildEthernet() => SingleChildScrollView(
    padding: const EdgeInsets.all(16),
    child: Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text(
          '🔌 واجهات الإيثرنت',
          style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
        ),
        const SizedBox(height: 16),
        if (_ethInterfaces.isEmpty)
          const Card(
            child: Padding(
              padding: EdgeInsets.all(32),
              child: Center(
                child: Text(
                  'لم يتم العثور على واجهات إيثرنت',
                  style: TextStyle(color: Colors.grey),
                ),
              ),
            ),
          )
        else
          ..._ethInterfaces.map(
            (e) => Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      children: [
                        Icon(
                          e.connected ? Icons.lan : Icons.lan_outlined,
                          color: e.connected ? Colors.green : Colors.grey,
                        ),
                        const SizedBox(width: 8),
                        Text(
                          e.name,
                          style: const TextStyle(
                            fontSize: 18,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                        const Spacer(),
                        Chip(
                          label: Text(e.connected ? 'متصل' : 'غير متصل'),
                          backgroundColor: e.connected
                              ? Colors.green.shade800
                              : Colors.grey.shade800,
                        ),
                      ],
                    ),
                    const Divider(),
                    _infoRow('MAC', e.macAddress),
                    if (e.connected) ...[
                      _infoRow('IP', e.ipAddress),
                      _infoRow('Gateway', e.gateway),
                      _infoRow('السرعة', '${e.speed} Mbps'),
                    ],
                    const SizedBox(height: 8),
                    Row(
                      children: [
                        if (e.connected)
                          ElevatedButton.icon(
                            onPressed: () async {
                              await _service.disableEthernet(e.name);
                              _refresh();
                            },
                            icon: const Icon(Icons.power_off),
                            label: const Text('إيقاف'),
                            style: ElevatedButton.styleFrom(
                              backgroundColor: Colors.red.shade700,
                            ),
                          )
                        else
                          ElevatedButton.icon(
                            onPressed: () async {
                              await _service.enableEthernet(e.name);
                              _refresh();
                            },
                            icon: const Icon(Icons.power),
                            label: const Text('تشغيل'),
                            style: ElevatedButton.styleFrom(
                              backgroundColor: Colors.green.shade700,
                            ),
                          ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
          ),
      ],
    ),
  );

  Widget _infoRow(String label, String value) => Padding(
    padding: const EdgeInsets.symmetric(vertical: 4),
    child: Row(
      children: [
        Text('$label: ', style: TextStyle(color: Colors.grey.shade400)),
        Text(value, style: const TextStyle(fontWeight: FontWeight.bold)),
      ],
    ),
  );
}

// ═══════════════════════════════════════════════════════════════════════════
// 📱 Bluetooth Device Tile (Enhanced)
// ═══════════════════════════════════════════════════════════════════════════

class _BluetoothDeviceTile extends StatefulWidget {
  final BluetoothDevice device;
  final NetworkService service;
  final void Function(String msg, {bool error}) onAction;
  final VoidCallback onRefresh;

  const _BluetoothDeviceTile({
    required this.device,
    required this.service,
    required this.onAction,
    required this.onRefresh,
  });

  @override
  State<_BluetoothDeviceTile> createState() => _BluetoothDeviceTileState();
}

class _BluetoothDeviceTileState extends State<_BluetoothDeviceTile> {
  BluetoothBattery? _battery;
  BluetoothProfiles? _profiles;
  bool _loading = false;

  @override
  void initState() {
    super.initState();
    _loadDetails();
  }

  Future<void> _loadDetails() async {
    if (widget.device.connected) {
      final battery = await widget.service.getBluetoothBattery(
        widget.device.address,
      );
      final profiles = await widget.service.getBluetoothProfiles(
        widget.device.address,
      );
      if (mounted) {
        setState(() {
          _battery = battery;
          _profiles = profiles;
        });
      }
    } else {
      final profiles = await widget.service.getBluetoothProfiles(
        widget.device.address,
      );
      if (mounted) {
        setState(() {
          _profiles = profiles;
        });
      }
    }
  }

  IconData _getBtIcon(String icon) {
    switch (icon) {
      case 'audio-headset':
        return Icons.headset;
      case 'audio-headphones':
        return Icons.headphones;
      case 'phone':
        return Icons.phone_android;
      case 'computer':
        return Icons.computer;
      case 'input-keyboard':
        return Icons.keyboard;
      case 'input-mouse':
        return Icons.mouse;
      default:
        return Icons.bluetooth;
    }
  }

  @override
  Widget build(BuildContext context) {
    final d = widget.device;

    return Card(
      margin: const EdgeInsets.symmetric(vertical: 4),
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(
                  _getBtIcon(d.icon),
                  color: d.connected ? Colors.blue : Colors.grey,
                  size: 32,
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        d.name.isEmpty ? d.address : d.name,
                        style: const TextStyle(
                          fontWeight: FontWeight.bold,
                          fontSize: 16,
                        ),
                      ),
                      const SizedBox(height: 4),
                      Row(
                        children: [
                          if (d.paired)
                            Container(
                              margin: const EdgeInsets.only(right: 4),
                              padding: const EdgeInsets.symmetric(
                                horizontal: 6,
                                vertical: 2,
                              ),
                              decoration: BoxDecoration(
                                color: Colors.green.shade800,
                                borderRadius: BorderRadius.circular(8),
                              ),
                              child: const Text(
                                'مقترن',
                                style: TextStyle(fontSize: 10),
                              ),
                            ),
                          if (d.connected)
                            Container(
                              margin: const EdgeInsets.only(right: 4),
                              padding: const EdgeInsets.symmetric(
                                horizontal: 6,
                                vertical: 2,
                              ),
                              decoration: BoxDecoration(
                                color: Colors.blue.shade800,
                                borderRadius: BorderRadius.circular(8),
                              ),
                              child: const Text(
                                'متصل',
                                style: TextStyle(fontSize: 10),
                              ),
                            ),
                          if (d.trusted)
                            Container(
                              padding: const EdgeInsets.symmetric(
                                horizontal: 6,
                                vertical: 2,
                              ),
                              decoration: BoxDecoration(
                                color: Colors.purple.shade800,
                                borderRadius: BorderRadius.circular(8),
                              ),
                              child: const Text(
                                'موثوق',
                                style: TextStyle(fontSize: 10),
                              ),
                            ),
                        ],
                      ),
                    ],
                  ),
                ),
                // Battery indicator
                if (_battery != null && _battery!.available)
                  Container(
                    padding: const EdgeInsets.symmetric(
                      horizontal: 8,
                      vertical: 4,
                    ),
                    decoration: BoxDecoration(
                      color: _getBatteryColor(_battery!.percentage),
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Icon(
                          _getBatteryIcon(_battery!.percentage),
                          size: 16,
                          color: Colors.white,
                        ),
                        const SizedBox(width: 4),
                        Text(
                          '${_battery!.percentage}%',
                          style: const TextStyle(
                            fontSize: 12,
                            color: Colors.white,
                          ),
                        ),
                      ],
                    ),
                  ),
              ],
            ),
            // Profiles
            if (_profiles != null &&
                (_profiles!.hasAudio || _profiles!.isInputDevice))
              Padding(
                padding: const EdgeInsets.only(top: 8),
                child: Text(
                  _profiles!.description,
                  style: TextStyle(color: Colors.grey.shade400, fontSize: 12),
                ),
              ),
            const SizedBox(height: 8),
            // Action buttons
            Row(
              children: [
                if (!d.paired)
                  _ActionButton(
                    icon: Icons.link,
                    label: 'إقران',
                    loading: _loading,
                    onPressed: () async {
                      setState(() => _loading = true);
                      widget.onAction('جارٍ الإقران...');
                      final res = await widget.service.pairDevice(d.address);
                      setState(() => _loading = false);
                      if (res.success) {
                        widget.onRefresh();
                        widget.onAction('✅ تم الإقران');
                      } else {
                        widget.onAction('❌ ${res.message}', error: true);
                      }
                    },
                  ),
                if (d.paired && !d.connected)
                  _ActionButton(
                    icon: Icons.bluetooth_connected,
                    label: 'اتصال',
                    color: Colors.blue,
                    loading: _loading,
                    onPressed: () async {
                      setState(() => _loading = true);
                      final res = await widget.service.connectDevice(d.address);
                      setState(() => _loading = false);
                      if (res.success) {
                        widget.onRefresh();
                        widget.onAction('✅ تم الاتصال');
                      } else {
                        widget.onAction('❌ ${res.message}', error: true);
                      }
                    },
                  ),
                if (d.connected)
                  _ActionButton(
                    icon: Icons.bluetooth_disabled,
                    label: 'قطع',
                    color: Colors.orange,
                    loading: _loading,
                    onPressed: () async {
                      setState(() => _loading = true);
                      final res = await widget.service.disconnectDevice(
                        d.address,
                      );
                      setState(() => _loading = false);
                      if (res.success) {
                        widget.onRefresh();
                        widget.onAction('✅ تم قطع الاتصال');
                      } else {
                        widget.onAction('❌ ${res.message}', error: true);
                      }
                    },
                  ),
                const Spacer(),
                IconButton(
                  icon: Icon(
                    d.trusted ? Icons.star : Icons.star_border,
                    color: d.trusted ? Colors.amber : Colors.grey,
                  ),
                  tooltip: d.trusted ? 'إزالة الثقة' : 'الوثوق',
                  onPressed: () async {
                    await widget.service.trustDevice(d.address, !d.trusted);
                    widget.onRefresh();
                  },
                ),
                IconButton(
                  icon: Icon(Icons.delete, color: Colors.red.shade400),
                  tooltip: 'إزالة',
                  onPressed: () async {
                    await widget.service.removeDevice(d.address);
                    widget.onRefresh();
                    widget.onAction('🗑️ تم الإزالة');
                  },
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Color _getBatteryColor(int percentage) {
    if (percentage > 50) return Colors.green.shade700;
    if (percentage > 20) return Colors.orange.shade700;
    return Colors.red.shade700;
  }

  IconData _getBatteryIcon(int percentage) {
    if (percentage > 80) return Icons.battery_full;
    if (percentage > 60) return Icons.battery_5_bar;
    if (percentage > 40) return Icons.battery_4_bar;
    if (percentage > 20) return Icons.battery_2_bar;
    return Icons.battery_1_bar;
  }
}

class _ActionButton extends StatelessWidget {
  final IconData icon;
  final String label;
  final Color? color;
  final bool loading;
  final VoidCallback onPressed;

  const _ActionButton({
    required this.icon,
    required this.label,
    this.color,
    required this.loading,
    required this.onPressed,
  });

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(right: 8),
      child: ElevatedButton.icon(
        onPressed: loading ? null : onPressed,
        icon: loading
            ? const SizedBox(
                width: 16,
                height: 16,
                child: CircularProgressIndicator(strokeWidth: 2),
              )
            : Icon(icon, size: 18),
        label: Text(label),
        style: ElevatedButton.styleFrom(
          backgroundColor: color,
          padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
        ),
      ),
    );
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// 🔧 Network Settings Sheet
// ═══════════════════════════════════════════════════════════════════════════

class _NetworkSettingsSheet extends StatefulWidget {
  final ConnectionDetails details;
  final NetworkService service;
  final VoidCallback onSaved;

  const _NetworkSettingsSheet({
    required this.details,
    required this.service,
    required this.onSaved,
  });

  @override
  State<_NetworkSettingsSheet> createState() => _NetworkSettingsSheetState();
}

class _NetworkSettingsSheetState extends State<_NetworkSettingsSheet> {
  late bool _isDhcp;
  late bool _autoConnect;
  final _ipCtrl = TextEditingController();
  final _gwCtrl = TextEditingController();
  final _subnetCtrl = TextEditingController();
  final _dns1Ctrl = TextEditingController();
  final _dns2Ctrl = TextEditingController();

  @override
  void initState() {
    super.initState();
    _isDhcp = widget.details.isDhcp;
    _autoConnect = widget.details.autoConnect;
    _ipCtrl.text = widget.details.ipAddress;
    _gwCtrl.text = widget.details.gateway;
    _subnetCtrl.text = widget.details.subnet.isEmpty
        ? '24'
        : widget.details.subnet;
    final dnsParts = widget.details.dns.split(', ');
    _dns1Ctrl.text = dnsParts.isNotEmpty ? dnsParts[0] : '';
    _dns2Ctrl.text = dnsParts.length > 1 ? dnsParts[1] : '';
  }

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(20),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              const Icon(Icons.settings, color: Colors.teal),
              const SizedBox(width: 8),
              Text(
                'إعدادات ${widget.details.ssid}',
                style: const TextStyle(
                  fontSize: 20,
                  fontWeight: FontWeight.bold,
                ),
              ),
            ],
          ),
          const SizedBox(height: 20),

          // الاتصال التلقائي
          SwitchListTile(
            title: const Text('🔄 الاتصال التلقائي'),
            subtitle: const Text('الاتصال بهذه الشبكة تلقائياً'),
            value: _autoConnect,
            activeTrackColor: Colors.teal,
            onChanged: (v) => setState(() => _autoConnect = v),
          ),
          const Divider(),

          // DHCP أو IP ثابت
          SwitchListTile(
            title: const Text('🌐 DHCP'),
            subtitle: Text(_isDhcp ? 'الحصول على IP تلقائياً' : 'IP ثابت'),
            value: _isDhcp,
            activeTrackColor: Colors.teal,
            onChanged: (v) => setState(() => _isDhcp = v),
          ),

          if (!_isDhcp) ...[
            const SizedBox(height: 16),
            TextField(
              controller: _ipCtrl,
              decoration: const InputDecoration(
                labelText: '📍 عنوان IP',
                hintText: '192.168.1.100',
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _gwCtrl,
              decoration: const InputDecoration(
                labelText: '🚪 البوابة (Gateway)',
                hintText: '192.168.1.1',
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _subnetCtrl,
              decoration: const InputDecoration(
                labelText: '📏 Subnet (prefix)',
                hintText: '24',
                border: OutlineInputBorder(),
              ),
              keyboardType: TextInputType.number,
            ),
          ],

          const SizedBox(height: 16),
          const Text(
            '🔧 DNS',
            style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
          ),
          const SizedBox(height: 8),
          TextField(
            controller: _dns1Ctrl,
            decoration: const InputDecoration(
              labelText: 'DNS أساسي',
              hintText: '8.8.8.8',
              border: OutlineInputBorder(),
            ),
          ),
          const SizedBox(height: 12),
          TextField(
            controller: _dns2Ctrl,
            decoration: const InputDecoration(
              labelText: 'DNS ثانوي',
              hintText: '8.8.4.4',
              border: OutlineInputBorder(),
            ),
          ),

          const SizedBox(height: 24),
          Row(
            children: [
              Expanded(
                child: OutlinedButton(
                  onPressed: () async {
                    await widget.service.forgetNetwork(widget.details.ssid);
                    if (context.mounted) Navigator.pop(context);
                  },
                  style: OutlinedButton.styleFrom(foregroundColor: Colors.red),
                  child: const Text('🗑️ نسيان الشبكة'),
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: ElevatedButton(
                  onPressed: _save,
                  style: ElevatedButton.styleFrom(backgroundColor: Colors.teal),
                  child: const Text('💾 حفظ'),
                ),
              ),
            ],
          ),
          const SizedBox(height: 20),
        ],
      ),
    );
  }

  Future<void> _save() async {
    final ssid = widget.details.ssid;

    // حفظ الاتصال التلقائي
    await widget.service.setAutoConnect(ssid, _autoConnect);

    // حفظ إعدادات IP
    if (_isDhcp) {
      await widget.service.setDHCP(ssid);
    } else {
      await widget.service.setStaticIP(
        ssid,
        _ipCtrl.text,
        _gwCtrl.text,
        _subnetCtrl.text,
        _dns1Ctrl.text,
      );
    }

    // حفظ DNS إذا تم تعديله
    if (_dns1Ctrl.text.isNotEmpty || _dns2Ctrl.text.isNotEmpty) {
      await widget.service.setDNS(ssid, _dns1Ctrl.text, _dns2Ctrl.text);
    }

    widget.onSaved();
  }
}
