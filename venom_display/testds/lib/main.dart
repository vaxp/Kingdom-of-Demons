import 'package:flutter/material.dart';
import 'services/display_service.dart';

void main() => runApp(const VenomDisplayTestApp());

class VenomDisplayTestApp extends StatelessWidget {
  const VenomDisplayTestApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Venom Display Test',
      debugShowCheckedModeBanner: false,
      theme: ThemeData.dark().copyWith(
        colorScheme: ColorScheme.dark(
          primary: Colors.purple.shade400,
          secondary: Colors.purpleAccent,
          surface: const Color(0xFF1E1E2E),
        ),
        scaffoldBackgroundColor: const Color(0xFF11111B),
        cardColor: const Color(0xFF1E1E2E),
        appBarTheme: AppBarTheme(backgroundColor: Colors.purple.shade900),
      ),
      home: const DisplayTestPage(),
    );
  }
}

class DisplayTestPage extends StatefulWidget {
  const DisplayTestPage({super.key});

  @override
  State<DisplayTestPage> createState() => _DisplayTestPageState();
}

class _DisplayTestPageState extends State<DisplayTestPage>
    with TickerProviderStateMixin {
  late final DisplayService _service;
  late final TabController _tabController;

  // State
  List<DisplayInfo> _displays = [];
  List<String> _profiles = [];
  List<AutoProfileRule> _autoRules = [];
  String _status = 'Connecting...';
  bool _isConnected = false;

  // Night Light
  bool _nightLightEnabled = false;
  int _nightLightTemp = 6500;

  // Backlight
  int _backlight = 100;
  int _maxBacklight = 100;

  // DPMS
  int _dpmsMode = 0;
  DPMSTimeouts _dpmsTimeouts = DPMSTimeouts(standby: 0, suspend: 0, off: 0);

  // Selected display for details
  String? _selectedDisplay;
  List<DisplayMode> _availableModes = [];
  EDIDInfo? _edidInfo;
  VRRInfo? _vrrInfo;
  double _displayScale = 1.0;

  @override
  void initState() {
    super.initState();
    _service = DisplayService();
    _tabController = TabController(length: 5, vsync: this);
    _refresh();
  }

  @override
  void dispose() {
    _service.dispose();
    _tabController.dispose();
    super.dispose();
  }

  Future<void> _refresh() async {
    try {
      _displays = await _service.getDisplays();
      _profiles = await _service.getProfiles();
      _autoRules = await _service.getAutoProfiles();

      final nl = await _service.getNightLight();
      _nightLightEnabled = nl.enabled;
      _nightLightTemp = nl.temperature;

      _backlight = await _service.getBacklight();
      _maxBacklight = await _service.getMaxBacklight();

      _dpmsMode = await _service.getDPMSMode();
      _dpmsTimeouts = await _service.getDPMSTimeouts();

      if (_selectedDisplay != null) {
        await _refreshSelectedDisplay();
      }

      setState(() {
        _status = 'Connected ✓';
        _isConnected = true;
      });
    } catch (e) {
      setState(() {
        _status = 'Error: $e';
        _isConnected = false;
      });
    }
  }

  Future<void> _refreshSelectedDisplay() async {
    if (_selectedDisplay == null) return;
    _availableModes = await _service.getModes(_selectedDisplay!);
    _edidInfo = await _service.getEDID(_selectedDisplay!);
    _vrrInfo = await _service.getVRRInfo(_selectedDisplay!);
    _displayScale = await _service.getScale(_selectedDisplay!);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('🖥️ Venom Display Test'),
        actions: [
          IconButton(icon: const Icon(Icons.refresh), onPressed: _refresh),
        ],
        bottom: TabBar(
          controller: _tabController,
          isScrollable: true,
          tabs: const [
            Tab(icon: Icon(Icons.monitor), text: 'Displays'),
            Tab(icon: Icon(Icons.nightlight), text: 'Night Light'),
            Tab(icon: Icon(Icons.power_settings_new), text: 'DPMS'),
            Tab(icon: Icon(Icons.save), text: 'Profiles'),
            Tab(icon: Icon(Icons.settings), text: 'Advanced'),
          ],
        ),
      ),
      body: Column(
        children: [
          _buildStatusBar(),
          Expanded(
            child: TabBarView(
              controller: _tabController,
              children: [
                _buildDisplaysTab(),
                _buildNightLightTab(),
                _buildDPMSTab(),
                _buildProfilesTab(),
                _buildAdvancedTab(),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildStatusBar() {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      color: _isConnected ? Colors.green.shade900 : Colors.red.shade900,
      child: Row(
        children: [
          Icon(_isConnected ? Icons.check_circle : Icons.error, size: 16),
          const SizedBox(width: 8),
          Text(_status),
          const Spacer(),
          Text('${_displays.length} displays'),
        ],
      ),
    );
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // Tab 1: Displays
  // ═══════════════════════════════════════════════════════════════════════════

  Widget _buildDisplaysTab() {
    return Row(
      children: [
        // Display List
        SizedBox(
          width: 280,
          child: ListView.builder(
            padding: const EdgeInsets.all(8),
            itemCount: _displays.length,
            itemBuilder: (_, i) => _buildDisplayListItem(_displays[i]),
          ),
        ),
        const VerticalDivider(width: 1),
        // Display Details
        Expanded(
          child: _selectedDisplay == null
              ? const Center(child: Text('Select a display'))
              : _buildDisplayDetails(),
        ),
      ],
    );
  }

  Widget _buildDisplayListItem(DisplayInfo d) {
    final isSelected = _selectedDisplay == d.name;
    return Card(
      color: isSelected ? Colors.purple.shade800 : null,
      child: ListTile(
        leading: Icon(
          d.connected ? Icons.monitor : Icons.monitor_outlined,
          color: d.primary ? Colors.amber : null,
        ),
        title: Text('${d.name} ${d.primary ? "⭐" : ""}'),
        subtitle: Text('${d.width}x${d.height}@${d.rate.toStringAsFixed(0)}Hz'),
        trailing: d.connected
            ? const Icon(Icons.check, color: Colors.green)
            : null,
        onTap: () async {
          setState(() => _selectedDisplay = d.name);
          await _refreshSelectedDisplay();
          setState(() {});
        },
      ),
    );
  }

  Widget _buildDisplayDetails() {
    final display = _displays.firstWhere((d) => d.name == _selectedDisplay);
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Header
          Text(display.name, style: Theme.of(context).textTheme.headlineMedium),
          const SizedBox(height: 16),

          // Info Cards Row
          Wrap(
            spacing: 16,
            runSpacing: 16,
            children: [
              _infoCard('Resolution', '${display.width}x${display.height}'),
              _infoCard(
                'Refresh Rate',
                '${display.rate.toStringAsFixed(1)} Hz',
              ),
              _infoCard('Position', '(${display.x}, ${display.y})'),
              _infoCard('Primary', display.primary ? 'Yes ⭐' : 'No'),
            ],
          ),
          const SizedBox(height: 24),

          // Actions
          Text('Actions', style: Theme.of(context).textTheme.titleLarge),
          const SizedBox(height: 8),
          Wrap(
            spacing: 8,
            runSpacing: 8,
            children: [
              _actionButton('Set Primary', Icons.star, () async {
                await _service.setPrimary(display.name);
                await _refresh();
              }),
              _actionButton('Enable', Icons.visibility, () async {
                await _service.enableOutput(display.name);
                await _refresh();
              }),
              _actionButton('Disable', Icons.visibility_off, () async {
                await _service.disableOutput(display.name);
                await _refresh();
              }),
              _actionButton('Extend Right', Icons.arrow_forward, () async {
                await _service.extendRight(display.name);
                await _refresh();
              }),
              _actionButton('Extend Left', Icons.arrow_back, () async {
                await _service.extendLeft(display.name);
                await _refresh();
              }),
            ],
          ),
          const SizedBox(height: 24),

          // Rotation
          Text('Rotation', style: Theme.of(context).textTheme.titleLarge),
          const SizedBox(height: 8),
          Wrap(
            spacing: 8,
            children: [
              _rotationButton('Normal', 0, Icons.crop_portrait),
              _rotationButton('Left', 90, Icons.rotate_left),
              _rotationButton('Inverted', 180, Icons.flip),
              _rotationButton('Right', 270, Icons.rotate_right),
            ],
          ),
          const SizedBox(height: 24),

          // Scale
          Text('Scale', style: Theme.of(context).textTheme.titleLarge),
          const SizedBox(height: 8),
          Row(
            children: [
              Text('${(_displayScale * 100).toStringAsFixed(0)}%'),
              Expanded(
                child: Slider(
                  min: 0.5,
                  max: 2.0,
                  value: _displayScale.clamp(0.5, 2.0),
                  divisions: 6,
                  label: '${(_displayScale * 100).toStringAsFixed(0)}%',
                  onChanged: (v) => setState(() => _displayScale = v),
                  onChangeEnd: (v) async {
                    await _service.setScale(display.name, v);
                    await _refreshSelectedDisplay();
                    setState(() {});
                  },
                ),
              ),
            ],
          ),
          const SizedBox(height: 24),

          // Available Modes
          Text(
            'Available Modes',
            style: Theme.of(context).textTheme.titleLarge,
          ),
          const SizedBox(height: 8),
          Wrap(
            spacing: 8,
            runSpacing: 8,
            children: _availableModes
                .map(
                  (m) => ChoiceChip(
                    label: Text(
                      '${m.width}x${m.height}@${m.rate.toStringAsFixed(0)}',
                    ),
                    selected:
                        display.width == m.width && display.height == m.height,
                    onSelected: (_) async {
                      await _service.setMode(
                        display.name,
                        m.width,
                        m.height,
                        m.rate,
                      );
                      await _refresh();
                    },
                  ),
                )
                .toList(),
          ),
          const SizedBox(height: 24),

          // EDID Info
          if (_edidInfo != null) ...[
            Text('EDID Info', style: Theme.of(context).textTheme.titleLarge),
            const SizedBox(height: 8),
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text('Manufacturer: ${_edidInfo!.manufacturer}'),
                    Text('Model: ${_edidInfo!.model}'),
                    Text('Serial: ${_edidInfo!.serial}'),
                    Text('Product Code: ${_edidInfo!.productCode}'),
                    Text(
                      'Max Resolution: ${_edidInfo!.maxWidth}x${_edidInfo!.maxHeight}',
                    ),
                    Text('HDR: ${_edidInfo!.hdrSupported ? "✓" : "✗"}'),
                    Text('VRR: ${_edidInfo!.vrrSupported ? "✓" : "✗"}'),
                  ],
                ),
              ),
            ),
          ],
          const SizedBox(height: 24),

          // VRR Info
          if (_vrrInfo != null) ...[
            Text(
              'VRR (Variable Refresh Rate)',
              style: Theme.of(context).textTheme.titleLarge,
            ),
            const SizedBox(height: 8),
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text('Capable: ${_vrrInfo!.capable ? "Yes" : "No"}'),
                    Text('Enabled: ${_vrrInfo!.enabled ? "Yes" : "No"}'),
                    if (_vrrInfo!.capable) ...[
                      Text(
                        'Range: ${_vrrInfo!.minRate}-${_vrrInfo!.maxRate} Hz',
                      ),
                      const SizedBox(height: 8),
                      ElevatedButton(
                        onPressed: () async {
                          await _service.setVRR(
                            display.name,
                            !_vrrInfo!.enabled,
                          );
                          await _refreshSelectedDisplay();
                          setState(() {});
                        },
                        child: Text(
                          _vrrInfo!.enabled ? 'Disable VRR' : 'Enable VRR',
                        ),
                      ),
                    ],
                  ],
                ),
              ),
            ),
          ],
        ],
      ),
    );
  }

  Widget _infoCard(String title, String value) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            Text(title, style: const TextStyle(color: Colors.grey)),
            const SizedBox(height: 4),
            Text(
              value,
              style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
          ],
        ),
      ),
    );
  }

  Widget _actionButton(String label, IconData icon, VoidCallback onPressed) {
    return ElevatedButton.icon(
      onPressed: onPressed,
      icon: Icon(icon),
      label: Text(label),
    );
  }

  Widget _rotationButton(String label, int rotation, IconData icon) {
    return ElevatedButton.icon(
      onPressed: () async {
        await _service.setRotation(_selectedDisplay!, rotation);
        await _refresh();
      },
      icon: Icon(icon),
      label: Text(label),
    );
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // Tab 2: Night Light
  // ═══════════════════════════════════════════════════════════════════════════

  Widget _buildNightLightTab() {
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '🌙 Night Light',
            style: Theme.of(context).textTheme.headlineMedium,
          ),
          const SizedBox(height: 8),
          const Text('Reduce blue light to ease eye strain at night'),
          const SizedBox(height: 24),

          SwitchListTile(
            title: const Text('Enable Night Light'),
            subtitle: Text(_nightLightEnabled ? 'Active' : 'Inactive'),
            value: _nightLightEnabled,
            onChanged: (v) async {
              await _service.setNightLight(v, _nightLightTemp);
              await _refresh();
            },
          ),
          const SizedBox(height: 24),

          Text('Color Temperature: ${_nightLightTemp}K'),
          Row(
            children: [
              const Text('🔥 Warm'),
              Expanded(
                child: Slider(
                  min: 1000,
                  max: 10000,
                  value: _nightLightTemp.toDouble(),
                  divisions: 18,
                  label: '${_nightLightTemp}K',
                  onChanged: (v) => setState(() => _nightLightTemp = v.round()),
                  onChangeEnd: (v) async {
                    await _service.setNightLight(_nightLightEnabled, v.round());
                    await _refresh();
                  },
                ),
              ),
              const Text('Cool ❄️'),
            ],
          ),
          const SizedBox(height: 24),

          // Backlight (Laptop)
          if (_maxBacklight > 0) ...[
            Text(
              '💡 Backlight',
              style: Theme.of(context).textTheme.headlineMedium,
            ),
            const SizedBox(height: 8),
            Text('Screen backlight (laptop displays): $_backlight%'),
            Slider(
              min: 0,
              max: _maxBacklight.toDouble(),
              value: _backlight.toDouble().clamp(0, _maxBacklight.toDouble()),
              label: '$_backlight%',
              onChanged: (v) => setState(() => _backlight = v.round()),
              onChangeEnd: (v) async {
                await _service.setBacklight(v.round());
                await _refresh();
              },
            ),
          ] else ...[
            const SizedBox(height: 24),
            const Text('💡 Backlight: Not available on this system'),
          ],
        ],
      ),
    );
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // Tab 3: DPMS
  // ═══════════════════════════════════════════════════════════════════════════

  Widget _buildDPMSTab() {
    final modes = ['On', 'Standby', 'Suspend', 'Off'];
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '🔌 Display Power Management',
            style: Theme.of(context).textTheme.headlineMedium,
          ),
          const SizedBox(height: 24),

          Text(
            'Current Mode: ${modes[_dpmsMode]}',
            style: const TextStyle(fontSize: 18),
          ),
          const SizedBox(height: 16),
          Wrap(
            spacing: 12,
            children: List.generate(
              4,
              (i) => ElevatedButton(
                onPressed: () async {
                  await _service.setDPMSMode(i);
                  await _refresh();
                },
                style: ElevatedButton.styleFrom(
                  backgroundColor: _dpmsMode == i ? Colors.purple : null,
                  padding: const EdgeInsets.symmetric(
                    horizontal: 24,
                    vertical: 16,
                  ),
                ),
                child: Text(modes[i]),
              ),
            ),
          ),
          const SizedBox(height: 32),

          Text('Timeouts', style: Theme.of(context).textTheme.titleLarge),
          const SizedBox(height: 16),
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                children: [
                  ListTile(
                    title: const Text('Standby'),
                    subtitle: Text('${_dpmsTimeouts.standby} seconds'),
                  ),
                  ListTile(
                    title: const Text('Suspend'),
                    subtitle: Text('${_dpmsTimeouts.suspend} seconds'),
                  ),
                  ListTile(
                    title: const Text('Off'),
                    subtitle: Text('${_dpmsTimeouts.off} seconds'),
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // Tab 4: Profiles
  // ═══════════════════════════════════════════════════════════════════════════

  Widget _buildProfilesTab() {
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '💾 Display Profiles',
            style: Theme.of(context).textTheme.headlineMedium,
          ),
          const SizedBox(height: 16),

          Row(
            children: [
              ElevatedButton.icon(
                onPressed: _showSaveProfileDialog,
                icon: const Icon(Icons.save),
                label: const Text('Save Current'),
              ),
              const SizedBox(width: 8),
              ElevatedButton.icon(
                onPressed: () async {
                  await _service.applyAutoProfiles();
                  await _refresh();
                  _showSnackBar('Auto profiles applied');
                },
                icon: const Icon(Icons.auto_fix_high),
                label: const Text('Apply Auto'),
              ),
            ],
          ),
          const SizedBox(height: 24),

          Text('Saved Profiles', style: Theme.of(context).textTheme.titleLarge),
          const SizedBox(height: 8),
          Expanded(
            child: ListView.builder(
              itemCount: _profiles.length,
              itemBuilder: (_, i) => Card(
                child: ListTile(
                  leading: const Icon(Icons.display_settings),
                  title: Text(_profiles[i]),
                  trailing: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      IconButton(
                        icon: const Icon(Icons.play_arrow),
                        onPressed: () async {
                          await _service.loadProfile(_profiles[i]);
                          await _refresh();
                          _showSnackBar('Profile loaded: ${_profiles[i]}');
                        },
                      ),
                      IconButton(
                        icon: const Icon(Icons.delete),
                        onPressed: () async {
                          await _service.deleteProfile(_profiles[i]);
                          await _refresh();
                          _showSnackBar('Profile deleted');
                        },
                      ),
                    ],
                  ),
                ),
              ),
            ),
          ),
          const SizedBox(height: 24),

          Text(
            'Auto Profile Rules',
            style: Theme.of(context).textTheme.titleLarge,
          ),
          const SizedBox(height: 8),
          if (_autoRules.isEmpty)
            const Text('No auto profile rules configured')
          else
            ...(_autoRules.map(
              (r) => Card(
                child: ListTile(
                  leading: const Icon(Icons.auto_awesome),
                  title: Text(r.profileName),
                  subtitle: Text('EDID: ${r.edidMatch}'),
                  trailing: IconButton(
                    icon: const Icon(Icons.delete),
                    onPressed: () async {
                      await _service.removeAutoProfile(r.edidMatch);
                      await _refresh();
                    },
                  ),
                ),
              ),
            )),
        ],
      ),
    );
  }

  void _showSaveProfileDialog() {
    final controller = TextEditingController();
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Save Profile'),
        content: TextField(
          controller: controller,
          decoration: const InputDecoration(labelText: 'Profile name'),
          autofocus: true,
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () async {
              if (controller.text.isNotEmpty) {
                await _service.saveProfile(controller.text);
                await _refresh();
                Navigator.pop(ctx);
                _showSnackBar('Profile saved: ${controller.text}');
              }
            },
            child: const Text('Save'),
          ),
        ],
      ),
    );
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // Tab 5: Advanced
  // ═══════════════════════════════════════════════════════════════════════════

  Widget _buildAdvancedTab() {
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '⚙️ Advanced',
            style: Theme.of(context).textTheme.headlineMedium,
          ),
          const SizedBox(height: 24),

          // Arrangement
          Text('Arrangement', style: Theme.of(context).textTheme.titleLarge),
          const SizedBox(height: 8),
          Wrap(
            spacing: 8,
            children: [
              ElevatedButton.icon(
                onPressed: () async {
                  await _service.autoArrange();
                  await _refresh();
                  _showSnackBar('Displays auto-arranged');
                },
                icon: const Icon(Icons.auto_fix_high),
                label: const Text('Auto Arrange'),
              ),
            ],
          ),
          const SizedBox(height: 24),

          // DDC/CI (if display selected)
          if (_selectedDisplay != null) ...[
            Text(
              'DDC/CI - Hardware Control',
              style: Theme.of(context).textTheme.titleLarge,
            ),
            const SizedBox(height: 8),
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  children: [
                    ListTile(
                      title: const Text('Hardware Brightness'),
                      trailing: Row(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          IconButton(
                            icon: const Icon(Icons.remove),
                            onPressed: () async {
                              final current = await _service.getHWBrightness(
                                _selectedDisplay!,
                              );
                              await _service.setHWBrightness(
                                _selectedDisplay!,
                                current - 10,
                              );
                            },
                          ),
                          IconButton(
                            icon: const Icon(Icons.add),
                            onPressed: () async {
                              final current = await _service.getHWBrightness(
                                _selectedDisplay!,
                              );
                              await _service.setHWBrightness(
                                _selectedDisplay!,
                                current + 10,
                              );
                            },
                          ),
                        ],
                      ),
                    ),
                    ListTile(
                      title: const Text('Hardware Contrast'),
                      trailing: Row(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          IconButton(
                            icon: const Icon(Icons.remove),
                            onPressed: () async {
                              final current = await _service.getHWContrast(
                                _selectedDisplay!,
                              );
                              await _service.setHWContrast(
                                _selectedDisplay!,
                                current - 10,
                              );
                            },
                          ),
                          IconButton(
                            icon: const Icon(Icons.add),
                            onPressed: () async {
                              final current = await _service.getHWContrast(
                                _selectedDisplay!,
                              );
                              await _service.setHWContrast(
                                _selectedDisplay!,
                                current + 10,
                              );
                            },
                          ),
                        ],
                      ),
                    ),
                    Row(
                      children: [
                        ElevatedButton.icon(
                          onPressed: () async {
                            await _service.powerOffMonitor(_selectedDisplay!);
                            _showSnackBar('Monitor powered off');
                          },
                          icon: const Icon(Icons.power_off),
                          label: const Text('Power Off'),
                          style: ElevatedButton.styleFrom(
                            backgroundColor: Colors.red,
                          ),
                        ),
                        const SizedBox(width: 8),
                        ElevatedButton.icon(
                          onPressed: () async {
                            await _service.powerOnMonitor(_selectedDisplay!);
                            _showSnackBar('Monitor powered on');
                          },
                          icon: const Icon(Icons.power),
                          label: const Text('Power On'),
                          style: ElevatedButton.styleFrom(
                            backgroundColor: Colors.green,
                          ),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
          ],
          const SizedBox(height: 24),

          // Mirror
          if (_displays.length > 1) ...[
            Text(
              'Mirror Displays',
              style: Theme.of(context).textTheme.titleLarge,
            ),
            const SizedBox(height: 8),
            Wrap(
              spacing: 8,
              children: _displays
                  .map(
                    (d) => ElevatedButton(
                      onPressed: () async {
                        final other = _displays.firstWhere(
                          (o) => o.name != d.name,
                        );
                        await _service.setMirror(d.name, other.name);
                        await _refresh();
                        _showSnackBar('Mirrored ${d.name} to ${other.name}');
                      },
                      child: Text('Mirror ${d.name}'),
                    ),
                  )
                  .toList(),
            ),
          ],
        ],
      ),
    );
  }

  void _showSnackBar(String message) {
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(SnackBar(content: Text(message)));
  }
}
